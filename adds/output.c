/*
 * output.c
 *
 *  Created on: 17-12-2012
 *      Author: radomir.mazon
 */
#include "output.h"

#define getSFR(i) _output_table.sfr[i]
#define getBit(i) (_output_table.bit[i]&0x0F)
#define getState(i) ((_output_table.bit[i]&0xF0)>>4)

#define setSFR(i,v) _output_table.sfr[i]=v
#define setBit(i,v) _output_table.bit[i]=v
#define setState(i,v) _output_table.bit[i] = (_output_table.bit[i]&0x0F) | (v<<4)

#define setLo(i) _SFR_IO8(getSFR(i) + _PORT) &= ~(1<<getBit(i));
#define setHi(i) _SFR_IO8(getSFR(i) + _PORT) |= 1<<getBit(i);

_io_type _output_table;
uint8_t _output_index = 0;

void output_add(uint8_t sfr, uint8_t bit, uint8_t isNegative) {
	setSFR(_output_index, sfr);
	setBit(_output_index, bit);
	setState(_output_index, isNegative);
	_output_index++;
	_output_setup(isNegative);
}

void _output_setup() {
	for (uint8_t i=0; i<_output_index; i++) {
		//DDR
		_SFR_IO8(getSFR(i) + _DDR) |= 1<<getBit(i);
		//state
		if (getState(i) == 0) {
			setLo(i);
		} else {
			setHi(i);
		}

	}
}

void output_onEvent(saf_Event event) {
	if (event.code == EVENT_OUT_DOWN) {
		if (event.value < _output_index){
				if (getState(event.value) == 0) {
					setLo(event.value);
				} else {
					setHi(event.value);
				}
		}
	}

	if (event.code == EVENT_OUT_UP) {
		if (event.value < _output_index){
			if (getState(event.value) == 0) {
				setHi(event.value);
			} else {
				setLo(event.value);
			}
		}
	}

	if (event.code == EVENT_OUT_BLINK) {
		saf_eventBusSend_(EVENT_OUT_UP, event.value);
		saf_startTimer(OUTPUT_BLINK_DELAY, EVENT_OUT_BLINK_END, event.value);
	}

	if (event.code == EVENT_OUT_BLINK_END) {
		saf_eventBusSend_(EVENT_OUT_DOWN, event.value & ~0x80);
		if ((event.value & 0x80) > 0) {
			saf_startTimer(OUTPUT_BLINK_DELAY, EVENT_OUT_BLINK2, event.value);
		}
	}

	if (event.code == EVENT_OUT_BLINK2) {
		saf_eventBusSend_(EVENT_OUT_UP, event.value & ~0x80);
		if ((event.value & 0x80) > 0) {
			saf_startTimer(OUTPUT_BLINK_DELAY, EVENT_OUT_BLINK_END, event.value & ~0x80);
		} else {
			saf_startTimer(OUTPUT_BLINK_DELAY, EVENT_OUT_BLINK_END, event.value | 0x80);
		}
	}
}
