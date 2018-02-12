/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          drv_util.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   driver utilities
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-02-12             1.00                    Chen Hao
*
******************************************************************************/
#ifndef __DRV_UTIL_H__
#define __DRV_UTIL_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"
#include "stm32f10x_conf.h"
#include "stm32f10x_gpio.h"
#include "misc.h"

/******************************************************************************
* Macros
******************************************************************************/

/******************************************************************************
* Types
******************************************************************************/
typedef enum
{
    GPIO_LOW = 0,
    GPIO_HIGH = 1
}GPIO_VAL;

/******************************************************************************
* Extern variable
******************************************************************************/
extern void drv_util_systick_init(void);
extern uint32 drv_util_systick_millis(void);
extern void drv_util_delay_us(uint32 us);
extern void drv_util_delay_ms(uint32 ms);

extern void drv_gpio_set_mode(GPIO_TypeDef* io, uint16 pin, GPIOMode_TypeDef mode);
extern void drv_gpio_set(GPIO_TypeDef* GPIOx, uint16 GPIO_Pin, GPIO_VAL val);
extern GPIO_VAL drv_gpio_val(GPIO_TypeDef* GPIOx, uint16 GPIO_Pin);

#endif /*__DRV_UTIL_H__*/

