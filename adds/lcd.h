#ifndef LCD_H
#define LCD_H

#include <avr/io.h>            
#include <util/delay.h>
#include <avr/pgmspace.h>  

#define PORT_lcd PORTD
#define PORT_RS  PORTD
#define PORT_EN  PORTD
#define DDR_lcd  DDRD
#define DDR_RS   DDRD
#define DDR_EN   DDRD
#define RS       PD4          
#define EN       PD5  

void lcd_init(void); 
void lcd_com(unsigned char p);
void lcd_dat(unsigned char p);

#endif



