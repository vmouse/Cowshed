#ifndef MENU_H
#define MENU_H

#include "lcd.h"
#include "saf2core.h"
#include "Cowshed.h"

#define MENU_ITEM_SET_TIME	1
#define MENU_ITEM_SET_DATE	2
#define MENU_ITEM_SET_PORT_DIRECT 3
#define MENU_ITEM_SET_TIMERS 4
#define MENU_ITEM_START_1 5
#define MENU_ITEM_START_2 6
//#define MENU_LEVEL_MAIN 0
//#define MENU_GO_SUBMENU 0x80

#ifdef  MAIN_FILE
uint8_t MenuCursor=0;
uint8_t SelectedTimer=0;		// индекс выбранного для конфигурирования таймера
#else
extern uint8_t MenuCursor;
extern uint8_t SelectedTimer;
#endif


typedef struct {
	char*	Title;
	uint8_t Code;
} _MenuItem;

void StartMenu(_cow_state *state);
void StopMenu(void);
void ShowMenuItem(_cow_state *state);
void ProcessMenu(uint8_t key, _cow_state *state);

#endif