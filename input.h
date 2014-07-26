/*
 * InputButtons.h
 *
 *  Created on: 23-09-2011
 *      Author: radomir.mazon
 */

#ifndef INPUTBUTTONS_H_
#define INPUTBUTTONS_H_

#include "saf2core.h"

//------------- setup
/*
 * przyklad dla portu PORTD bit 0
 * input_add(_D, 0);
 */
#define INPUT_SIZE 4 // inputs number
#define _DDR 1
#define _PORT 2

//-------------------
#if defined(__AVR_ATmega168__)
#define _B    0x03
#define _C    0x06
#define _D    0x09
#elif defined(__AVR_ATmega8__)
#define _B    0x16
#define _C    0x13
#define _D    0x10
#else
#error "Processor type not supported !"
#endif

/**
 * Stan zostal zapisany w tablicy bit pod maska 0xF0
 */
typedef struct {
	uint8_t bit[INPUT_SIZE];
	uint8_t sfr[INPUT_SIZE];
} _io_type;

void input_add(uint8_t sfr, uint8_t bit);
void _input_setup(void);
void input_onEvent(saf_Event event);

#endif /* INPUTBUTTONS_H_ */
