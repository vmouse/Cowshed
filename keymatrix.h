#ifndef KEYMATRIX_H
#define KEYMATRIX_H

#include "saf2core.h"
#include "lcd.h"

#define KEY_REPEAT_DELAY 10

#ifdef  MAIN_FILE
uint8_t LastMatrixKey=0;
uint8_t CurSensors=0;
#else
extern	uint8_t LastMatrixKey;
extern  uint8_t CurSensors;
#endif

void Interface_Read(void);
void KeyMatrix_onEvent(saf_Event event);

#endif
