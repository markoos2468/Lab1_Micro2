#include <Keypad.h>
//#include <avr/io.h>

typedef enum state {
  WAIT_DURATION,
  SET_DURATION,
  TRAFFICKING,
  FAILURE_MODE
} State;

//First display begins at index 0 and ends at index n
byte sevenSegment[] = {
  //8 bits to LED -> a,b,c,d,e,f,g,dp
  0b11111100,  // seven segment 0
  0b01100000,  // seven segment 1
  0b11011010,  // segment seg 2
  0b11110010,  // 3
  0b01100110,  // 4
  0b10110110,  // 5
  0b10111110,  // 6
  0b11100000,  // 7
  0b11111110,  // 8
  0b11100110,  // 9
};

// Keypad parameters
char keys[4][4] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};
byte rowPins[] = { 22, 23, 24, 25 };
byte colPins[] = { 26, 27, 28, 29 };
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, 4, 4);

// Shift register pins
unsigned int SER = 19;    //serial in
unsigned int RCLK = 38;   //latch
unsigned int SRCLK = 18;  //shift register

unsigned int digitPins[] = { 21, 20 };
int numDigits;

//green, yellow, red
const int NUM_TRAFFIC_LIGHTS = 2;
unsigned int lightPins[NUM_TRAFFIC_LIGHTS][3] = {
  { 32, 31, 30 },
  { 35, 34, 33 }
};

// Color indices
const int GREEN = 0;
const int YELLOW = 1;
const int RED = 2;

int buzzerPin = 36;

double timerFrequency = 2;  // Hz
int duration = 10;
double time = duration;

char colorSelect;
int redDuration;
int greenDuration;
int yellowDuration = 3;  // Constant

State state;


//configures pins found on specific ports
//PORTA is only read
//PORTC D are write only
//ports take a 8bit value to determine which pin on port is being effected
extern "C"{
  extern void configurePORTA(uint8_t pin);  
  extern void configurePORTC(uint8_t pin);
  extern void configurePORTD(uint8_t pin);

  //Read only from port A because it is user input
  //Write only for port C and D becayse it is user interface
  extern void readPORTA(uint8_t pin);
  extern void writePORTC(uint8_t pin, uint8_t enable);
  extern void writePORTD(uint8_t pin, uint8_t enable);
}

void setup() {
  Serial.begin(9600);

  numDigits = sizeof(digitPins) / sizeof(unsigned int);

  initPins();
  initTimerInterrupt(timerFrequency);
  state = WAIT_DURATION;
}

ISR(TIMER1_COMPA_vect) {
  time += 1 / timerFrequency;
  if (time >= duration)
    time = 0;
}

void loop() {


  //state = TRAFFICKING;
  switch (state) {
    case WAIT_DURATION:
      flash(0, RED, 1);
      flash(1, RED, 1);
      pollKeypad();
      break;
    case SET_DURATION:
      flash(0, RED, 1);
      flash(1, RED, 1);
      pollDuration();
      break;
    case TRAFFICKING:
      cycleLights();
      displayTime();
      checkFailureMode();
      break;
    case FAILURE_MODE:
      flash(0, RED, 0.5);
      flash(1, RED, 0.5);
      pollKeypad();
      break;
  }
}

void pollKeypad() {
  char key = keypad.getKey();
  switch (key) {
    case 'A':
      redDuration = 0;
      colorSelect = key;
      state = SET_DURATION;
      break;
    case 'B':
      greenDuration = 0;
      colorSelect = key;
      state = SET_DURATION;
      break;
    case '*':
      time = 0;
      duration = redDuration + yellowDuration + greenDuration;
      state = TRAFFICKING;
      break;
  }
}

void pollDuration() {
  char key = keypad.getKey();

  if (key >= '0' && key <= '9') {  // If key is digit
    int digit = key - '0';
    if (colorSelect == 'A') {
      redDuration = redDuration * 10 + digit;
    } else if (colorSelect == 'B') {
      greenDuration = greenDuration * 10 + digit;
    }
  } else if (key == '#') {
    state = WAIT_DURATION;
  }
}

void cycleLights() {
  if (time < greenDuration - 3) {
    light(0, GREEN);
    light(1, RED);
  } else if(time < greenDuration) {
    flash(0, GREEN, 0.5);
    buzz();
  } else if (time - greenDuration < yellowDuration) {
    light(0, YELLOW);
    flash(1, RED, 0.5);
    // buzz();
  } else if (time - greenDuration - yellowDuration < redDuration - 6) {
    light(0, RED);
    light(1, GREEN);
  } else if (time - greenDuration - yellowDuration < redDuration - 3) {
    flash(1, GREEN, 0.5);
    // buzz();
  } else {
    flash(0, RED, 0.5);
    light(1, YELLOW);
    buzz();
  }
}

void light(int light_num, int color) {
  int numLights = sizeof(lightPins[light_num]) / sizeof(int);
  if (color >= 0 && color <= numLights) {
    digitalWrite(lightPins[light_num][color], HIGH);
    digitalWrite(lightPins[light_num][(color + 1) % 3], LOW);
    digitalWrite(lightPins[light_num][(color + 2) % 3], LOW);
  } else {
    for (int i = 0; i < numLights; i++) {
      digitalWrite(lightPins[light_num][i], LOW);
    }
  }
}

void flash(int light_num, int color, double period) {
  if ((int) (time / period) % 2 == 1) {
      light(light_num, color);
    } else {
      light(light_num, -1);
    }
}

void buzz() {
  digitalWrite(buzzerPin, HIGH);
  digitalWrite(buzzerPin, LOW);
}

void displayTime() {
  int timeLeft = 0;
  if (time < greenDuration) {
    timeLeft = greenDuration - (int) time;
  } else if (time < greenDuration + yellowDuration) {
    timeLeft = greenDuration + yellowDuration - (int) time;
  } else {
    timeLeft = duration - (int) time;
  }
  displayNumber(timeLeft);
}

int failureCount = 0;
void checkFailureMode() {
  if(keypad.getKey() == '#')
    failureCount++;
  if(failureCount >= 2) {
    state = FAILURE_MODE;
    failureCount = 0;
  }
}

//input a decimal value with the same number of digits as displays and sorts through each decimal value individually and sent the register
//and the corresponding display number is set ON and outputting the correspodning value.
void displayNumber(int num) {
  for (int i = 0; i < numDigits; i++) {
    int digit = (int)(num / pow(10, i)) % 10;

    for (int j = 0; j < numDigits; j++) {
      digitalWrite(digitPins[j], LOW);
    }

    digitalWrite(RCLK, LOW);
    shiftOut(SER, SRCLK, LSBFIRST, sevenSegment[digit]);
    digitalWrite(RCLK, HIGH);
    digitalWrite(digitPins[i], HIGH);
    delay(5);
  }
}

// Set all pins to outputs
void initPins() {
  configurePORTA(1);
  configurePORTC(1);
  configurePORTD(1);
}

// Set timer1 interrupt
void initTimerInterrupt(double frequency) {
  cli();
  TCCR1A = 0;  // set entire TCCR1A register to 0
  TCCR1B = 0;  // same for TCCR1B
  TCNT1 = 0;   //initialize counter value to 0
  // set compare match register for 1hz increments
  OCR1A = (int)((16 * 1000000) / (frequency * 1024) - 1);  // (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS12 and CS10 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei();
}
