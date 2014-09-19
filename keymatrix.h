#ifndef KEYMATRIX_H
#define KEYMATRIX_H

#include "saf2core.h"
#include "lcd.h"

#define KEY_REPEAT_DELAY 10
#define MAX_INPUT_BUF 16

#ifdef  MAIN_FILE
uint8_t LastMatrixKey=0;
uint8_t CurSensors=0;
uint8_t InputBuffer[MAX_INPUT_BUF+1];
#else
extern	uint8_t LastMatrixKey;
extern  uint8_t CurSensors;
extern  uint8_t InputBuffer[MAX_INPUT_BUF+1];
#endif

void Interface_Read(void);
void KeyMatrix_onEvent(saf_Event event);
void StartInput(uint8_t EventValue, char *Mask, uint8_t Pos, char *DefValue);
void ProcessInput(uint8_t key);
uint8_t Hex2Int(char str[]);

#endif
