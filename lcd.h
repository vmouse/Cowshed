#ifndef LCD_H
#define LCD_H

#include <util/delay.h>
#include "bits.h"
#include <avr/io.h>

#define PortInterface PORTC
#define Ishift		PC5 // Register strobe (shift clock)
#define Idata		PC3 // Register data
#define Ilatch		PC4 // Register out (latch clock)
#define Keyboard	PC2	// Keyboard multiplexor
#define LCD_RS		2	// бит LCD RS
#define LCD_EN		3   // бит LCD EN

#define 	LCD_CLR          	0      // DB0: clear display
#define 	LCD_HOME         	1      // DB1: return to home position

#define  	LCD_ENTRY_MODE   	2      // DB2: set entry mode
#define 	LCD_ENTRY_INC    	1      //   DB1: increment
#define 	LCD_ENTRY_SHIFT  	0      //   DB2: shift

#define 	LCD_ON		      	3      // DB3: turn lcd/cursor on
#define  	LCD_ON_DISPLAY   	2      //   DB2: turn display on
#define  	LCD_ON_CURSOR     	1      //   DB1: turn cursor on
#define  	LCD_ON_BLINK      	0      //   DB0: blinking cursor

#define  	LCD_MOVE          	4      // DB4: move cursor/display
#define 	LCD_MOVE_DISP       3      //   DB3: move display (0-> move cursor)
#define  	LCD_MOVE_RIGHT      2      //   DB2: move right (0-> left)

#define  	LCD_F		        5      // DB5: function set
#define 	LCD_F_8B		   	4      //   DB4: set 8BIT mode (0->4BIT mode)
#define  	LCD_F_2L			3      //   DB3: two lines (0->one line)
#define  	LCD_F_10D			2      //   DB2: 5x10 font (0->5x7 font)
#define  	LCD_CGRAM           6      // DB6: set CG RAM address
#define  	LCD_DDRAM           7      // DB7: set DD RAM address

void lcd_init(void); 
void lcd_com(unsigned char p);
void lcd_dat(unsigned char p);
void lcd_out(uint8_t pos, char *str);

#endif



