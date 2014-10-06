/*
 * strfunc.h
 *
 * Created: 22.09.2014 17:56:01
 *  Author: vlad
 */ 
#ifndef STRFUNC_H_
#define STRFUNC_H_
#include <stdint.h>
#include "strfunc.h"

char* shift_and_mul_utoa16(uint16_t n, char *buffer, char zerro_char); 
char* IntToBitsStr(uint8_t n, char buffer[], char clear_bit_char, char set_bit_char);
uint8_t Dec2Int(char str[]);
uint8_t Hex2Int(char str[]);
uint8_t BitsToInt(char *str, char set_bit_char);
char* SecondsToTimeStr(uint16_t n, char buffer[]);

#endif /* STRFUNC_H_ */