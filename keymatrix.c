#include "keymatrix.h"

uint8_t CurMatrixKey=0;
uint8_t KeyDelay=0;
char	InputMask[];
uint8_t InputPos=0;

// read sensors and keyboard
void Interface_Read() {
	char str[] = "4*1750286#39BDAC";
	uint8_t col = 0; // 2,3 bits = row (4,5,6,7), 0,1 bits = col (0,1,2,3)
	CurSensors = 0; CurMatrixKey = 0;
	while (col<0x10) {
		Set_Interface_Byte( ((~(1<<(col>>2)))<<4) | (col & 0x03));
		if ((PortSensors & BIT(Sensors))==0) {
			CurSensors|=col & 0x03;
		}
		if ((PortKeyboard & BIT(Keyboard))==0) {
			CurMatrixKey=str[col];
		}
		col++;
	}
}

void KeyMatrix_onEvent(saf_Event event) {
	if (event.code == EVENT_SAFTICK) {
		Interface_Read();
		if (CurMatrixKey | LastMatrixKey) { // ���-�� ������ ��� ������
			if ((CurMatrixKey != LastMatrixKey) || (KeyDelay == 0)) { // ������� �������� ��������
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

void StartInput(char *Mask, uint8_t Pos) {
	lcd_cursor_on;
	lcd_pos(Pos);
	InputPos = 0;
	InputMask = Mask;
}

void StopInput(void) {
	lcd_cursor_off;
}

void ProcessInput(uint8_t key) {
	lcd_dat(key);
	uint8_t maskchar = InputMask[InputPos];
	if (maskchar != 0 && InputPos < MAX_INPUT_BUF) {
			if (maskchar != '#') { lcd_dat( maskchar); }
			InputPos++;
	}
}