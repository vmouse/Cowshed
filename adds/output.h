/*
 * output.h
 *
 *  Created on: 17-12-2012
 *      Author: Administrator
 */

#ifndef OUTPUT_H_
#define OUTPUT_H_

#include "../saf2core.h"
#include "input.h"

#define OUTPUT_SIZE 8
#define OUTPUT_BLINK_DELAY 255

void output_add(uint8_t sfr, uint8_t bit, uint8_t isNegative);
void _output_setup();
void output_onEvent(saf_Event event);

#endif /* OUTPUT_H_ */
