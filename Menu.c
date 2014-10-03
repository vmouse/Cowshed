/*
 * Menu.c
 *
 * Created: 19.09.2014 18:14:00
 *  Author: vlad
 */ 
#include "Menu.h"

const _MenuItem MenuItems[] = {
	{"Start prog - 1", MENU_ITEM_START_1},		
	{"Start prog - 2", MENU_ITEM_START_2},		
	{"Manual control", MENU_ITEM_SET_PORT_DIRECT},	
	{"Set timers", MENU_ITEM_SET_TIMERS}, 
	{"Set Time", MENU_ITEM_SET_TIME},		
	{"Set Date", MENU_ITEM_SET_DATE},		
};

uint8_t MenuCursor=0;

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

void ProcessMenu(uint8_t key) {
	if (state.bits.settimers == 1) {
		ProcessTimersSet(key);
		} else {
		ProcessMenuKey(key);
	}
}

