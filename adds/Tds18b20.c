#include "Tds18b20.h"

void _ds_init()
{
	PORTC |= _BV(PINPORT);
	DDRC |= _BV(PINPORT);		//mozliwosc pisania
}

void _ds_waitU(uint16_t time)
{
	uint16_t index;
	for (index=0; index<time; index++)
		_delay_us(1);
}

int8_t _ds_resetPresence()
{
	int8_t result=0;
	
	PORTC &= ~_BV(PINPORT);	//zerowy reset 500us
	_ds_waitU(600);
	
	//przejscie na czytanie po podpieciu pull-up

	PORTC |= _BV(PINPORT);
	DDRC &= ~_BV(PINPORT);
	_ds_waitU(15);
	uint16_t counter = 285;
	do
	{
		counter--;
		_delay_us(1);
	}
	while ( ( PINC & _BV(PINPORT) ) && counter>0 );
	if ( counter == 0 )
	{
		result = -1;	//time's out,
	}
	else
	{
		//czekamy dopoki na lini jest 0
		counter=0;
		while (!( PINC & _BV(PINPORT) ))
		{
			counter++;
			_ds_waitU(1);
		}
	}
	_ds_waitU(480-counter);
	return result;
}

void _ds_writeBit(uint8_t bicik)
{

	DDRC |= _BV(PINPORT);		//ustawienie zero
	PORTC &= ~_BV(PINPORT);
	if ( bicik )
	{
		_delay_us(5);
		PORTC |= _BV(PINPORT);
		DDRC &= ~_BV(PINPORT);
		_delay_us(74);
	}
	else
	{
		_delay_us(15+15+30+15);
		PORTC |= _BV(PINPORT);
		DDRC &= ~_BV(PINPORT);
	}
	
}

void _ds_writeByte(uint8_t bajt)
{
	
	int8_t index;
	for (index=0;index<8;index++)
	{
		_delay_us(5);
		_ds_writeBit( bajt & (1<<index) );
	}
	_delay_us(3);
}

uint8_t _ds_readBit()
{
	uint8_t result=0;
	//wyzerowanie
	DDRC |= _BV(PINPORT);
	PORTC &= ~_BV(PINPORT);
	_delay_us(5);
	//albo odwrotnie aby najpierw zaczac czytac nastepnie wrzucic jedynke
	PORTC |= _BV(PINPORT);	//ustawienie pullup	
	DDRC &= ~_BV(PINPORT);	////ustawienie IN z pullup
	
	uint8_t i=70;
	do
	{
		if (!( PINC & _BV(PINPORT) ))	//jesli wartosc zero to koncz petle
		{
			while (!( PINC & _BV(PINPORT) ));
			result = 0;
			break;
		}
		i--;
		_delay_us(1);		
	}
	while (i>0);
	if ( i == 0 )
		result = 1;
 	_delay_us(i);
	return result;
}

uint8_t _ds_readByte()
{
	uint8_t result=0;
	int8_t index;
	for(index=0;index<8;index++)
	{
		_delay_us(5);
		result |= _ds_readBit()<<index;
	}
	_delay_us(3);	
	return result;
}

/*void ds_setResolution(uint8_t res)
{
	//SetResolution
	PORTC |= _BV(PINPORT);
	DDRC &= ~_BV(PINPORT);
	_ds_waitU(250);

	_ds_resetPresence();
	//	skip rom
	_ds_writeByte(0xCC);
	//	komenda 0x4E 	Write Scratchpad
	_ds_writeByte(0x4E);
	//alarmy TH i TL ustawione na zero
	_ds_writeByte(0x12);
	_ds_writeByte(0x34);
	//ustawienie resolution
	_ds_writeByte(0x1F | ((res & 0x3)<<5));
	_ds_resetPresence();
	//skip rom
	_ds_writeByte(0xCC);
	//	komenda 0x48	Copy Scratchpad
	_ds_writeByte(0x48);
	_delay_ms(10);
	while ( !_ds_readBit() );	//dopoki linia jest low to trwa kopiowanie
	_ds_resetPresence();

	//Skonczenie procedury SetResolution
}
*/
int16_t ds_getTemp()
{
	cli();
	if (_ds_resetPresence()==-1)
	{
		sei();
		return 0xff92;
	}
	//	Skip ROM
	_ds_writeByte(0xCC);
	//	Convert T
	_ds_writeByte(0x44);
	while( !_ds_readBit() );	//dopoki linia jest low to trwa konwersja
	_ds_resetPresence();
	//	Skip ROM
	_ds_writeByte(0xCC);
	//	Read Scratchpad
	_ds_writeByte(0xBE);
	uint8_t scratchpad[9];
	uint8_t index;
	for (index=0;index<9;index++) {
		scratchpad[index] = _ds_readByte();
	}

	_ds_resetPresence();
	//	sprawdzenie CRC
	if (_checkCRC(scratchpad,9) != 0)
	{
		sei();
		return 0xff92;
	}
	int16_t result;
	uint8_t *tab = (uint8_t*)&result;
	tab[0] = scratchpad[0];
	tab[1] = scratchpad[1];
    //Skonczenie czytania temp
	sei();
	return result;
}

uint8_t _checkCRC(uint8_t *tab,int tab_size)
{
	uint8_t crc = 0, i;
	for (i = 0; i < tab_size; i++)
		crc = _crc_ibutton_update(crc, tab[i]);
	return crc; // must be 0
}

char* ds_tempToAscii(int16_t temp, char* buffor)
{
	strcpy(buffor, "??,?\xdf""C  ");

	if (temp == 0xff92) {
		return buffor;
	}
	itoa(temp/2, buffor, 10);
	if (temp%2!=0) {
		strcat(buffor, ",5\xdf""C ");
	} else {
		strcat(buffor, ",0\xdf""C ");
	}
	if (temp == 0xffff) {
		char buff[10];
		strcpy(buff, buffor);
		strcpy(buffor+1, buff);
		buffor[0] = '-';
	}
	return buffor;
}

void ds_onEvent(saf_Event e) {
	if (e.code == EVENT_INIT) {
		_ds_init();
	}
}
