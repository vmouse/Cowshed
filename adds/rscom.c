/*
 * rscom.c
 *
 *  Created on: 26-11-2012
 *      Author: rma
 */

#include "rscom.h"
#include "../saf2core.h"

char _rs_buffor;
uint8_t _rs_isAvalible = 0;

void _rs_init() {
	//Set baud rate
	unsigned int ubrr = UBRRVAL;
	_UBRRH = (unsigned char)(ubrr>>8);
	_UBRRL = (unsigned char)ubrr;

	//Enable Transmitter and Receiver and Interrupt on receive complete
	_UCSRB=(1<<_RXCIE)|(1<<_TXCIE)|(0<<_UDRIE)|(1<<_RXEN)|(1<<_TXEN)|(0<<_UCSZ2)|(0<<_RXB8)|(0<<_TXB8);

	//Set data frame format: asynchronous mode,no parity, 1 stop bit, 8 bit size
	//Parzystosc UPM01 =1 (wylaczona 0) UPM00: even = 0 odd = 1
	/*ATMEGA8
	 * 7 - URSEL - Register select
	 * 6 - UMSEL - USART Model select: 0-Asynch, 1-Synch
	 * 5 - UPM1 - 00-disable, 10-Even parity, 11-Odd parity
	 * 4 - UPM0
	 * 3 - USBS - Stop bit 0-1bit, 1-2bit
	 * 2 - UCSZ2 - 000 : 5-bit
	 * 1 - UZSZ1   001 : 6-bit
	 * 0 - UZSZ0   010 : 7-bit
	 * 		       011 : 8-bit
	 * 		       111 : 9-bit
	 */
	_UCSRC=(1<<_URSEL)|(0<<_UMSEL)|(0<<_UPM1)|(0<<_UPM0)|(0<<_USBS)|(1<<_UCSZ1)|(1<<_UCSZ0)|(1<<_UCPOL);


}

void rs_onEvent(saf_Event event) {
	if (event.code == EVENT_RS_SEND) {
		_rs_onTx((char)event.value);
	}
	if (event.code == EVENT_INIT) {
		_rs_init();
	}
}

void rs_sendLine(char* buffer) {
	while(*buffer != 0) {
		_rs_onTx(*buffer);
		buffer++;
	}
	_rs_onTx(EOL);
}

void _rs_onTx(char c) {
	while(!(_UCSRA & (1<<_UDRE)));
	_UDR = c;
}

void _rs_onRx() {
	if (bit_is_clear(_UCSRA, _RXC)) {
		return;
	}
	saf_Event newEvent;
	newEvent.code = EVENT_RS_RECEIVE;
	newEvent.value = _UDR;
	saf_eventBusSend(newEvent);
}

#if defined(__AVR_ATmega168__)
	SIGNAL(USART_TX_vect) {
	}

	SIGNAL(USART_RX_vect) {
		_rs_onRx();
	}
#elif defined(__AVR_ATmega8__)
	SIGNAL(USART_TXC_vect) {
	}

	SIGNAL(USART_RXC_vect) {
		_rs_onRx();
	}
#else
#error "Processor type not supported !"
#endif


