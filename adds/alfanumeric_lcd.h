/*
 * alfanumeric_lcd.h
 *
 *  Created on: 03-12-2012
 *      Author: Administrator
 */

#ifndef ALFANUMERIC_LCD_H_
#define ALFANUMERIC_LCD_H_
#include "../saf2core.h"
#include <util/delay.h>

//-----------------setup-------------------------
#define LCD_POWER_ENABLED 0
#define LCD_POWER_DDR	DDRB
#define LCD_POWER_PORT	PORTB
#define LCD_POWER_BIT	2

#define LCD_D7_DDR	DDRD
#define LCD_D7_PORT	PORTD
#define LCD_D7_BIT	5

#define LCD_D6_DDR	DDRD
#define LCD_D6_PORT	PORTD
#define LCD_D6_BIT	6

#define LCD_D5_DDR	DDRD
#define LCD_D5_PORT	PORTD
#define LCD_D5_BIT	4

#define LCD_D4_DDR	DDRB
#define LCD_D4_PORT	PORTB
#define LCD_D4_BIT	1

#define LCD_RS_DDR		DDRB
#define LCD_RS_PORT		PORTB
#define LCD_RS_BIT		0

#define LCD_E_DDR		DDRD
#define LCD_E_PORT		PORTD
#define LCD_E_BIT		7
//-------------------------------------------------
//Display on/off control
//D2:D - display on/off
//D1:C - cursor on/off
//D0:B - blink on/off
#define LCD_ON		0B00001100
#define LCD_OFF		0B00001000
#define LCD_HOME	0B01000000
#define LCD_CLEAR	0B00000001
#define LCD_POSITION 0B10000000
#define LCD_LINE1	 0x00
#define LCD_LINE2	 0x40
#define _lcd_strobeE() sbi(LCD_E_PORT, LCD_E_BIT); _delay_us(50); cbi(LCD_E_PORT, LCD_E_BIT); _delay_us(50)

void lcd_init();
void lcd_writeCommand(uint8_t);
void lcd_writeData(uint8_t);
void lcd_puts(char *str);
void _lcd_sendData(uint8_t);
void _lcd_setData(uint8_t);
void lcd_onEvent(saf_Event event);
#endif /* ALFANUMERIC_LCD_H_ */
