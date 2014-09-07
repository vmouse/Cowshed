#ifndef KEYMATRIX_H
#define KEYMATRIX_H

#include "saf2core.h"
#include "lcd.h"

char KeyMatrix_Read();
void KeyMatrix_onEvent(saf_Event event);

#endif
