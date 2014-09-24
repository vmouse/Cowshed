/*
 * Cowshed.c
 *
 * Created: 22.04.2014 20:37:33
 *  Author: vlad 
 */ 
#define MAIN_FILE
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
#include "Menu.h"

#define CoolStart	(MCUSR & ( 1 << PORF )) // не ноль, если запуск был включением питания

#define PortControl	PORTB
#define PCshift		PB1 // Register strobe (shift clock)
#define PCdata		PB0 // Register data
#define PClatch		PB2 // Register out (latch clock)

#define Sensor		PD3 // Sensor multiplexor

# define PSTR(s) (__extension__( {static char __c[] PROGMEM = (s); &__c[0];}))
//Эта фича позволяет писать внутри функций так:
//char * mystr = PSTR( ":)" );


uint8_t ControlPortState = 0;			// что сейчас в порту
uint8_t cmd_index = 0;			// индекс текущей команды в программе


void OutDataPort(uint8_t data) {
	data=~data; // в этой версии включаем нулями, поэтому инвертируем
	for (uint8_t i=0;i<8;i++) {
		SENDBIT(PortControl, PCdata, data);
		STROBE(PortControl, PCshift);   // Shift
		data = data << 1;
	}
	STROBE(PortControl, PClatch);  // Out enable
}

void Set_Control_Byte(uint8_t data) {
	ControlPortState = data; // запоминаем состояние порта
	OutDataPort(0);		// очищаем значения порта
	OutDataPort(data);	// выставляем значения порта
	lcd_pos(0x0e); lcd_hex(ControlPortState);
}



uint8_t EEMEM flash_cmd_index = 0;	// сюда запоминаем последний индекс команды записи в порт
uint8_t EEMEM flash_portdata = 0;	// запоминаем что вывели порт
uint8_t EEMEM flash_state = 0;		// запоминаем состояние
uint8_t EEMEM flash_wait_mask = 0;


#define STATE_VALUE_START 0x01; // комбинация бит на старт программы
union { struct
	{
		uint8_t started : 1;	// обязательно должен быть младшим битом
		uint8_t waiting	: 1;
		uint8_t end		: 1;
		uint8_t error	: 1;
		uint8_t config  : 1;   // конфигурирование, работа с меню (подавляет вывод другой информации)
		uint8_t userinput: 1;  // идет пользовательский ввод с клавиатуры
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
uint16_t TimersArray[] = {1/*0*/, 30, 30, 5*60, 15*60, 15*60, 11*60, 2*60, 5*60, 5*60};
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

// test prog
   {'P', 0x02},
   {'T', 0x00},
   {'W', 0x00},
   {'P', 0x04},
   {'T', 0x00},
   {'W', 0x00},
   {'P', 0x08},
   {'T', 0x00},
   {'W', 0x00},
   {'P', 0x10},
   {'T', 0x00},
   {'W', 0x00},
   {'P', 0x20},
   {'T', 0x00},
   {'W', 0x00},
   {'P', 0x40},
   {'T', 0x00},
   {'W', 0x00},
   {'P', 0x80},
   {'T', 0x00},
   {'W', 0x00},
   {'P', 0x01},
   {'T', 0x00},
   {'W', 0x00},
/*
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
	if (state.bits.config==1) return;
	_cmd_type Cmd = CmdArray[cmd_index];
	uint8_t buf[6];
	shift_and_mul_utoa16(cmd_index+1, buf, 0x30); // bcd convertion
	lcd_pos(0x10);
	lcd_out(buf+2); lcd_out(": "); 
	lcd_dat(Cmd.cmd_name); lcd_hex(Cmd.cmd_data);
}

// Show seconds counter value 
void ShowTime(uint16_t data) {
	if (state.bits.config==1) return;

	uint8_t mins = data / 60; if (mins>99) mins=99;
	uint8_t secs = data % 60;

	lcd_pos(0x1b);
	uint8_t buf[6];
	shift_and_mul_utoa16(mins, buf, 0x30); 
	lcd_out(buf+3);lcd_dat(':');
	shift_and_mul_utoa16(secs, buf, 0x30); 
	lcd_out(buf+3);
}

void ShowSensors(void)
{
	if (state.bits.config==1) return; 
	lcd_pos(0x1E); lcd_hex(CurSensors);
	lcd_pos(0x0E); lcd_hex(ControlPortState);
}

void ShowError(uint8_t ErrorClass, uint8_t ErrorCode) {
	timer_stop(-1); // stop all timers
	state.bits.error=1;
	state.bits.started = 0;
	if (state.bits.config == 1) {
		StopMenu();
		state.bits.config = 0;
	}

	uint8_t buf[6];
	shift_and_mul_utoa16(ErrorCode+1, buf, 0x30); // bcd convertion

	lcd_clear();
	lcd_pos(0x04);	lcd_out((char[]){79,193,184,178,186,97,33,0}); // "Ошибка!"
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
	if (state.bits.config == 1) {
		StopMenu();
		state.bits.config = 0;
	}
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

void StartProg(uint8_t ProgIndex) {
	if (state.bits.started == 0) {
	cmd_index = 0;
	state.value = STATE_VALUE_START; // все флаги сбрасываем, кроме старта
		lcd_clear();
	}
}

void onEvent(saf_Event event)
{	
	if (event.code == EVENT_KEY_DOWN)
	{
		if (state.bits.userinput == 1) {
			ProcessInput(event.value);
		} else	
		if (state.bits.config == 1) {
			ProcessMenu(event.value);
		} else
			switch (event.value) {
			case 'A': // start button
				StartProg(0);
				break;
			case 'C': // config button
				state.bits.config = 1;
				StartMenu();
				break; 
			case '*': // reset
				ResetState();
				break;
			default:
				break;
		}
	} else
	
	if (event.code == EVENT_INPUT_CANCELED)
	{
		state.bits.userinput = 0;
		if (state.bits.config == 1) { ShowMenuItem(); }
	} else
	if (event.code == EVENT_INPUT_COMPLETED)
	{
		state.bits.userinput = 0;
		switch (event.value) {
			case 'T': 
				SetTime(Hex2Int(&InputBuffer[0]), Hex2Int(&InputBuffer[3]), Hex2Int(&InputBuffer[6]));
				break;
			case 'D':
				SetDate(Hex2Int(&InputBuffer[8]), Hex2Int(&InputBuffer[3]), Hex2Int(&InputBuffer[0]));
				break;
			case 'P':
				Set_Control_Byte(BitsToInt(&InputBuffer, '1'));
				break;
			case 'W':
				// set timer value
				TimersArray[SelectedTimer] = Hex2Int(&InputBuffer[0])*3600 + Hex2Int(&InputBuffer[3])*60 + Hex2Int(&InputBuffer[6]);
			break;
			default: break;
		}
		// return to menu
		ProcessMenu(0);
	} else

	if (event.code == EVENT_MENU_EXECUTE) {
		char buf[sizeof("##.##.####")];
		switch (event.value) {
			case MENU_ITEM_SET_TIME:
				state.bits.userinput = 1;
				lcd_clear(); lcd_pos(0x04); lcd_out("Set time:");
				StartInput('T', "##:##:##", 0x14, DS1307_GetTimeStr(buf));
				break;
			case MENU_ITEM_SET_DATE:
				state.bits.userinput = 1;
				lcd_clear(); lcd_pos(0x04); lcd_out("Set date:");
				StartInput('D', "##.##.####", 0x13, DS1307_GetDateStr(buf));
				break;
			case MENU_ITEM_SET_PORT_DIRECT:
				state.bits.userinput = 1;
				lcd_clear(); lcd_pos(0x00); lcd_out("Set port direct:");
				StartInput('P', "########", 0x15, IntToBitsStr(ControlPortState, buf, '0', '1'));
				break;
			case MENU_ITEM_SET_TIMERS:
				state.bits.userinput = 1;
				lcd_clear(); lcd_pos(0x04); lcd_out("Set timer "); lcd_dat(SelectedTimer+'0');
				StartInput('T', "##:##:##", 0x14, SecondsToTimeStr(TimersArray[SelectedTimer], buf));
				break;
			case MENU_ITEM_START_1:
				StartProg(0);
				break;
			case MENU_ITEM_START_2:
				StartProg(1);
				break;
			default:
				break;
		}	
	} else
	if (event.code == EVENT_MENU_EXIT) {
		state.bits.config = 0;
	} else

	if (event.code == EVENT_SENSORS)
	{
		switch (event.value) {
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
	} else 
	if (event.code == EVENT_TIMER_TICK) // отображаем прогресс таймера 0
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
	Set_Control_Byte(0);
	lcd_init();	lcd_clear(); 
	lcd_pos(0x03); lcd_out("Cowshed-2");
}

void SaveFlash(void) {
	flash_cmd_index = cmd_index;
	flash_portdata = ControlPortState;
	flash_state = state.value;
	flash_wait_mask = wait_mask.value;
}

void RestFlash(void) {
	cmd_index = flash_cmd_index;
	ControlPortState = flash_portdata;
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

//	char buf[]="----------";
//	SecondsToTimeStr(3600*2 + 60 *3 + 4, buf);
//lcd_out(buf);
//	StartInput("P", "#\n", 0x10, IntToBitsStr(0x0f, buf, '0', '1'));

//	IntToBitsStr(0x07, buf, '0','1');
//	uint8_t a = BitsToInt(buf, '1');
//	lcd_out(a);
//	StartInput("D", "##.##.20##", 0x10, DS1307_GetDateStr(buf));


//StartInput("##:##", 0x10);	
//ProcessInput('1');
//ProcessInput('0');
//ProcessInput('1');
//ProcessInput('0');
//ProcessInput('#');
//uint8_t n = BitsToInt(InputBuffer, '1');
//lcd_out(n);
/*
ProcessInput(0x35);
ProcessInput(0x36);
ProcessInput(0x37);
ProcessInput(0x38);
ProcessInput(0x39);
ProcessInput(0x30);
ProcessInput(0x31);
ProcessInput(0x32);
*/	
	// Hardware initialization
	DDRD = 0x0;
	DDRC = 1<<Ishift|1<<Idata|1<<Ilatch;
	DDRB = 1<<PCshift|1<<PCdata|1<<PClatch;
	PORTC = 0xff;
	PORTB = 0xff;
	PORTD = 0xff;

//while (1) {
	//Set_Control_Byte(2);
//}
	// State machine initialization
	ResetState();

//	lcd_init(); // lcd initialized in reset
	DS1307_Init();
//	SetTimeDate(0x14, 0x09, 0x06, 0x02, 0x37); // Set time into 1307 chip

	// Simple AVR framework (SAF) initializationwaa
	saf_init();
	saf_addEventHandler(onEvent);
	saf_addEventHandler(KeyMatrix_onEvent);
	saf_addEventHandler(timers_onEvent);
	timers_init(EVENT_INT0, 0);

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
			lcd_pos(0x14); LCD_Time(); ShowSensors();
		} else if (state.bits.waiting == 0 && state.bits.started == 1) {
			Do_Command();
		}

    }
}


