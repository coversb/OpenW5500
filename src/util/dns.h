/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          dns.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   DNS client
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-02-12             1.00                    Chen Hao
*
******************************************************************************/
#ifndef __DNS_H__
#define __DNS_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"

/******************************************************************************
* Macros
******************************************************************************/
#define DNS_BUF_SIZE 512
#define DNS_MAX_DOMAIN_NAME 60
/* Maximum amount of cname recursion */
#define DNS_MAXCNAME (DNS_MAX_DOMAIN_NAME + (DNS_MAX_DOMAIN_NAME>>1))

#define DNS_MAX_RETRY 2
#define DNS_WAIT_TIME 3

#define DNS_CLIENT_PORT 0
#define DNS_IPPORT 53    ///< DNS server port number

#define DNS_MSG_ID 0x1122    ///< ID for DNS message. You can be modifyed it any number

//DNS DHDR define
#define QUERY 0
#define RESPONSE 1
#define IQUERY 1
#define NO_ERROR 0
#define FORMAT_ERROR 1
#define SERVER_FAIL 2
#define NAME_ERROR 3
#define NOT_IMPL 4
#define REFUSED 5

//DNS ANWSER
#define	TYPE_A		1	   /* Host address */
#define	TYPE_NS		2	   /* Name server */
#define	TYPE_MD		3	   /* Mail destination (obsolete) */
#define	TYPE_MF		4	   /* Mail forwarder (obsolete) */
#define	TYPE_CNAME	5	   /* Canonical name */
#define	TYPE_SOA	   6	   /* Start of Authority */
#define	TYPE_MB		7	   /* Mailbox name (experimental) */
#define	TYPE_MG		8	   /* Mail group member (experimental) */
#define	TYPE_MR		9	   /* Mail rename name (experimental) */
#define	TYPE_NULL	10	   /* Null (experimental) */
#define	TYPE_WKS	   11	   /* Well-known sockets */
#define	TYPE_PTR	   12	   /* Pointer record */
#define	TYPE_HINFO	13	   /* Host information */
#define	TYPE_MINFO	14	   /* Mailbox information (experimental)*/
#define	TYPE_MX		15	   /* Mail exchanger */
#define	TYPE_TXT	   16	   /* Text strings */
#define	TYPE_ANY	   255	/* Matches any type */

#define	CLASS_IN	   1	   /* The ARPA Internet */

/******************************************************************************
* Enum
******************************************************************************/
typedef enum
{
    DNS_NONE = 0,
    DNS_ERR,
    DNS_OK
}DNS_MESSAGE_TYPE;

/******************************************************************************
* Type
******************************************************************************/
typedef struct
{
    uint16 id;   /* Identification */
    uint8 qr;      /* Query/Response */
    uint8 opcode;
    uint8 aa;      /* Authoratative answer */
    uint8 tc;      /* Truncation */
    uint8 rd;      /* Recursion desired */
    uint8 ra;      /* Recursion available */
    uint8 rcode;   /* Response code */
    uint16 qdcount;	/* Question count */
    uint16 ancount;	/* Answer count */
    uint16 nscount;	/* Authority (name server) count */
    uint16 arcount;	/* Additional record count */
}DNS_DHDR;

typedef struct
{
    uint8 retryCnt;
    uint32 lastTime;
    uint16 msgID;
    uint8 dnsIP[4];

    /*udp function*/
    uint8 sock;
    uint16 (*sendto)(uint8 s, uint8 *pbuf, uint16 len, uint8 *addr, uint16 port);
    uint16 (*recvfrom)(uint8 s, uint8 *pbuf, uint16 len, uint8 *addr, uint16 *port);
}DNS_CLIENT;

/******************************************************************************
* Extern functions
******************************************************************************/
extern void dns_client_init(DNS_CLIENT *dns);
extern uint8 dns_process(DNS_CLIENT *dns, char *domain, uint8 *resIP);

#endif	/* __DNS_H__ */
