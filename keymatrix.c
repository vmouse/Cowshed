#include "keymatrix.h"

char LastMatrixKey=0;

char KeyMatrix_Read() {
	char str[] = "4*1750286#39BDAC";
	uint8_t col = 0; // 2,3 bits = row (4,5,6,7), 0,1 bits = col (0,1,2,3)
	while (col<0x10) {
		Set_Interface_Byte( ((~(1<<(col>>2)))<<4) | (col & 0x03));
		if ((PINC & BIT(Keyboard))==0) {
			return str[col];
		}
		col++;
	}
	return 0;
}

void KeyMatrix_onEvent(saf_Event event) {
	if (event.code == EVENT_SAFTICK) {
		char curKey = KeyMatrix_Read();
		if (curKey != LastMatrixKey) {
			saf_Event newEvent;
			if (curKey==0) {
				newEvent.value = LastMatrixKey;
				newEvent.code = EVENT_KEY_UP;
				} else {
				newEvent.value = curKey;
				newEvent.code = EVENT_KEY_DOWN;
			}
			LastMatrixKey  = curKey;
			saf_eventBusSend(newEvent);
		}
	}
}
