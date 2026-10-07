/*
 * iic_basic.h
 *
 *  Created on: Aug 27, 2026
 *      Author: Navumchyk P.
 */

#ifndef INC_IIC_BASIC_H_
#define INC_IIC_BASIC_H_

#include "stm32g0xx.h"

/*
 Структура для определения нужного i2c и его периферии
 */
typedef struct
{
	I2C_TypeDef *Instance; // I2C_TypeDef это описание регистров периферии I2C

	GPIO_TypeDef *SCL_Port; // GPIO_TypeDef это описание регистров периферии GPIO
	uint8_t SCL_Pin;
	uint8_t SCL_AF;

	GPIO_TypeDef *SDA_Port;
	uint8_t SDA_Pin;
	uint8_t SDA_AF;

	uint32_t Timing; // Параметры для настройки таймингов сигнала I2C, регистр TIMGR, зависит от частоты работы ядра и режима работы i2c

} I2C_Handle_t;


void I2C_Init(I2C_Handle_t *hi2c);
uint8_t I2C_ReadOneReg(I2C_Handle_t *hi2c, uint8_t addr, uint8_t reg);
void I2C_ReadRegs(I2C_Handle_t *hi2c, uint8_t addr, uint8_t reg, uint8_t *buf,uint8_t len);
void I2C_WriteOneReg(I2C_Handle_t *hi2c, uint8_t addr, uint8_t reg, uint8_t data);
void I2C_WriteRegs(I2C_Handle_t *hi2c, uint8_t addr, uint8_t reg, uint8_t *databuf, uint8_t len);
uint8_t I2C_Find_First_Address(I2C_Handle_t *hi2c);





#endif /* INC_IIC_BASIC_H_ */
