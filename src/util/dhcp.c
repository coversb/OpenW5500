/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          dhcp.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   DHCP client
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

#include "dhcp.h"
#include "util.h"

/******************************************************************************
* Macros
******************************************************************************/
#define DHCP_IP_TEST "IP_CONFLICT"
#define DHCP_IP_TEST_PORT 5000

/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : swaps
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint16 swaps(uint16 i)
{
    uint16 ret=0;
    ret = (i & 0xFF) << 8;
    ret |= ((i >> 8)& 0xFF);
    return ret;	
}

/******************************************************************************
* Function    : swapl
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint32 swapl(uint32 l)
{
    uint32 ret=0;
    ret = (l & 0xFF) << 24;
    ret |= ((l >> 8) & 0xFF) << 16;
    ret |= ((l >> 16) & 0xFF) << 8;
    ret |= ((l >> 24) & 0xFF);
    return ret;
}

/******************************************************************************
* Function    : dhcp_state_update
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void dhcp_state_update(DHCP_CLIENT *dhcp, uint8 state)
{
    //set state
    dhcp->state = state;

    //reset counter
    dhcp->retryCnt = 0;
    dhcp->lastTime = util_systick_second();
}

/******************************************************************************
* Function    : dhcp_send_discover
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : broadcast DHCP DISCOVER message to DHCP server
******************************************************************************/
static void dhcp_send_discover(DHCP_CLIENT *dhcp)
{
    uint8 ip[4] = {255, 255, 255, 255};
    uint16 i = 0;
    uint8 hostText[12];

    RIP_MSG ripMsg;
    memset(&ripMsg, 0, sizeof(RIP_MSG));
  
    ripMsg.op = DHCP_BOOTREQUEST;
    ripMsg.htype = DHCP_HTYPE10MB;
    ripMsg.hlen = DHCP_HLENETHERNET;
    ripMsg.hops = DHCP_HOPS;
    ripMsg.xid = swapl(dhcp->xid);
    ripMsg.secs = DHCP_SECS;
    ripMsg.flags = swaps(DHCP_FLAGSBROADCAST);
    ripMsg.chaddr[0] = dhcp->mac[0];
    ripMsg.chaddr[1] = dhcp->mac[1];
    ripMsg.chaddr[2] = dhcp->mac[2];
    ripMsg.chaddr[3] = dhcp->mac[3];
    ripMsg.chaddr[4] = dhcp->mac[4];
    ripMsg.chaddr[5] = dhcp->mac[5];
  
    /* MAGIC_COOKIE */
    ripMsg.OPT[i++] = (uint8)((MAGIC_COOKIE >> 24)& 0xFF);
    ripMsg.OPT[i++] = (uint8)((MAGIC_COOKIE >> 16)& 0xFF);
    ripMsg.OPT[i++] = (uint8)((MAGIC_COOKIE >> 8)& 0xFF);
    ripMsg.OPT[i++] = (uint8)(MAGIC_COOKIE& 0xFF);
  
    /* Option Request Param */
    ripMsg.OPT[i++] = dhcpMessageType;
    ripMsg.OPT[i++] = 0x01;
    ripMsg.OPT[i++] = DHCP_DISCOVER;
  
    // Client identifier
    ripMsg.OPT[i++] = dhcpClientIdentifier;
    ripMsg.OPT[i++] = 0x07;
    ripMsg.OPT[i++] = 0x01;
    ripMsg.OPT[i++] = dhcp->mac[0];
    ripMsg.OPT[i++] = dhcp->mac[1];
    ripMsg.OPT[i++] = dhcp->mac[2];
    ripMsg.OPT[i++] = dhcp->mac[3];
    ripMsg.OPT[i++] = dhcp->mac[4];
    ripMsg.OPT[i++] = dhcp->mac[5];
  
    // host name
    ripMsg.OPT[i++] = hostName;
    uint8 hostNameLen;
    hostNameLen = snprintf((char*)hostText, sizeof(hostText),
                                    "W-%02X%02X%02X",
                                    dhcp->mac[3], dhcp->mac[4], dhcp->mac[5]);

    ripMsg.OPT[i++] = hostNameLen;
    memcpy(&(ripMsg.OPT[i]), hostText, hostNameLen); 
    i += hostNameLen;
 
    ripMsg.OPT[i++] = dhcpParamRequest;
    ripMsg.OPT[i++] = 0x06;
    ripMsg.OPT[i++] = subnetMask;
    ripMsg.OPT[i++] = routersOnSubnet;
    ripMsg.OPT[i++] = dns;
    ripMsg.OPT[i++] = domainName;
    ripMsg.OPT[i++] = dhcpT1value;
    ripMsg.OPT[i++] = dhcpT2value;
    ripMsg.OPT[i++] = endOption;

    dhcp->sendto(dhcp->sock, (uint8 *)&ripMsg, sizeof(RIP_MSG), ip, DHCP_SERVER_PORT);
    DEBUG_TRACE("DHCP SEND DISCOVER to %d.%d.%d.%d:%d",
                                ip[0], ip[1], ip[2], ip[3], DHCP_SERVER_PORT);
}

/******************************************************************************
* Function    : dhcp_send_request
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : send DHCP REQUEST messsage to DHCP server
******************************************************************************/
static void dhcp_send_request(DHCP_CLIENT *dhcp)
{
    uint8 ip[4] = {255, 255, 255, 255};
    uint16 i = 0;  
    uint8 hostText[12];
    
    RIP_MSG ripMsg;
    memset(&ripMsg, 0, sizeof(RIP_MSG));
  
    ripMsg.op = DHCP_BOOTREQUEST;
    ripMsg.htype = DHCP_HTYPE10MB;
    ripMsg.hlen = DHCP_HLENETHERNET;
    ripMsg.hops = DHCP_HOPS;
    ripMsg.xid = swapl(dhcp->xid);
    ripMsg.secs = DHCP_SECS;
    
    if (dhcp->state < DHCP_STATE_LEASED)
    {
        ripMsg.flags = swaps(DHCP_FLAGSBROADCAST);
    }
    else
    {
        ripMsg.flags = 0; // For Unicast
        memcpy(ripMsg.ciaddr, dhcp->ip, 4);
    }		
    ripMsg.chaddr[0] = dhcp->mac[0];
    ripMsg.chaddr[1] = dhcp->mac[1];
    ripMsg.chaddr[2] = dhcp->mac[2];
    ripMsg.chaddr[3] = dhcp->mac[3];
    ripMsg.chaddr[4] = dhcp->mac[4];
    ripMsg.chaddr[5] = dhcp->mac[5];
  
    /* MAGIC_COOKIE */
    ripMsg.OPT[i++] = (uint8)((MAGIC_COOKIE >> 24) & 0xFF);
    ripMsg.OPT[i++] = (uint8)((MAGIC_COOKIE >> 16) & 0xFF);
    ripMsg.OPT[i++] = (uint8)((MAGIC_COOKIE >> 8) & 0xFF);
    ripMsg.OPT[i++] = (uint8)(MAGIC_COOKIE & 0xFF);
  
    /* Option Request Param */
    ripMsg.OPT[i++] = dhcpMessageType;
    ripMsg.OPT[i++] = 0x01;
    ripMsg.OPT[i++] = DHCP_REQUEST;

    // Client identifier
    ripMsg.OPT[i++] = dhcpClientIdentifier;
    ripMsg.OPT[i++] = 0x07;
    ripMsg.OPT[i++] = 0x01;
    ripMsg.OPT[i++] = dhcp->mac[0];
    ripMsg.OPT[i++] = dhcp->mac[1];
    ripMsg.OPT[i++] = dhcp->mac[2];
    ripMsg.OPT[i++] = dhcp->mac[3];
    ripMsg.OPT[i++] = dhcp->mac[4];
    ripMsg.OPT[i++] = dhcp->mac[5];

    if (dhcp->state < DHCP_STATE_LEASED)
    {
        ripMsg.OPT[i++] = dhcpRequestedIPaddr;
        ripMsg.OPT[i++] = 0x04;
        ripMsg.OPT[i++] = dhcp->ip[0];
        ripMsg.OPT[i++] = dhcp->ip[1];
        ripMsg.OPT[i++] = dhcp->ip[2];
        ripMsg.OPT[i++] = dhcp->ip[3];

        ripMsg.OPT[i++] = dhcpServerIdentifier;
        ripMsg.OPT[i++] = 0x04;
        ripMsg.OPT[i++] = dhcp->serverIP[0];
        ripMsg.OPT[i++] = dhcp->serverIP[1];
        ripMsg.OPT[i++] = dhcp->serverIP[2];
        ripMsg.OPT[i++] = dhcp->serverIP[3];
    }
  
    ripMsg.OPT[i++] = hostName;
    uint8 hostNameLen;
    hostNameLen = snprintf((char*)hostText, sizeof(hostText),
                                    "W-%02X%02X%02X",
                                    dhcp->mac[3], dhcp->mac[4], dhcp->mac[5]);

    ripMsg.OPT[i++] = hostNameLen;
    memcpy(&(ripMsg.OPT[i]), hostText, hostNameLen); 
    i += hostNameLen;
    
    ripMsg.OPT[i++] = dhcpParamRequest;
    ripMsg.OPT[i++] = 0x08;
    ripMsg.OPT[i++] = subnetMask;
    ripMsg.OPT[i++] = routersOnSubnet;
    ripMsg.OPT[i++] = dns;
    ripMsg.OPT[i++] = domainName;
    ripMsg.OPT[i++] = dhcpT1value;
    ripMsg.OPT[i++] = dhcpT2value;
    ripMsg.OPT[i++] = performRouterDiscovery;
    ripMsg.OPT[i++] = staticRoute;
    ripMsg.OPT[i++] = endOption;
  
    /* send broadcasting packet */
    if (dhcp->state >= DHCP_STATE_LEASED)
    {
        memcpy(ip, dhcp->serverIP, 4);
    }

    dhcp->sendto(dhcp->sock, (uint8*)&ripMsg, sizeof(RIP_MSG), ip, DHCP_SERVER_PORT);
    DEBUG_TRACE("DHCP SEND REQUEST to %d.%d.%d.%d:%d",
                                ip[0], ip[1], ip[2], ip[3], DHCP_SERVER_PORT);
}

/******************************************************************************
* Function    : dhcp_send_release_decline
* 
* Author      : Chen Hao
* 
* Parameters  : true : rsend RELEASE message
*                     fasle : send DECLINE message
* 
* Return      : 
* 
* Description : send DHCP RELEASE or DECLINE message
******************************************************************************/
static void dhcp_send_release_decline(DHCP_CLIENT *dhcp, bool release)
{
    uint16 i = 0;
    uint8  ip[4] = {255, 255, 255, 255};

    RIP_MSG ripMsg;
    memset(&ripMsg, 0, sizeof(RIP_MSG));

    ripMsg.op = DHCP_BOOTREQUEST;
    ripMsg.htype = DHCP_HTYPE10MB;
    ripMsg.hlen = DHCP_HLENETHERNET;
    ripMsg.hops = DHCP_HOPS;
    ripMsg.xid = swapl(dhcp->xid);
    ripMsg.secs = DHCP_SECS;
    ripMsg.flags = 0;// DHCP_FLAGSBROADCAST;
    ripMsg.chaddr[0] = dhcp->mac[0];
    ripMsg.chaddr[1] = dhcp->mac[1];
    ripMsg.chaddr[2] = dhcp->mac[2];
    ripMsg.chaddr[3] = dhcp->mac[3];
    ripMsg.chaddr[4] = dhcp->mac[4];
    ripMsg.chaddr[5] = dhcp->mac[5];

    /* MAGIC_COOKIE */
    ripMsg.OPT[i++] = (uint8)((MAGIC_COOKIE >> 24) & 0xFF);
    ripMsg.OPT[i++] = (uint8)((MAGIC_COOKIE >> 16) & 0xFF);
    ripMsg.OPT[i++] = (uint8)((MAGIC_COOKIE >> 8) & 0xFF);
    ripMsg.OPT[i++] = (uint8)(MAGIC_COOKIE & 0xFF);
    
    /* Option Request Param. */
    ripMsg.OPT[i++] = dhcpMessageType;
    ripMsg.OPT[i++] = 0x01;
    ripMsg.OPT[i++] = (release ? DHCP_RELEASE : DHCP_DECLINE);

    // Client identifier
    ripMsg.OPT[i++] = dhcpClientIdentifier;
    ripMsg.OPT[i++] = 0x07;
    ripMsg.OPT[i++] = 0x01;
    ripMsg.OPT[i++] = dhcp->mac[0];
    ripMsg.OPT[i++] = dhcp->mac[1];
    ripMsg.OPT[i++] = dhcp->mac[2];
    ripMsg.OPT[i++] = dhcp->mac[3];
    ripMsg.OPT[i++] = dhcp->mac[4];
    ripMsg.OPT[i++] = dhcp->mac[5];

    // Server identifier
    ripMsg.OPT[i++] = dhcpServerIdentifier;
    ripMsg.OPT[i++] = 0x04;
    ripMsg.OPT[i++] = dhcp->serverIP[0];
    ripMsg.OPT[i++] = dhcp->serverIP[1];
    ripMsg.OPT[i++] = dhcp->serverIP[2];
    ripMsg.OPT[i++] = dhcp->serverIP[3];

    if (!release)
    {
        ripMsg.OPT[i++] = dhcpRequestedIPaddr;
        ripMsg.OPT[i++] = 0x04;
        ripMsg.OPT[i++] = dhcp->ip[0];
        ripMsg.OPT[i++] = dhcp->ip[1];
        ripMsg.OPT[i++] = dhcp->ip[2];
        ripMsg.OPT[i++] = dhcp->ip[3];		
    }
    ripMsg.OPT[i++] = endOption;	
    
    if (release) 
    {
        memcpy(ip, dhcp->serverIP, 4);
    }
    
    dhcp->sendto(dhcp->sock, (uint8*)&ripMsg, sizeof(RIP_MSG), ip, DHCP_SERVER_PORT);
    DEBUG_TRACE("DHCP SEND RELEASE to %d.%d.%d.%d:%d",
                                ip[0], ip[1], ip[2], ip[3], DHCP_SERVER_PORT);

}

/******************************************************************************
* Function    : dhcp_parse_msg
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : recv data from DHCP server and parse RIP option value
******************************************************************************/
static uint8 dhcp_parse_msg(DHCP_CLIENT *dhcp)
{
    uint8  serAddr[4];
    uint16 serPort;
    uint16 recvLen;
    uint8  *p;
    uint8  *endOfMsg;
    uint8  msgType = DHCP_NONE;
    uint8  optLen = 0;
    RIP_MSG ripMsg;
    memset(&ripMsg, 0, sizeof(RIP_MSG));
  
    recvLen = dhcp->recvfrom(dhcp->sock, (uint8 *)(&ripMsg), sizeof(RIP_MSG), serAddr, &serPort);
    if (recvLen == 0)
    {
        return DHCP_NONE;
    }
  
    if ((ripMsg.op != DHCP_BOOTREPLY) 
        || (serPort != DHCP_SERVER_PORT))
    {
        DEBUG_TRACE("DHCP bad op[%d] port [%d]", ripMsg.op, serPort);
        return DHCP_NONE;
    }
    
    if ((memcmp(ripMsg.chaddr, dhcp->mac, 6) != 0)
        || (ripMsg.xid != swapl(dhcp->xid)))
    {
        DEBUG_TRACE("DHCP bad mac[%02X.%02X.%02X.%02X.%02X.%02], xid[%08X]", 
                                    ripMsg.chaddr[0], ripMsg.chaddr[1], ripMsg.chaddr[2],
                                    ripMsg.chaddr[3], ripMsg.chaddr[4], ripMsg.chaddr[5], ripMsg.xid);
        return DHCP_NONE;
    }
  
    if (*((uint32*)(dhcp->serverIP)) != 0x00000000)
    {
        if ((*((uint32*)(dhcp->realServerIP)) != *((uint32*)serAddr)) 
            && (*((uint32*)(dhcp->serverIP)) != *((uint32*)serAddr))) 
        {
            DEBUG_TRACE("DHCP bad server[%d.%d.%d.%d]",
                                        serAddr[0], serAddr[1], serAddr[2], serAddr[3]);
            return DHCP_NONE;								
        }
    }
  
    memcpy(dhcp->ip, ripMsg.yiaddr, 4);

    //parse DHCP option value
    p = (uint8 *)(&(ripMsg.op));
    p = p + 240;
    endOfMsg = p + (recvLen - 240);
    while (p < endOfMsg) 
    {
        switch (*p++) 
        {
            case endOption:
            {
                return msgType;
            }
            case padOption:break;
            case dhcpMessageType:
            {
                optLen = *p++;
                msgType = *p;
                break;
            }
            case subnetMask:
            {
                optLen = *p++;
                memcpy(dhcp->mask, p, 4);
                break;
            }
            case routersOnSubnet:
            {
                optLen = *p++;
                memcpy(dhcp->gateway, p, 4);
                break;
            }
            case dns:
            {
                optLen = *p++;
                memcpy(dhcp->dns,p,4);
                break;
            }
            case dhcpIPaddrLeaseTime:
            {
                optLen = *p++;
                dhcp->leaseTime = swapl(*((uint32*)p));
                break;
            }
            case dhcpServerIdentifier:
            {
                optLen = *p++;
                if ( *((uint32*)dhcp->serverIP) == 0 
                    || *((uint32*)dhcp->serverIP) == *((uint32*)serAddr)
                    || *((uint32*)dhcp->realServerIP) == *((uint32*)serAddr))
                {
                    memcpy(dhcp->serverIP, p, 4);
                    // Copy real ip address of the DHCP server
                    memcpy(dhcp->realServerIP, serAddr, 4);
                }
                else
                {
                    DEBUG_TRACE("DHCP find server [%d.%d.%d.%d]",
                                                serAddr[0], serAddr[1], serAddr[2], serAddr[3]);
                }
                break;
            }
            default:
            {
                optLen = *p++;
                break;
            }
        } // switch
        
        p += optLen;
    } // while
      
    return DHCP_NONE;
}

/******************************************************************************
* Function    : dhcp_try_to_resend_msg
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : check counter and re-send message according to current dhcp state
******************************************************************************/
static void dhcp_try_to_resend_msg(DHCP_CLIENT *dhcp)
{
    uint32 curTime = util_systick_second();
    
    if (dhcp->retryCnt < DHCP_MAX_RETRY) 
    {
        //timeout and re-send message
        if ((curTime - dhcp->lastTime) > DHCP_WAIT_TIMEOUT) 
        {
            dhcp->lastTime = curTime;
            dhcp->retryCnt++;
            switch (dhcp->state) 
            {
                case DHCP_STATE_DISCOVER:
                {
                    dhcp_send_discover(dhcp);
                    break;
                }
                case DHCP_STATE_REQUEST:
                {
                    dhcp_send_request(dhcp);
                    break;
                }
                case DHCP_STATE_REREQUEST:
                {
                    dhcp_send_request(dhcp);
                    break;
                }
                default:break;
            }
            DEBUG_TRACE("DHCP[%d]timeout, resend", dhcp->state);
        }
        else
        {
            util_delay(DELAY_50_MS);
        }
    } 
    else 
    {
        dhcp_send_discover(dhcp);
        dhcp_state_update(dhcp, DHCP_STATE_DISCOVER);
        DEBUG_TRACE("DHCP[%d]retry cnt out, restart");
    }
}

/******************************************************************************
* Function    : dhcp_leasedIP_valid
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : check leased IP validity
******************************************************************************/
static bool dhcp_leasedIP_valid(DHCP_CLIENT *dhcp)
{
    // sendto is complete. that means there is a node which has a same IP
    DEBUG_TRACE("DHCP SEND LEASED IP to %d.%d.%d.%d:%d",
                                dhcp->ip[0], dhcp->ip[1], dhcp->ip[2], dhcp->ip[3], DHCP_IP_TEST_PORT);

    if (dhcp->sendto(dhcp->sock, (uint8*)DHCP_IP_TEST, strlen(DHCP_IP_TEST), dhcp->ip, DHCP_IP_TEST_PORT))
    {
        DEBUG_TRACE("DHCP leased IP conflict, decline");
        dhcp_send_release_decline(dhcp, false);
        return false;
    }
    
    return true;
}

/******************************************************************************
* Function    : dhcp_client_process
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : check dhcp state
******************************************************************************/
uint8 dhcp_client_process(DHCP_CLIENT *dhcp)
{
    uint8  msgType = DHCP_NONE;
    
    if (dhcp->state != DHCP_STATE_INIT)
    {
        msgType = dhcp_parse_msg(dhcp);
        if (msgType != DHCP_NONE)
        {
            DEBUG_TRACE("DHCP parse msg[%d]", msgType);
        }
    }

    switch (dhcp->state)
    {
        case DHCP_STATE_INIT:
        default:
        {
            // broadcast DISCOVER message to find DHCP server
            dhcp_send_discover(dhcp);   
            dhcp_state_update(dhcp, DHCP_STATE_DISCOVER);
            break;
        }
        case DHCP_STATE_DISCOVER:
        {
            if (msgType == DHCP_OFFER)
            {
                 // send REQUEST message to DHCP server
                dhcp_send_request(dhcp);
                dhcp_state_update(dhcp, DHCP_STATE_REQUEST);
            }
            break;
        }
        case DHCP_STATE_REQUEST:
        {
            if (msgType == DHCP_ACK)
            {
                if (dhcp_leasedIP_valid(dhcp)) // check validity of leased IP
                {
                    dhcp_state_update(dhcp, DHCP_STATE_LEASED);
                    return DHCP_RET_UPDATE;
                } 
                else 
                {
                    dhcp_state_update(dhcp, DHCP_STATE_DISCOVER);
                    return DHCP_RET_CONFLICT;
                }
            }
            else 
            if (msgType == DHCP_NAK)
            {
                //recv NAK, go to re-try
                dhcp_state_update(dhcp, DHCP_STATE_DISCOVER);
            }
            break;
        }
        case DHCP_STATE_LEASED:
        {
            //When lease time pass half, we need renew ip
            uint32 curTime = util_systick_second();
            if ((dhcp->leaseTime != INFINITE_LEASETIME) 
                && ((curTime - dhcp->lastTime) >(dhcp->leaseTime/2))) 
            {
                memcpy(dhcp->oldIP, dhcp->ip, 4);
                dhcp->xid++;

                dhcp_send_request(dhcp);
                dhcp_state_update(dhcp, DHCP_STATE_REREQUEST);
            }
            break;
        }
        case DHCP_STATE_REREQUEST:
        {
            if (msgType == DHCP_ACK)
            {
                dhcp_state_update(dhcp, DHCP_STATE_LEASED);
                if (memcmp(dhcp->oldIP, dhcp->ip, 4)!=0)
                {
                    //use new leased IP, need update net config
                    return DHCP_RET_UPDATE;
                }
                //use old leased IP
                return DHCP_RET_LEASED_IP;
            } 
            else 
            if (msgType == DHCP_NAK)
            {
                //renew falied re-init
                dhcp_state_update(dhcp, DHCP_STATE_DISCOVER);
            } 
            break;
        }
    }

    dhcp_try_to_resend_msg(dhcp);
    
    return DHCP_RET_NONE;
}

/******************************************************************************
* Function    : dhcp_client_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : init dhcp client context
******************************************************************************/
void dhcp_client_init(DHCP_CLIENT *dhcp)
{
    memset(dhcp, 0, sizeof(DHCP_CLIENT));
    dhcp->state = DHCP_STATE_INIT;
    dhcp->xid = DEFAULT_XID + util_get_random_num(DEFAULT_XID);
    dhcp->retryCnt = 0;
    dhcp->lastTime = 0;
    dhcp->leaseTime = 0;

    memset(dhcp->oldIP, 0, sizeof(dhcp->oldIP));
    memset(dhcp->serverIP, 0, sizeof(dhcp->serverIP));
    memset(dhcp->realServerIP, 0, sizeof(dhcp->realServerIP));

    memset(dhcp->mac, 0, sizeof(dhcp->mac));
    memset(dhcp->ip, 0, sizeof(dhcp->ip));
    memset(dhcp->mask, 0, sizeof(dhcp->mask));
    memset(dhcp->gateway, 0, sizeof(dhcp->gateway));
    memset(dhcp->dns, 0, sizeof(dhcp->dns));

    DEBUG_TRACE("DHCP Init %08X", dhcp->xid);
}

