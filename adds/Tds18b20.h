/*
	orginal od Franciszek Mikłaszewicz 31.03.2006
	(zmieniona przez radomir.mazon)
*/
#ifndef TDS18B20_H_
#define TDS18B20_H_

#include <inttypes.h>
#include <avr/io.h>
#include <util/delay.h>
#include <util/crc16.h>
#include <stdlib.h>
#include <string.h>
#include <avr/interrupt.h>
#include "../saf2core.h"

//--------------- setup
//musi to byc port typu C: domyslnie PC4
#define PINPORT 		PC4
//---------------------

void _ds_init();
int8_t _tds18b20_resetPresence();
void _ds_waitU(uint16_t time);
void _ds_writeBit(uint8_t bicik);
void _ds_writeByte(uint8_t bajt);
uint8_t _ds_readBit();
uint8_t _ds_readByte();
/*
	Default 3;
	0	-	9bit
	1	-	10bit
	2	-	11bit
	3	-	12bit
*/
//void ds_setResolution(uint8_t res);
int16_t ds_getTemp();
char* ds_tempToAscii(int16_t temp, char* buffor);	//zwrocenie zera oznacza blad


//sprawdzanie CRC. Jesli zwroci zero, to dane najprawdopodobniej poprawne
uint8_t _checkCRC(uint8_t *tab,int tab_size);

//SAF support
void ds_onEvent(saf_Event e);

#endif /* TDS18B20_H_ */
