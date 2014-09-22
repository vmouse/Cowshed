/*
 * strfunc.h
 *
 * Created: 22.09.2014 17:56:01
 *  Author: vlad
 */ 


#ifndef STRFUNC_H_
#define STRFUNC_H_


/*
 * strfunc.c
 *
 * Created: 22.09.2014 17:55:19
 *  Author: vlad
 */ 
#include <stdint.h>
#include "strfunc.h"

char* IntToBitsStr(uint8_t n, char buffer[], char clear_bit_char, char set_bit_char);
uint8_t Hex2Int(char str[]);
uint8_t BitsToInt(char *str, char set_bit_char);

#endif /* STRFUNC_H_ */