/*
 * logic.c
 *
 * Created: 28.04.2014 22:59:51
 *  Author: vlad
 */ 
#include <stdint.h>
#include "logic.h"

void onEvent(uint8_t code, int value)
{
	if (code == EVENT_SAFTICK && timePrescaler(256)) {
		DDRC = 0xff;
		PORTC = ~PORTC;
	}
}