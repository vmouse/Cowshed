/*
 * Menu.c
 *
 * Created: 19.09.2014 18:14:00
 *  Author: vlad
 */ 
#include "Menu.h"

uint8_t MenuCursor=0;
const _MenuItem MenuItems[] = {
	{"Start prog1", 0},		// 0
	{"Start prog2", 0},		// 1
	{"Manual control", 0},	//2
	{"Set timers", 4},		//3
	{"Timer 1", 0}, //4
	{"Timer 2", 0}, //5
	{"Timer 3", 0}, //6
	{"Set Time", 0},		//7
	{"Set Date", 0},		//8
};

void ShowMenuItem(void) {
	lcd_clear();
	lcd_pos(0x05);
	lcd_out("Menu:");
	lcd_pos(0x10);
	lcd_out(MenuItems[MenuCursor].Title);
	if (MenuItems[MenuCursor].SubMenuIndex != 0) { lcd_dat('>'); }
}

void StartMenu() {
	MenuCursor=8;
	ShowMenuItem();
}

void ProcessMenu(uint8_t key) {
	saf_Event newEvent;
	switch (key) {
		case 'C': // next menu item
			MenuCursor++;
			if (MenuCursor >= (sizeof(MenuItems)/sizeof(MenuItems[0]))) {
				MenuCursor = 0;
			}
			//ShowMenuItem();
			break;
		case '#': // enter menu item
			newEvent.value = MenuCursor;
			newEvent.code = EVENT_MENU_EXECUTE;
			saf_eventBusSend(newEvent);
			break;
		case '*': // cancel menu
			newEvent.value = MenuCursor;
			newEvent.code = EVENT_MENU_EXIT;
			saf_eventBusSend(newEvent);
			break;
		default:
			break;
	}
	ShowMenuItem();
}

