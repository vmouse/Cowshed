#ifndef MENU_H
#define MENU_H

#include "lcd.h"
#include "saf2core.h"

#ifdef  MAIN_FILE
uint8_t SelectedTimer=0;
#else
extern	uint8_t SelectedTimer;
#endif

#define MENU_ITEM_SET_TIME	1
#define MENU_ITEM_SET_DATE	2
#define MENU_ITEM_SET_PORT_DIRECT 3
#define MENU_ITEM_SET_TIMERS 4
#define MENU_ITEM_START_1 5
#define MENU_ITEM_START_2 6


typedef struct {
	char*	Title;
	uint8_t Code;
} _MenuItem;

void StartMenu(void);
void StopMenu(void);
void ShowMenuItem(void);
void ProcessMenu(uint8_t key);

#endif