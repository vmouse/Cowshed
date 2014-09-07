//  ---------------------------------------------------------------------------//  DS1307 RTC ROUTINES
#include "ds1307.h"
#include "lcd.h"

void DS1307_Init() {
	I2C_WriteRegister(DS1307,CONTROL_REGISTER, 0x10); // set 1Hz frequence into SQW pin
}

void DS1307_GetTime(uint8_t *hours, uint8_t *minutes, uint8_t *seconds)
// returns hours, minutes, and seconds in BCD format
{
	*hours = I2C_ReadRegister(DS1307,HOURS_REGISTER);
	*minutes = I2C_ReadRegister(DS1307,MINUTES_REGISTER);
	*seconds = I2C_ReadRegister(DS1307,SECONDS_REGISTER);
	if (*hours & 0x40)  // 12hr mode:
	*hours &= 0x1F;    // use bottom 5 bits (pm bit = temp & 0x20)
	else *hours &= 0x3F;  // 24hr mode: use bottom 6 bits
}

void DS1307_GetDate(uint8_t *months, uint8_t *days, uint8_t *years)
// returns months, days, and years in BCD format
{
	*months = I2C_ReadRegister(DS1307,MONTHS_REGISTER);
	*days = I2C_ReadRegister(DS1307,DAYS_REGISTER);
	*years = I2C_ReadRegister(DS1307,YEARS_REGISTER);
}

// data in BCD format 0x14, 0x9, 0x5, 0x11, 0x37 => 2014-09-05 11:37
void SetTimeDate(uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min)
// simple, hard-coded way to set the date.
{
	I2C_WriteRegister(DS1307,MONTHS_REGISTER,  month);
	I2C_WriteRegister(DS1307,DAYS_REGISTER,  day);
	I2C_WriteRegister(DS1307,YEARS_REGISTER,  year);
	I2C_WriteRegister(DS1307,HOURS_REGISTER, hour);		// 0x08+0x40);  // add 0x40 for PM
	I2C_WriteRegister(DS1307,MINUTES_REGISTER, min);
	I2C_WriteRegister(DS1307,SECONDS_REGISTER, 0x00);
}

void DS1307_Save8(uint8_t addr, uint8_t data) {
	addr+=RAM_BEGIN;
	if (addr<=RAM_END) {
		I2C_WriteRegister(DS1307, addr,  data);
	}
}

void DS1307_Save16(uint8_t addr, uint16_t data) {
	addr+=RAM_BEGIN;
	if (addr<RAM_END) {
		I2C_WriteRegister(DS1307, addr,  data & 0xff);
		I2C_WriteRegister(DS1307, addr,  data >> 8);
	}
}

void ShowDevices(void)
// Scan I2C addresses and display addresses of all devices found
{
	lcd_clear(); lcd_out("Found: ");
	uint8_t addr = 1;
	while (addr>0)
	{
		addr = I2C_FindDevice(addr);
		if (addr>0) lcd_hex(addr++);
	}
}

void LCD_TwoDigits(uint8_t data)
// helper function for WriteDate()
// input is two digits in BCD format
// output is to LCD display at current cursor position
{
	uint8_t temp = data>>4;
	lcd_dat(temp+'0');
	data &= 0x0F;
	lcd_dat(data+'0');
}

void LCD_Date(void)
{
	uint8_t months, days, years;
	DS1307_GetDate(&months,&days,&years);
	LCD_TwoDigits(days);
	lcd_dat('.');
	LCD_TwoDigits(months);
	lcd_out(".20");
	LCD_TwoDigits(years);
}

void LCD_Time(void)
{
	uint8_t hours, minutes, seconds;
	DS1307_GetTime(&hours,&minutes,&seconds);
	LCD_TwoDigits(hours);
	lcd_dat(':');
	LCD_TwoDigits(minutes);
	lcd_dat(':');
	LCD_TwoDigits(seconds);
}

void LCD_TimeDate(void)
{
	lcd_pos(0x03); LCD_Date();
	lcd_pos(0x14);  LCD_Time();
}

