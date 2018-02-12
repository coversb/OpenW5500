/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          dns.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   DNS client
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

#include "util.h"
#include "dns.h"

/******************************************************************************
* Macros
******************************************************************************/

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/

/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : dns_get16
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
uint16 dns_get16(uint8 *s)
{
    uint16 i;
    i = *s++ << 8;
    i = i + *s;
    return i;
}

/******************************************************************************
* Function    : dns_put16
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : copies uint16_t to the network buffer with network byte order
******************************************************************************/
uint8* dns_put16(uint8 *s, uint16 i)
{
    *s++ = i >> 8;
    *s++ = i;
    return s;
}

/******************************************************************************
* Function    : dns_assemble_query_message
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : send dns query message
******************************************************************************/
static uint16 dns_assemble_query_message(DNS_CLIENT *dns, char *name, uint8 *sendBuf)
{
    uint16 len;
    uint16 op = 0;

    uint8 *cp = sendBuf;
    char *cp1;
    char sname[DNS_MAXCNAME];
    char *dname;
    uint16 p;
    uint16 dlen;

    dns->msgID++;

    cp = dns_put16(cp, dns->msgID);
    p = (op << 11) | 0x0100;			/* Recursion desired */
    cp = dns_put16(cp, p);
    cp = dns_put16(cp, 1);
    cp = dns_put16(cp, 0);
    cp = dns_put16(cp, 0);
    cp = dns_put16(cp, 0);

    strcpy(sname, name);
    dname = sname;
    dlen = strlen(dname);
    for (;;)
    {
        /* Look for next dot */
        cp1 = strchr(dname, '.');

        if (cp1 != NULL) len = cp1 - dname;	/* More to come */
        else len = dlen;			/* Last component */

        *cp++ = len;				/* Write length of component */
        if (len == 0) break;

        /* Copy component up to (but not including) dot */
        strncpy((char *)cp, dname, len);
        cp += len;
        if (cp1 == NULL)
        {
            *cp++ = 0;			/* Last one; write null and finish */
            break;
        }
        dname += len+1;
        dlen -= len+1;
    }

    cp = dns_put16(cp, 0x0001);				/* type */
    cp = dns_put16(cp, 0x0001);				/* class */

    return ((uint16)((uint32)(cp) - (uint32)(sendBuf)));
}

/******************************************************************************
* Function    : dns_parse_name
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
uint16 dns_parse_name(uint8 *msg, uint8 *compressed, char *buf, int16 len)
{
    uint16 slen;		/* Length of current segment */
    uint8 * cp;
    int clen = 0;		/* Total length of compressed name */
    int indirect = 0;	/* Set if indirection encountered */
    int nseg = 0;		/* Total number of segments in name */

    cp = compressed;

    for (;;)
    {
        slen = *cp++;	/* Length of this segment */

        if (!indirect) clen++;

        if ((slen & 0xc0) == 0xc0)
        {
            if (!indirect)
            {
                clen++;
            }
            indirect = 1;
            /* Follow indirection */
            cp = &msg[((slen & 0x3f)<<8) + *cp];
            slen = *cp++;
        }

        if (slen == 0)	/* zero length == all done */
        {
            break;
        }
        
        len -= slen + 1;

        if (len < 0) return 0;

        if (!indirect) clen += slen;

        while (slen-- != 0) *buf++ = (char)*cp++;
        *buf++ = '.';
        nseg++;
    }

    if (nseg == 0)
    {
        /* Root name; represent as single dot */
        *buf++ = '.';
        len--;
    }

    *buf++ = '\0';
    len--;

    return clen;	/* Length of compressed message */
}

/******************************************************************************
* Function    : dns_question
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint8* dns_question(uint8 *msg, uint8 *cp)
{
    int len;
    char name[DNS_MAXCNAME];

    len = dns_parse_name(msg, cp, name, DNS_MAXCNAME);

    if (len == 0) return NULL;

    cp += len;
    cp += 2;		/* type */
    cp += 2;		/* class */

    return cp;
}

/******************************************************************************
* Function    : dns_answer
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint8* dns_answer(uint8 *msg, uint8 *cp, uint8 *resIP)
{
    int len, type;
    char name[DNS_MAXCNAME];

    len = dns_parse_name(msg, cp, name, DNS_MAXCNAME);

    if (len == -1) return NULL;

    cp += len;
    type = dns_get16(cp);
    cp += 2;		/* type */
    cp += 2;		/* class */
    cp += 4;		/* ttl */
    cp += 2;		/* len */

    switch (type)
    {
        case TYPE_A:
        {
            /* Just read the address directly into the structure */
            resIP[0] = *cp++;
            resIP[1] = *cp++;
            resIP[2] = *cp++;
            resIP[3] = *cp++;
            break;
        }
        case TYPE_CNAME:
        case TYPE_MB:
        case TYPE_MG:
        case TYPE_MR:
        case TYPE_NS:
        case TYPE_PTR:
        {
            /* These types all consist of a single domain name */
            /* convert it to ascii format */
            len = dns_parse_name(msg, cp, name, DNS_MAXCNAME);
            if (len == -1) return NULL;

            cp += len;
            break;
        }
        case TYPE_HINFO:
        {
            len = *cp++;
            cp += len;

            len = *cp++;
            cp += len;
            break;
        }
        case TYPE_MX:
        {
            cp += 2;
            /* Get domain name of exchanger */
            len = dns_parse_name(msg, cp, name, DNS_MAXCNAME);
            if (len == -1) return NULL;

            cp += len;
            break;
        }
        case TYPE_SOA:
        {
            /* Get domain name of name server */
            len = dns_parse_name(msg, cp, name, DNS_MAXCNAME);
            if (len == -1) return NULL;

            cp += len;

            /* Get domain name of responsible person */
            len = dns_parse_name(msg, cp, name, DNS_MAXCNAME);
            if (len == -1) return NULL;

            cp += len;

            cp += 4;
            cp += 4;
            cp += 4;
            cp += 4;
            cp += 4;
            break;
        }
        case TYPE_TXT:
        {
        /* Just stash */
            break;
        }
        default:break;
    }

    return cp;
}

/******************************************************************************
* Function    : dns_parse_respond
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint8 dns_parse_respond(DNS_CLIENT *dns, uint8 *resIP)
{
    uint8 recvBuf[DNS_BUF_SIZE];
    uint8  serAddr[4];
    uint16 serPort;
    uint16 recvLen;

    recvLen = dns->recvfrom(dns->sock, recvBuf, sizeof(recvBuf), serAddr, &serPort);
    if (recvLen == 0)
    {
        DEBUG_TRACE("DNS recv none");
        return DNS_NONE;
    }

    uint16 tmp;
    uint16 i;
    uint8 * msg;
    uint8 * cp;
    DNS_DHDR dhdr;

    msg = recvBuf;
    memset(&dhdr, 0, sizeof(dhdr));

    dhdr.id = dns_get16(&msg[0]);
    tmp = dns_get16(&msg[2]);
    if (tmp & 0x8000) dhdr.qr = 1;

    dhdr.opcode = (tmp >> 11) & 0xf;

    if (tmp & 0x0400) dhdr.aa = 1;
    if (tmp & 0x0200) dhdr.tc = 1;
    if (tmp & 0x0100) dhdr.rd = 1;
    if (tmp & 0x0080) dhdr.ra = 1;

    dhdr.rcode = tmp & 0xf;
    dhdr.qdcount = dns_get16(&msg[4]);
    dhdr.ancount = dns_get16(&msg[6]);
    dhdr.nscount = dns_get16(&msg[8]);
    dhdr.arcount = dns_get16(&msg[10]);

    /* Now parse the variable length sections */
    cp = &msg[12];

    /* Question section */
    for (i = 0; i < dhdr.qdcount; i++)
    {
        cp = dns_question(msg, cp);
        if(NULL == cp) return DNS_ERR;
    }

    /* Answer section */
    for (i = 0; i < dhdr.ancount; i++)
    {
        cp = dns_answer(msg, cp, resIP);
        if(NULL == cp) return DNS_ERR;
    }

    /* Name server (authority) section */
    for (i = 0; i < dhdr.nscount; i++)
    {
        ;
    }

    /* Additional section */
    for (i = 0; i < dhdr.arcount; i++)
    {
        ;
    }

    if(dhdr.rcode == 0) return DNS_OK;  // No error
    else return DNS_ERR;
}

/******************************************************************************
* Function    : dns_try_to_resend
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool dns_try_to_resend(DNS_CLIENT *dns, uint8 *sendBuf, uint16 sendLen)
{
    uint32 curTime = util_systick_second();
    DEBUG_TRACE("DNS retry[%d], cur[%d] last[%d]", dns->retryCnt, curTime, dns->lastTime);

    if (dns->retryCnt < DNS_MAX_RETRY) 
    {
        //timeout and re-send message
        if ((curTime - dns->lastTime) > DNS_WAIT_TIME) 
        {
            dns->retryCnt++;
            dns->sendto(dns->sock, sendBuf, sendLen, dns->dnsIP, DNS_IPPORT);
            dns->lastTime = curTime;

            DEBUG_TRACE("DNS timeout, resend");
        }
    }
    else
    {
        return false;
    }

    return true;
}

/******************************************************************************
* Function    : dns_process
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
uint8 dns_process(DNS_CLIENT *dns, char *domain, uint8 *resIP)
{
    uint8 ret = DNS_ERR;
    uint16 dnsMsgLen = 0;
    uint8 dnsQueryMsg[DNS_BUF_SIZE];
    memset(dnsQueryMsg, 0, sizeof(dnsQueryMsg));

    DEBUG_TRACE("DNS SERVER %d.%d.%d.%d",
                                  dns->dnsIP[0],  dns->dnsIP[1],  dns->dnsIP[2],  dns->dnsIP[3]);
    
    dnsMsgLen = dns_assemble_query_message(dns, domain, dnsQueryMsg);
    
    if (0 == dns->sendto(dns->sock, dnsQueryMsg, dnsMsgLen, dns->dnsIP, DNS_IPPORT))
    {
        return DNS_ERR;
    }
    dns->lastTime = util_systick_second();
    
    //parse DNS respond
    while (1)
    {
        ret = dns_parse_respond(dns, resIP);
        if (DNS_OK == ret)
        {
            return DNS_OK;
        }
        else
        if (DNS_ERR == ret)
        {
            return DNS_ERR;
        }

        if (!dns_try_to_resend(dns, dnsQueryMsg, dnsMsgLen))
        {
            DEBUG_TRACE( "DNS FAILED, TIMEOUT");
            return DNS_ERR;
        }
        util_delay(DELAY_50_MS);
    }
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
void dns_client_init(DNS_CLIENT *dns)
{
    DEBUG_TRACE("DNS Init");

    dns->msgID = DNS_MSG_ID;
    dns->lastTime = 0;
    dns->retryCnt = 0;
}

