/*
 * rscom.h
 *
 *  Created on: 26-11-2012
 *      Author: rma
 */

#ifndef RSCOM_H_
#define RSCOM_H_

#define BAUDRATE 28800
#define UBRRVAL ((F_CPU/(BAUDRATE*16UL))-1)
#define EOL	((char)13)

#include "../saf2core.h"

#if defined(EVENT_RS_SEND)
#else
#error "EVENT_RS_SEND and EVENT_RS_RECEIVE are required";
#endif

#if defined(EVENT_RS_RECEIVE)
#else
#error "EVENT_RS_SEND and EVENT_RS_RECEIVE are required";
#endif

#if defined(__AVR_ATmega168__)

#define _UBRRH UBRR0H
#define _UBRRL UBRR0L

#define _UDR UDR0

#define _UCSRA UCSR0A
#define _RXC RXC0
#define _TXC TXC0
#define _UDRE UDRE0
#define _FE FE0
#define _DOR DOR0
#define _PE PE0
#define _U2X U2X0
#define _MPCM MPCM0

#define _UCSRB UCSR0B
#define _RXCIE RXCIE0
#define _TXCIE TXCIE0
#define _UDRIE UDRIE0
#define _RXEN RXEN0
#define _TXEN TXEN0
#define _UCSZ2 UCSZ02
#define _RXB8 RXB80
#define _TXB8 TXB80

#define _UCSRC UCSR0C
#define _URSEL UMSEL01
#define _UMSEL UMSEL00
#define _UPM1 UPM01
#define _UPM0 UPM00
#define _USBS USBS0
#define _UCSZ1 UCSZ01
#define _UCSZ0 UCSZ00
#define _UCPOL UCPOL0

#elif defined(__AVR_ATmega8__)

#define _UBRRH UBRRH
#define _UBRRL UBRRL

#define _UDR UDR

#define _UCSRA UCSRA
#define _RXC RXC
#define _TXC TXC
#define _UDRE UDRE
#define _FE FE
#define _DOR DOR
#define _PE PE
#define _U2X U2X
#define _MPCM MPCM

#define _UCSRB UCSRB
#define _RXCIE RXCIE
#define _TXCIE TXCIE
#define _UDRIE UDRIE
#define _RXEN RXEN
#define _TXEN TXEN
#define _UCSZ2 UCSZ2
#define _RXB8 RXB8
#define _TXB8 TXB8

#define _UCSRC UCSRC
#define _URSEL URSEL
#define _UMSEL UMSEL
#define _UPM1 UPM1
#define _UPM0 UPM0
#define _USBS USBS
#define _UCSZ1 UCSZ1
#define _UCSZ0 UCSZ0
#define _UCPOL UCPOL

#else
//
// unsupported type
//
#error "Processor type not supported !"
#endif


void rs_onEvent(saf_Event event);
void rs_sendLine(char* buffer);
void _rs_init();
void _rs_onTx(char c);
void _rs_onRx();

#endif /* RSCOM_H_ */
