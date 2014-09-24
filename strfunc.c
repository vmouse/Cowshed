/*
 * strfunc.c
 *
 * Created: 22.09.2014 17:55:19
 *  Author: vlad
 */ 
#include "strfunc.h"
#include "lcd.h"

// BCD function from http://homepage.cs.uiowa.edu/~jones/bcd/decimal.html
// Need buffer size >=6
char* shift_and_mul_utoa16(uint16_t n, char *buffer, char zerro_char)
{
	uint8_t d4, d3, d2, d1, q, d0;

	d1 = (n>>4)  & 0xF;
	d2 = (n>>8)  & 0xF;
	d3 = (n>>12) & 0xF;

	d0 = 6*(d3 + d2 + d1) + (n & 0xF);
	q = (d0 * 0xCD) >> 11;
	d0 = d0 - 10*q;

	d1 = q + 9*d3 + 5*d2 + d1;
	q = (d1 * 0xCD) >> 11;
	d1 = d1 - 10*q;

	d2 = q + 2*d2;
	q = (d2 * 0x1A) >> 8;
	d2 = d2 - 10*q;

	d3 = q + 4*d3;
	d4 = (d3 * 0x1A) >> 8;
	d3 = d3 - 10*d4;

	char *ptr = buffer;
	*ptr++ = ( d4 + zerro_char );
	*ptr++ = ( d3 + zerro_char );
	*ptr++ = ( d2 + zerro_char );
	*ptr++ = ( d1 + zerro_char );
	*ptr++ = ( d0 + zerro_char );
	*ptr = 0;

	//	while(buffer[0] == '0') ++buffer;
	return buffer;
}


char* IntToBitsStr(uint8_t n, char buffer[], char clear_bit_char, char set_bit_char) {
	char *buf = buffer;
	for (int i=0; i<8; i++) {
		if ((n & 0x80) == 0) {
			*buf = clear_bit_char;
			} else {
			*buf = set_bit_char;
		}
		n = n << 1; buf++;
	}
	*buf = 0;
	return buffer;
}


uint8_t Hex2Int(char str[]) {
	uint8_t num1 = str[0]-'0';
	if (num1 > 9) { num1 -= 7; }
	num1 = num1 << 4;
	uint8_t num2 = str[1]-'0';
	if (num2 > 9) { num2 -= 7; }
	return num1 + num2;
}

uint8_t Dec2Int(char str[]) {
	return (str[0]-'0') * 10 + str[1]-'0';
}

uint8_t BitsToInt(char *str, char set_bit_char) {
	uint8_t res = 0;
	while (*str != 0) {
		res = res << 1;
		if ((char)*str == set_bit_char) {
			res |= 1;
		}
		str++;
	}
	return res;
}

char* SecondsToTimeStr(uint16_t n, char buffer[]) {
	char buf[6];
	char *ptr = buffer;
	uint16_t tmp = n / 3600;
	shift_and_mul_utoa16(tmp, buf, '0');
	*ptr++ = buf[3];
	*ptr++ = buf[4];
	*ptr++ = ':';
	n -= tmp * 3600;
	tmp = n / 60;
	lcd_pos(n);
	shift_and_mul_utoa16(tmp, buf, '0');
	*ptr++ = buf[3];
	*ptr++ = buf[4];
	*ptr++ = ':';
	n -= tmp * 60;
	shift_and_mul_utoa16(n, buf, '0');
	*ptr++ = buf[3];
	*ptr++ = buf[4];
	*ptr++ = 0;
	return buffer;
}