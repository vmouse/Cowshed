//����� ������� ��� ������������ ��� ��������� 4-���� 	
#include "lcd.h"            

void Set_Interface_Byte(uint8_t data) {
	for (uint8_t i=0;i<8;i++) {
		SENDBIT(PortInterface, Idata, data);
		STROBE(PortInterface, Ishift);   // Shift
		data = data << 1;
	}
	STROBE(PortInterface, Ilatch);  // Out enable
}

//-----------������� ������ ������� � ���---------------
void lcd_com(unsigned char p)
{
	Set_Interface_Byte(0<<LCD_RS | 1<<LCD_EN | hi_h(p));
	Set_Interface_Byte(0<<LCD_RS | 0<<LCD_EN | hi_h(p));
	_delay_us(10);
	Set_Interface_Byte(0<<LCD_RS | 1<<LCD_EN | lo_h(p));
	Set_Interface_Byte(0<<LCD_RS | 0<<LCD_EN | lo_h(p));
	_delay_us(10);
}

//-----------������� ������ ������ � ���----------------
void lcd_dat(unsigned char p)
{
	Set_Interface_Byte(1<<LCD_RS | 1<<LCD_EN | hi_h(p));
	Set_Interface_Byte(1<<LCD_RS | 0<<LCD_EN | hi_h(p));
	_delay_us(10);
	Set_Interface_Byte(1<<LCD_RS | 1<<LCD_EN | lo_h(p));
	Set_Interface_Byte(1<<LCD_RS | 0<<LCD_EN | lo_h(p));
	_delay_us(10);
}

// ����� ������ �� ����� � ������� y=hi(pos), x=lo(pos)
void lcd_out(uint8_t pos, char *str)
{
	if (hi(pos)>0) {
		pos = (pos & 0x0f) + 0x40;
	}
	lcd_com(0x80 + pos);
	_delay_us(200);

	unsigned char i =0;
	while(str[i])
	lcd_dat(str[i++]) ;
}

void lcd_init(void)
{
	lcd_com(0x20);		// ������������� �� 4 ������ ����
	lcd_com(0x20);
	_delay_us(100);
	lcd_com(0x20);
	lcd_com(0x28);		// 4 ����, 2 ������
	lcd_com(0x06);		// ����� ����� �������
	lcd_com(0x0c);		// ��������, ��� �������
	_delay_us(200);
	lcd_com(0x01);		// ������� ������
	_delay_us(200);
}

