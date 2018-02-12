/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          util.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   utility functions
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-2-12      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "drv_util.h"
#include "util.h"

/******************************************************************************
* Macros
******************************************************************************/
#define TRACE_BUFF_SIZE 255

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/

/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : util_trace_debug
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void util_trace_debug(const char *file, const uint32 line, const char *fmt, ...)
{
    char trace_buff[TRACE_BUFF_SIZE + 1];
    uint32 len = 0;

    va_list va;
    va_start(va, fmt);
    len += vsnprintf((trace_buff), (TRACE_BUFF_SIZE - len), fmt, va);
    va_end(va);
    
    //add file name and line
    char *pFileName = NULL;
    pFileName = strrchr (file, '\\');   //find the last '\' to get file name without path
    if (pFileName == NULL)
    {
        pFileName = (char*)file;
    }
    else
    {
        pFileName++;
    }
    len += snprintf((trace_buff + len), (TRACE_BUFF_SIZE - len), " F-%s, L-%d", pFileName, line);

    trace_buff[len] = '\0';
    printf("%s\r\n", trace_buff);
}

/******************************************************************************
* Function    : util_delay
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void util_delay(uint32 ms)
{
    drv_util_delay_ms(ms);
}

/******************************************************************************
* Function    : util_systick_second
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
uint32 util_systick_second(void)
{
    return drv_util_systick_millis() / 1000;
}

/******************************************************************************
* Function    : util_get_random_num
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
uint32 util_get_random_num(uint32 div)
{
    return drv_util_systick_millis() % (div + 1);
}

/******************************************************************************
* Function    : util_malloc
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void *util_malloc(uint32 size)
{
    return malloc(size);
}

/******************************************************************************
* Function    : util_free
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void util_free(void *p)
{
    free(p);
}

/******************************************************************************
* Function    : util_check_is_ip
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
bool util_check_is_ip(char *str, uint16 len)
{
    bool ret = true;
    for (uint16 idx = 0; idx < len; ++idx)
    {
        if (str[idx] == '.') continue;
        if (str[idx] < '0' || str[idx] > '9')
        {
            ret = false;
            break;
        }
    }

    return ret;
}

/******************************************************************************
* Function    : util_get_url_param
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get parameters from url
******************************************************************************/
bool util_get_url_param(char *src, uint16 maxLen, char *addr, char *path, char *fname)
{
    char *p = src;
    char *pPath = NULL;
    char *pFname = NULL;
    uint16 len;

    //find file name in url
    if (NULL != (pFname = (strrchr(p, '/'))))//find last '/'
    {
        len = strlen(pFname) - 1;
        if (len > maxLen)
        {
            DEBUG_TRACE("Bad fname[%d]", len);
            return false;
        }
        memcpy(fname, (pFname+1), len);
        fname[len] = '\0';
        DEBUG_TRACE("fname[%s]", fname);
    }
    else
    {
        return false;
    }

    // find ip path in url
    if (NULL != (pPath = (strchr(p, '/'))))//find first '/'
    {
        len = pFname - pPath + 1;
        if (len > maxLen)
        {
            DEBUG_TRACE("Bad path[%d]", len);
            return false;
        }
        memcpy(path, pPath, len);
        path[len] = '\0';
        DEBUG_TRACE("path[%s]", path);
    }
    else
    {
        return false;
    }

    //find address in url
    len = pPath - p;
    if (len > maxLen)
    {
        DEBUG_TRACE("Bad addr[%d]", len);
        return false;
    }
    memcpy(addr, p, len);
    addr[len] = '\0';
    DEBUG_TRACE("addr[%s]", addr);

    return true;
}


