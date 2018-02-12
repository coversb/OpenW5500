/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          drv_util.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   driver utilities
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-02-12             1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include <stdio.h>
#include "System_stm32f10x.h"
#include "drv_board.h"
#include "drv_util.h"
#include "drv_usart.h"

/******************************************************************************
* Macros
******************************************************************************/

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
static volatile uint32 sysTick = 0;
//ticks for 1us
static uint32 US_TICKS = 0;
//ticks for 1ms
static uint32 MS_TICKS = 0;

/******************************************************************************
* Local Functions
******************************************************************************/
/*printf redirect to debug com begin*/
#pragma import(__use_no_semihosting)             
struct __FILE 
{ 
    int handle; 
};

FILE __stdout;

void _sys_exit(int x) 
{ 
    x = x; 
} 

int fputc(int ch, FILE *f)
{
    uint8 str[1] = {0};
    str[0] = (uint8)ch;
    hwSerial1.write(str[0]);
    
    return ch;
}

void _ttywrch(int ch)
{
    ch = ch;
}
/*printf redirect to debug com end*/

/******************************************************************************
* Function    : SysTick_Handler
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void SysTick_Handler(void)
{	
    sysTick++;    
}

/******************************************************************************
* Function    : drv_util_systick_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void drv_util_systick_init(void)
{
    US_TICKS = (SystemCoreClock / 1000000);
    MS_TICKS = (SystemCoreClock / 1000);
    
    SysTick_Config(MS_TICKS);
    NVIC_SetPriority(SysTick_IRQn, DRV_BOARD_IQR_PRIO_SYSTICK);
}

/******************************************************************************
* Function    : drv_util_systick_millis
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
uint32 drv_util_systick_millis(void)
{
    return sysTick;
}

/******************************************************************************
* Function    : drv_util_systick_micros
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint32 drv_util_systick_micros(void)
{
    register uint32 ms, cycle_cnt;

    do 
    {
        ms = sysTick;
        cycle_cnt = SysTick->VAL;
    } while (ms != sysTick);

    return (ms * 1000) + (MS_TICKS - cycle_cnt) / US_TICKS;
}

/******************************************************************************
* Function    : drv_delay_us
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void drv_util_delay_us(uint32 us)
{
    uint32 now = drv_util_systick_micros();

    while (drv_util_systick_micros() - now < us);
}

/******************************************************************************
* Function    : drv_util_delay_ms
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void drv_util_delay_ms(uint32 ms)
{
    while (ms--) 
    {
        drv_util_delay_us(1000);
    }
}

/******************************************************************************
* Function    : drv_gpio_set_mode
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : set gpio pin mode
******************************************************************************/
void drv_gpio_set_mode(GPIO_TypeDef* io, uint16 pin, GPIOMode_TypeDef mode)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = pin;
    GPIO_InitStructure.GPIO_Mode = mode;    
    GPIO_Init(io, &GPIO_InitStructure);
}

/******************************************************************************
* Function    : drv_gpio_set
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : set gpio pin value
******************************************************************************/
void drv_gpio_set(GPIO_TypeDef* GPIOx, uint16 GPIO_Pin, GPIO_VAL val)
{
    if (val == GPIO_LOW)
    {
        GPIO_ResetBits(GPIOx, GPIO_Pin);
    }
    else
    if (val == GPIO_HIGH)
    {
        GPIO_SetBits(GPIOx, GPIO_Pin);
    }
}

/******************************************************************************
* Function    : drv_gpio_val
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get gpio pin value
******************************************************************************/
GPIO_VAL drv_gpio_val(GPIO_TypeDef* GPIOx, uint16 GPIO_Pin)
{
    return (GPIO_VAL)GPIO_ReadInputDataBit(GPIOx, GPIO_Pin);
}

