/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          drv_usart.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   usart driver
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:  2018-2-12      1.00                      Chen Hao
*
******************************************************************************/
#ifndef __DRV_USART_H__
#define __DRV_USART_H__

/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"

/******************************************************************************
* Macros
******************************************************************************/
#define DRV_USART1_RX_BUFFSIZE 256

/******************************************************************************
* Types
******************************************************************************/
typedef struct
{
    void (*begin)(uint32 baundrate);
    void (*end)(void);
    uint16 (*available)(void);
    void (*write)(uint8 byte);
    uint16 (*writeBytes)(uint8 *buff, uint16 len);
    uint8 (*read)(void);
    uint16 (*readBytes)(uint8 *buff, uint16 len);
    void (*print)(char* str);
    void (*println)(char* str);
}DRV_USART_TYPE;

/******************************************************************************
* Extern variable
******************************************************************************/
extern const DRV_USART_TYPE hwSerial1;   //USART1 for debug

#endif /*__PB_DRV_USART_H__*/
