/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          dhcp.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   DHCP client
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-02-12             1.00                    Chen Hao
*
******************************************************************************/
#ifndef __DHCP_H__
#define __DHCP_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"

/******************************************************************************
* Macros
******************************************************************************/
#define DHCP_MAX_RETRY 3
#define DHCP_WAIT_TIMEOUT 5  // 5 seconds timeout

#define DHCP_FLAGSBROADCAST 0x8000

/* UDP port numbers for DHCP */
#define DHCP_SERVER_PORT 67 //from server to client
#define DHCP_CLIENT_PORT 68 //from client to server

#define DHCP_HLENETHERNET 6
#define DHCP_HOPS 0
#define DHCP_SECS 0

#define MAGIC_COOKIE 0x63825363
#define DEFAULT_XID 0x12345678

/* infinite lease time */
#define INFINITE_LEASETIME 0xffffffff

#define MAX_DHCP_OPT 16

/******************************************************************************
* Enum
******************************************************************************/
/* DHCP return state */
typedef enum
{
    DHCP_RET_NONE = 0,
    DHCP_RET_ERR,
    DHCP_RET_UPDATE,
    DHCP_RET_LEASED_IP,
    DHCP_RET_CONFLICT
}DHCP_RET_STAT;

/* DHCP state machine. */
typedef enum
{
    DHCP_STATE_INIT = 0,
    DHCP_STATE_DISCOVER,
    DHCP_STATE_REQUEST,
    DHCP_STATE_LEASED,
    DHCP_STATE_REREQUEST,
    DHCP_STATE_RELEASE
}DHCP_STATE;

/* DHCP message OP code */
typedef enum
{
    DHCP_BOOTREQUEST = 1,
    DHCP_BOOTREPLY = 2
}DHCP_OP_CODE;

/* DHCP message type */
typedef enum
{
    DHCP_NONE = 0,
    DHCP_DISCOVER = 1,
    DHCP_OFFER,
    DHCP_REQUEST,
    DHCP_DECLINE,
    DHCP_ACK,
    DHCP_NAK,
    DHCP_RELEASE,
    DHCP_INFORM
}DHCP_MESSAGE_TYPE;

typedef enum
{
    DHCP_HTYPE10MB = 1,
    DHCP_HTYPE100MB = 2
}DHCP_HTYPE;

/* DHCP option and value (cf. RFC1533) */
typedef enum
{
   padOption               = 0,
   subnetMask              = 1,
   timerOffset             = 2,
   routersOnSubnet         = 3,
   timeServer              = 4,
   nameServer              = 5,
   dns                     = 6,
   logServer               = 7,
   cookieServer            = 8,
   lprServer               = 9,
   impressServer           = 10,
   resourceLocationServer  = 11,
   hostName                = 12,
   bootFileSize            = 13,
   meritDumpFile           = 14,
   domainName              = 15,
   swapServer              = 16,
   rootPath                = 17,
   extentionsPath          = 18,
   IPforwarding            = 19,
   nonLocalSourceRouting   = 20,
   policyFilter            = 21,
   maxDgramReasmSize       = 22,
   defaultIPTTL            = 23,
   pathMTUagingTimeout     = 24,
   pathMTUplateauTable     = 25,
   ifMTU                   = 26,
   allSubnetsLocal         = 27,
   broadcastAddr           = 28,
   performMaskDiscovery    = 29,
   maskSupplier            = 30,
   performRouterDiscovery  = 31,
   routerSolicitationAddr  = 32,
   staticRoute             = 33,
   trailerEncapsulation    = 34,
   arpCacheTimeout         = 35,
   ethernetEncapsulation   = 36,
   tcpDefaultTTL           = 37,
   tcpKeepaliveInterval    = 38,
   tcpKeepaliveGarbage     = 39,
   nisDomainName           = 40,
   nisServers              = 41,
   ntpServers              = 42,
   vendorSpecificInfo      = 43,
   netBIOSnameServer       = 44,
   netBIOSdgramDistServer  = 45,
   netBIOSnodeType         = 46,
   netBIOSscope            = 47,
   xFontServer             = 48,
   xDisplayManager         = 49,
   dhcpRequestedIPaddr     = 50,
   dhcpIPaddrLeaseTime     = 51,
   dhcpOptionOverload      = 52,
   dhcpMessageType         = 53,
   dhcpServerIdentifier    = 54,
   dhcpParamRequest        = 55,
   dhcpMsg                 = 56,
   dhcpMaxMsgSize          = 57,
   dhcpT1value             = 58,
   dhcpT2value             = 59,
   dhcpClassIdentifier     = 60,
   dhcpClientIdentifier    = 61,
   endOption               = 255
}DHCP_OPTION;

/******************************************************************************
* Type
******************************************************************************/
typedef struct
{
    uint8  op; 
    uint8  htype; 
    uint8  hlen;
    uint8  hops;
    uint32 xid;
    uint16 secs;
    uint16 flags;
    uint8  ciaddr[4];
    uint8  yiaddr[4];
    uint8  siaddr[4];
    uint8  giaddr[4];
    uint8  chaddr[16];
    uint8  sname[64];
    uint8  file[128];
    uint8  OPT[312];
}RIP_MSG;

typedef struct
{
    uint8 state;
    uint32 xid;
    uint8 retryCnt;
    uint32 lastTime;
    uint32  leaseTime;    // Leased time
    uint8 oldIP[4]; // Previous local ip address received from DHCP server
    uint8 serverIP[4];  // DHCP server ip address is discovered
    uint8 realServerIP[4];  // For extract DHCP server in a few DHCP servers

    /*network config*/
    uint8 mac[6];
    uint8 ip[4];
    uint8 mask[4];
    uint8 gateway[4];
    uint8 dns[4];

    /*udp function*/
    uint8 sock;
    uint16 (*sendto)(uint8 s, uint8 *pbuf, uint16 len, uint8 *addr, uint16 port);
    uint16 (*recvfrom)(uint8 s, uint8 *pbuf, uint16 len, uint8 *addr, uint16 *port);
}DHCP_CLIENT;

/******************************************************************************
* Extern functions
******************************************************************************/
extern void  dhcp_client_init(DHCP_CLIENT *dhcp);
extern uint8 dhcp_client_process(DHCP_CLIENT *dhcp);

#endif	/* __DHCP_H__ */
