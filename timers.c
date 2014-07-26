/*
 * timers.c
 *
 * Created: 28.05.2014 23:41:20
 *  Author: vlad
 */ 
#include "timers.h"

_timers_type _timers_table;
_timers_type EEMEM flash_timers_table;
uint16_t _timer_prescaler_cnt = TIMERS_PRESCALER;

void timer_setup(uint8_t timer_index, uint16_t timer_count) {
	if (timer_index < TIMERS_MAX ) {
		_timers_table.timer_state[timer_index] = TIMER_STATE_STOP;
		_timers_table.timer_count[timer_index] = timer_count;
	} else {
		// а собственно что?
	}
}

void timer_start(uint8_t timer_index) {
	_timers_table.timer_state[timer_index] = TIMER_STATE_START;
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

void timers_init(void) {
	_timers_table.timers_prescaler_cnt=TIMERS_PRESCALER; // предварительный делитель
}

void timer_onEvent(saf_Event event) {
	if (event.code == EVENT_SAFTICK) {
		if (_timer_prescaler_cnt==0) {
			_timer_prescaler_cnt = TIMERS_PRESCALER;

			union { struct  
			{
				uint8_t all_end : 1;
				uint8_t any_end : 1;
				uint8_t tick	: 1;
			} bits;
			uint8_t value;
			} flags = {.value = 0x01 }; //set all_end only
			
			saf_Event newEvent;

			// цикл по всему массиву таймеров
			for (uint8_t i=0; i<TIMERS_MAX; i++) {
				if (_timers_table.timer_state[i] != TIMER_STATE_STOP) { // подразумеваем, что не стопнутый таймер имеет счетчик >0
					if (_timers_table.timer_state[i] == TIMER_STATE_START) { // работающий таймер надо уменьшать, а тот что в паузе просто учитывать как еще не закончивший
						_timers_table.timer_count[i]--; 
						flags.bits.tick = 1;
						if (_timers_table.timer_count[i] == 0) {		// таймер досчитал - событие
							_timers_table.timer_state[i] = TIMER_STATE_STOP;
							newEvent.value = i;
							newEvent.code = EVENT_TIMER_END;
							saf_eventBusSend(newEvent);
							flags.bits.any_end = 1;							// флаг, что есть закончивший
						} else flags.bits.all_end = 0;						// флаг, что есть незакончивший 
					} else { flags.bits.all_end = 0; }						// есть таймер в паузе, т.е. есть незакончивший
				}
			}
			
			if (flags.bits.tick==1 && flags.bits.all_end==1) { // все таймеры закончили (и никого нет в паузе)
				newEvent.code = EVENT_ALL_TIMERS_END;
				saf_eventBusSend(newEvent);
			}
	
			if (flags.bits.tick==1 && flags.bits.any_end==0 ) { // какой-то из таймеров тикнул
				newEvent.code = EVENT_TIMER_TICK;
				saf_eventBusSend(newEvent);
			}
			
		} else _timer_prescaler_cnt--;

	}
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