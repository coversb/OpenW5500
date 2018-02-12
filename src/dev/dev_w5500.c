/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          dev_w5500.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   wiznet w5500 operation
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
#include <stdlib.h>
#include <string.h>

#include "drv_util.h"
#include "drv_spi.h"
#include "dev_w5500.h"
#include "util.h"
#include "dhcp.h"
#include "dns.h"

/******************************************************************************
* Macros
******************************************************************************/
#define DRV_BOARD_W5500_RST GPIOG, GPIO_Pin_14

#define LOCAL_PORT_POOL_SIZE 8000
#define COMM_LOCAL_PORT 10000
#define FTP_CTRL_LOCAL_PORT 20000
#define FTP_DATA_LOCAL_PORT 30000
#define DNS_LOCAL_PORT 40000

#define w5500_spi hwSPI2

#define DHCP_CHECK_TIMEOUT 200   // 100ms * 200 count

#define DEV_W5500_CMD_WATI DELAY_500_MS
#define DEV_W5500_CMD_RETRY 10

#define DHCP SOCKET0
#define DNS SOCKET0
#define SOCKET SOCKET1
#define FTP_CTRL SOCKET2
#define FTP_DATA SOCKET3

#define FTP_CMD_LEN 128
#define DEV_W5500_FTP_FILED_LEN 64
#define FTP_RSP_TIMEOUT_CNT 100   // 100 * 50MS
#define FTP_FILE_DATABLOCK_SIZE 2048
#define FTP_CONNECT_OK "220"
#define FTP_USERNAME_OK "331"
#define FTP_LOGIN_OK "230"
#define FTP_REQ_DIR_OK "250"
#define FTP_TYPE_I_OK "200"
#define FTP_PASV_OK "227"
#define FTP_OPENING_FILE_OK "150"
#define FTP_GET_FILE_OK "226"
#define FTP_RSET_OK "350"
#define FTP_REST_CMD "REST 0\r\n"
#define FTP_PASV_CMD "PASV\r\n"

//#define __ETH_DEBUG__
/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
//DNS server
static const uint8 DEF_DNS_SERVER[4] = {114, 114, 114, 114};
static const uint8 DEF_W5500_MAC[6] = {0x08, 0x60, 0x02, 0x10, 0xAA, 0xBB};
#ifdef __ETH_DEBUG__
//IP addr
static const uint8 DEF_IP_ADDR[4] = {192, 16, 8, 222};
//Net mask
static const uint8 DEF_NETMASK[4] = {255, 255, 255, 0};
//Gateway
static const uint8 DEF_GATEWAY[4] = {192, 16, 8, 254};
#endif /*__ETH_DEBUG__*/

//dhcp client context
static DHCP_CLIENT w5500_dhcp_client;
static DNS_CLIENT w5500_dns_client;

/******************************************************************************
* Local Functions define
******************************************************************************/
static uint8 dev_w5500_read_sock_byte(DEV_SOCKET s, uint16 reg);
static uint16 dev_w5500_read_sock_short(DEV_SOCKET s, uint16 reg);

/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : dev_w5500_write_byte
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : write byte to w5500 reg
******************************************************************************/
static void dev_w5500_write_byte(uint16 reg, uint8 dat)
{
    w5500_spi.select(true);

    w5500_spi.write_u16(reg);
    w5500_spi.write(FDM1 | RWB_WRITE | COMMON_R);
    w5500_spi.write(dat);
    
    w5500_spi.select(false);
}

/******************************************************************************
* Function    : dev_w5500_write_short
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : write short to w5500 reg
******************************************************************************/
static void dev_w5500_write_short(uint16 reg, uint16 dat)
{
    w5500_spi.select(true);

    w5500_spi.write_u16(reg);
    w5500_spi.write(FDM2 | RWB_WRITE | COMMON_R);
    w5500_spi.write_u16(dat);
    
    w5500_spi.select(false);
}

/******************************************************************************
* Function    : dev_w5500_write_nbyte
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : write n bytes to w5500 reg
******************************************************************************/
static void dev_w5500_write_nbyte(uint16 reg, const uint8 *pdat, uint16 size)
{
    uint16 idx;

    w5500_spi.select(true);

    w5500_spi.write_u16(reg);
    w5500_spi.write(VDM | RWB_WRITE | COMMON_R);

    for(idx = 0; idx < size; idx++) 
    {
        w5500_spi.write(*pdat);
        ++pdat;
    }
    
    w5500_spi.select(false);
}

/******************************************************************************
* Function    : dev_w5500_read_byte
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : read byte from reg
******************************************************************************/
static uint8 dev_w5500_read_byte(uint16 reg)
{
    uint8 byte = 0;

    w5500_spi.select(true);

    w5500_spi.write_u16(reg);
    w5500_spi.write(FDM1 | RWB_READ | COMMON_R);
    byte = w5500_spi.read();

    w5500_spi.select(false);

    return byte;
}

/******************************************************************************
* Function    : dev_w5500_write_sock_byte
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : write byte to socket reg
******************************************************************************/
static void dev_w5500_write_sock_byte(DEV_SOCKET s, uint16 reg, uint8 dat)
{
    w5500_spi.select(true);

    w5500_spi.write_u16(reg);
    w5500_spi.write(FDM1 | RWB_WRITE | (s*0x20+0x08));
    w5500_spi.write(dat);
    
    w5500_spi.select(false);
}

/******************************************************************************
* Function    : dev_w5500_write_sock_short
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : write u16 to socket reg
******************************************************************************/
static void dev_w5500_write_sock_short(DEV_SOCKET s, uint16 reg, uint16 dat)
{
    w5500_spi.select(true);

    w5500_spi.write_u16(reg);
    w5500_spi.write(FDM2 | RWB_WRITE | (s*0x20+0x08));
    w5500_spi.write_u16(dat);
    
    w5500_spi.select(false);
}

/******************************************************************************
* Function    : dev_w5500_write_sock_4byte
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : write 4 bytes to socket reg
******************************************************************************/
static void dev_w5500_write_sock_4byte(DEV_SOCKET s, uint16 reg, const uint8 *pdat)
{
    uint16 idx;

    w5500_spi.select(true);

    w5500_spi.write_u16(reg);
    w5500_spi.write(FDM4 | RWB_WRITE | (s*0x20+0x08));
	
    for(idx = 0; idx < 4; idx++) 
    {
        w5500_spi.write(*pdat);
        ++pdat;
    }

    w5500_spi.select(false);
}

/******************************************************************************
* Function    : dev_w5500_write_sock_buffer
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : write datas to socket
******************************************************************************/
static uint16 dev_w5500_write_sock_buffer(DEV_SOCKET s, uint8 *pbuf, uint16 size)
{
    uint16 cal_offset = 0;
    uint16 record_offset = 0;
    uint16 idx = 0;

    cal_offset = dev_w5500_read_sock_short(s, Sn_TX_WR);
    record_offset = cal_offset;

    cal_offset &= (DEV_W5500_TX_BUFF - 1); //计算实际的物理地址

    w5500_spi.select(true);
    
    w5500_spi.write_u16(cal_offset);
    w5500_spi.write(VDM | RWB_WRITE | (s*0x20+0x10));

    //如果最大地址未超过W5500发送缓冲区寄存器的最大地址
    if ((cal_offset + size) < DEV_W5500_TX_BUFF)
    {
        for (idx = 0; idx < size; idx++) 
        {
            w5500_spi.write(*pbuf);
            ++pbuf;
        }
    }
    else 
    {
        cal_offset = DEV_W5500_TX_BUFF - cal_offset;
        for (idx = 0; idx < cal_offset; idx++) 
        {
            w5500_spi.write(*pbuf);
            ++pbuf;
        }
        w5500_spi.select(false);

        w5500_spi.select(true);
        w5500_spi.write_u16(0x00);
        w5500_spi.write(VDM | RWB_WRITE | (s*0x20+0x10));
        
        for (; idx < size; idx++) 
        {
            w5500_spi.write(*pbuf);
            ++pbuf;
        }
    }
    
    w5500_spi.select(false);
	
    record_offset += size;
    //更新实际物理地址,即下次写待发送数据到发送数据缓冲区的起始地址
    dev_w5500_write_sock_short(s, Sn_TX_WR, record_offset);
    dev_w5500_write_sock_byte(s, Sn_CR, SEND); //发送启动发送命令

    uint8 timeout = 0;
    while (dev_w5500_read_sock_byte(s, Sn_CR))
    {
        util_delay(DEV_W5500_CMD_WATI);
        timeout++;
        if (timeout > DEV_W5500_CMD_RETRY)
        {
            DEBUG_TRACE("Sock send err");
            return 0;
        }
    }

    timeout = 0;
    uint8 sendStat;
    while (1)
    {
        sendStat = dev_w5500_read_sock_byte(s, Sn_IR);
        DEBUG_TRACE("Write buff[%02X]", sendStat);
        if (sendStat & IR_SEND_OK)
        {
            dev_w5500_write_sock_byte(s, Sn_IR, IR_SEND_OK);
            break;
        }
        else
        if (sendStat & IR_TIMEOUT)
        {
            dev_w5500_write_sock_byte(s, Sn_IR, IR_TIMEOUT);
            DEBUG_TRACE("Sock send timeout");
            return 0;
        }
        
        util_delay(DEV_W5500_CMD_WATI);
        timeout++;
        if (timeout > DEV_W5500_CMD_RETRY)
        {
            DEBUG_TRACE("Sock send cnt out");
            return 0;
        }
    }

    return size;
}

/******************************************************************************
* Function    : dev_w5500_read_sock_byte
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : read byte from socket reg
******************************************************************************/
static uint8 dev_w5500_read_sock_byte(DEV_SOCKET s, uint16 reg)
{
    uint8 byte = 0;
	
    w5500_spi.select(true);

    w5500_spi.write_u16(reg);
    w5500_spi.write(FDM1 | RWB_READ | (s*0x20+0x08));
    byte = w5500_spi.read();

    w5500_spi.select(false);

    return byte;
}

/******************************************************************************
* Function    : dev_w5500_read_sock_short
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : read u16 from socket reg
******************************************************************************/
static uint16 dev_w5500_read_sock_short(DEV_SOCKET s, uint16 reg)
{
    uint16 dat;
	
    w5500_spi.select(true);

    w5500_spi.write_u16(reg);
    w5500_spi.write(FDM2 | RWB_READ | (s*0x20+0x08));
    dat = w5500_spi.read();
    dat <<= 8;
    dat |= w5500_spi.read();

    w5500_spi.select(false);

    return dat;
}

/******************************************************************************
* Function    : dev_w5500_read_sock_buffer
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : read datas from socket
******************************************************************************/
static uint16 dev_w5500_read_sock_buffer(DEV_SOCKET s, uint8 *pbuf, uint16 mxsize)
{
    uint16 rx_size = 0;
    uint16 available_rx_size = 0;
    uint16 offset = 0;
    uint16 record_offset = 0;
    uint16 idx = 0;
    uint8 read_byte = 0;
    unsigned int count = 0;
 
    available_rx_size = dev_w5500_read_sock_short(s, Sn_RX_RSR);

    if (available_rx_size == 0)
    {
        return 0;
    }
    

    rx_size = MIN_VALUE(available_rx_size, DEV_W5500_SOCKET_RECV_MAX_SIZE);
    rx_size = MIN_VALUE(rx_size, mxsize);

    offset = dev_w5500_read_sock_short(s, Sn_RX_RD);
    record_offset = offset;

    //calculate real phy address
    offset &= (DEV_W5500_RX_BUFF - 1);

    w5500_spi.select(true);

    w5500_spi.write_u16(offset);//write phy address
    w5500_spi.write(VDM | RWB_READ | (s*0x20+0x18));

    if ((offset + rx_size) < DEV_W5500_RX_BUFF) 
    {
        for (idx = 0; idx < rx_size; idx++)
        {
            read_byte = w5500_spi.read();
            if (++count <= mxsize) 
            {
                *pbuf = read_byte;
                ++pbuf;
            }
        }
    } 
    else 
    {
        offset = DEV_W5500_RX_BUFF - offset;
        for (idx = 0; idx < offset; idx++) 
        {
            read_byte = w5500_spi.read();
            if (++count <= mxsize) 
            {
                *pbuf = read_byte;
                ++pbuf;
            }
        }
        w5500_spi.select(false);
		
        w5500_spi.select(true);
		
        w5500_spi.write_u16(0x00);
        w5500_spi.write(VDM | RWB_READ | (s*0x20+0x18));
        for (; idx < rx_size; idx++) 
        {
            read_byte = w5500_spi.read();
            if (++count <= mxsize) 
            {
                *pbuf = read_byte;
                ++pbuf;
            }
         }
    }
	
    w5500_spi.select(false);
	
    record_offset += rx_size; //更新实际物理地址,即下次读取接收到的数据的起始地址
    dev_w5500_write_sock_short(s, Sn_RX_RD, record_offset);
    dev_w5500_write_sock_byte(s, Sn_CR, RECV); //发送启动接收命令

    return count < mxsize ? rx_size : mxsize;
}

/******************************************************************************
* Function    : dev_w5500_get_random_local_port
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint16 dev_w5500_get_random_local_port(uint16 min)
{
    uint16 port = (uint16)(util_get_random_num(0xFFFF) % (LOCAL_PORT_POOL_SIZE + 1) + min);
    DEBUG_TRACE("Get local port %d", port);
    return port;
}

/******************************************************************************
* Function    : dev_w5500_tcp_socket_create
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : create socket
******************************************************************************/
static bool dev_w5500_socket(DEV_SOCKET s, uint8 protType, uint16 localPort)
{
    bool ret = false;
    uint8 timeout = 0;
    
    dev_w5500_write_sock_short(s, Sn_MSSR, DEV_W5500_SOCKET_RECV_MAX_SIZE);
    dev_w5500_write_sock_short(s, Sn_PORT, localPort);

    dev_w5500_write_sock_byte(s, Sn_MR, protType);

    dev_w5500_write_sock_byte(s, Sn_CR, OPEN);
    ret = true;
    while (dev_w5500_read_sock_byte(s, Sn_CR))
    {
        util_delay(DEV_W5500_CMD_WATI);
        timeout++;
        if (timeout > DEV_W5500_CMD_RETRY)
        {
            ret = false;
            break;
        }
    }

    if (!ret)
    {
        DEBUG_TRACE("Socket[%02X] create err", protType);
    }

    return ret;
}

/******************************************************************************
* Function    : dev_w5500_connect
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : tcp socket connect to server
******************************************************************************/
static bool dev_w5500_connect(DEV_SOCKET s, uint8 *ip, uint16 port)
{
    bool ret = true;
    uint8 timeout = 0;

    //set server IP
    dev_w5500_write_sock_4byte(s, Sn_DIPR, ip);
    //set server port
    dev_w5500_write_sock_short(s, Sn_DPORTR, port);

    dev_w5500_write_sock_byte(s, Sn_CR, CONNECT);

    while (dev_w5500_read_sock_byte(s, Sn_CR))
    {
        util_delay(DEV_W5500_CMD_WATI);
        timeout++;
        if (timeout > DEV_W5500_CMD_RETRY)
        {
            ret = false;
            DEBUG_TRACE("Connect err");
            break;
        }
    }

    timeout = 0;
    while(dev_w5500_read_sock_byte(s, Sn_SR) != SOCK_ESTABLISHED)
    {
        uint8 stat = dev_w5500_read_sock_byte(s, Sn_IR);
        DEBUG_TRACE("Connect[%02X]", stat);
        if (stat & IR_TIMEOUT || stat & IR_DISCON)
        {
            dev_w5500_write_sock_byte(s, Sn_IR, IR_TIMEOUT);
            dev_w5500_write_sock_byte(s, Sn_IR, IR_DISCON);

            ret = false;
            DEBUG_TRACE("Connect timeout");
            break;
        }
        
        util_delay(DEV_W5500_CMD_WATI);
        timeout++;
        if (timeout > DEV_W5500_CMD_RETRY)
        {
            ret = false;
            DEBUG_TRACE("Connect cnt out");
            break;
        }
    }
    
    return ret;
}

/******************************************************************************
* Function    : dev_w5500_disconnect
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : tcp socket disconnect to server
******************************************************************************/
static bool dev_w5500_disconnect(DEV_SOCKET s)
{
    bool ret = true;
    uint8 timeout = 0;

    dev_w5500_write_sock_byte(s, Sn_CR, DISCON);

    while (dev_w5500_read_sock_byte(s, Sn_CR))
    {
        util_delay(DEV_W5500_CMD_WATI);
        timeout++;
        if (timeout > DEV_W5500_CMD_RETRY)
        {
            ret = false;
            DEBUG_TRACE("Sock disconnect err");
            break;
        }
    }

    timeout = 0;
    while(dev_w5500_read_sock_byte(s, Sn_SR) != SOCK_CLOSED)
    {
        if (dev_w5500_read_sock_byte(s, Sn_IR) & IR_TIMEOUT)
        {
            dev_w5500_write_sock_byte(s, Sn_IR, IR_TIMEOUT);
            ret = false;
            DEBUG_TRACE("Sock disconnect timeout");
            break;
        }
        
        util_delay(DEV_W5500_CMD_WATI);
        timeout++;
        if (timeout > DEV_W5500_CMD_RETRY)
        {
            ret = false;
            DEBUG_TRACE("Sock disconnect cnt out");
            break;
        }
    }
    
    return ret;
}

/******************************************************************************
* Function    : dev_w5500_close
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : tcp socket close
******************************************************************************/
static bool dev_w5500_close(DEV_SOCKET s)
{
    bool ret = true;
    uint8 timeout = 0;

    //close socket
    dev_w5500_write_sock_byte(s, Sn_CR, CLOSE);

    while (dev_w5500_read_sock_byte(s, Sn_CR))
    {
        util_delay(DEV_W5500_CMD_WATI);
        timeout++;
        if (timeout > DEV_W5500_CMD_RETRY)
        {
            ret = false;
            DEBUG_TRACE("Sock close err");
            break;
        }
    }
    
    //clear all interrupt of the socket
    dev_w5500_write_sock_byte(s, Sn_IR, 0xFF);

    timeout = 0;
    while(dev_w5500_read_sock_byte(s, Sn_SR) != SOCK_CLOSED)
    {
        if (dev_w5500_read_sock_byte(s, Sn_IR) & IR_TIMEOUT)
        {
            dev_w5500_write_sock_byte(s, Sn_IR, IR_TIMEOUT);
            ret = false;
            DEBUG_TRACE("Sock close timeout");
            break;
        }
        
        util_delay(DEV_W5500_CMD_WATI);
        timeout++;
        if (timeout > DEV_W5500_CMD_RETRY)
        {
            ret = false;
            DEBUG_TRACE("Sock close cnt out");
            break;
        }
    }
    
    return ret;
}

/******************************************************************************
* Function    : dev_w500_socket_send
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : send datas to socket
******************************************************************************/
static bool dev_w500_socket_send(DEV_SOCKET s, uint8 *pbuf, uint16 buf_size)
{
    if (dev_w5500_read_sock_byte(s, Sn_SR)  != SOCK_ESTABLISHED)
    {
        DEBUG_TRACE("Sock stat error");
        return false;
    }
    
    if (0 == dev_w5500_write_sock_buffer(s, pbuf, buf_size))
    {
        DEBUG_TRACE("Sock send none");
        return false;
    }

    return true;
}

/******************************************************************************
* Function    : dev_w500_socket_recv
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : recv datas from socket
******************************************************************************/
static uint16 dev_w500_socket_recv(DEV_SOCKET s, uint8 *pbuf, uint16 maxLen)
{
    return dev_w5500_read_sock_buffer(s, pbuf, maxLen);
}

/******************************************************************************
* Function    : dev_w5500_socket_sendto
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint16 dev_w5500_socket_sendto(DEV_SOCKET s, uint8 *pbuf, uint16 len, uint8 *addr, uint16 port)
{
    uint16 sendLen = 0;
    
    if (addr[0] == 0 && addr[1] == 0 && addr[2] == 0 && addr[3] == 0)
    {
        return 0;
    }
    
    //set server IP
    dev_w5500_write_sock_4byte(s, Sn_DIPR, addr);
    //set server port
    dev_w5500_write_sock_short(s, Sn_DPORTR, port);

    if (0 == (sendLen = dev_w5500_write_sock_buffer(s, pbuf, len)))
    {
        DEBUG_TRACE("Sock sendto err");
        return 0;
    }
    DEBUG_TRACE("Sendto[%d.%d.%d.%d:%d][%d]", addr[0], addr[1], addr[2], addr[3], port, sendLen);
    
    return sendLen;
}

/******************************************************************************
* Function    : dev_w5500_socket_recvfrom
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint16 dev_w5500_socket_recvfrom(DEV_SOCKET s, uint8 *pbuf, uint16 maxLen, uint8 *addr, uint16 *port)
{
    uint8 head[8];
    uint16 recvLen;
    uint16 realRead;
    
    realRead = dev_w5500_read_sock_buffer(s, head, 8);

    if (realRead != 0)
    {
        addr[0] = head[0];
        addr[1] = head[1];
        addr[2] = head[2];
        addr[3] = head[3];
        *port = (head[4] << 8) + head[5];
        recvLen = (head[6] << 8) + head[7];

        DEBUG_TRACE("Recvfrom[%d][%d.%d.%d.%d:%d][%d]",
                                    realRead, addr[0], addr[1], addr[2], addr[3], *port, recvLen);

        recvLen = MIN_VALUE(recvLen, maxLen);
        realRead = dev_w5500_read_sock_buffer(s, pbuf, recvLen);
    }
    
    DEBUG_TRACE("Recvfrom[%d]", realRead);
    return realRead;
}

/******************************************************************************
* Function    : dev_w5500_hw_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : w5500 gpio init
******************************************************************************/
void dev_w5500_hw_init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOG, ENABLE);

    drv_gpio_set_mode(DRV_BOARD_W5500_RST, GPIO_Mode_Out_PP);   //W5500 hw reset
    
    DEBUG_TRACE("HW INIT OK");
}

/******************************************************************************
* Function    : dev_w5500_hw_reset
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : w5500 module re-open and spi2 re-init
******************************************************************************/
void dev_w5500_hw_reset(void)
{
    //w5500 hw reset
    drv_gpio_set(DRV_BOARD_W5500_RST, GPIO_LOW);
    util_delay(DELAY_100_MS);
    drv_gpio_set(DRV_BOARD_W5500_RST, GPIO_HIGH);

    w5500_spi.end();
    w5500_spi.begin();
    //wait spi and w5500 stable
    util_delay(DELAY_1_S*2);

    uint8 ver = 0;
    ver = dev_w5500_read_byte(VERR);
    
    DEBUG_TRACE("HW RST OK, VER[%d]", ver);
}

/******************************************************************************
* Function    : dev_w5500_cable_link
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : check cable link check with 5s timeout
******************************************************************************/
bool dev_w5500_cable_link(void)
{
    #if 0   //Added 2 second delay in w5500 hw reset, so it's unnecessery to add delay here
    uint16 timeout = 30;
    while (0 == (dev_w5500_read_byte(PHYCFGR) & LINK))
    {
        //util_delay(DELAY_100_MS);
        util_delay(DELAY_1_MS);
        if (--timeout == 0)
        {
            DEBUG_TRACE("No eth cable link");
            return false;
        }
    }
    return true;
    #else
    if (0 == (dev_w5500_read_byte(PHYCFGR) & LINK))
    {
        DEBUG_TRACE("No eth cable link");
        return false;
    }
    return true;
    #endif
}

/******************************************************************************
* Function    : dev_w5500_net_util_sock_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : set util sock buffer (for dhcp, dns)
******************************************************************************/
static void dev_w5500_net_util_sock_init(void)
{
    //send/recv buff 4KB
    dev_w5500_write_sock_byte(DHCP, Sn_RXBUF_SIZE, DEV_W5500_RX_CFG);
    dev_w5500_write_sock_byte(DHCP, Sn_TXBUF_SIZE, DEV_W5500_TX_CFG);

    dev_w5500_write_sock_short(DHCP, Sn_RX_RD, 0);
    dev_w5500_write_sock_short(DHCP, Sn_TX_WR, 0);
}

/******************************************************************************
* Function    : dev_w5500_net_dhcp_client_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : init for dhcp client to get local ip addr
******************************************************************************/
static void dev_w5500_net_dhcp_client_init(void)
{
    uint8 nullIP[4] = {0};

    //mac
    dev_w5500_write_nbyte(SHAR, DEF_W5500_MAC, sizeof(DEF_W5500_MAC));
    //gate way
    dev_w5500_write_nbyte(GAR, nullIP, sizeof(nullIP));
    //network mask
    dev_w5500_write_nbyte(SUBR, nullIP, sizeof(nullIP));
    //ip addr
    dev_w5500_write_nbyte(SIPR, nullIP, sizeof(nullIP));

    dhcp_client_init(&w5500_dhcp_client);
    
    memcpy(w5500_dhcp_client.mac, DEF_W5500_MAC, sizeof(w5500_dhcp_client.mac));
    w5500_dhcp_client.sock = DHCP;
    w5500_dhcp_client.sendto = dev_w5500_socket_sendto;
    w5500_dhcp_client.recvfrom = dev_w5500_socket_recvfrom;

    DEBUG_TRACE("DHCP client init OK");
}

/******************************************************************************
* Function    : dev_w5500_net_config_update
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : update ip addr by DHCP
******************************************************************************/
static void dev_w5500_net_config_update(DHCP_CLIENT *dhcp)
{
    DEBUG_TRACE("DHCP IP update, IP[%d.%d.%d.%d], GW[%d.%d.%d.%d], MASK[%d.%d.%d.%d], DNS[%d.%d.%d.%d]",
                                dhcp->ip[0], dhcp->ip[1], dhcp->ip[2], dhcp->ip[3],
                                dhcp->gateway[0], dhcp->gateway[1], dhcp->gateway[2], dhcp->gateway[3], 
                                dhcp->mask[0], dhcp->mask[1], dhcp->mask[2], dhcp->mask[3],
                                dhcp->dns[0], dhcp->dns[1], dhcp->dns[2], dhcp->dns[3]);

    //gateway
    dev_w5500_write_nbyte(GAR, dhcp->gateway, sizeof(dhcp->gateway));
    //network mask
    dev_w5500_write_nbyte(SUBR, dhcp->mask, sizeof(dhcp->mask));
    //ip addr
    dev_w5500_write_nbyte(SIPR, dhcp->ip, sizeof(dhcp->ip));
}

/******************************************************************************
* Function    : dev_w5500_net_dhcp_check
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : check dhcp state and update ip address
******************************************************************************/
static bool dev_w5500_net_dhcp_check(bool bInit)
{
    bool ret = false;
    bool dhcp_process = true;
    uint8 timeoutCnt = 0;
    uint8 stat;

    dev_w5500_net_util_sock_init();

    if (bInit)
    {
        dev_w5500_net_dhcp_client_init();
    }

    ret = dev_w5500_socket(DHCP, SOCK_DGRAM, DHCP_CLIENT_PORT);
    if (!ret)
    {
        DEBUG_TRACE("DHCP client sock create err");
        goto err;
    }

    while(dhcp_process)
    {
        stat = dhcp_client_process(&w5500_dhcp_client);
        switch(stat)
        {
            case DHCP_RET_UPDATE:
            {
                dev_w5500_net_config_update(&w5500_dhcp_client);
                timeoutCnt = 0;
                dhcp_process = false;
                ret = true;
                break;
            }
            case DHCP_RET_CONFLICT:
            {
                DEBUG_TRACE("DHCP IP conflict");
                timeoutCnt = 0;
                dhcp_process = false;
                ret = false;
                break;
            }
            case DHCP_RET_LEASED_IP:
            {
                DEBUG_TRACE("DHCP IP LEASED");
                timeoutCnt = 0;
                dhcp_process = false;
                ret = true;
                break;
            }
            case DHCP_RET_NONE:
            default:
            {
                break;
            }
        }

        util_delay(DELAY_100_MS);
        if (++timeoutCnt > DHCP_CHECK_TIMEOUT)
        {
            DEBUG_TRACE("DHCP timeout");
            ret = false;
            break;
        }
    }

err:
    if (!dev_w5500_close(DHCP))
    {
        DEBUG_TRACE("DHCP client sock close err");
    }

    return ret;
}

/******************************************************************************
* Function    : dev_w5500_dhcp_check_lease_time
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool dev_w5500_dhcp_check_lease_time(void)
{
    //When lease time pass half, we need renew ip
    uint32 curTime = util_systick_second();
    if ((w5500_dhcp_client.leaseTime != INFINITE_LEASETIME) 
        && ((curTime - w5500_dhcp_client.lastTime) >(w5500_dhcp_client.leaseTime/2))) 
    {
        DEBUG_TRACE("Need lease IP by DHCP[%d]", (curTime - w5500_dhcp_client.lastTime));
        if (!dev_w5500_net_dhcp_check(false))
        {
            DEBUG_TRACE("DHCP lease IP err");
            return false;
        }
        else
        {
            DEBUG_TRACE("DHCP[%d] lease time[%d]", 
                                        w5500_dhcp_client.state, w5500_dhcp_client.leaseTime);
        }
    }
    return true;
}

/******************************************************************************
* Function    : dev_w5500_net_dns_client_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void dev_w5500_net_dns_client_init(void)
{
    dns_client_init(&w5500_dns_client);
    
    w5500_dns_client.sock = DNS;
    w5500_dns_client.sendto = dev_w5500_socket_sendto;
    w5500_dns_client.recvfrom = dev_w5500_socket_recvfrom;

    DEBUG_TRACE("DNS client init OK");
}

/******************************************************************************
* Function    : dev_w5500_net_dns_check
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : DNS to check IP
******************************************************************************/
static bool dev_w5500_net_dns_check(char *domain, uint8 *ip)
{
    bool ret = false;

    if (util_check_is_ip(domain, strlen(domain)))
    {
        int32 tmpIP[4];
        sscanf(domain, "%d.%d.%d.%d", &tmpIP[0], &tmpIP[1], &tmpIP[2], &tmpIP[3]);
        ip[0] = tmpIP[0];
        ip[1] = tmpIP[1];
        ip[2] = tmpIP[2];
        ip[3] = tmpIP[3];

        return true;
    }

    dev_w5500_net_util_sock_init();
    dev_w5500_net_dns_client_init();

    uint16 localPort = DNS_LOCAL_PORT;
    localPort = dev_w5500_get_random_local_port(DNS_LOCAL_PORT);
    ret = dev_w5500_socket(DNS, SOCK_DGRAM, localPort);
    if (!ret)
    {
        DEBUG_TRACE("DNS client sock create err");
        goto err;
    }

    memcpy(w5500_dns_client.dnsIP, DEF_DNS_SERVER, sizeof(DEF_DNS_SERVER));
    w5500_dns_client.lastTime = 0;
    w5500_dns_client.retryCnt = 0;
    
    if (DNS_ERR == dns_process(&w5500_dns_client, domain, ip))
    {
        DEBUG_TRACE("DNS process failed");
        
        ret = false;
        goto err;
    }
    
    //check last good dns, if changed, save it
    DEBUG_TRACE("DNS get IP %d.%d.%d.%d",
                                ip[0], ip[1], ip[2], ip[3]);
    
err:
    if (!dev_w5500_close(DNS))
    {
        DEBUG_TRACE("DNS client sock close err");
    }

    return ret;
}

/******************************************************************************
* Function    : dev_w5500_net_comm_sock_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : set buffer for comminute socket
******************************************************************************/
static void dev_w5500_net_comm_sock_init(void)
{
    //send/recv buff 4KB
    dev_w5500_write_sock_byte(SOCKET, Sn_RXBUF_SIZE, DEV_W5500_RX_CFG);
    dev_w5500_write_sock_byte(SOCKET, Sn_TXBUF_SIZE, DEV_W5500_TX_CFG);

    dev_w5500_write_sock_short(SOCKET, Sn_RX_RD, 0);
    dev_w5500_write_sock_short(SOCKET, Sn_TX_WR, 0);
}

#ifdef __ETH_DEBUG__
/******************************************************************************
* Function    : dev_w5500_net_default_config
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void dev_w5500_net_default_config(void)
{
    DEBUG_TRACE("Set default IP");

    //mac
    dev_w5500_write_nbyte(SHAR, util_get_mac(), MAC_LEN);

    //gate way
    dev_w5500_write_nbyte(GAR, DEF_GATEWAY, sizeof(DEF_GATEWAY));
    //network mask
    dev_w5500_write_nbyte(SUBR, DEF_NETMASK, sizeof(DEF_NETMASK));
    //ip addr
    dev_w5500_write_nbyte(SIPR, DEF_IP_ADDR, sizeof(DEF_IP_ADDR));
}
#endif /*__ETH_DEBUG__*/

/******************************************************************************
* Function    : dev_m26_net_config
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : net config
******************************************************************************/
bool dev_w5500_net_config(void)
{
    //software reset
    dev_w5500_write_byte(MR, RST);
    util_delay(DELAY_100_MS);

    //w5500 global config
    dev_w5500_write_short(RTR, DEV_W5500_SOCKET_RETRY_TIMEOUT);

    //re-try times
    dev_w5500_write_byte(RCR, DEV_W5500_SOCKET_MAX_RETRY);
    //open socket 0 ir
    dev_w5500_write_byte(SIMR, S0_IMR);
    dev_w5500_write_byte(SIMR, S1_IMR);
    dev_w5500_write_byte(SIMR, S2_IMR);
    dev_w5500_write_byte(SIMR, S3_IMR);

    dev_w5500_net_comm_sock_init();

    #ifdef __ETH_DEBUG__
    //set default net config
    dev_w5500_net_default_config();
    #else
    //get dynamic ip addr by DHCP
    if (!dev_w5500_net_dhcp_check(true))
    {
        DEBUG_TRACE("DHCP get IP failed");
        return false;
    }
    else
    {
        DEBUG_TRACE("DHCP[%d] lease time[%d]", w5500_dhcp_client.state, w5500_dhcp_client.leaseTime);
    }
    #endif /*__ETH_DEBUG__*/
		
    return true;
}

/******************************************************************************
* Function    : dev_w5500_net_connect
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : connet to server
******************************************************************************/
bool dev_w5500_net_connect(const char *domainName, uint16 port)
{
    bool ret = false;
    uint8 serIP[4] = {0, 0, 0, 0};
    uint16 localPort = 0;
    
    DEBUG_TRACE("Net connecting[%s:%d]", domainName, port);
    
    ret = dev_w5500_net_dns_check((char*)domainName, serIP);
    if (!ret)
    {
        DEBUG_TRACE("Domain DNS err, use default server IP");
        goto err;
    }

    localPort = dev_w5500_get_random_local_port(COMM_LOCAL_PORT);
    ret = dev_w5500_socket(SOCKET, SOCK_STREAM, localPort);
    if (!ret)
    {
        DEBUG_TRACE("Sock create err");
        goto err;
    }

    ret = dev_w5500_connect(SOCKET, serIP, port);
    if (!ret)
    {
        DEBUG_TRACE("Net connect err");
    }

err:
    return ret;
}

/******************************************************************************
* Function    : dev_w5500_net_disconnect
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : close socket and prepare to reset w5500
******************************************************************************/
void dev_w5500_net_disconnect(void)
{
    //disconnect 
    dev_w5500_disconnect(SOCKET);

    //close
    dev_w5500_close(SOCKET);
    
    DEBUG_TRACE("Net disconnect OK");
}

/******************************************************************************
* Function    : dev_w5500_net_check_stat
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : check w550 current network stat
******************************************************************************/
bool dev_w5500_net_check_stat(void)
{
    //check cable fitst
    if (!dev_w5500_cable_link())
    {
        return false;
    }
    
    uint8 netStat;
    netStat = dev_w5500_read_sock_byte(SOCKET, Sn_SR);

    DEBUG_TRACE("Net stat[%02X]", netStat);

    if (netStat != SOCK_ESTABLISHED)
    {
        return false;
    }

    return true;
}

/******************************************************************************
* Function    : dev_w5500_net_send
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : send data to socket
******************************************************************************/
bool dev_w5500_net_send(uint8 *pdata, uint16 len)
{
    //check lease time
    if (!dev_w5500_dhcp_check_lease_time())
    {
        return false;
    }
    
    DEBUG_TRACE("Send[%d]", len);

    return dev_w500_socket_send(SOCKET, pdata, len);
}

/******************************************************************************
* Function    : dev_w5500_net_recv
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : recv data from socket
******************************************************************************/
uint16 dev_w5500_net_recv(uint8 *pdata, uint16 maxLen)
{
    return dev_w500_socket_recv(SOCKET, pdata, maxLen);
}

/******************************************************************************
* Function    : dev_w5500_net_ftp_sock_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void dev_w5500_net_ftp_sock_init(void)
{
    //send/recv buff 4KB
    dev_w5500_write_sock_byte(FTP_CTRL, Sn_RXBUF_SIZE, DEV_W5500_RX_CFG);
    dev_w5500_write_sock_byte(FTP_CTRL, Sn_TXBUF_SIZE, DEV_W5500_TX_CFG);

    dev_w5500_write_sock_short(FTP_CTRL, Sn_RX_RD, 0);
    dev_w5500_write_sock_short(FTP_CTRL, Sn_TX_WR, 0);

    //send/recv buff 4KB
    dev_w5500_write_sock_byte(FTP_DATA, Sn_RXBUF_SIZE, DEV_W5500_RX_CFG);
    dev_w5500_write_sock_byte(FTP_DATA, Sn_TXBUF_SIZE, DEV_W5500_TX_CFG);

    dev_w5500_write_sock_short(FTP_DATA, Sn_RX_RD, 0);
    dev_w5500_write_sock_short(FTP_DATA, Sn_TX_WR, 0);
}

/******************************************************************************
* Function    : dev_w5500_net_ftp_check_resp
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool dev_w5500_net_ftp_check_resp(DEV_SOCKET s, const char *resp)
{
    uint16 timeout = 0;
    uint8 respond[FTP_CMD_LEN + 1] = {0};

    while (0 == dev_w500_socket_recv(s, (uint8*)respond, FTP_CMD_LEN))
    {
        if (timeout++ < FTP_RSP_TIMEOUT_CNT)//100 * 50ms = 5s time out
        {
            util_delay(DELAY_50_MS);//wait for respond
        }
        else
        {
            break;
        }
    }

    DEBUG_TRACE("RSP[%s]", respond);
    
    if (NULL != strstr((char*)respond, resp))
    {
        return true;
    }

    return false;
}

/******************************************************************************
* Function    : dev_w5500_net_ftp_disconnect
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void dev_w5500_net_ftp_ctrl_disconnect(void)
{
    dev_w500_socket_send(FTP_CTRL, FTP_REST_CMD, strlen(FTP_REST_CMD));
    dev_w5500_net_ftp_check_resp(FTP_CTRL, FTP_CONNECT_OK);
    //disconnect 
    dev_w5500_disconnect(FTP_CTRL);
    //close
    dev_w5500_close(FTP_CTRL);
    DEBUG_TRACE("FTP ctrl disconnect OK");
}

/******************************************************************************
* Function    : dev_w5500_net_ftp_data_disconnect
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void dev_w5500_net_ftp_data_disconnect(void)
{
    //disconnect 
    dev_w5500_disconnect(FTP_DATA);
    //close
    dev_w5500_close(FTP_DATA);
    DEBUG_TRACE("FTP data disconnect OK");
}

/******************************************************************************
* Function    : dev_w5500_net_get_pasv_port
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool dev_w5500_net_get_pasv_port(uint8 *ip, uint16 *port)
{
    uint16 timeout = 0;
    uint8 respond[FTP_CMD_LEN + 1] = {0};

    DEBUG_TRACE("CMD[%S]", FTP_PASV_CMD);
    dev_w500_socket_send(FTP_CTRL, FTP_PASV_CMD, strlen(FTP_PASV_CMD));

    while (0 == dev_w500_socket_recv(FTP_CTRL, (uint8*)respond, FTP_CMD_LEN))
    {
        if (timeout++ < FTP_RSP_TIMEOUT_CNT)//100 * 50ms = 5s time out
        {
            util_delay(DELAY_50_MS);//wait for respond
        }
        else
        {
            break;
        }
    }

    DEBUG_TRACE("RSP[%s]", respond);

    if (NULL == strstr((char*)respond, FTP_PASV_OK))
    {
        return false;
    }

    char dataPort[64+1] = {0};
    char *pStart = NULL;
    char *pStop = NULL;
    uint16 dataLen = 0;

    if (NULL == (pStart = strstr((char*)respond, "(")))
    {
        return false;
    }
    if (NULL == (pStop = strstr((char*)respond, ")")))
    {
        return false;
    }

    dataLen = (pStop - pStart) + 1;
    dataLen = MIN_VALUE(dataLen, 64);
    memcpy(dataPort, pStart, dataLen);
    dataPort[dataLen] = '\0';

    int32 pasvPort[2];
    int32 tmpIP[4];
    sscanf(dataPort, "(%d,%d,%d,%d,%d,%d)", 
             &tmpIP[0], &tmpIP[1], &tmpIP[2], &tmpIP[3], 
             &pasvPort[0], &pasvPort[1]);

    ip[0] = tmpIP[0];
    ip[1] = tmpIP[1];
    ip[2] = tmpIP[2];
    ip[3] = tmpIP[3];
    (*port) = (pasvPort[0] << 8) + pasvPort[1];
    if (*port == 0)
    {
        return false;
    }

    return true;
}

/******************************************************************************
* Function    : dev_w5500_net_ftp_connect_data_port
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool dev_w5500_net_ftp_connect_data_port(uint8 *ip, uint16 port, uint16 localPort)
{
    if (!dev_w5500_socket(FTP_DATA, SOCK_STREAM, localPort))
    {
        DEBUG_TRACE("FTP data socket err");
        return false;
    }

    if (!dev_w5500_connect(FTP_DATA, ip, port))
    {
        DEBUG_TRACE("FTP data [%d]connect err[%d.%d.%d.%d:%d]", localPort, ip[0], ip[1], ip[2], ip[3], port);
        return false;
    }

    return true;
}

/******************************************************************************
* Function    : dev_w5500_net_ftp_get_file_size
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint32 dev_w5500_net_ftp_get_file_size(char *fname)
{
    uint16 timeout = 0;
    uint16 sendLen;
    char ftpCMD[FTP_CMD_LEN+1];
    uint8 respond[FTP_CMD_LEN + 1] = {0};
    
    sendLen = snprintf(ftpCMD, FTP_CMD_LEN, "RETR %s\r\n", fname);
    DEBUG_TRACE("CMD[%d][%S]", sendLen, ftpCMD);
    
    dev_w500_socket_send(FTP_CTRL, (uint8*)ftpCMD, sendLen);

    while (0 == dev_w500_socket_recv(FTP_CTRL, (uint8*)respond, FTP_CMD_LEN))
    {
        if (timeout++ < FTP_RSP_TIMEOUT_CNT)//100 * 50ms = 5s time out
        {
            util_delay(DELAY_50_MS);//wait for respond
        }
        else
        {
            break;
        }
    }

    DEBUG_TRACE("RSP[%s]", respond);

    if (NULL == strstr((char*)respond, FTP_OPENING_FILE_OK))
    {
        return 0;
    }

    char dataSize[64+1] = {0};
    char *pStart = NULL;
    char *pStop = NULL;
    uint16 dataLen = 0;

    if (NULL == (pStart = strstr((char*)respond, "(")))
    {
        return 0;
    }
    if (NULL == (pStop = strstr((char*)respond, ")")))
    {
        return 0;
    }

    dataLen = (pStop - pStart) + 1;
    dataLen = MIN_VALUE(dataLen, 64);
    memcpy(dataSize, pStart, dataLen);
    dataSize[dataLen] = '\0';

    uint32 fileSize = 0;
    int32 dwnSize = -1;
    sscanf(dataSize, "(%d bytes)", &dwnSize);
    fileSize = MAX_VALUE(dwnSize, 0);

    return fileSize;
}

/******************************************************************************
* Function    : dev_w5500_net_ftp_download
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint8 dev_w5500_net_ftp_download(uint32 totalSize)
{
    uint8 ret = 0;
    uint8 retryCnt = 0;
    uint32 totalLen = 0;
    uint32 dataLen = 0;

    uint8 data[FTP_FILE_DATABLOCK_SIZE];
    do 
    {
        memset(data, 0, FTP_FILE_DATABLOCK_SIZE);
        if (totalLen < totalSize)
        {
            dataLen = dev_w500_socket_recv(FTP_DATA, data, FTP_FILE_DATABLOCK_SIZE);
            if (dataLen == 0 && totalLen != totalSize)
            {
                retryCnt++;
                //Added some delay before try to read w5500 rx buff
                util_delay(DELAY_500_MS);
                if (retryCnt > 3)
                {
                    DEBUG_TRACE("FTP recv timeout %d", retryCnt);
                    break;
                }
                else
                {
                    DEBUG_TRACE("FTP recv none[%d], retry[%d]", dataLen, retryCnt);
                    continue;
                }
            }
            retryCnt = 0;
            
            DEBUG_TRACE("recv %d", dataLen);
			{
				//deal with download data
			}

            totalLen += dataLen;
        }
        else
        {
            DEBUG_TRACE("Download OK[%d]", totalLen);
            ret = 1;
            break;
        }
    }
    while (1);

    return ret;
}

/******************************************************************************
* Function    : dev_w5500_net_ftp
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : ftp by w5500
******************************************************************************/
uint8 dev_w5500_net_ftp(uint8 *url, uint16 port, uint8* usrName, uint8 *usrPass)
{
    uint16 ftpCtrlLocalPort = dev_w5500_get_random_local_port(FTP_CTRL_LOCAL_PORT);
    uint16 ftpDataLocalPort = dev_w5500_get_random_local_port(FTP_DATA_LOCAL_PORT);
	uint8 curRetryCnt;
    bool ret = false;
    bool needCloseFTPC = false;
    bool needCloseFTPD = false;
    char addr[DEV_W5500_FTP_FILED_LEN+1];
    char path[DEV_W5500_FTP_FILED_LEN+1];
    char fname[DEV_W5500_FTP_FILED_LEN+1];
    uint8 ipAddr[4] = {0};

    uint16 sendLen;
    char ftpCMD[FTP_CMD_LEN+1];

    if (!util_get_url_param((char*)url, DEV_W5500_FTP_FILED_LEN, addr, path, fname))
    {
        DEBUG_TRACE("Invalid url param");
        return 0;
    }
    DEBUG_TRACE("[%s][%s][%s]", addr, path, fname);

    if (util_check_is_ip(addr, strlen(addr)))
    {
        int32 tmpIP[4];
        sscanf(addr, "%d.%d.%d.%d", &tmpIP[0], &tmpIP[1], &tmpIP[2], &tmpIP[3]);
        ipAddr[0] = tmpIP[0];
        ipAddr[1] = tmpIP[1];
        ipAddr[2] = tmpIP[2];
        ipAddr[3] = tmpIP[3];
    }
    else
    {
        if (!dev_w5500_net_dns_check(addr, ipAddr))
        {
            return 0;
        }
    }
    DEBUG_TRACE("Get server %d.%d.%d.%d", ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3]);

retry:
    if (needCloseFTPC)
    {
        dev_w5500_net_ftp_ctrl_disconnect();
        needCloseFTPC = false;
    }
    if (needCloseFTPD)
    {
        dev_w5500_net_ftp_data_disconnect();
        needCloseFTPD = false;
    }

    curRetryCnt++;
    //better to use random local port
    ftpCtrlLocalPort++;
    ftpDataLocalPort++;
    if (curRetryCnt > 3)
    {
        DEBUG_TRACE("retry cnt out[%d>%d]", curRetryCnt, 3);
        return 0;
    }
    
    dev_w5500_net_ftp_sock_init();

    //create socket
    ret = dev_w5500_socket(FTP_CTRL, SOCK_STREAM, ftpCtrlLocalPort);
    if (!ret)
    {
        DEBUG_TRACE("FTP client sock create err");
        goto retry;
    }

    //connect to FTP server
    ret = dev_w5500_connect(FTP_CTRL, ipAddr, port);
    if (!ret)
    {
        DEBUG_TRACE("FTP connect err");
        goto retry;
    }
    //FTP connect OK
    if (!dev_w5500_net_ftp_check_resp(FTP_CTRL, FTP_CONNECT_OK))
    {
        goto retry;
    }
    needCloseFTPC = true;

    //set ftp username
    sendLen = snprintf(ftpCMD, FTP_CMD_LEN, "USER %s\r\n", usrName);
    DEBUG_TRACE("CMD[%S]", ftpCMD);
    dev_w500_socket_send(FTP_CTRL, (uint8*)ftpCMD, sendLen);
    if (!dev_w5500_net_ftp_check_resp(FTP_CTRL, FTP_USERNAME_OK))
    {
        goto retry;
    }

    //set ftp password
    sendLen = snprintf(ftpCMD, FTP_CMD_LEN, "PASS %s\r\n", usrPass);
    DEBUG_TRACE("CMD[%S]", ftpCMD);
    dev_w500_socket_send(FTP_CTRL, (uint8*)ftpCMD, sendLen);
    if (!dev_w5500_net_ftp_check_resp(FTP_CTRL, FTP_LOGIN_OK))
    {
        goto retry;
    }

    //set ftp server path
    sendLen = snprintf(ftpCMD, FTP_CMD_LEN, "CWD %s\r\n", path);
    DEBUG_TRACE("CMD[%S]", ftpCMD);
    dev_w500_socket_send(FTP_CTRL, (uint8*)ftpCMD, sendLen);
    if (!dev_w5500_net_ftp_check_resp(FTP_CTRL, FTP_REQ_DIR_OK))
    {
        goto retry;
    }

    //set ftp type binary mode
    sendLen = snprintf(ftpCMD, FTP_CMD_LEN, "TYPE I\r\n");
    DEBUG_TRACE("CMD[%S]", ftpCMD);
    dev_w500_socket_send(FTP_CTRL, (uint8*)ftpCMD, sendLen);
    if (!dev_w5500_net_ftp_check_resp(FTP_CTRL, FTP_TYPE_I_OK))
    {
        goto retry;
    }

    //enter ftp PASV mode to get data port
    uint8 dataIP[4] = {0};
    uint16 dataPort = 0;
    if (!dev_w5500_net_get_pasv_port(dataIP, &dataPort))
    {
        goto retry;
    }
    DEBUG_TRACE("DATA[%d.%d.%d.%d:%d]", dataIP[0], dataIP[1], dataIP[2], dataIP[3], dataPort);

    /*connect to data server port to get file*/
    ret = dev_w5500_net_ftp_connect_data_port(dataIP, dataPort, ftpDataLocalPort);
    if (!ret)
    {
        DEBUG_TRACE("FTP data connect err");
        goto retry;
    }
    needCloseFTPD = true;

    //get ftp file size
	uint32 totalSize;
    if (0 == (totalSize = dev_w5500_net_ftp_get_file_size(fname)))
    {
        DEBUG_TRACE("FTP file size[%d] err", totalSize);
        goto retry;
    }
    DEBUG_TRACE("FTP file size[%d]", totalSize);

    uint8 retStat = 0;
    retStat = dev_w5500_net_ftp_download(totalSize);
    if (retStat != 1)
    {
        DEBUG_TRACE("FTP download err");
        goto retry;
    }

    dev_w5500_net_ftp_check_resp(FTP_CTRL, FTP_GET_FILE_OK);

    //close data and ctrl socket
    dev_w5500_net_ftp_data_disconnect();
    dev_w5500_net_ftp_ctrl_disconnect();
    
    return retStat;
}

