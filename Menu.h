#ifndef MENU_H
#define MENU_H

#include "lcd.h"
#include "saf2core.h"

typedef struct {
	char*	Title;
	uint8_t SubMenuIndex;
} _MenuItem;

void StartMenu();
void ProcessMenu(uint8_t key);


#endif