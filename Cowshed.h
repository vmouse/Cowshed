
#ifndef MAIN_H_
#define MAIN_H_

#include <avr/io.h>
#include <string.h>
#include <stdint.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include "saf2core.h"

#define CoolStart	(MCUSR & ( 1 << PORF )) // не ноль, если запуск был включением питания
#define PSTR(s) (__extension__( {static char __c[] PROGMEM = (s); &__c[0];}))
//Эта фича позволяет писать внутри функций так:
//char * mystr = PSTR( ":)" );


#define PortControl	PORTB
#define PCshift		PB1 // Register strobe (shift clock)
#define PCdata		PB0 // Register data
#define PClatch		PB2 // Register out (latch clock)
#define Sensor		PD3 // Sensor multiplexor

#define STATE_VALUE_START 0x01; // комбинация бит на старт программы

typedef union { struct
	{
		uint8_t started : 1;	// обязательно должен быть младшим битом
		uint8_t waiting	: 1;
		uint8_t end		: 1;
		uint8_t error	: 1;
		uint8_t config  : 1;   // конфигурирование, работа с меню (подавляет вывод другой информации)
		uint8_t settimers: 1;  // специальный режим конфигурирования таймеров (подавляет работу меню, но вернется в него)
		uint8_t userinput: 1;  // идет пользовательский ввод с клавиатуры
	} bits;
	uint8_t value;
} _cow_state;
	
typedef union { struct
	{
		uint8_t sensor_high : 1;
		uint8_t sensor_low : 1;
		uint8_t reserved : 2;
		uint8_t timer_num : 3;
		uint8_t error_timeout : 1;
	} bits;
	uint8_t value;
} _cow_waiting_state;

typedef struct {
	char	cmd_name;
	uint8_t cmd_data;
} _cmd_type;


void OutDataPort(uint8_t data);
void Set_Control_Byte(uint8_t data);
void SaveFlash(void);
void RestFlash(void);
void ResetState(void);
// Show command by index
void ShowCmd(uint8_t cmd_index);
// Show seconds counter value 
void ShowTime(uint16_t data);
void ShowSensors(void);
void ShowError(uint8_t ErrorClass, uint8_t ErrorCode);
void ShowEnd(void);
void Do_Command(void);
void StartProg(_cmd_type Prog[]);

#endif