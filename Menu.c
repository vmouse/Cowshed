/*
 * Menu.c
 *
 * Created: 19.09.2014 18:14:00
 *  Author: vlad
 */ 
#include "Menu.h"
#include "ProgArray.h"
#include "keymatrix.h"
#include "ds1307.h"

const _MenuItem MenuItems[] = {
	{"Start prog - 1", MENU_ITEM_START_1},		
	{"Start prog - 2", MENU_ITEM_START_2},		
	{"Manual control", MENU_ITEM_SET_PORT_DIRECT},	
	{"Set timers", MENU_ITEM_SET_TIMERS}, 
	{"Set Time", MENU_ITEM_SET_TIME},		
	{"Set Date", MENU_ITEM_SET_DATE},		
};

void ShowMenuItem(void) {
	lcd_clear();
	lcd_pos(0x05);
	lcd_out("Menu:");
	lcd_pos(0x10);
	lcd_out(MenuItems[MenuCursor].Title);
}

void StartMenu() {
	MenuCursor=0;
	ShowMenuItem();
}

void StopMenu(void) {
	lcd_clear();
	lcd_out("Exit");
}

void ProcessMenuKey(uint8_t key) {	
	saf_Event newEvent;
	switch (key) {
		case 'C': // next menu item
			MenuCursor++;
			if (MenuCursor >= (sizeof(MenuItems)/sizeof(MenuItems[0]))) {
				MenuCursor = 0;
			}
			ShowMenuItem();
			break;
		case '#': // enter menu item
			newEvent.value = MenuItems[MenuCursor].Code;
			newEvent.code = EVENT_MENU_EXECUTE;
			saf_eventBusSend(newEvent);
			break;
		case '*': // cancel menu
			StopMenu();
			newEvent.value = MenuItems[MenuCursor].Code;
			newEvent.code = EVENT_MENU_EXIT;
			saf_eventBusSend(newEvent);
			break;
		default:
			ShowMenuItem();
			break;
	}
}

void	ProcessTimersSet(uint8_t key, _cow_state *state) {
	char buf[sizeof("##:##:##")];
	switch (key) {
		case 'C': // next timer
			SelectedTimer++;
			if (SelectedTimer>=sizeof(TimersArray)/sizeof(TimersArray[0])) {
				SelectedTimer = 0;
			}
			// вывод значения таймера
			lcd_clear(); 
			lcd_out("Timer "); lcd_hexdigit(SelectedTimer); 
			lcd_pos(0x14); 
//			lcd_out(DS1307_GetTimeStr(buf));
//			lcd_out(SecondsToTimeStr(TimersArray[SelectedTimer], buf));
			lcd_out(SecondsToTimeStr(30, buf));
//			lcd_out(shift_and_mul_utoa16(TimersArray[SelectedTimer], buf, '0'));
			break;
		case '#': // enter timer value
			(*state).bits.userinput = 1;
			lcd_clear(); lcd_pos(0x02); lcd_out("Set timer "); lcd_hexdigit(SelectedTimer);
			StartInput('W', "##:##:##", 0x14, SecondsToTimeStr(TimersArray[SelectedTimer], buf));
			break;
		case '*': // exit from timer setting
			(*state).bits.settimers = 0; 
			SelectedTimer = 0;
			ProcessMenu(0, state);
			break;
		default:
			break;
	}
}

void ProcessMenu(uint8_t key, _cow_state *state) {
	if ((*state).bits.settimers == 1) {
		ProcessTimersSet(key, state);
		} else {
		ProcessMenuKey(key);
	}
}

