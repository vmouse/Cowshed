/*
 * bits.h
 *
 * Created: 23.08.2014 1:15:49
 *  Author: vlad
 */ 


#ifndef BITS_H_
#define BITS_H_

// bit operations
#define bit_get(p,m) ((p) & (m))
#define bit_set(p,m) ((p) |= (m))
#define bit_clear(p,m) ((p) &= ~(m))
#define bit_flip(p,m) ((p) ^= (m))
#define bit_write(c,p,m) (c ? bit_set(p,m) : bit_clear(p,m))
#define not_bit_write(c,p,m) (c ? bit_clear(p,m) : bit_set(p,m))
#define BIT(x) (0x01 << (x))

#define hi(data)	(data>>4)
#define lo(data)	(data & 0x0f)
#define hi_h(data)	(data & 0xf0)
#define lo_h(data)	(data << 4)

#define NOP asm("nop")
#define DELAY NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;
//#define DELAY _delay_us(20);

#define STROBE(OutPort,StrobePin) DELAY;bit_set(OutPort,BIT(StrobePin));DELAY;bit_clear(OutPort,BIT(StrobePin));
#define SENDBIT(OutPort,DataPin,data)	bit_write(data & 0x80, OutPort, BIT(DataPin));

#endif /* BITS_H_ */