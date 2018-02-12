/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          basetype.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   Define all the basic variables type
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-02-12            1.00                    Chen Hao
*
******************************************************************************/
#ifndef __BASETYPE_H__
#define __BASETYPE_H__

/******************************************************************************
* Types
******************************************************************************/
#ifndef NULL
#define NULL ((void *)0)
#endif

typedef enum {false = 0, true = !false} bool;
typedef unsigned char uint8;
typedef char int8;
typedef unsigned short uint16;
typedef short int16;
typedef unsigned int uint32;
typedef int int32;
typedef unsigned long long int uint64;
typedef long long int int64;

#endif /*__BASETYPE_H__*/
