//Набор функций для обслуживания ЖКИ интерфейс 4-бита 	
#include "lcd.h"            
#include "bits.h"

// it is a russian alphabet translation
// except 0401 --> 0xa2 = ╗, 0451 --> 0xb5
char utf_recode[] = 
       { 0x41,0xa0,0x42,0xa1,0xe0,0x45,0xa3,0xa4,0xa5,0xa6,0x4b,0xa7,0x4d,0x48,0x4f,
         0xa8,0x50,0x43,0x54,0xa9,0xaa,0x58,0xe1,0xab,0xac,0xe2,0xad,0xae,0x62,0xaf,0xb0,0xb1,
         0x61,0xb2,0xb3,0xb4,0xe3,0x65,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0x6f,
         0xbe,0x70,0x63,0xbf,0x79,0xe4,0x78,0xe5,0xc0,0xc1,0xe6,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7
        };   
		
void Set_Interface_Byte(uint8_t data) {
	for (uint8_t i=0;i<8;i++) {
		SENDBIT(PortInterface, Idata, data);
		STROBE(PortInterface, Ishift);   // Shift
		data = data << 1;
	}
	STROBE(PortInterface, Ilatch);  // Out enable
	DELAY;
}

// send low 4 bit from data, RS_bit =0 for cmd mode, =1 for datamode
void lcd_send_niddle(uint8_t data, uint8_t RS_bit) {
	data = data << 4; // тут тонкость, т.к. у нас работает старшие 4 бита, поэтому двигаем на 4 разряда влево
	Set_Interface_Byte(RS_bit<<LCD_RS | 0<<LCD_EN | data); 
	Set_Interface_Byte(RS_bit<<LCD_RS | 1<<LCD_EN | data); // строб
	Set_Interface_Byte(RS_bit<<LCD_RS | 0<<LCD_EN | data); // строб
	_delay_us(100);
}


//-----------Функция записи команды в ЖКИ---------------
void lcd_com(uint8_t data)
{
	lcd_send_niddle(data>>4, 0);
	lcd_send_niddle(data, 0);
}

//-----------Функция записи данных в ЖКИ----------------
void lcd_dat(uint8_t data)
{
	lcd_send_niddle(data>>4, 1);
	lcd_send_niddle(data, 1);
}

void lcd_pos(uint8_t pos) {
	if (hi(pos)>0) {
		pos = (pos & 0x0f) + 0x40;
	}
	lcd_com(0x80 + pos);
}

// вывод строки на экран в позицию y=hi(pos), x=lo(pos)
void lcd_out(char *str)
{
	uint8_t i=0;
	while(str[i]) {
//		if ((uint8_t)str[i]<0xd0) { // unicode skip first byte
			lcd_dat(str[i]); 
//		}
		i++;
	}
}

void lcd_clear() {
	lcd_com(LCD_CLEARDISPLAY); _delay_us(2000);
}

void lcd_init(void)
{
	_delay_us(50000);		// положено ждать после включения > 40ms

	// инициализация на 4 битную шину
	lcd_send_niddle(0x03, 0);
	_delay_us(4500);		// положено ждать > 4.1ms
	lcd_send_niddle(0x03, 0);
	_delay_us(4500);		// положено ждать > 4.1ms
	lcd_send_niddle(0x03, 0);
	_delay_us(150);			
	lcd_send_niddle(0x02, 0);


	lcd_com(LCD_FUNCTIONSET | LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS);	
	lcd_com(LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF);
	lcd_clear();
	lcd_com(LCD_ENTRYMODESET | LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT);

// custom zerro char
	lcd_com(0x40);
	lcd_dat(0b00000000);
	lcd_dat(0b00010001);
	lcd_dat(0b00001110);
	lcd_dat(0b00010001);
	lcd_dat(0b00010001);
	lcd_dat(0b00001110);
	lcd_dat(0b00010001);
	lcd_dat(0b00000000);
	lcd_com(0x80); // set cursor to home
}

void lcd_bits(uint8_t n, char clear_bit_char, char set_bit_char)
{
	for (int i=0; i<8; i++) {
		if ((n & 0x80) == 0) { 
			lcd_dat(clear_bit_char);
		} else {
			lcd_dat(set_bit_char);
		}
		n = n << 1;
	}
}

// display one hex digit (0...F)
void lcd_hexdigit(uint8_t data) {
	data = data + '0';
	if (data>'9') data+=7; // characters A-F
	lcd_dat(data);
}

// displays the hex value of DATA at current LCD cursor position
void lcd_hex(uint8_t data)
{
	lcd_hexdigit(data>>4);
	lcd_hexdigit(data&0x0f);
}

