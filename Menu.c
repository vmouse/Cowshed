/*
 * Menu.c
 *
 * Created: 19.09.2014 18:14:00
 *  Author: vlad
 */ 
#include "Menu.h"

uint8_t MenuCursor=0;
const _MenuItem MenuItems[] = {
	{"Start prog1", 0},		
	{"Start prog2", 0},		
	{"Manual control", MENU_ITEM_SET_PORT_DIRECT},	
	{"Set timers", 0}, 
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

void StartMenu(void) {
	MenuCursor=0;
	ShowMenuItem();
}

void StopMenu(void) {
	lcd_clear();
	lcd_out("Exit");
}

void ProcessMenu(uint8_t key) {
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
			break;
	}
}

