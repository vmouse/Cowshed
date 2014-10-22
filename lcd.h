#ifndef LCD_H
#define LCD_H

#include <util/delay.h>
#include "bits.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include "strfunc.h"

#define PortInterface PORTC
#define PinKeyboard PINC
#define PinSensors	PIND
#define Ishift		PC1 // Register strobe (shift clock)
#define Idata		PC3 // Register data
#define Ilatch		PC0 // Register out (latch clock)
#define Keyboard	PC2	// Keyboard multiplexor
#define Sensors		PD3 // Sensors multiplexor
#define LCD_RS		2	// bit LCD RS
#define LCD_EN		3   // bit LCD EN

// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

// enum for
#define LCD_DRAM_Normal 0x00
#define LCD_DRAM_WH1601 0x01

void Set_Interface_Byte(uint8_t data);
void lcd_send_niddle(uint8_t data, uint8_t RS_bit);
void lcd_init(void); 
void lcd_com(unsigned char p);
void lcd_dat(unsigned char p);
void lcd_clear(void);
void lcd_pos(uint8_t pos);
void lcd_out(char *str);
void lcd_hexdigit(uint8_t data);
void lcd_hex(uint8_t data);
void lcd_bits(uint8_t n, char clear_bit_char, char set_bit_char);

#define lcd_cursor_on lcd_com(LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSORON | LCD_BLINKON)
#define lcd_cursor_off lcd_com(LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF)

#endif



