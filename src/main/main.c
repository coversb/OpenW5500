/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          main.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   demo
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
#include <string.h>
#include "drv_board.h"
#include "drv_util.h"
#include "drv_usart.h"
#include "dev_w5500.h"
#include "util.h"

/******************************************************************************
* Macros
******************************************************************************/
#define DEBUG_COM hwSerial1

//#define SERVER_DOMAIN "cloud.yourserver.com"
#define SERVER_DOMAIN "192.168.1.100"
#define SERVER_PORT 60000

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/

/******************************************************************************
* Local Functions
******************************************************************************/
void sys_init(void)
{
    //board hw init
    drv_board_init();
    drv_util_systick_init();
    DEBUG_COM.begin(115200);
    util_delay(DELAY_100_MS);

    DEBUG_TRACE("Open-W500 %s @%s-%s", OPENW5500_VERSION, __DATE__, __TIME__);
}

int main (void)
{
    sys_init();

reset:
    //init w5500
    dev_w5500_hw_reset();
    //check cable
    while (!dev_w5500_cable_link())
    {
        util_delay(DELAY_1_S);//wait link ok
    }
    //config w5500
    dev_w5500_net_config();
    //connect to server
    char serverDomain[] = SERVER_DOMAIN;
    uint16 serverPort = SERVER_PORT;
    dev_w5500_net_connect(serverDomain, serverPort);

    uint8 sendData[] = "TEST";
    uint8 recvData[1024] = {0};
    uint16 recvLen = 0;
    while (1)
    {
        //try to send data
        if (false == dev_w5500_net_send(sendData, sizeof(sendData)))
        {
            DEBUG_TRACE("Send data error");
            goto reset;
        }

        //try to recv data
        memset(recvData, 0, sizeof(recvData));
        recvLen = dev_w5500_net_recv(recvData, sizeof(recvData));
        DEBUG_TRACE("RECV[%d]:%s", recvLen, recvData);
        
        util_delay(DELAY_1_S);
    }
    
}

