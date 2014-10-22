#ifndef KEYMATRIX_H
#define KEYMATRIX_H

#include "saf2core.h"
#include "lcd.h"
#include "strfunc.h"

#define KEY_REPEAT_DELAY 100
#define MAX_INPUT_BUF 16

#ifdef  MAIN_FILE
uint8_t CurMatrixKey=0;
uint8_t LastMatrixKey=0;
uint8_t CurSensors=0;
char InputBuffer[MAX_INPUT_BUF+1];
#else
extern  uint8_t CurMatrixKey;
extern	uint8_t LastMatrixKey;
extern  uint8_t CurSensors;
extern  char InputBuffer[MAX_INPUT_BUF+1];
#endif

void Interface_Read(void);
void KeyMatrix_onEvent(saf_Event event);
void StartInput(uint8_t EventValue, char *Mask, uint8_t Pos, char *DefValue);
void ProcessInput(uint8_t key);

#endif
