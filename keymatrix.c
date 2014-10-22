#include "keymatrix.h"
#include <string.h>

uint8_t KeyDelay=0;
char	InputMask[MAX_INPUT_BUF];
uint8_t InputPos=0;
uint8_t MaskPos=0;
uint8_t	InputEventValue=0;

// read sensors and keyboard
void Interface_Read() {
	char str[] = "4*1750286#39BDAC";
	uint8_t col = 0; // 2,3 bits = row (4,5,6,7), 0,1 bits = col (0,1,2,3)
	CurSensors = 0; CurMatrixKey = 0;
	while (col<0x10) {
		Set_Interface_Byte( ((~(1<<(col>>2)))<<4) | (col & 0x03));
		_delay_us(50); // ждем мультиплексоры
		if ((PinSensors & BIT(Sensors))==0) {
			CurSensors|=col & 0x03;
		}
		if ((PinKeyboard & BIT(Keyboard))==0) {
			CurMatrixKey=str[col];
//			CurMatrixKey=col;
		}
//		_delay_us(50);
		col++;
	}
}

void KeyMatrix_onEvent(saf_Event event) {
	if (event.code == EVENT_SAFTICK) {
		Interface_Read();
		if (CurMatrixKey | LastMatrixKey) { // что-то нажали или отжали
			if ((CurMatrixKey != LastMatrixKey) || (KeyDelay == 0)) { // таймаут дребезга закончен
				KeyDelay = KEY_REPEAT_DELAY; 
				saf_Event newEvent;
				if (CurMatrixKey == 0) {
					newEvent.value = LastMatrixKey;
					newEvent.code = EVENT_KEY_UP;
					} else {
					newEvent.value = CurMatrixKey;
					newEvent.code = EVENT_KEY_DOWN;
				}
				saf_eventBusSend(newEvent);
			} else {
				KeyDelay--;
			}
			LastMatrixKey = CurMatrixKey;
		}

		if (CurSensors != 0) {
			saf_Event newEvent;
			newEvent.value = CurSensors;
			newEvent.code = EVENT_SENSORS;
			saf_eventBusSend(newEvent);
		}
	}
}

void StopInput(uint8_t EventCode) {
	lcd_cursor_off; lcd_clear();
	saf_Event newEvent;
	newEvent.value = InputEventValue;
	newEvent.code = EventCode;
	saf_eventBusSend(newEvent);
}

// работа с маской ввода, возвращает 0, если маска закончилась
uint8_t ParseInputMask(void) {
	// обработка маски
	while ((InputMask[MaskPos] != '#') && (MaskPos < MAX_INPUT_BUF) && (InputMask[MaskPos] != 0 )) {
		lcd_dat(InputMask[MaskPos]);
		InputBuffer[InputPos] = InputMask[MaskPos];
		InputPos++;
		MaskPos++; 
	} 	
	if ((MaskPos >= MAX_INPUT_BUF) || (InputMask[MaskPos] == 0 )) {
		return 1;
	}
	return 0;
}

void StartInput(uint8_t EventValue, char *Mask, uint8_t Pos, char *DefValue) {
	InputPos = 0;
	MaskPos = 0;
	InputEventValue = EventValue;
	strncpy(InputMask, Mask, sizeof(InputMask));
	strncpy(InputBuffer, DefValue, sizeof(InputBuffer));

	lcd_pos(Pos);
	lcd_out(InputBuffer);
	lcd_pos(Pos);
	lcd_cursor_on;

	ParseInputMask(); // в начале маски ввода могут быть символы для вывода
}

// обработка очередного символа
void ProcessInput(uint8_t key) {
	if (key == '*') {
		StopInput(EVENT_INPUT_CANCELED);
		return;
	} else
	if (key == '#') {
		key = 0;
	} else 
	if ((key < 'A') || (InputMask[MaskPos] != '#')) { // если в маске #, то допустимы только цифры
		lcd_dat(key);
		InputBuffer[InputPos] = key;
		InputPos++; MaskPos++;
	}
	if ((ParseInputMask() != 0) || (InputPos >= MAX_INPUT_BUF) || (key == 0)) {
		StopInput(EVENT_INPUT_COMPLETED);
	}
}
