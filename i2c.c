//  ---------------------------------------------------------------------------//  I2C (TWI) ROUTINES
//
// On the AVRmega series, PA4 is the data line (SDA) and PA5 is the clock (SCL
// The standard clock rate is 100 KHz, and set by I2C_Init. It depends on the AVR osc. freq.
#include "i2c.h"

void I2C_Init()
// at 16 MHz, the SCL frequency will be 16/(16+2(TWBR)), assuming prescalar of 0.
// so for 100KHz SCL, TWBR = ((F_CPU/F_SCL)-16)/2 = ((16/0.1)-16)/2 = 144/2 = 72.
{
	TWSR = 0;  // set prescalar to zero
	TWBR = ((F_CPU/F_SCL)-16)/2;  // set SCL frequency in TWI bit register
}

uint8_t I2C_Detect(uint8_t addr)
// look for device at specified address; return 1=found, 0=not found
{
	TWCR = TW_START;  // send start condition
	while (!TW_READY);  // wait
	TWDR = addr;  // load device's bus address
	TWCR = TW_SEND;  // and send it
	while (!TW_READY);  // wait
	return (TW_STATUS==0x18);  // return 1 if found; 0 otherwise
}

uint8_t I2C_FindDevice(uint8_t start)
// returns with address of first device found; 0=not found
{
	for (uint8_t addr=start;addr<0xFF;addr++)  // search all 256 addresses
	{
		if (I2C_Detect(addr))  // I2C detected?
		return addr;  // leave as soon as one is found
	}
	return 0;  // none detected, so return 0.
}

void I2C_Start (uint8_t slaveAddr)
{
	I2C_Detect(slaveAddr);
}

uint8_t I2C_Write (uint8_t data)  // sends a data uint8_t to slave
{
	TWDR = data;  // load data to be sent
	TWCR = TW_SEND;  // and send it
	while (!TW_READY);  // wait
	return (TW_STATUS!=0x28);
}

uint8_t I2C_ReadACK ()  // reads a data uint8_t from slave
{
	TWCR = TW_ACK;  // ack = will read more data
	while (!TW_READY);  // wait
	return TWDR;
	//return (TW_STATUS!=0x28);
}

uint8_t I2C_ReadNACK ()  // reads a data uint8_t from slave
{
	TWCR = TW_NACK;  // nack = not reading more data
	while (!TW_READY);  // wait
	return TWDR;
	//return (TW_STATUS!=0x28);
}

void I2C_WriteByte(uint8_t busAddr, uint8_t data)
{
	I2C_Start(busAddr);  // send bus address
	I2C_Write(data);  // then send the data byte
	I2C_Stop();
}

void I2C_WriteRegister(uint8_t busAddr, uint8_t deviceRegister, uint8_t data)
{
	I2C_Start(busAddr);  // send bus address
	I2C_Write(deviceRegister);  // first uint8_t = device register address
	I2C_Write(data);  // second uint8_t = data for device register
	I2C_Stop();
}

uint8_t I2C_ReadRegister(uint8_t busAddr, uint8_t deviceRegister)
{
	uint8_t data = 0;
	I2C_Start(busAddr);  // send device address
	I2C_Write(deviceRegister);  // set register pointer
	I2C_Start(busAddr+READ);  // restart as a read operation
	data = I2C_ReadNACK();  // read the register data
	I2C_Stop();  // stop
	return data;
}

char* I2C_ReadRegisterAsHEX(char *buffer, uint8_t busAddr, uint8_t deviceRegister)
{
	uint8_t data = 0x12;
	data = I2C_ReadRegister(busAddr, deviceRegister);
	char *ptr = buffer;
	*ptr++ = ( (data >> 4) + '0' );
	*ptr++ = ( (data & 0x0f) + '0' );
	*ptr = 0;
	return ptr;
}
