//Набор функций для обслуживания ЖКИ интерфейс 4-бита 	
#define F_CPU 8000000UL

#include "lcd.h"            

void Set_Interface_Byte(uint8_t data) {
	for (uint8_t i=0;i<8;i++) {
		SENDBIT(PortInterface, Idata, data);
		STROBE(PortInterface, Ishift);   // Shift
		data = data << 1;
	}
	STROBE(PortInterface, Ilatch);  // Out enable
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
//	Set_Interface_Byte(0<<LCD_RS | 1<<LCD_EN | hi_h(p));
//	Set_Interface_Byte(0<<LCD_RS | 0<<LCD_EN | hi_h(p));
	//Set_Interface_Byte(0<<LCD_RS | 1<<LCD_EN | lo_h(p));
	//Set_Interface_Byte(0<<LCD_RS | 0<<LCD_EN | lo_h(p));
	//_delay_us(300);
}

//-----------Функция записи данных в ЖКИ----------------
void lcd_dat(uint8_t data)
{
	lcd_send_niddle(data>>4, 0);
	lcd_send_niddle(data, 0);
//	Set_Interface_Byte(1<<LCD_RS | 1<<LCD_EN | hi_h(p));
//	Set_Interface_Byte(1<<LCD_RS | 0<<LCD_EN | hi_h(p));
	//Set_Interface_Byte(1<<LCD_RS | 1<<LCD_EN | lo_h(p));
	//Set_Interface_Byte(1<<LCD_RS | 0<<LCD_EN | lo_h(p));
	//_delay_us(10);
}

// вывод строки на экран в позицию y=hi(pos), x=lo(pos)
void lcd_out(uint8_t pos, char *str)
{
//	cli();
	if (hi(pos)>0) {
		pos = (pos & 0x0f) + 0x40;
	}
	lcd_com(0x80 + pos);
	unsigned char i =0;
	while(str[i])
	lcd_dat(str[i++]) ;
//	sei();
}

void lcd_init(void)
{
//	cli();
	_delay_us(50000);		// положено ждать после включения > 40ms

	// инициализация на 4 битную шину
	lcd_send_niddle(0x03, 0);
	_delay_us(4500);		// положено ждать > 4.1ms
	lcd_send_niddle(0x03, 0);
	_delay_us(4500);		// положено ждать > 4.1ms
	lcd_send_niddle(0x03, 0);
	_delay_us(150);			
	lcd_send_niddle(0x02, 0);


	lcd_com(LCD_FUNCTIONSET | LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS);	
	lcd_com(LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF);
	lcd_com(LCD_CLEARDISPLAY); _delay_us(2000);
	lcd_com(LCD_ENTRYMODESET | LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT);

	//lcd_com(0x20);		// инициализация на 4 битную шину
	//lcd_com(0x20);
	//lcd_com(0x28);		// 4 бита, 2 строки
	//lcd_com(0x06);		// пишем слева направо
	//lcd_com(0x0c);		// включить, без курсора
	//lcd_com(0x01);		// очистка экрана
//	sei();
}

