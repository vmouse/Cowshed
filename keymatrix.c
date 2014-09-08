#include "keymatrix.h"

uint8_t CurMatrixKey;

// read sensors and keyboard
void Interface_Read() {
	char str[] = "4*1750286#39BDAC";
	uint8_t col = 0; // 2,3 bits = row (4,5,6,7), 0,1 bits = col (0,1,2,3)
	CurSensors = 0;
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
		if (CurMatrixKey != LastMatrixKey) {
			saf_Event newEvent;
			if (CurMatrixKey==0) {
				newEvent.value = LastMatrixKey;
				newEvent.code = EVENT_KEY_UP;
				} else {
				newEvent.value = CurMatrixKey;
				newEvent.code = EVENT_KEY_DOWN;
			}
			LastMatrixKey  = CurMatrixKey;
			saf_eventBusSend(newEvent);
		}
		if (CurSensors != 0) {
			saf_Event newEvent;
			newEvent.value = CurSensors;
			newEvent.code = EVENT_SENSORS;
			saf_eventBusSend(newEvent);
		}
	}
}
