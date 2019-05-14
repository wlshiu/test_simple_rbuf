/**
 * Copyright (c) 2019 Wei-Lun Hsu. All Rights Reserved.
 */
/** @file rbuf.h
 *
 * @author Wei-Lun Hsu
 * @version 0.1
 * @date 2019/05/14
 * @license
 * @description
 */


#ifndef __rbuf_H_4d5e77a5_0a1c_4acf_b2cd_c07535bc9989__
#define __rbuf_H_4d5e77a5_0a1c_4acf_b2cd_c07535bc9989__

#ifdef __cplusplus
extern "C" {
#endif

//#include <stdint.h>
//=============================================================================
//                  Constant Definition
//=============================================================================

//=============================================================================
//                  Macro Definition
//=============================================================================

//=============================================================================
//                  Structure Definition
//=============================================================================
typedef struct rbuf
{
    unsigned long   rd_idx;
    unsigned long   wr_idx;
    unsigned char   *pBuf;
    unsigned long   buf_len;
} rbuf_t;
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
    unsigned long   buf_size);


int
rbuf_push(
    rbuf_t          *pHRbuf,
    unsigned char   *pData,
    unsigned long   data_size);


int
rbuf_pop(
    rbuf_t          *pHRbuf,
    unsigned char   *pData,
    unsigned long   *pData_size);


#ifdef __cplusplus
}
#endif

#endif


