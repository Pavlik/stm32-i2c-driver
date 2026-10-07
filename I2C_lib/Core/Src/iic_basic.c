/*
 * iic_basic.c
 *
 *  Created on: Aug 27, 2026
 *      Author: Navumchyk
 */

#include "iic_basic.h"



// включение тактирования GPIO
static void GPIO_EnableClock(GPIO_TypeDef *port)
{
    if (port == GPIOA)
        RCC->IOPENR |= RCC_IOPENR_GPIOAEN;

    else if (port == GPIOB)
        RCC->IOPENR |= RCC_IOPENR_GPIOBEN;

    else if (port == GPIOC)
        RCC->IOPENR |= RCC_IOPENR_GPIOCEN;
}

// конфигурация gpio на альтернативную функцию для i2c
static void GPIO_ConfigAF(GPIO_TypeDef *port, uint8_t pin, uint8_t af)
{
	port->MODER &= ~(3U << (pin * 2));
	port->MODER |= (2U << (pin * 2));

	port->OTYPER &= ~(1U << pin);
	port->OTYPER |= (1U << pin);

	port->OSPEEDR &= ~(3U << (pin * 2));
	port->OSPEEDR |= (3U << (pin * 2));

	if (pin < 8)
	{
		port->AFR[0] &= ~(0xF << (pin * 4));
		port->AFR[0] |= (af << (pin * 4));
	}
	else
	{
		port->AFR[1] &= ~(0xF << ((pin - 8) * 4));
		port->AFR[1] |= (af << ((pin - 8) * 4));
	}
}

// Включение тактирования I2C
static void I2C_EnableClock(I2C_TypeDef *i2c)
{
    if (i2c == I2C1)
        RCC->APBENR1 |= RCC_APBENR1_I2C1EN;

    else if (i2c == I2C2)
        RCC->APBENR1 |= RCC_APBENR1_I2C2EN;
}

// Инициализация I2C
void I2C_Init(I2C_Handle_t *hi2c)
{
    GPIO_EnableClock(hi2c->SCL_Port);
    GPIO_EnableClock(hi2c->SDA_Port);

    I2C_EnableClock(hi2c->Instance);

    GPIO_ConfigAF(
        hi2c->SCL_Port,
        hi2c->SCL_Pin,
        hi2c->SCL_AF);

    GPIO_ConfigAF(
        hi2c->SDA_Port,
        hi2c->SDA_Pin,
        hi2c->SDA_AF);

    hi2c->Instance->CR1 &= ~I2C_CR1_PE;

    hi2c->Instance->TIMINGR = hi2c->Timing;

    hi2c->Instance->CR1 |= I2C_CR1_PE;
}

static uint8_t WaitTXIS(I2C_TypeDef *i2c)
{
	uint32_t timeout = 10000;
    while(!(i2c->ISR & I2C_ISR_TXIS))
    {
        if(i2c->ISR & I2C_ISR_NACKF)
        {
           i2c->ICR = I2C_ICR_NACKCF;

           if(i2c->ISR & I2C_ISR_STOPF) // Чтобы не оставлять флаги висеть.
        	   i2c->ICR = I2C_ICR_STOPCF;
           return 0;
        }
       if(--timeout == 0)
    	   return 0;
    }
   return 1;
}


// for reading one byte
uint8_t I2C_ReadOneReg(I2C_Handle_t *hi2c, uint8_t addr, uint8_t reg)
{
	uint8_t data;
    while (hi2c->Instance->ISR & I2C_ISR_BUSY);

    hi2c->Instance->CR2 =
    						((uint32_t)addr << 1)
						| (1 << I2C_CR2_NBYTES_Pos);
    hi2c->Instance->CR2 |= I2C_CR2_START;
    if(!WaitTXIS(hi2c->Instance)) // проверка TXIS
    	return 0;
    hi2c->Instance->TXDR = reg;
    while (!(hi2c->Instance->ISR & I2C_ISR_TC));
    hi2c->Instance->CR2 =
						  (addr << 1)
						| (1 << I2C_CR2_NBYTES_Pos)
						| I2C_CR2_RD_WRN
						| I2C_CR2_AUTOEND;
    hi2c->Instance->CR2 |= I2C_CR2_START;
    while (!(hi2c->Instance->ISR & I2C_ISR_RXNE));
    data = hi2c->Instance->RXDR;
    while (!(hi2c->Instance->ISR & I2C_ISR_STOPF));
    hi2c->Instance->ICR = I2C_ICR_STOPCF;
    return data;
}

void I2C_ReadRegs(I2C_Handle_t *hi2c, uint8_t addr, uint8_t reg, uint8_t *buf,uint8_t len)
{
	while (hi2c->Instance->ISR & I2C_ISR_BUSY);
    hi2c->Instance->CR2 = ((uint32_t)addr << 1)
    					| (1 << I2C_CR2_NBYTES_Pos);

    hi2c->Instance->CR2 |= I2C_CR2_START;
    if(!WaitTXIS(hi2c->Instance)) // проверка TXIS
    	return;
    hi2c->Instance->TXDR = reg;
    while (!(hi2c->Instance->ISR & I2C_ISR_TC));
    hi2c->Instance->CR2 =
          (addr << 1)
          | (len << I2C_CR2_NBYTES_Pos)
          | I2C_CR2_RD_WRN
          | I2C_CR2_AUTOEND;
    hi2c->Instance->CR2 |= I2C_CR2_START;
    for(uint8_t i = 0; i < len; i++)
    {
        while (!(hi2c->Instance->ISR & I2C_ISR_RXNE));
        buf[i] = hi2c->Instance->RXDR;
    }
    while (!(hi2c->Instance->ISR & I2C_ISR_STOPF));
    hi2c->Instance->ICR = I2C_ICR_STOPCF;
}

void I2C_WriteOneReg(I2C_Handle_t *hi2c, uint8_t addr, uint8_t reg, uint8_t data)
{
	while (hi2c->Instance->ISR & I2C_ISR_BUSY);
	hi2c->Instance->ICR = I2C_ICR_STOPCF;
	hi2c->Instance->CR2 =
            ((uint32_t)addr << 1)
          | (2 << I2C_CR2_NBYTES_Pos)
          | I2C_CR2_AUTOEND;
	hi2c->Instance->CR2 |= I2C_CR2_START;
	if(!WaitTXIS(hi2c->Instance))
	    return;
	hi2c->Instance->TXDR = reg;

	if(!WaitTXIS(hi2c->Instance))
	    return;
    hi2c->Instance->TXDR = data;
    while (!(hi2c->Instance->ISR & I2C_ISR_STOPF));
    hi2c->Instance->ICR = I2C_ICR_STOPCF;
}


void I2C_WriteRegs(I2C_Handle_t *hi2c, uint8_t addr, uint8_t reg, uint8_t *databuf, uint8_t len)
{
    while (hi2c->Instance->ISR & I2C_ISR_BUSY);
    hi2c->Instance->ICR = I2C_ICR_STOPCF;
    hi2c->Instance->CR2 =
          ((uint32_t)addr << 1)
        | ((len + 1) << I2C_CR2_NBYTES_Pos)
        | I2C_CR2_AUTOEND;
    hi2c->Instance->CR2 |= I2C_CR2_START;

    if(!WaitTXIS(hi2c->Instance))
        return;
    hi2c->Instance->TXDR = reg;

    for(uint8_t i = 0; i < len; i++)
    {
    	if(!WaitTXIS(hi2c->Instance))
    	    return;
        hi2c->Instance->TXDR = databuf[i];
    }
    while (!(hi2c->Instance->ISR & I2C_ISR_STOPF));
    hi2c->Instance->ICR = I2C_ICR_STOPCF;
}

uint8_t I2C_Find_First_Address(I2C_Handle_t *hi2c)
{
	for(uint8_t a = 0; a < 128; a++)
	{
		hi2c->Instance->ICR = 0xFFFFFFFF;
		hi2c->Instance->CR2 =
							  ((uint32_t)a << 1)
							| (1 << I2C_CR2_NBYTES_Pos)
							| I2C_CR2_AUTOEND;

		hi2c->Instance->CR2 |= I2C_CR2_START;

	    while(!(hi2c->Instance->ISR & (I2C_ISR_NACKF | I2C_ISR_TXIS)));
	    if(hi2c->Instance->ISR & I2C_ISR_TXIS)
	    	return a;

	    while(!(hi2c->Instance->ISR & I2C_ISR_STOPF));
	    hi2c->Instance->ICR = I2C_ICR_STOPCF;
	}
	return 0xF;
}




