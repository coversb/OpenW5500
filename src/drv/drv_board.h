/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          drv_board.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   board info define
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-02-12             1.00                    Chen Hao
*
******************************************************************************/
#ifndef __DRV_BOARD_H__
#define __DRV_BOARD_H__

/******************************************************************************
* Include Files
******************************************************************************/
#include "misc.h"
#include "stm32f10x_conf.h"
#include "stm32f10x_gpio.h"
#include "basetype.h"

/******************************************************************************
* Macros
******************************************************************************/
#define DRV_BOARD_BASE FLASH_BASE
#define DRV_BOARD_APP_OFFSET 0

/*IRQ PRIORITY DEFINE*/
#define DRV_BOARD_IQR_PRIO_SYSTICK 0
#define DRV_BOARD_IQR_PRIO_USART1 1
#define DRV_BOARD_IQR_SUB_PRIO_USART1 2

/*GPIO DEFINE*/
//debug com
#define DRV_BOARD_USART1_TX GPIOA, GPIO_Pin_9
#define DRV_BOARD_USART1_RX GPIOA, GPIO_Pin_10
//W5500
#define DRV_BOARD_SPI2_CLK GPIOB, GPIO_Pin_13
#define DRV_BOARD_SPI2_MISO GPIOB, GPIO_Pin_14
#define DRV_BOARD_SPI2_MOSI GPIOB, GPIO_Pin_15
#define DRV_BOARD_W5500_CS GPIOG, GPIO_Pin_15
#define DRV_BOARD_W5500_RST GPIOG, GPIO_Pin_14

/******************************************************************************
* Types
******************************************************************************/

/******************************************************************************
* Extern variable
******************************************************************************/
extern void drv_board_init(void);
extern void drv_board_nvic_set_irq(uint8 IRQChannel, uint8 PreemptionPriority, uint8 SubPriority, FunctionalState Cmd);

#endif /*__DRV_BOARD_H__*/

