#define __SFR_OFFSET 0x00

#include "avr/io.h"

#define __SFR_OFFSET 0x00

.global configurePORTA
.global configurePORTC
.global configurePORTD
.global readPORTA
.global writePORTC
.global writePORTD



.section .text
;out command was not working porperly
configurePORTA: 
    cbi DDRA, 0
    cbi DDRA, 1
    cbi DDRA, 2
    cbi DDRA, 3
    cbi DDRA, 4
    cbi DDRA, 5
    cbi DDRA, 6
    cbi DDRA, 7
  ret
configurePORTC: 
    sbi DDRC, 0
    sbi DDRC, 1
    sbi DDRC, 2
    sbi DDRC, 3
    sbi DDRC, 4
    sbi DDRC, 5
    sbi DDRC, 6
    sbi DDRC, 7
  ret 
configurePORTD: 
    sbi DDRD, 0
    sbi DDRD, 1
    sbi DDRD, 2
    sbi DDRD, 3
    sbi DDRD, 4
    sbi DDRD, 5
    sbi DDRD, 6
    sbi DDRD, 7
  ret
readPORTA:
  call shiftForPort
  in  r26, PINA
  ret
writePORTC:
  call shiftForPort
  out PORTC, r26
  ret
writePORTD:
  call shiftForPort
  out PORTD, r26
  ret

shiftForPort:
    tst r24           
    breq shift_done

shift_loop:
    lsl r26            
    dec r24            
    brne shift_loop    

shift_done:
    ret