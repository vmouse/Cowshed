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

#define STATE_INIT	0	// начальное состояние. ждем команд
#define STATE_WORK	1	// состояние выполнение процесса
#define STATE_PAUSE	2	// пауза процесса. выход "ручным" способом
#define STATE_ERROR	3	// ошибка в процессе. остановка в этом месте 
#define STATE_WAIT	4	// процесс в ожидании или таймера или события порта (использует маску ожидания wait_mask)
#define STATE_END	5	// программа завершена

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

#define CoolStart	(MCUSR & ( 1 << PORF )) // не ноль, если запуск был включением питания

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


uint8_t portdata = 0;			// что выводим в порты
uint8_t ind_digit = 0;			// номер выводимого знака в динамической индикации
uint8_t ind_data[] = {~2,~2,~2,~2};  // что отображать
uint8_t cmd_index = 0;			// индекс текущей команды в программе
uint8_t EEMEM flash_cmd_index = 0;	// сюда запоминаем последний индекс команды записи в порт
uint8_t EEMEM flash_portdata = 0;	// запоминаем что вывели порт
uint8_t EEMEM flash_state = 0;		// запоминаем состояние
uint8_t EEMEM flash_wait_mask = 0;

union { struct
	{
		uint8_t started : 1;	// обязательно должен быть младшим битом
		uint8_t waiting	: 1;
		uint8_t end		: 1;
		uint8_t error	: 1;
	} bits;
	uint8_t value;
} state = {.value = 0x00 }; // состояние системы
	
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


// цифра в 7-сегментов
const uint8_t sseg[] = { 3,159,37,13,153,73,65,31,1,9,17,193,99,133,97,113,
254, // #16 точка
253  // #17 минус
};

/*
Выходы управления:
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

// Фиксированные периоды для таймеров
uint16_t TimersArray[] = {0, 30, 30, 5*60, 15*60, 15*60, 11*60, 2*60, 5*60, 5*60};
// 0 - сюда будем замерять время
// 1 - 30 сек, перекачка моющего раствора 1
// 2 - 30 сек, перекачка моющего раствора 2
// 3 - 5 мин, вакуумый насос полоскания
// 4 - 15 мин, время обязательной мойки раствором 1
// 5 - 15 мин, время обязательной мойки раствором 2
// 6 - 11 мин, время просушки
// 7 - 2 мин. доп.просушка
// 8 - 10 мин, максимальное время заполнения танка 
// 9 - 15 мин, максимальное время слива танка

// Process commands array, cmd style:: Cmd, Arg, Reserved, Indicator
const _cmd_type CmdArray[] = {
/*   {'T', 0x02}, // 0: Взводим таймер на t4 = 15 мин
   {'W', 0x02}, // 1: Кончилась вода|Закончилась мойка
   {'P', 0x78}, // 2: Иначе долив полбака горячей
   {'T', 0x14}, // 3: Взводим доп.таймер на посчитанную длительность полбака
   {'W', 0x11}, // 4: Ждем таймер (полбака) или переполнение
   {'P', 0x70}, // 5: Продолжаем мойку и слив
   {'W', 0x02}, // 6: Ждем окончания или пустого бака
 */ 


   {'P', 0x2C}, // 00: Наполнение основного бака для полоскания
   {'T', 0x08}, // 01: Взводим таймер на t8 = 10 мин
   {'W', 0x81}, // 02: Ждем наполнения или ошибки окончания таймера
   {'P', 0x70}, // 03: начать полоскание достаточно теплой водой
   {'T', 0x03}, // 04: Взводим таймер на t3 = 5 мин
   {'W', 0x00}, // 05: Ждем таймер
   {'P', 0x00}, // 06: Слив
   {'T', 0x09}, // 07: Взводим таймер на t9 = 15 мин
   {'W', 0x82}, // 08: Ждем слива или ошибки окончания таймера
   {'P', 0x29}, // 09: Наполнение основного бака для мойки 1
   {'C', 0x10}, // 0A: Засекаем время для расчетного периода, используя таймер 1
   {'T', 0x01}, // 0B: Взводим таймер на t1 = 30 сек
   {'W', 0x01}, // 0C: Ждем наполнения или T1
   {'P', 0x28}, // 0D: Откл.моющую жидкость
   {'T', 0x08}, // 0E: Взводим таймер на t8 = 10 мин
   {'W', 0x81}, // 0F: Ждем наполнения или ошибки окончания таймера
   {'S', 0x12}, // 10: Запоминаем время заполнения бака / 2 в длительность N 0
   {'P', 0x70}, // 11: Мойка
   {'T', 0x04}, // 12: Взводим таймер на t4 = 15 мин
   {'W', 0x02}, // 13: Кончилась вода|Закончилась мойка
   {'I', 0x06}, // 14: Если кончилась мойка - переход
   {'P', 0x78}, // 15: Иначе долив полбака горячей
   {'T', 0x10}, // 16: Взводим доп.таймер на посчитанную длительность полбака
   {'W', 0x11}, // 17: Ждем таймер (полбака) или переполнение
   {'P', 0x70}, // 18: Продолжаем мойку и слив
   {'W', 0x02}, // 19: Ждем окончания или пустого бака
   {'P', 0x50}, // 1A: Слив
   {'T', 0x09}, // 1B: Взводим таймер на t9 = 15 мин
   {'W', 0x82}, // 1C: Ждем слива или ошибки окончания таймера
   {'P', 0x24}, // 1D: Наполнение основного бака для полоскания - холодная вода
   {'T', 0x08}, // 1E: Взводим таймер на t8 = 10 мин
   {'W', 0x81}, // 1F: Ждем наполнения или ошибки окончания таймера
   {'P', 0x50}, // 20: Полоскание + слив
   {'T', 0x09}, // 21: Взводим таймер на t9 = 15 мин
   {'W', 0x82}, // 22: Ждем слива или ошибки окончания таймера
   {'P', 0x2A}, // 23: Наполнение основного бака для мойки 2
   {'T', 0x02}, // 24: Взводим таймер на t2 = 30 сек
   {'W', 0x01}, // 25: Ждем наполнения или T2
   {'P', 0x28}, // 26: Откл.моющую жидкость
   {'T', 0x08}, // 27: Взводим таймер на t8 = 10 мин
   {'W', 0x81}, // 28: Ждем наполнения или ошибки окончания таймера
   {'P', 0x70}, // 29: Мойка
   {'T', 0x05}, // 2A: Взводим таймер на t5 = 15 мин
   {'W', 0x02}, // 2B: Кончилась вода|Закончилась мойка
   {'I', 0x06}, // 2C: Если кончилась мойка - переход
   {'P', 0x78}, // 2D: Иначе долив полбака горячей
   {'T', 0x10}, // 2E: Взводим доп.таймер на посчитанную длительность полбака
   {'W', 0x11}, // 2F: Ждем таймер (полбака) или переполнение
   {'P', 0x70}, // 30: Продолжаем мойку
   {'W', 0x02}, // 31: Ждем окончания
   {'P', 0x50}, // 32: Слив
   {'T', 0x09}, // 33: Взводим таймер на t9 = 15 мин
   {'W', 0x82}, // 34: Ждем слива или ошибки окончания таймера
   {'P', 0x24}, // 35: Наполнение основного бака для полоскания - холодная вода
   {'T', 0x08}, // 36: Взводим таймер на t8 = 10 мин
   {'W', 0x81}, // 37: Ждем наполнения или ошибки окончания таймера
   {'P', 0x50}, // 38: Полоскание + слив
   {'T', 0x09}, // 39: Взводим таймер на t9 = 15 мин
   {'W', 0x82}, // 3A: Ждем слива или ошибки окончания таймера
   {'T', 0x07}, // 3B: Взводим таймер на t7 = 2 мин
   {'W', 0x00}, // 3C: Ждем еще 2 минуты
   {'P', 0x00}, // 3D: Все отключаем и оставляем слив
};  // test port output


void write_data(uint8_t data) {
	for (uint8_t i=0;i<8;i++) {
		write_data_bit(data); data = data >> 1;
	}
}

void Dynamic_Indication(void) {
	write_data((1<<ind_digit)<<4);  // выбрать сегмент
	write_data(ind_data[ind_digit]);  // вывести число
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
	ind_data[2]=sseg[17]; // минус
	ind_data[3]=sseg[17];
}

void Do_Command(void) {			// выполнение комманды программы
	if (cmd_index>=sizeof(CmdArray)/sizeof(CmdArray[0])) {
		state.bits.started = 0;
		state.bits.end = 1;
	} else {

	_cmd_type Cmd = CmdArray[cmd_index];

	ShowData(cmd_index, Cmd.cmd_data);

	uint16_t time;

	switch (Cmd.cmd_name) {
		case 'P':	// вывод в порт
			OutSignalPort(Cmd.cmd_data);
			break;
		case 'T':	// инициализация таймера
			timer_setup(hi(Cmd.cmd_data), TimersArray[lo(Cmd.cmd_data)]);
			timer_start(hi(Cmd.cmd_data));
			break;
		case 'W':  // ожидание таймера или порта
			wait_mask.value = Cmd.cmd_data;
			state.bits.waiting = 1;
			// тут есть фокус - таймер может уже и закончить, а может стоять в паузе и нужно это проверить
			if (timer_iswork(hi(Cmd.cmd_data) & 7) == 0) { // таймер стоит
				if (timer_get(hi(Cmd.cmd_data) & 7) >0) { // таймер в паузе (т.к. стоит, но не досчитал до нуля)
					timer_start(hi(Cmd.cmd_data) & 7); // выходим из паузы
					state.bits.waiting = 1;
				} else { // таймер стоит и досчитал до нуля - генерируем повторный таймаут
					saf_Event newEvent;
					newEvent.value = hi(Cmd.cmd_data) & 7;
					newEvent.code = EVENT_TIMER_END;
					saf_eventBusSend(newEvent);				
				}
			}
			break;
		case 'G': // переход на несколько шагов
			if ((Cmd.cmd_data & 0x80) == 0) {
				cmd_index+=Cmd.cmd_data-1;
			} else	cmd_index-=(Cmd.cmd_data & 0x7f)+1;
			ShowError(3,cmd_index);
			break;			
		case 'I': // переход на несколько шагов вперед если таймер досчитал до нуля
			if (timer_get(hi(Cmd.cmd_data)) == 0) {
				cmd_index+=lo(Cmd.cmd_data)-1;
			}															
			break;
		case 'C':  // замер времени таймера
			timer_setup(hi(Cmd.cmd_data), 0);
			timer_start(hi(Cmd.cmd_data));
			break;
		case 'S':  // сохранение времени таймера
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
		default: // неизвестная комманда
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
					state.value = 1; // все флаги сбрасываем, кроме старта
					
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
	} else if (event.code == EVENT_TIMER_TICK) // отображаем прогресс таймера 0
	{	
		if (timer_iswork(0)==1) {
			ShowTime(cmd_index-1, timer_get(0));
		} else 	ShowTime(cmd_index-1, timer_get(1));
	} else if (event.code == EVENT_TIMER_END) // таймер закончил
	{
		if (state.bits.waiting == 1) {
			if (event.value == wait_mask.bits.timer_num) // сработал таймер, которого ждем
			{
				if ( wait_mask.bits.error_timeout !=0 ) { // таймер должен вызвать ошибку
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
		if (state.bits.end !=0 ) { // запомнилось окончание программы
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


