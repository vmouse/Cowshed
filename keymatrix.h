#ifndef KEYMATRIX_H
#define KEYMATRIX_H

#include "saf2core.h"
#include "lcd.h"

#define KEY_REPEAT_DELAY 10
#define MAX_INPUT_BUF 16

#ifdef  MAIN_FILE
uint8_t LastMatrixKey=0;
uint8_t CurSensors=0;
uint8_t InputBuffer[MAX_INPUT_BUF];
#else
extern	uint8_t LastMatrixKey;
extern  uint8_t CurSensors;
extern  uint8_t InputBuffer[MAX_INPUT_BUF];
#endif

void Interface_Read(void);
void KeyMatrix_onEvent(saf_Event event);
void StartInput(char *Mask, uint8_t Pos);
void ProcessInput(uint8_t key);

#endif
