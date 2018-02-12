/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          drv_spi.c
*  author:             Chen Hao
*  version:            1.00
*  file description:   spi driver
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-02-12             1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include <string.h>
#include "stm32f10x_spi.h"
#include "stm32f10x_rcc.h"
#include "misc.h"

#include "drv_board.h"
#include "drv_spi.h"
#include "drv_util.h"

/******************************************************************************
* Macros
******************************************************************************/

/******************************************************************************
* Local Functions define
******************************************************************************/
static void drv_spi2_begin(void);
static void drv_spi2_end(void);
static void drv_spi2_select(bool sw);
static void drv_spi2_write(uint8 byte);
static void drv_spi2_write_u16(uint16 dat);
static uint8 drv_spi2_read(void);

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
const DRV_SPI_TYPE hwSPI2 = 
{
    drv_spi2_begin,
    drv_spi2_end,
    drv_spi2_select,
    drv_spi2_write,
    drv_spi2_write_u16,
    drv_spi2_read,
};

/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : drv_spi_config
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : spi config
******************************************************************************/
static void drv_spi_config(SPI_TypeDef * SPIx)
{
    SPI_I2S_DeInit(SPIx);
    SPI_InitTypeDef SPI_InitStruct; 
    SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
    SPI_InitStruct.SPI_Direction= SPI_Direction_2Lines_FullDuplex;
    SPI_InitStruct.SPI_Mode = SPI_Mode_Master;
    SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStruct.SPI_CRCPolynomial = 7;

    SPI_Init(SPIx, &SPI_InitStruct);
}

/*SPI2 begin*/
/******************************************************************************
* Function    : drv_spi2_begin
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : enable SPI2
******************************************************************************/
static void drv_spi2_begin(void)
{
    /*RCC config*/
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOG|RCC_APB2Periph_AFIO, ENABLE);

    /*GPIO config*/
    drv_gpio_set_mode(DRV_BOARD_SPI2_CLK, GPIO_Mode_AF_PP);   //SPI2 CLK
    drv_gpio_set_mode(DRV_BOARD_SPI2_MISO, GPIO_Mode_AF_PP);   //SPI2 MISO
    drv_gpio_set_mode(DRV_BOARD_SPI2_MOSI, GPIO_Mode_AF_PP);   //SPI2 MOSI
    drv_gpio_set_mode(DRV_BOARD_W5500_CS, GPIO_Mode_Out_PP);  //SPI2 W5500 CS

    /*SPI2 config*/
    drv_spi_config(SPI2);

    /*enable spi2*/
    SPI_SSOutputCmd(SPI2, ENABLE);
    SPI_Cmd(SPI2, ENABLE);
}

/******************************************************************************
* Function    : drv_spi2_end
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : disable SPI2
******************************************************************************/
static void drv_spi2_end(void)
{
    SPI_SSOutputCmd(SPI2, DISABLE);
    SPI_Cmd(SPI2, DISABLE);
}

/******************************************************************************
* Function    : drv_spi2_select
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : SPI2 select w5500
******************************************************************************/
static void drv_spi2_select(bool sw)
{
    if (sw)
    {
        drv_gpio_set(DRV_BOARD_W5500_CS, GPIO_LOW);
    }
    else
    {
        drv_gpio_set(DRV_BOARD_W5500_CS, GPIO_HIGH);
    }
}

/******************************************************************************
* Function    : drv_spi2_write
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : write byte to SPI2
******************************************************************************/
static void drv_spi2_write(uint8 byte)
{
    while((SPI2->SR & SPI_I2S_FLAG_TXE) == 0);
    SPI2->DR = byte;
    while((SPI2->SR & SPI_I2S_FLAG_RXNE) == 0);
    SPI2->DR;
}

/******************************************************************************
* Function    : drv_spi2_write_short
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : write u16 data to SPI2
******************************************************************************/
static void drv_spi2_write_u16(uint16 dat)
{
    drv_spi2_write((uint8)(dat >> 8));
    drv_spi2_write((uint8)(dat & 0x00ff));
}

/******************************************************************************
* Function    : drv_spi2_read
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : read byte from SPI2
******************************************************************************/
static uint8 drv_spi2_read(void)
{
    while((SPI2->SR & SPI_I2S_FLAG_TXE) == 0);
    SPI2->DR = 0xFF;
    while((SPI2->SR & SPI_I2S_FLAG_RXNE) == 0);

    return SPI2->DR;
}
/*SPI2 end*/

