/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          util.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   utility functions
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-02-12             1.00                    Chen Hao
*
******************************************************************************/
#ifndef __UTIL_H__
#define __UTIL_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"

/******************************************************************************
* Firmware version definition
******************************************************************************/
#define OPENW5500_VERSION "V1.00.00"

/******************************************************************************
* Macros
******************************************************************************/
#define _DEBUG_TRACE

#ifdef _DEBUG_TRACE
#define DEBUG_TRACE(...)  util_trace_debug(__FILE__, __LINE__, __VA_ARGS__)
#else /*_DEBUG_TRACE*/
#define DEBUG_TRACE(...)
#endif /*_DEBUG_TRACE*/

#define MIN_VALUE(a, b) ((a) > (b) ? (b) : (a))
#define MAX_VALUE(a, b) ((a) < (b) ? (b) : (a))
#define BIT_SET(a, b) ((a) |= (1<<(b)))
#define BIT_CLEAR(a, b) ((a) &= ~(1<<(b)))
#define BIT_CHECK(a, b) ((a) & (1<<(b)))
#define CEIL_VALUE(a, b) (((a) + (b) - 1) / (b))
#define ASSERT(x) 

#define DELAY_1_MS 1
#define DELAY_50_MS 50
#define DELAY_100_MS 100
#define DELAY_500_MS 500
#define DELAY_1_S 1000

/******************************************************************************
* Enums
******************************************************************************/

/******************************************************************************
* Global Functions
******************************************************************************/
extern void util_trace_debug(const char *file, const uint32 line, const char *fmt, ...);
extern void util_delay(uint32 ms);
extern uint32 util_systick_second(void);
extern uint32 util_get_random_num(uint32 div);
extern void *util_malloc(uint32 size);
extern void util_free(void *p);
extern bool util_check_is_ip(char *str, uint16 len);
extern bool util_get_url_param(char *src, uint16 maxLen, char *addr, char *path, char *fname);

#endif /* __UTIL_H__ */

