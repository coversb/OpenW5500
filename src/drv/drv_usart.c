/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          drv_usart.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   usart driver
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-2-12      1.00                     Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include <string.h>
#include "misc.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"

#include "datastruct.h"
#include "drv_board.h"
#include "drv_usart.h"
#include "drv_util.h"

/******************************************************************************
* Macros
******************************************************************************/

/******************************************************************************
* Local Functions define
******************************************************************************/
/*USART1*/
static void drv_usart1_begin(u32 baundrate);
static void drv_usart1_end(void);
static uint16 drv_usart1_available(void);
static void drv_usart1_write(uint8 byte);
static uint16 drv_usart1_write_bytes(uint8 *buff, uint16 length);
static uint8 drv_usart1_read(void);
static uint16 drv_usart1_read_bytes(uint8 *buff, uint16 length);
static void drv_usart1_print(char* str);
static void drv_usart1_println(char* str);

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
static ARRAY_QUEUE_TYPE drv_usart1_rx_que;
static uint8 DRV_USART1_RX_BUFF[DRV_USART1_RX_BUFFSIZE];

const DRV_USART_TYPE hwSerial1 = 
{
    drv_usart1_begin,
    drv_usart1_end,
    drv_usart1_available,
    drv_usart1_write,
    drv_usart1_write_bytes,
    drv_usart1_read,
    drv_usart1_read_bytes,
    drv_usart1_print,
    drv_usart1_println
};

/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : drv_usart_config
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : config usart function
******************************************************************************/
static void drv_usart_config(USART_TypeDef* USARTx, u32 baundrate)
{
    USART_DeInit(USARTx);
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = baundrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b; //8 data bit
    USART_InitStructure.USART_StopBits = USART_StopBits_1;  //stop bit
    USART_InitStructure.USART_Parity = USART_Parity_No; //no parity
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USARTx, &USART_InitStructure);
}

/*USART1 begin*/
/******************************************************************************
* Function    : USART1_IRQHandler
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : USART1 irq handler
******************************************************************************/
void USART1_IRQHandler(void)
{
    uint8 res = 0;
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        res = USART_ReceiveData(USART1);
        array_que_push(&drv_usart1_rx_que, res);
    }
}

/******************************************************************************
* Function    : drv_usart1_begin
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : setup and enable usart1 with baundrate
******************************************************************************/
static void drv_usart1_begin(u32 baundrate)
{
    /***************************************************************************
    * NOTE:
    * if you use this in "multithreading environment", 
    * it's better to add lock to protect the data in queue!
    ****************************************************************************/
    array_que_create(&drv_usart1_rx_que, DRV_USART1_RX_BUFF, sizeof(DRV_USART1_RX_BUFF));

    /*RCC config*/
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA|RCC_APB2Periph_AFIO, ENABLE);

    /*GPIO config*/
    drv_gpio_set_mode(DRV_BOARD_USART1_TX, GPIO_Mode_AF_PP);   //USART1 TX
    drv_gpio_set_mode(DRV_BOARD_USART1_RX, GPIO_Mode_IN_FLOATING);    //USART1 RX

    /*Usart 1 config*/
    drv_usart_config(USART1, baundrate);
    //enable rx irq
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
    //enable uasrt1
    USART_Cmd(USART1, ENABLE);
    //clear send flag
    USART_ClearFlag(USART1, USART_FLAG_TC);      

    /*irq priority config*/
    drv_board_nvic_set_irq(USART1_IRQn, DRV_BOARD_IQR_PRIO_USART1, DRV_BOARD_IQR_SUB_PRIO_USART1, ENABLE);
}

/******************************************************************************
* Function    : drv_usart1_end
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : disable usart1
******************************************************************************/
static void drv_usart1_end(void)
{
    USART_Cmd(USART1, DISABLE);
    array_que_destroy(&drv_usart1_rx_que);
    memset(DRV_USART1_RX_BUFF, 0x00, sizeof(DRV_USART1_RX_BUFF));
}

/******************************************************************************
* Function    : drv_usart1_available
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get queue vaild data size
******************************************************************************/
static uint16 drv_usart1_available(void)
{
    return array_que_size(&drv_usart1_rx_que);
}

/******************************************************************************
* Function    : drv_usart1_write
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : wtire one byte to usart1
******************************************************************************/
static void drv_usart1_write(uint8 byte)
{    
    USART_ClearFlag(USART1, USART_FLAG_TC);

    USART_SendData(USART1, byte);
    while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET){};
}

/******************************************************************************
* Function    : drv_usart1_write_bytes
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : write bytes to USART1
******************************************************************************/
static uint16 drv_usart1_write_bytes(uint8 *buff, uint16 length)
{
    USART_ClearFlag(USART1, USART_FLAG_TC);

    for (uint16 i = 0; i < length; i++)
    {
        USART_SendData(USART1, buff[i]);
        while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET){};
    }

    return length;
}

/******************************************************************************
* Function    : drv_usart1_read
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : read one byte from usart1
******************************************************************************/
static uint8 drv_usart1_read(void)
{
    return array_que_pop(&drv_usart1_rx_que);
}

/******************************************************************************
* Function    : drv_usart1_read_bytes
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : read bytes to buffer
******************************************************************************/
static uint16 drv_usart1_read_bytes(uint8 *buff, uint16 length)
{
    return array_que_packet_out(&drv_usart1_rx_que, (uint8*)buff, length);
}

/******************************************************************************
* Function    : drv_usart1_print
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void drv_usart1_print(char* str)
{
    USART_ClearFlag(USART1, USART_FLAG_TC);

    for (uint16 i = 0; i < strlen(str); i++)
    {
        USART_SendData(USART1, str[i]);
        while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET){};
    }
}

/******************************************************************************
* Function    : drv_usart1_println
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void drv_usart1_println(char* str)
{
    drv_usart1_print(str);
    drv_usart1_print("\r\n");
}
/*USART1 end*/

