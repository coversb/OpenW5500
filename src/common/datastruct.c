/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          datastruct.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   define the common data struct
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-2-12      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include <string.h>
#include "datastruct.h"
#include "util.h"

/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : array_que_create
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : create a new queue datastruct for "q"
******************************************************************************/
uint16 array_que_create(ARRAY_QUEUE_TYPE *q, uint8 *buff, uint16 size)
{
    q->buffSize = size;
    q->pBuff = buff;
    q->pHead = buff;
    q->pTail = buff;

    return 0;
}

/******************************************************************************
* Function    : array_que_destroy
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : destroy queue
******************************************************************************/
uint16 array_que_destroy(ARRAY_QUEUE_TYPE *q)
{
    q->buffSize = 0;
    q->pBuff = NULL;
    q->pHead = NULL;
    q->pTail = NULL;

    return 0;
}

/******************************************************************************
* Function    : array_que_size
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get que size
******************************************************************************/
uint16 array_que_size(ARRAY_QUEUE_TYPE *q)
{
    volatile uint8 *pHead = NULL;
    volatile uint8 *pTail = NULL;
    uint16 size = 0;

    pHead = q->pHead;
    pTail = q->pTail;
    
    if (pTail - pHead >= 0)
    {
        size = pTail - pHead;
    }
    else
    {
        size = pTail - pHead + q->buffSize;
    }

    return size;
}

/******************************************************************************
* Function    : array_que_push
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : push one byte back to the q
******************************************************************************/
uint16 array_que_push(ARRAY_QUEUE_TYPE *q, uint8 byte)
{
    volatile uint8 *pTail = NULL;

    pTail = q->pTail;

    if (++pTail >= (q->pBuff + q->buffSize))//back to buffer area header
    {
        pTail = q->pBuff;
    }

    if (pTail == q->pHead) //que is full
    {
        return 0;
    }

    *(q->pTail) = byte;

    q->pTail = pTail;

    return 1;
}

/******************************************************************************
* Function    : array_que_pop
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : pop one byte from q's head
******************************************************************************/
uint16 array_que_pop(ARRAY_QUEUE_TYPE *q)
{
    uint8 byte = 0;
    
    if (q->pHead != q->pTail)
    {
        byte = *(q->pHead);
        q->pHead++;
        
        if (q->pHead >= q->pBuff + q->buffSize) 
        {
            q->pHead= q->pBuff;
        }
    }

    return byte;
}

/******************************************************************************
* Function    : array_que_packet_in
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : put bytes back to the queue
******************************************************************************/
uint16 array_que_packet_in(ARRAY_QUEUE_TYPE *q, uint8 *buff, uint16 len)
{
    volatile uint8 *pTail = NULL;
    uint16 idx = 0;

    pTail = q->pTail;
    
    for (idx = 0; idx < len; ++idx)
    {
        if (++pTail >= q->pBuff + q->buffSize)
        {
            pTail = q->pBuff;
        }
        if (pTail == q->pHead) 
        {
            break;
        }
        
        *(q->pTail) = *(buff);
        buff++;
        
        q->pTail = pTail;
    }
    
    return idx;
}

/******************************************************************************
* Function    : array_que_packet_out
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get bytes from the queue front
******************************************************************************/
uint16 array_que_packet_out(ARRAY_QUEUE_TYPE *q, uint8 *buff, uint16 len)
{
    uint16 idx = 0;

    while ((q->pHead != q->pTail) && (idx < len) && (idx < q->buffSize))
    {
        buff[idx++] = *(q->pHead);
        q->pHead++;
        
        if (q->pHead >= q->pBuff + q->buffSize) 
        {
            q->pHead = q->pBuff;
        }
    }

    return idx;
}

/******************************************************************************
* Function    : list_que_size
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
uint16 list_que_size(LIST_QUEUE_TYPE *q)
{
    return q->queueSize;
}

/******************************************************************************
* Function    : list_que_append
* 
* Author	  : Chen Hao
* 
* Parameters  : 
* 
* Return	  : 
* 
* Description : 
******************************************************************************/
bool list_que_append(LIST_QUEUE_TYPE *q, const uint16 queMaxSize, const uint8* data, uint16 maxLen, uint16 len)
{
    if (len == 0)
    {
        return false;
    }
    
    bool ret = false;
    unsigned int malloc_size = 0;
    LIST_QUEUE_NODE_TYPE *addNode_p = NULL;

    if ((q->queueSize + 1) > queMaxSize)
    {
        goto addEnd;
    }

    malloc_size = MIN_VALUE(maxLen, len);

    addNode_p = (LIST_QUEUE_NODE_TYPE*)util_malloc(sizeof(LIST_QUEUE_NODE_TYPE) + malloc_size);
    if (NULL == addNode_p)
    {
        DEBUG_TRACE("Que malloc err");
        goto addEnd;
    }

    addNode_p->length = malloc_size;
    addNode_p->nextNode_p = NULL;
    memcpy((void*)addNode_p->data, data, malloc_size);

    if (NULL == q->tail_p)
    {
        q->head_p = addNode_p;
        q->tail_p = addNode_p;
    }
    else
    {
        q->tail_p->nextNode_p = addNode_p;
        q->tail_p = addNode_p;
    }
    q->queueSize++;

    ret = true;
addEnd:

    return ret;
}

/******************************************************************************
* Function	  : list_que_remove_head
* 
* Author	  : Chen Hao
* 
* Parameters  : 
* 
* Return	  : 
* 
* Description : 
******************************************************************************/
void list_que_remove_head(LIST_QUEUE_TYPE *q)
{
	LIST_QUEUE_NODE_TYPE *delNode_p = NULL;

	if ((0 != q->queueSize) && (NULL != q->head_p))
	{
		delNode_p = q->head_p;
		q->head_p = delNode_p->nextNode_p;

		if (q->tail_p == delNode_p)
		{
			q->head_p = NULL;
			q->tail_p = q->head_p;
			q->queueSize = 0;
		}
		else
		{
			q->queueSize--;
		}
		util_free(delNode_p);
	}
}

/******************************************************************************
* Function    : list_que_head_data
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
bool list_que_head_data(LIST_QUEUE_TYPE *q, uint8 **pdata, uint16 *len)
{
    if (list_que_size(q) == 0)
    {
        DEBUG_TRACE("Que is empty");
        return false;
    }
    
    (*pdata) = q->head_p->data;
    (*len) = q->head_p->length;
    
    return true;
}

