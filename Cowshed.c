/*
 * Cowshed.c
 *
 * Created: 22.04.2014 20:37:33
 *  Author: vlad
 */ 

#define F_CPU 8000000UL
#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include "saf2core.h"
#include "adds/input.h"
#include "timers.h"

#define STATE_INIT	0	// ��������� ���������. ���� ������
#define STATE_WORK	1	// ��������� ���������� ��������
#define STATE_PAUSE	2	// ����� ��������. ����� "������" ��������
#define STATE_ERROR	3	// ������ � ��������. ��������� � ���� ����� 
#define STATE_WAIT	4	// ������� � �������� ��� ������� ��� ������� ����� (���������� ����� �������� wait_mask)
#define STATE_END	5	// ��������� ���������

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

#define CoolStart	(MCUSR & ( 1 << PORF )) // �� ����, ���� ������ ��� ���������� �������

#define NOP asm("nop")

#define Pshift		PB1 // Register strobe (shift clock)
#define Pdata		PB0 // Register data
#define Platch		PB2 // Register out (latch clock)
#define Sensor		PD3 // Sensor multiplexor
#define Ishift		PC5 // Register strobe (shift clock)
#define Idata		PC3 // Register data
#define Ilatch		PC4 // Register out (latch clock)
#define Keyboard	PC2	// Keyboard multiplexor

#define DELAY NOP;
// raise level strobe 010
#define SHIFT_REG DELAY;bit_set(PORTD,BIT(Pshift));DELAY;bit_clear(PORTD,BIT(Pshift));
#define OUT_REG DELAY;bit_set(PORTD,BIT(Platch));DELAY;bit_clear(PORTD,BIT(Platch));
// write data low bit in port
#define write_data_bit(data)	bit_write(data & 1, PORTD, BIT(Pdata)); SHIFT_REG;


uint8_t portdata = 0;			// ��� ������� � �����
uint8_t ind_digit = 0;			// ����� ���������� ����� � ������������ ���������
uint8_t ind_data[] = {~2,~2,~2,~2};  // ��� ����������
uint8_t cmd_index = 0;			// ������ ������� ������� � ���������
uint8_t EEMEM flash_cmd_index = 0;	// ���� ���������� ��������� ������ ������� ������ � ����
uint8_t EEMEM flash_portdata = 0;	// ���������� ��� ������ ����
uint8_t EEMEM flash_state = 0;		// ���������� ���������
uint8_t EEMEM flash_wait_mask = 0;

union { struct
	{
		uint8_t started : 1;	// ����������� ������ ���� ������� �����
		uint8_t waiting	: 1;
		uint8_t end		: 1;
		uint8_t error	: 1;
	} bits;
	uint8_t value;
} state = {.value = 0x00 }; // ��������� �������
	
union { struct
	{
		uint8_t sensor_high : 1;
		uint8_t sensor_low : 1;
		uint8_t reserved : 2;
		uint8_t timer_num : 3;
		uint8_t error_timeout : 1;
	} bits;
	uint8_t value;
} wait_mask = { .value = 0 };


// ����� � 7-���������
const uint8_t sseg[] = { 3,159,37,13,153,73,65,31,1,9,17,193,99,133,97,113,
254, // #16 �����
253  // #17 �����
};

/*
������ ����������:
PC0-k1
PC5-k6
PB0-k7
PB1-k7 ?
*/

typedef struct {
	char	cmd_name;
	uint8_t cmd_data;
//	uint8_t cmd_reserved;
//	uint8_t cmd_indicator;
} _cmd_type;

void SaveFlash(void);

// ������������� ������� ��� ��������
uint16_t TimersArray[] = {0, 30, 30, 5*60, 15*60, 15*60, 11*60, 2*60, 5*60, 5*60};
// 0 - ���� ����� �������� �����
// 1 - 30 ���, ��������� ������� �������� 1
// 2 - 30 ���, ��������� ������� �������� 2
// 3 - 5 ���, �������� ����� ����������
// 4 - 15 ���, ����� ������������ ����� ��������� 1
// 5 - 15 ���, ����� ������������ ����� ��������� 2
// 6 - 11 ���, ����� ��������
// 7 - 2 ���. ���.��������
// 8 - 10 ���, ������������ ����� ���������� ����� 
// 9 - 15 ���, ������������ ����� ����� �����

// Process commands array, cmd style:: Cmd, Arg, Reserved, Indicator
const _cmd_type CmdArray[] = {
/*   {'T', 0x02}, // 0: ������� ������ �� t4 = 15 ���
   {'W', 0x02}, // 1: ��������� ����|����������� �����
   {'P', 0x78}, // 2: ����� ����� ������� �������
   {'T', 0x14}, // 3: ������� ���.������ �� ����������� ������������ �������
   {'W', 0x11}, // 4: ���� ������ (�������) ��� ������������
   {'P', 0x70}, // 5: ���������� ����� � ����
   {'W', 0x02}, // 6: ���� ��������� ��� ������� ����
 */ 


   {'P', 0x2C}, // 00: ���������� ��������� ���� ��� ����������
   {'T', 0x08}, // 01: ������� ������ �� t8 = 10 ���
   {'W', 0x81}, // 02: ���� ���������� ��� ������ ��������� �������
   {'P', 0x70}, // 03: ������ ���������� ���������� ������ �����
   {'T', 0x03}, // 04: ������� ������ �� t3 = 5 ���
   {'W', 0x00}, // 05: ���� ������
   {'P', 0x00}, // 06: ����
   {'T', 0x09}, // 07: ������� ������ �� t9 = 15 ���
   {'W', 0x82}, // 08: ���� ����� ��� ������ ��������� �������
   {'P', 0x29}, // 09: ���������� ��������� ���� ��� ����� 1
   {'C', 0x10}, // 0A: �������� ����� ��� ���������� �������, ��������� ������ 1
   {'T', 0x01}, // 0B: ������� ������ �� t1 = 30 ���
   {'W', 0x01}, // 0C: ���� ���������� ��� T1
   {'P', 0x28}, // 0D: ����.������ ��������
   {'T', 0x08}, // 0E: ������� ������ �� t8 = 10 ���
   {'W', 0x81}, // 0F: ���� ���������� ��� ������ ��������� �������
   {'S', 0x12}, // 10: ���������� ����� ���������� ���� / 2 � ������������ N 0
   {'P', 0x70}, // 11: �����
   {'T', 0x04}, // 12: ������� ������ �� t4 = 15 ���
   {'W', 0x02}, // 13: ��������� ����|����������� �����
   {'I', 0x06}, // 14: ���� ��������� ����� - �������
   {'P', 0x78}, // 15: ����� ����� ������� �������
   {'T', 0x10}, // 16: ������� ���.������ �� ����������� ������������ �������
   {'W', 0x11}, // 17: ���� ������ (�������) ��� ������������
   {'P', 0x70}, // 18: ���������� ����� � ����
   {'W', 0x02}, // 19: ���� ��������� ��� ������� ����
   {'P', 0x50}, // 1A: ����
   {'T', 0x09}, // 1B: ������� ������ �� t9 = 15 ���
   {'W', 0x82}, // 1C: ���� ����� ��� ������ ��������� �������
   {'P', 0x24}, // 1D: ���������� ��������� ���� ��� ���������� - �������� ����
   {'T', 0x08}, // 1E: ������� ������ �� t8 = 10 ���
   {'W', 0x81}, // 1F: ���� ���������� ��� ������ ��������� �������
   {'P', 0x50}, // 20: ���������� + ����
   {'T', 0x09}, // 21: ������� ������ �� t9 = 15 ���
   {'W', 0x82}, // 22: ���� ����� ��� ������ ��������� �������
   {'P', 0x2A}, // 23: ���������� ��������� ���� ��� ����� 2
   {'T', 0x02}, // 24: ������� ������ �� t2 = 30 ���
   {'W', 0x01}, // 25: ���� ���������� ��� T2
   {'P', 0x28}, // 26: ����.������ ��������
   {'T', 0x08}, // 27: ������� ������ �� t8 = 10 ���
   {'W', 0x81}, // 28: ���� ���������� ��� ������ ��������� �������
   {'P', 0x70}, // 29: �����
   {'T', 0x05}, // 2A: ������� ������ �� t5 = 15 ���
   {'W', 0x02}, // 2B: ��������� ����|����������� �����
   {'I', 0x06}, // 2C: ���� ��������� ����� - �������
   {'P', 0x78}, // 2D: ����� ����� ������� �������
   {'T', 0x10}, // 2E: ������� ���.������ �� ����������� ������������ �������
   {'W', 0x11}, // 2F: ���� ������ (�������) ��� ������������
   {'P', 0x70}, // 30: ���������� �����
   {'W', 0x02}, // 31: ���� ���������
   {'P', 0x50}, // 32: ����
   {'T', 0x09}, // 33: ������� ������ �� t9 = 15 ���
   {'W', 0x82}, // 34: ���� ����� ��� ������ ��������� �������
   {'P', 0x24}, // 35: ���������� ��������� ���� ��� ���������� - �������� ����
   {'T', 0x08}, // 36: ������� ������ �� t8 = 10 ���
   {'W', 0x81}, // 37: ���� ���������� ��� ������ ��������� �������
   {'P', 0x50}, // 38: ���������� + ����
   {'T', 0x09}, // 39: ������� ������ �� t9 = 15 ���
   {'W', 0x82}, // 3A: ���� ����� ��� ������ ��������� �������
   {'T', 0x07}, // 3B: ������� ������ �� t7 = 2 ���
   {'W', 0x00}, // 3C: ���� ��� 2 ������
   {'P', 0x00}, // 3D: ��� ��������� � ��������� ����
};  // test port output


void write_data(uint8_t data) {
	for (uint8_t i=0;i<8;i++) {
		write_data_bit(data); data = data >> 1;
	}
}

void Dynamic_Indication(void) {
	write_data((1<<ind_digit)<<4);  // ������� �������
	write_data(ind_data[ind_digit]);  // ������� �����
	OUT_REG;
	ind_digit++;
	if (ind_digit>3) { ind_digit=0; }
}

void OutSignalPort(uint8_t data) {
	PORTC = ~(data & 0b00111111);
	PORTB = ~((data >> 6) & 1);
}

// BCD function from http://homepage.cs.uiowa.edu/~jones/bcd/decimal.html
char* shift_and_mul_utoa16(uint16_t n, uint8_t *buffer, uint8_t zerro_char)
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

	while(buffer[0] == '0') ++buffer;
	return buffer;
}

void ShowData(uint8_t step, uint16_t data) {
	if (data>99) { data = data / 100; } else data = data / 100;

	uint8_t buf[5];
	shift_and_mul_utoa16(data, buf, 0); // bcd convertion
	uint8_t bufind  = 3;
	
	ind_data[2] = sseg[step & 15] & 254 ; step >>= 4;
	ind_data[3] = sseg[step & 15];

	ind_data[1] = sseg[buf[bufind]]; bufind++;
	ind_data[0] = sseg[buf[bufind]]; 
}

void ShowTime(uint8_t step, uint16_t data) {
	if (data>59) { data = data / 59; } else data = data % 60;

	uint8_t buf[5];
	shift_and_mul_utoa16(data, buf, 0); // bcd convertion
	uint8_t bufind  = 3;
	
	ind_data[2] = sseg[step & 15] & 254 ; step >>= 4;
	ind_data[3] = sseg[step & 15];

	ind_data[1] = sseg[buf[bufind]]; bufind++;
	ind_data[0] = sseg[buf[bufind]];
}

void ShowError(uint8_t ErrorClass, uint8_t ErrorCode) {
	timer_stop(-1); // stop all timers
	state.bits.error=1;
	state.bits.started = 0;
	ind_data[0] = sseg[lo(ErrorCode)];
	ind_data[1] = sseg[hi(ErrorCode)];
	ind_data[2] = ~2;
	switch (ErrorClass) {
		case 0: // Execution error
			ind_data[3] = 97;	//E
			break;
		case 1:	// Synax error 
			ind_data[3] = 73;	//S
			break;
		case 2:	// timeout error
			ind_data[3] = 225;	//t
			break;
		default: // other error
			ind_data[3] = sseg[ErrorClass & 0x0f];	//E
			break;
	}
}

void ShowEnd(void) {
	timer_stop(-1); // stop all timers
	ind_data[3] = 97;	//E
	ind_data[2] = 213;	//n
	ind_data[1] = 133;	//d
	ind_data[0] = 254;	//.
}

void ShowMinus(void) {
	ind_data[0]=sseg[17];
	ind_data[1]=sseg[17];
	ind_data[2]=sseg[17]; // �����
	ind_data[3]=sseg[17];
}

void Do_Command(void) {			// ���������� �������� ���������
	if (cmd_index>=sizeof(CmdArray)/sizeof(CmdArray[0])) {
		state.bits.started = 0;
		state.bits.end = 1;
	} else {

	_cmd_type Cmd = CmdArray[cmd_index];

	ShowData(cmd_index, Cmd.cmd_data);

	uint16_t time;

	switch (Cmd.cmd_name) {
		case 'P':	// ����� � ����
			OutSignalPort(Cmd.cmd_data);
			break;
		case 'T':	// ������������� �������
			timer_setup(hi(Cmd.cmd_data), TimersArray[lo(Cmd.cmd_data)]);
			timer_start(hi(Cmd.cmd_data));
			break;
		case 'W':  // �������� ������� ��� �����
			wait_mask.value = Cmd.cmd_data;
			state.bits.waiting = 1;
			// ��� ���� ����� - ������ ����� ��� � ���������, � ����� ������ � ����� � ����� ��� ���������
			if (timer_iswork(hi(Cmd.cmd_data) & 7) == 0) { // ������ �����
				if (timer_get(hi(Cmd.cmd_data) & 7) >0) { // ������ � ����� (�.�. �����, �� �� �������� �� ����)
					timer_start(hi(Cmd.cmd_data) & 7); // ������� �� �����
					state.bits.waiting = 1;
				} else { // ������ ����� � �������� �� ���� - ���������� ��������� �������
					saf_Event newEvent;
					newEvent.value = hi(Cmd.cmd_data) & 7;
					newEvent.code = EVENT_TIMER_END;
					saf_eventBusSend(newEvent);				
				}
			}
			break;
		case 'G': // ������� �� ��������� �����
			if ((Cmd.cmd_data & 0x80) == 0) {
				cmd_index+=Cmd.cmd_data-1;
			} else	cmd_index-=(Cmd.cmd_data & 0x7f)+1;
			ShowError(3,cmd_index);
			break;			
		case 'I': // ������� �� ��������� ����� ������ ���� ������ �������� �� ����
			if (timer_get(hi(Cmd.cmd_data)) == 0) {
				cmd_index+=lo(Cmd.cmd_data)-1;
			}															
			break;
		case 'C':  // ����� ������� �������
			timer_setup(hi(Cmd.cmd_data), 0);
			timer_start(hi(Cmd.cmd_data));
			break;
		case 'S':  // ���������� ������� �������
			time = timer_get(hi(Cmd.cmd_data));
			if ((Cmd.cmd_data & 0b00001000)==0) {
				time = 0-time;
			}
			if ((Cmd.cmd_data & 0b00000111) > 1) {
				time = time / (Cmd.cmd_data & 0b00000111);
			}
			timer_stop(hi(Cmd.cmd_data));
			TimersArray[0] = time;
			break;
		default: // ����������� ��������
			ShowError(Cmd.cmd_data, cmd_index);
			break;	
	}
	cmd_index++;
//	SaveFlash();
	}
}

void onEvent(saf_Event event)
{
	if (event.code == EVENT_IN_DOWN)
	{
		switch (event.value) {
			case 0: // start button
				if (state.bits.started == 0) {
					cmd_index = 0;
					state.value = 1; // ��� ����� ����������, ����� ������
					
				}
				break;
			case 1: // high level sensor
				if ((wait_mask.bits.sensor_high !=0) && (state.bits.waiting == 1)) { 
					state.bits.waiting = 0; 
					timer_stop(wait_mask.bits.timer_num);
				}
				break;
			case 2: // low level sensor
				if ((wait_mask.bits.sensor_low !=0) && (state.bits.waiting == 1)) { 
					state.bits.waiting = 0;
					timer_stop(wait_mask.bits.timer_num);
				}
			break;
			default:
				break;	
		}
	} else if (event.code == EVENT_TIMER_TICK) // ���������� �������� ������� 0
	{	
		if (timer_iswork(0)==1) {
			ShowTime(cmd_index-1, timer_get(0));
		} else 	ShowTime(cmd_index-1, timer_get(1));
	} else if (event.code == EVENT_TIMER_END) // ������ ��������
	{
		if (state.bits.waiting == 1) {
			if (event.value == wait_mask.bits.timer_num) // �������� ������, �������� ����
			{
				if ( wait_mask.bits.error_timeout !=0 ) { // ������ ������ ������� ������
					ShowError(2, cmd_index-1);
				} else {
					state.bits.waiting = 0;
				}
			}
		}
	} 
}

void	ResetState(void) {
	state.value = 0;
	wait_mask.value = 0;
	cmd_index = 0;
	ShowMinus();
	OutSignalPort(0);
}

void SaveFlash(void) {
	flash_cmd_index = cmd_index;
	flash_portdata = portdata;
	flash_state = state.value;
	flash_wait_mask = wait_mask.value;
}

void RestFlash(void) {
	cmd_index = flash_cmd_index;
	portdata = flash_portdata;
	state.value = flash_state;
	wait_mask.value = flash_wait_mask;
}

int main(void)
{

	DDRD = 0xff;
	DDRC = 1<<Ishift|1<<Idata|1<<Ilatch;
	DDRB = 1<<Pshift|1<<Pdata|1<<Platch;
	PORTC = 0xff;
	PORTB = 0xff;


	ResetState();


	// Simple AVR framework
	saf_init();
	timers_init();

	input_add(_D, 7); // Start button
	input_add(_D, 2); // Liquid high level sensor
	input_add(_D, 3); // Liquid Low level sensor
	
	saf_addEventHandler(onEvent);
	saf_addEventHandler(input_onEvent);
	saf_addEventHandler(timer_onEvent);

	sei();

/*
	if (CoolStart != 0) {
		RestFlash();
		if (state.bits.end !=0 ) { // ����������� ��������� ���������
			ResetState();
		}
	}
*/	
    while(1)
    {
		saf_process();
		
		if (state.bits.end) {
			ShowEnd(); 
		} else if (state.bits.waiting == 0 && state.bits.started == 1) {
			Do_Command();
		}

		Dynamic_Indication();
    }
}


