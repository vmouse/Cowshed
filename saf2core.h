/*
 * saf2core.h
 *
 *  Version: 2.3
 *      Author: radomir mazon
 */

#ifndef SAF2CORE_H_
#define SAF2CORE_H_
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include "Event.h"

//-----------| SETUP |--------------------------------------------------
#define SAF_LISTENER_SIZE 					4
#define SAF_EVENT_BUFFOR_SIZE				32
#define SAF_EVENT_VALUE_T					uint8_t
#define SAF_TIMER_ENABLED					1
#define SAF_TIMER_SIZE						4
#define SAF_TICK_ENABLED					1
#define SAF_CONFIG_EXTRA_EVENT_VALUE_ENABLE 0
#define SAF_CONFIG_EXTRA_EVENT_VALUE1	//mozesz dodac dodatkowe pola struktury saf_Event
#define SAF_CONFIG_EXTRA_EVENT_VALUE2	//przyklad
#define SAF_CONFIG_EXTRA_EVENT_VALUE3	//int extraVal;

//----------------------------------------------------------------------

#if defined(EVENT_SAFTICK)
#else
#error "event EVENT_SAFTICK is required (in event.h)";
#endif

#if defined(EVENT_NULL)
#else
#error "event EVENT_NULL is required (in event.h)";
#endif

#if defined(EVENT_START_APP)
#else
#error "event EVENT_START_APP is required (in event.h)";
#endif

//----------------
//util
#define sbi(port, bit) (port) |= (1 << (bit))
#define cbi(port, bit) (port) &= ~(1 << (bit))
//----------------
typedef struct
{
	uint8_t code;
	SAF_EVENT_VALUE_T value;
#if SAF_CONFIG_EXTRA_EVENT_VALUE_ENABLE == 1
	SAF_CONFIG_EXTRA_EVENT_VALUE1
	SAF_CONFIG_EXTRA_EVENT_VALUE2
	SAF_CONFIG_EXTRA_EVENT_VALUE3
#endif
} saf_Event;

typedef struct
{
  int listenerCount;
  void (*listenerList[SAF_LISTENER_SIZE])(saf_Event);
} _saf_EventListener;

typedef struct
{
	saf_Event buffer[SAF_EVENT_BUFFOR_SIZE];
	uint8_t head;
	uint8_t tail;
} _saf_RingBuffer;

typedef struct
{
	_saf_EventListener list;
	_saf_RingBuffer buffer;
#if SAF_TICK_ENABLED == 1
	SAF_EVENT_VALUE_T timeCounter;
#endif
	uint8_t startApp;
} SAF;

void saf_init(void);

void saf_addEventHandler(void (*callback)(saf_Event));

void saf_process(void);

void saf_eventBusSend(saf_Event event);

//ring buffer
void 		_saf_ringbufferAdd(saf_Event c);
saf_Event 	_saf_ringbufferGet(void);
uint8_t 	_saf_ringbufferAvailable(void);
void _saf_ringbufferFlush(void);

//helper
#if _SAF_CONFIG_EXTRA_EVENT_VALUE_ENABLE == 0
	void saf_eventBusSend_(uint8_t code, SAF_EVENT_VALUE_T value);
#endif

//timer
#if SAF_TIMER_ENABLED == 1
	typedef struct {
		uint8_t eventCode;
		uint16_t couter;
		uint8_t value;
	} _saf_timer_t;
	void saf_startTimer(uint16_t interval, uint8_t eventCode, uint8_t value);
	void _saf_timerProcess(void);
	void _saf_timerInit(void);
#endif

#endif /* SAF2CORE_H_ */
