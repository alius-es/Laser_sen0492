#pragma once

#ifndef F_CPU
//#warning DEFINE F_CPU
#define F_CPU 2000000
#endif

#define CYCLES_PER_US (F_CPU/1000000)	/* the count of tacts per 1 us */
#define C4PUS (CYCLES_PER_US/4) 	/* the count of 4tacts per 1 us */
#define DVUS(x) (CYCLES_PER_US*x/4) // was (C4PUS*x) but 2/4 is bad so it is better to (2*x/4), if x=1000 there is no problem

extern void _delay_us(unsigned short u);

#define delay_us(x) _delay_us(DVUS(x))

/* because _delay_us decrese X counter to make a delay:

_delay_us:
loop:        
        DECW X       // X-1
        JREQ fin     // if X==0, go to fin
        JRNE loop    // else loop
fin:
        RET          // return

so _delay_us(1) consumes approx. 4tacts */
