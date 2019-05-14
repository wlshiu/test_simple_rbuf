/**
 * Copyright (c) 2019 Wei-Lun Hsu. All Rights Reserved.
 */
/** @file rbuf.c
 *
 * @author Wei-Lun Hsu
 * @version 0.1
 * @date 2019/05/14
 * @license
 * @description
 */

#include <stdio.h>
#include <string.h>
#include "rbuf.h"

//=============================================================================
//                  Constant Definition
//=============================================================================

//=============================================================================
//                  Macro Definition
//=============================================================================

//=============================================================================
//                  Structure Definition
//=============================================================================

//=============================================================================
//                  Global Data Definition
//=============================================================================

//=============================================================================
//                  Private Function Definition
//=============================================================================

//=============================================================================
//                  Public Function Definition
//=============================================================================
int
rbuf_init(
    rbuf_t          *pHRbuf,
    unsigned char   *pBuf,
    unsigned long   buf_size)
{
    if( !pHRbuf )   return -1;

    pHRbuf->rd_idx   = pHRbuf->wr_idx = 0;
    pHRbuf->pBuf     = pBuf;
    pHRbuf->buf_len  = buf_size;

    return 0;
}

int
rbuf_push(
    rbuf_t          *pHRbuf,
    unsigned char   *pData,
    unsigned long   data_size)
{
    int             rval = 0;
    unsigned long   wr_idx = pHRbuf->wr_idx;
    unsigned long   rd_idx = pHRbuf->rd_idx;

    do {
        unsigned long   pos = 0;
        unsigned long   len = 0;

        if( !pHRbuf )
        {
            rval = -(__LINE__);
            break;
        }

        pos = wr_idx + data_size;
        pos = (pos < pHRbuf->buf_len) ? pos : pos - pHRbuf->buf_len;

        // buffer full checking
        if( (wr_idx > rd_idx && pos >= rd_idx && pos < wr_idx) ||
            (wr_idx < rd_idx && (pos >= rd_idx || pos < wr_idx)) )
        {
            rval = -(__LINE__);
            break;
        }

        len = pHRbuf->buf_len - wr_idx;
        len = (data_size < len) ? data_size : len;
        memcpy(&pHRbuf->pBuf[wr_idx], pData, len);

        if( (data_size - len) )
        {
            // two part copy
            unsigned long   remain = data_size - len;
            memcpy((void*)&pHRbuf->pBuf[0], (void*)((unsigned long)pData + len), remain);
        }

        pHRbuf->wr_idx = pos;
    } while(0);

    return rval;
}

int
rbuf_pop(
    rbuf_t          *pHRbuf,
    unsigned char   *pData,
    unsigned long   *pData_size)
{
    int             rval = 0;
    unsigned long   wr_idx = pHRbuf->wr_idx;
    unsigned long   rd_idx = pHRbuf->rd_idx;

    do {
        unsigned long   pos = 0;
        unsigned long   len = *pData_size;
        unsigned long   remain = pHRbuf->buf_len - rd_idx;

        *pData_size = 0;

        if( !pHRbuf )
        {
            rval = -(__LINE__);
            break;
        }

        // buffer empty
        if( wr_idx == rd_idx )
        {
            rval = -(__LINE__);
            break;
        }

        if( rd_idx < wr_idx )
            len = (len < (wr_idx - rd_idx)) ? len : (wr_idx - rd_idx);
        else
            len = (len < (pHRbuf->buf_len - rd_idx + wr_idx)) ? len : (pHRbuf->buf_len - rd_idx + wr_idx);

        remain = (remain < len) ? remain : len;

        memcpy(pData, &pHRbuf->pBuf[rd_idx], remain);

        if( (len - remain) )
        {
            // two part copy
            unsigned long   remain_2 = len - remain;
            memcpy((void*)((unsigned long)pData + remain), &pHRbuf->pBuf[0], remain_2);
        }

        *pData_size = len;
        pos = rd_idx + len;
        pos = (pos < pHRbuf->buf_len) ? pos : pos - pHRbuf->buf_len;

        pHRbuf->rd_idx = pos;
    } while(0);
    return rval;
}
