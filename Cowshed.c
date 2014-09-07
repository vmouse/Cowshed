/*
 * Cowshed.c
 *
 * Created: 22.04.2014 20:37:33
 *  Author: vlad 
 */ 
#include <avr/io.h>
#include <string.h>
#include <stdint.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include "saf2core.h"
//#include "input.h"
#include "timers.h"
#include "bits.h"
#include "lcd.h"
#include "i2c.h"
#include "ds1307.h"
#include "keymatrix.h"

#define STATE_INIT	0	// начальное состояние. ждем команд
#define STATE_WORK	1	// состояние выполнение процесса
#define STATE_PAUSE	2	// пауза процесса. выход "ручным" способом
#define STATE_ERROR	3	// ошибка в процессе. остановка в этом месте 
#define STATE_WAIT	4	// процесс в ожидании или таймера или события порта (использует маску ожидания wait_mask)
#define STATE_END	5	// программа завершена

#define CoolStart	(MCUSR & ( 1 << PORF )) // не ноль, если запуск был включением питания

#define PortControl	PORTB
#define PCshift		PB1 // Register strobe (shift clock)
#define PCdata		PB0 // Register data
#define PClatch		PB2 // Register out (latch clock)

#define Sensor		PD3 // Sensor multiplexor



void Set_Control_Byte(uint8_t data) {
	for (uint8_t i=0;i<8;i++) {
		SENDBIT(PortControl, PCdata, data);
		STROBE(PortControl, PCshift);   // Shift
		data = data << 1;
	}
	STROBE(PortControl, PClatch);  // Out enable
}


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


typedef struct {
	char	cmd_name;
	uint8_t cmd_data;
} _cmd_type;

void SaveFlash(void);
void ResetState(void);

// Фиксированные периоды для таймеров
uint16_t TimersArray[] = {0, 30, 30, 5*60, 15*60, 15*60, 11*60, 2*60, /*5*60*/10, 5*60};
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
//   {'T', 0x02}, // 0: Взводим таймер на t4 = 15 мин
//   {'W', 0x02}, // 1: Кончилась вода|Закончилась мойка
   {'P', 0x2C}, // 00: Наполнение основного бака для полоскания
   {'T', 0x08}, // 01: Взводим таймер на t8 = 10 мин
   {'W', 0x81}, // 02: Ждем наполнения или ошибки окончания таймера
/*
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
*/
};  // test port output


/*
void Dynamic_Indication(void) {
	write_data((1<<ind_digit)<<4);  // выбрать сегмент
	write_data(ind_data[ind_digit]);  // вывести число
	OUT_REG;
	ind_digit++;
	if (ind_digit>3) { ind_digit=0; }
}
*/

// Show command by index
void ShowCmd(uint16_t cmd_index) {
	_cmd_type Cmd = CmdArray[cmd_index];
	char buf[6];
	shift_and_mul_utoa16(cmd_index+1, buf, 0x30); // bcd convertion
	lcd_pos(0x10);
	lcd_out(buf+2); lcd_out(": "); 
	lcd_dat(Cmd.cmd_name); lcd_hex(Cmd.cmd_data);
}

// Show seconds counter value 
void ShowTime(uint16_t data) {
	uint8_t mins = data / 60; if (mins>99) mins=99;
	uint8_t secs = data % 60;

	lcd_pos(0x1b);
	char buf[6];
	shift_and_mul_utoa16(mins, buf, 0x30); 
	lcd_out(buf+3);lcd_dat(':');
	shift_and_mul_utoa16(secs, buf, 0x30); 
	lcd_out(buf+3);
}

void ShowError(uint8_t ErrorClass, uint8_t ErrorCode) {
	timer_stop(-1); // stop all timers
	state.bits.error=1;
	state.bits.started = 0;

	char buf[6];
	shift_and_mul_utoa16(ErrorCode+1, buf, 0x30); // bcd convertion

	lcd_clear();
	lcd_pos(0x04);	lcd_out((uint8_t[]){79,193,184,178,186,97,33,0}); // "Ошибка!"
//	lcd_pos(0x10);	lcd_out((uint8_t[]){99,191,112,58,0}); lcd_out(buf+2); // "стр:"+ErrorCode
//	lcd_out((uint8_t[]){32,191,184,190,58,0}); //" тип: "
	ShowCmd(ErrorCode);
	lcd_dat(' ');
	switch (ErrorClass) {
		case 0: // Execution error
			lcd_out("Execute");
			break;
		case 1:	// Synax error 
			lcd_out("Syntax");
			break;
		case 2:	// timeout error
			lcd_out("Timeout");
			break;
		default: // other error
			lcd_hex(ErrorClass);
			break;
	}
}

void ShowEnd(void) {
	timer_stop(-1); // stop all timers
	lcd_clear(); 
	lcd_pos(0x03);
	lcd_out((uint8_t[]){66,195,190,111,187,189,101,189,111,46,0}); // Выполнено.
}

void Do_Command(void) {			// выполнение комманды программы
	if (cmd_index>=sizeof(CmdArray)/sizeof(CmdArray[0])) {
		state.bits.started = 0;
		state.bits.end = 1;
		ShowEnd();
	} else {

	_cmd_type Cmd = CmdArray[cmd_index];

	ShowCmd(cmd_index);

	uint16_t time;

	switch (Cmd.cmd_name) {
		case 'P':	// вывод в порт
			Set_Control_Byte(Cmd.cmd_data);
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
	if (event.code == EVENT_KEY_DOWN)
	{
		switch (event.value) {
			case 'A': // start button
				if (state.bits.started == 0) {
					cmd_index = 0;
					state.value = 1; // все флаги сбрасываем, кроме старта
					lcd_clear();
				}
				break;
			case '*': // reset
				ResetState();
				break;
			default: break;
		}
	}

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
			ShowTime(timer_get(0));
		} else 	ShowTime(timer_get(1));
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

void ResetState(void) {
	state.value = 0;
	wait_mask.value = 0;
	cmd_index = 0;
	timer_stop(-1);
	lcd_clear(); lcd_out(" Covshed  ver.2");
	Set_Control_Byte(0);
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

void onEvent_test(saf_Event event)
{
	if (event.code == EVENT_KEY_DOWN)
	{
		lcd_dat(event.value);
	} else 	if (event.code == EVENT_INT0)
	{
//		lcd_dat('!');
//		lcd_pos(0x14);  LCD_Time();
		ShowCmd(cmd_index);
		ShowTime(60);
		cmd_index++;
	}

}

int main(void)
{

	// Hardware initialization
	DDRD = 0x0;
	DDRC = 1<<Ishift|1<<Idata|1<<Ilatch;
	DDRB = 1<<PCshift|1<<PCdata|1<<PClatch;
	PORTC = 0xff;
	PORTB = 0xff;
	PORTD = 0xff;

	lcd_init();
	DS1307_Init();
//	SetTimeDate(0x14, 0x09, 0x06, 0x02, 0x37); // Set time into 1307 chip

	// State machine initialization
	ResetState();

	// Simple AVR framework (SAF) initializationwaa
	saf_init();
//	timers_init();
	timers_init(EVENT_INT0, 0);

//	input_add(_D, 7); // Start button
//	input_add(_D, 2); // Liquid high level sensor
//	input_add(_D, 3); // Liquid Low level sensor
	
//	saf_addEventHandler(onEvent);
//	saf_addEventHandler(input_onEvent);

	saf_addEventHandler(onEvent);
	saf_addEventHandler(KeyMatrix_onEvent);
	saf_addEventHandler(timers_onEvent);


	// Start
	sei();


//	if (CoolStart != 0) {
//		RestFlash();
//		if (state.bits.end !=0 ) { // запомнилось окончание программы
//			ResetState();
//		}
//	}



    while(1)
    {
//		LCD_TimeDate();
		saf_process();
		
		if (state.bits.end || state.value==0) {
			lcd_pos(0x14); LCD_Time(); 
		} else if (state.bits.waiting == 0 && state.bits.started == 1) {
			Do_Command();
		}
    }
}


