/*
 * alfanumeric_lcd.c
 *
 *  Created on: 03-12-2012
 *      Author: radomir.mazon
 */

#include "alfanumeric_lcd.h"

void lcd_init()
{
	cli();
	_delay_ms(15);
	#if LCD_POWER_ENABLED == 1
	sbi(LCD_POWER_DDR, LCD_POWER_BIT);
	sbi(LCD_POWER_PORT, LCD_POWER_BIT);
	#endif
	sbi(LCD_D7_DDR, LCD_D7_BIT);
	sbi(LCD_D6_DDR, LCD_D6_BIT);
	sbi(LCD_D5_DDR, LCD_D5_BIT);
	sbi(LCD_D4_DDR, LCD_D4_BIT);
	sbi(LCD_RS_DDR, LCD_RS_BIT);
	sbi(LCD_E_DDR, LCD_E_BIT);

	_lcd_setData(0x03);
	cbi(LCD_E_PORT, LCD_E_BIT);
	cbi(LCD_RS_PORT, LCD_RS_BIT);

	_delay_ms(15);
	_lcd_strobeE();

	_delay_ms(5);
	_lcd_strobeE();
	_delay_ms(1);
	_lcd_strobeE();
	_lcd_setData(0x02);
	_lcd_strobeE();

	//Function set
	//D4:DL	-1: 8 bits, 0:  4 bits
	//D3:N	-1: 2 lines, 0:  1 line
	//D2:F 	-1: 5 × 10 dots, 0:  5 × 8 dots
	lcd_writeCommand(0b00101000);

	//Cursor or display shift
	//D3:S/C - 0:Cursor move, 1:Display shift
	//D2:R/L - 0:Shift to left, 1:Shift to right
	//[Cursor move][Shift to left]
	lcd_writeCommand(0b00010000);

	//Entry mode set
	//D1:I/D - 0:Decrement, 1:Increment
	//D0:S   - 1:Accompanies display shift, 0:Disable
	lcd_writeCommand(0b00000110);

	lcd_writeCommand(LCD_CLEAR);

	//Display on/off control
	//D2:D - display on/off
	//D1:C - cursor on/off
	//D0:B - blink on/off
	lcd_writeCommand(LCD_ON);

	lcd_writeCommand(LCD_HOME);

	sei();
}

void lcd_writeCommand(uint8_t data)
{
	cbi(LCD_RS_PORT, LCD_RS_BIT);
	_lcd_sendData(data);
}

void lcd_writeData(uint8_t data)
{
	sbi(LCD_RS_PORT, LCD_RS_BIT);
	_lcd_sendData(data);
}

void lcd_puts(char *str)
{
    unsigned char i =0;

    while( str[i])
        lcd_writeData(str[i++]) ;
}

void _lcd_sendData(uint8_t data) {
	_lcd_setData(data>>4);
	_lcd_strobeE();
	_lcd_setData(data);
	_lcd_strobeE();
	_delay_ms(4);
}

void _lcd_setData(uint8_t data) {
	if ((data&0x01) == 0) {
		cbi(LCD_D4_PORT, LCD_D4_BIT);
	} else {
		sbi(LCD_D4_PORT, LCD_D4_BIT);
	}
	if ((data&0x02) == 0) {
		cbi(LCD_D5_PORT, LCD_D5_BIT);
	} else {
		sbi(LCD_D5_PORT, LCD_D5_BIT);
	}
	if ((data&0x04) == 0) {
		cbi(LCD_D6_PORT, LCD_D6_BIT);
	} else {
		sbi(LCD_D6_PORT, LCD_D6_BIT);
	}
	if ((data&0x08) == 0) {
		cbi(LCD_D7_PORT, LCD_D7_BIT);
	} else {
		sbi(LCD_D7_PORT, LCD_D7_BIT);
	}
}

//---- saf support
void lcd_onEvent(saf_Event e) {
	switch(e.code) {
	case EVENT_INIT:
		lcd_init();
		break;

	case EVENT_LCD_ON:
		#if LCD_POWER_ENABLED == 1
		lcd_init();
		#endif
		lcd_writeCommand(LCD_ON);
		break;

	case EVENT_LCD_OFF:
		lcd_writeCommand(LCD_OFF);
		#if LCD_POWER_ENABLED == 1
		cbi(LCD_POWER_PORT, LCD_POWER_BIT);
		#endif
		break;

	case EVENT_LCD_GOTO:
		lcd_writeCommand(LCD_POSITION|e.value);
		break;
	case EVENT_LCD_SEND:
		lcd_writeData(e.value);
		break;
	case EVENT_LCD_HOME1:
		lcd_writeCommand(LCD_POSITION|LCD_LINE1);
		break;
	case EVENT_LCD_HOME2:
		lcd_writeCommand(LCD_POSITION|LCD_LINE2);
		break;
	case EVENT_LCD_CLEAR:
		lcd_writeCommand(LCD_CLEAR);
		break;
	}
}
