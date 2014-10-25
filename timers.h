/*
 * timers.h
 *
 * Created: 28.05.2014 23:41:36
 *  Author: vlad
 */ 


#ifndef TIMERS_H_
#define TIMERS_H_

#include "saf2core.h"
#include <avr/eeprom.h>

#define TIMERS_MAX 4		// inputs number

#define TIMER_STATE_STOP	0
#define TIMER_STATE_START	1
#define TIMER_STATE_PAUSE	2

typedef struct {
//	uint16_t timers_prescaler_cnt;
	uint16_t timer_count[TIMERS_MAX];
	uint8_t	timer_state[TIMERS_MAX];
} _timers_type;

char* GetTimerName(uint8_t timer_index);
void timer_setup(uint8_t timer_index, uint16_t timer_count);
void timer_start(uint8_t timer_index);
void timer_stop(uint8_t timer_index);
void timer_set_end_state(uint8_t timer_index);
void timers_onEvent(saf_Event event);
void timers_init(uint8_t TickEvent, uint16_t presc);
void _reset_timer(uint8_t timer_index);
uint16_t timer_get(uint8_t timer_index);
uint8_t timer_iswork(uint8_t timer_index);
void SaveTimersToFlash(void);
void LoadTimersFromFlash(void);

#endif /* TIMERS_H_ */