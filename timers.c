﻿/*
 * timers.c
 *
 * Created: 28.05.2014 23:41:20
 *  Author: vlad
 */ 
#include "timers.h"

_timers_type _timers_table;
_timers_type EEMEM flash_timers_table;

uint8_t  _timers_event;
uint16_t _timers_prescaler;
uint16_t _timers_prescaler_cnt;


char *msg_table[] =
{
	(char[]){224,111,187,184,179,32,32,32,32,32,32,0},	// долив
	(char[]){80,97,99,191,179,111,112,32,49,32,0},		// раствор 1
	(char[]){80,97,99,191,179,111,112,32,50,32,0},		// раствор 2
	(char[]){168,111,187,111,99,186,97,189,184,101,0},	// полоскание
	(char[]){77,111,185,186,97,32,49,32,32,32,0},		// мойка 1
	(char[]){77,111,185,186,97,32,50,32,32,32,0},		// мойка 1
	(char[]){80,101,182,184,188,32,54,32,32,32,0},		// режим 6
	(char[]){79,186,111,189,192,97,189,184,101,32,0},	// окончание
	(char[]){72,97,190,111,187,189,101,189,184,101,0},	// наполнение
	(char[]){67,187,184,179,32,32,32,32,32,32,0}		// слив
};

char* GetTimerName(uint8_t timer_index) {
	return msg_table[timer_index];
}

void timer_setup(uint8_t timer_index, uint16_t timer_count) {
	if (timer_index < TIMERS_MAX ) {
		_timers_table.timer_state[timer_index] = TIMER_STATE_STOP;
		_timers_table.timer_count[timer_index] = timer_count;
	} else {
		// а собственно что?
	}
}

void timer_set_end_state(uint8_t timer_index) {
	saf_Event newEvent;
	_timers_table.timer_state[timer_index] = TIMER_STATE_STOP;
	newEvent.value = timer_index;
	newEvent.code = EVENT_TIMER_END;
	saf_eventBusSend(newEvent);
}

void timer_start(uint8_t timer_index) {
	if (_timers_table.timer_count[timer_index] > 0) {
		_timers_table.timer_state[timer_index] = TIMER_STATE_START;
	} else { // стартуем нулевой таймер
		timer_set_end_state(timer_index);
	}
}

void _reset_timer(uint8_t timer_index) {
	_timers_table.timer_state[timer_index] = TIMER_STATE_STOP;
	_timers_table.timer_count[timer_index] = 0;
}

void timer_stop(uint8_t timer_index) {
	if (timer_index == 0xff) {
		for (uint8_t i=0; i<TIMERS_MAX; i++) { 
			_timers_table.timer_state[i] = TIMER_STATE_STOP;
		}
	} else _timers_table.timer_state[timer_index] = TIMER_STATE_STOP;
}

uint16_t timer_get(uint8_t timer_index) {
	return _timers_table.timer_count[timer_index];
}

uint8_t timer_iswork(uint8_t timer_index) {
	return (_timers_table.timer_state[timer_index] == TIMER_STATE_START)?1:0;
}

void timers_onEvent(saf_Event event) {
	if (event.code == _timers_event) {
		if (_timers_prescaler_cnt==0) {
			_timers_prescaler_cnt = _timers_prescaler;

			union { struct  
			{
				uint8_t all_end : 1;
				uint8_t any_end : 1;
				uint8_t tick	: 1;
			} bits;
			uint8_t value;
			} flags = {.value = 0x01 }; //set all_end only
			
			// цикл по всему массиву таймеров
			for (uint8_t i=0; i<TIMERS_MAX; i++) {
				if (_timers_table.timer_state[i] != TIMER_STATE_STOP) { // подразумеваем, что не стопнутый таймер имеет счетчик >0
					if (_timers_table.timer_state[i] == TIMER_STATE_START) { // работающий таймер надо уменьшать, а тот что в паузе просто учитывать как еще не закончивший
						_timers_table.timer_count[i]--; 
						flags.bits.tick = 1;
						if (_timers_table.timer_count[i] == 0) {		// таймер досчитал - событие
							timer_set_end_state(i);
							flags.bits.any_end = 1;							// флаг, что есть закончивший
						} else flags.bits.all_end = 0;						// флаг, что есть незакончивший 
					} else { flags.bits.all_end = 0; }						// есть таймер в паузе, т.е. есть незакончивший
				}
			}
			
			saf_Event newEvent;

			if (flags.bits.tick==1 && flags.bits.all_end==1) { // все таймеры закончили (и никого нет в паузе)
				newEvent.code = EVENT_ALL_TIMERS_END;
				saf_eventBusSend(newEvent);
			}
	
			if (flags.bits.tick==1 && flags.bits.any_end==0 ) { // какой-то из таймеров тикнул
				newEvent.code = EVENT_TIMER_TICK;
				saf_eventBusSend(newEvent);
			}
			
		} else _timers_prescaler_cnt--;

	}
}

void timers_init(uint8_t TickEvent, uint16_t presc) {
	_timers_prescaler=presc; // предварительный делитель
	_timers_event=TickEvent;
}

void SaveTimersToFlash(void) {
	for (uint8_t i=0; i<TIMERS_MAX; i++) {
		flash_timers_table.timer_state[i] = _timers_table.timer_state[i];
		flash_timers_table.timer_count[i] = _timers_table.timer_count[i];
	}
}

void LoadTimersFromFlash(void) {
	for (uint8_t i=0; i<TIMERS_MAX; i++) {
		_timers_table.timer_state[i] = flash_timers_table.timer_state[i];
		_timers_table.timer_count[i] = flash_timers_table.timer_count[i];
	}
}