/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          datastruct.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   define the common data struct
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-2-12             1.00                    Chen Hao
*
******************************************************************************/
#ifndef __DATASTRUCT_H__
#define __DATASTRUCT_H__

/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"

/******************************************************************************
* Types
******************************************************************************/
typedef volatile struct
{
    volatile uint16 buffSize;
    volatile uint8 *pHead;
    volatile uint8 *pTail;
    volatile uint8 *pBuff;
}ARRAY_QUEUE_TYPE;

typedef struct list_queue_node_t
{
    struct list_queue_node_t *nextNode_p;
    uint16 length;
    uint8 data[1];
}LIST_QUEUE_NODE_TYPE;

typedef struct list_queue_t
{
    uint8 queueSize;
    LIST_QUEUE_NODE_TYPE *head_p;
    LIST_QUEUE_NODE_TYPE *tail_p;
}LIST_QUEUE_TYPE;

/******************************************************************************
* Global Functions
******************************************************************************/
/*Array queue functions*/
extern uint16 array_que_create(ARRAY_QUEUE_TYPE *q, uint8 *buff, uint16 size);
extern uint16 array_que_destroy(ARRAY_QUEUE_TYPE *q);
extern uint16 array_que_size(ARRAY_QUEUE_TYPE *q);
extern uint16 array_que_push(ARRAY_QUEUE_TYPE *q, uint8 byte);
extern uint16 array_que_pop(ARRAY_QUEUE_TYPE *q);
extern uint16 array_que_packet_in(ARRAY_QUEUE_TYPE *q, uint8 *buff, uint16 len);
extern uint16 array_que_packet_out(ARRAY_QUEUE_TYPE *q, uint8 *buff, uint16 len);

/*List queue functions*/
extern uint16 list_que_size(LIST_QUEUE_TYPE *q);
extern bool list_que_append(LIST_QUEUE_TYPE *q, const uint16 queMaxSize, const uint8* data, uint16 maxLen, uint16 len);
extern void list_que_remove_head(LIST_QUEUE_TYPE *q);
extern bool list_que_head_data(LIST_QUEUE_TYPE *q, uint8 **pdata, uint16 *len);

#endif /*__DATASTRUCT_H__*/
