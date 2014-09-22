/*
 * strfunc.c
 *
 * Created: 22.09.2014 17:55:19
 *  Author: vlad
 */ 
#include "strfunc.h"

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
