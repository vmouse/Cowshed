/*
 * Cowshed.c
 *
 * Created: 22.04.2014 20:37:33
 *  Author: vlad 
 */ 
#include "Cowshed.h"
#define MAIN_FILE

//#include "input.h"
#include "saf2core.h"
#include "timers.h"
#include "bits.h"
#include "lcd.h"
#include "i2c.h"
#include "ds1307.h"
#include "keymatrix.h"
#include "Menu.h"
#include "ProgArray.h"

uint8_t ControlPortState = 0;	// что сейчас в порту
uint8_t cmd_index = 0;			// индекс текущей команды в программе
//uint8_t flash_cmd_index EEMEM ;	// сюда запоминаем последний индекс команды записи в порт
//uint8_t flash_portdata EEMEM ;	// запоминаем что вывели порт
//uint8_t flash_state EEMEM ;		// запоминаем состояние
//uint8_t flash_wait_mask EEMEM ;
_cow_state state = {.value = 0x00 }; // состояние системы
_cow_waiting_state wait_mask = { .value = 0 }; // маска ожидания события таймера или датчика

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



uint8_t flash_cmd_index EEMEM ;	// сюда запоминаем последний индекс команды записи в порт
uint8_t flash_portdata EEMEM ;	// запоминаем что вывели порт
uint8_t flash_state EEMEM ;		// запоминаем состояние
uint8_t flash_wait_mask EEMEM ;

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
	char buf[6];
	shift_and_mul_utoa16(cmd_index+1, buf, 0x30); // bcd convertion
	lcd_pos(0x10);
	lcd_out(buf+2); lcd_out(": "); 
	lcd_dat(Cmd.cmd_name); lcd_hex(Cmd.cmd_data);
}

// Show seconds counter value 
void ShowTime(uint16_t data) {
	if (state.bits.config==1) return;

	uint16_t mins = data / 60; if (mins>99) mins=99;
	uint16_t secs = data % 60;

	lcd_pos(0x1b);
	char buf[6];
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

	char buf[6];
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
	lcd_out((char[]){66,195,190,111,187,189,101,189,111,46,0}); // Выполнено.
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
			ProcessMenu(event.value, state);
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
				Set_Control_Byte(BitsToInt(&InputBuffer[0], '1'));
				break;
			case 'W':
				// set timer value
				TimersArray[SelectedTimer] = Dec2Int(&InputBuffer[0])*3600 + Dec2Int(&InputBuffer[3])*60 + Dec2Int(&InputBuffer[6]);
				eeprom_write_word (&flash_TimersArray[SelectedTimer], TimersArray[SelectedTimer]);
				break;
			default: break;
		}
		// return to menu
		ProcessMenu(0, state);
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
				state.bits.settimers = 1; // timers config mode
				lcd_clear(); lcd_pos(0x00); lcd_out("Set timer "); lcd_hexdigit(SelectedTimer);
//				state.bits.userinput = 1;
//				lcd_clear(); lcd_pos(0x02); lcd_out("Set timer "); lcd_dat(SelectedTimer+'0');
//				StartInput('W', "##:##:##", 0x14, SecondsToTimeStr(TimersArray[SelectedTimer], buf));
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
	RestFlash(); // Resote saved values;

	lcd_pos(0x03); lcd_out("Cowshed-2");
}

void SaveFlash(void) {
//	flash_cmd_index = cmd_index;
//	flash_portdata = ControlPortState;
//	flash_state = state.value;
//	flash_wait_mask = wait_mask.value;
	eeprom_write_block(TimersArray, flash_TimersArray, MAX_FIXED_TIMERS * sizeof(flash_TimersArray[0]));

//	for (int i=0; i<MAX_FIXED_TIMERS; i++) {
//		eeprom_write_word(&flash_TimersArray[i], TimersArray[i]);
//	}
}

void RestFlash(void) {
//	cmd_index = flash_cmd_index;
//	ControlPortState = flash_portdata;
//	state.value = flash_state;
//	wait_mask.value = flash_wait_mask;
//	for (int i=0; i<MAX_FIXED_TIMERS; i++) {
//		TimersArray[i] = eeprom_read_word (&flash_TimersArray[i]);
//	}
	eeprom_read_block(TimersArray, flash_TimersArray, MAX_FIXED_TIMERS * sizeof(flash_TimersArray[0]));
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
//	char buf0
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

//	RestFlash();

	lcd_init();	lcd_clear();
	lcd_pos(0x00); lcd_out("Initialization");

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
		if ((state.bits.end || state.value==0) & (state.bits.config == 0)) {
			lcd_pos(0x14); LCD_Time(); ShowSensors();
		} else if (state.bits.waiting == 0 && state.bits.started == 1) {
			Do_Command();
		}

    }
}


