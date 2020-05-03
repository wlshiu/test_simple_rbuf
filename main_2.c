/**
 * Copyright (c) 2018 Wei-Lun Hsu. All Rights Reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include <windows.h>

#include "rbuf_opt.h"
//=============================================================================
//                  Constant Definition
//=============================================================================
#define CONFIG_MAX_BUF_SIZE            (10 << 10)
//=============================================================================
//                  Macro Definition
//=============================================================================
#define err(str, ...)       do{ printf("[%s:%d] " str, __func__, __LINE__, ## __VA_ARGS__); while(1);}while(0)

#define STREAM_EOS          (((' ') << 24) | (('S') <<16) | (('O') << 8) | ('E'))
//=============================================================================
//                  Structure Definition
//=============================================================================

//=============================================================================
//                  Global Data Definition
//=============================================================================
static unsigned char    g_rbuf[CONFIG_MAX_BUF_SIZE] = {0};
static rb_operator_t    g_hRB = {0};
static int              g_eof = 0;
//=============================================================================
//                  Private Function Definition
//=============================================================================

//=============================================================================
//                  Public Function Definition
//=============================================================================
static int
_get_data_size(
    rb_data_info_t  *pInfo)
{
    static int      end_flag = 0;
    int             rval = 0;
    unsigned char   *pTmp = pInfo->r_ptr + (pInfo->data_size - 4);

    if( g_eof && pInfo->read_idx == RB_READ_TYPE_FETCH )
    {
        g_eof = 0;
        pInfo->is_dummy = 1;
        return STREAM_EOS;
    }

    if( pTmp[0] == 0xFF && pTmp[1] == 0xFF && pTmp[2] == 0xFF && pTmp[3] == 0xFF )
    {
        pInfo->data_size = pInfo->data_size - 4;
        g_eof = 1;
    }

    pInfo->data_size &= ~(0x3);
    printf("data size = %d\n", pInfo->data_size);
    pInfo->is_dummy = 0;
    return rval;
}

static void*
_task_read(void *argv)
{
    int     *pIs_running = (int*)argv;
    char    *pFilename = "dump.jpg";
    FILE    *fout = 0;

    do {
        if( !(fout = fopen(pFilename, "wb")) )
        {
            err("open %s fail \n", pFilename);
            break;
        }

        while( *pIs_running )
        {
            rb_err_t        rb_err = RB_ERR_OK;
            unsigned char   *pData = 0;
            unsigned int    data_size = 0;

            rb_err = rb_opt_update_r(&g_hRB, RB_READ_TYPE_FETCH, &pData, &data_size, _get_data_size);
            switch( rb_err )
            {
                case RB_ERR_OK:
                    if( data_size && data_size != RB_INVALID_SIZE && fout )
                    {
                        fwrite(pData, 1, data_size, fout);
                    }
                    break;
                case RB_ERR_NO_SPACE:
                    printf("no space \n");
                    break;
                case RB_ERR_W_CATCH_R:
                    printf("w_idx catch r_idx \n");
                    break;
                case RB_ERR_INVALID_PARAM:
                    printf("invalid param\n");
                    break;
                case STREAM_EOS:
                    if( fout )
                    {
                        fclose(fout);
                        fout = 0;
                    }
                    break;
                case RB_ERR_R_CATCH_W:
                    break;
                case RB_ERR_NO_DATA:
                    break;
                default:
                    printf("unknown error %d \n", rb_err);
                    break;
            }

            Sleep(1);

            data_size = 0;
            rb_opt_update_r(&g_hRB, RB_READ_TYPE_REMOVE, &pData, &data_size, _get_data_size);
        }
    } while(0);

    if( fout )      fclose(fout);
    pthread_exit(0);
    return 0;
}

static void*
_task_write(void *argv)
{
    int     *pIs_running = (int*)argv;
    char    *pFilename = "test.jpg";
    FILE    *fin = 0;

    do {
        if( !(fin = fopen(pFilename, "rb")) )
        {
            err("open %s fail \n", pFilename);
            break;
        }

        while( *pIs_running )
        {
            rb_err_t        rb_err = RB_ERR_OK;
            int             data_size = (rand() >> 10) & 0x1FF;
            unsigned char   buf[512] = {0};

            if( fin && feof(fin) )
            {
                fclose(fin);
                fin = 0;

                memset(buf, 0xFF, sizeof(buf));
                rb_opt_update_w(&g_hRB, buf, 8); // It MUST send more than 8 bytes data

                g_eof = 1;
            }

            data_size = (data_size) ? data_size : 512;
            data_size = fread(buf, 1, data_size, fin);

            while( (rb_err = rb_opt_update_w(&g_hRB, buf, data_size)) )
            {
                switch( rb_err )
                {
                    case RB_ERR_NO_SPACE:
                        printf("no space \n");
                        break;
                    case RB_ERR_W_CATCH_R:
                        printf("w_idx catch r_idx \n");
                        break;
                    case RB_ERR_INVALID_PARAM:
                        printf("invalid param\n");
                        break;
                    default:
                        printf("unknown error %d \n", rb_err);
                        break;
                }

                Sleep(1);
            }

            Sleep(5);
        }
    } while(0);

    if( fin )   fclose(fin);
    pthread_exit(0);
    return 0;
}

int main()
{
    int             is_running = 1;
    pthread_t       td_r, td_w;
//    unsigned char   buf[2048] = {0};

    srand(time(0));

    rb_opt_init(&g_hRB, g_rbuf, CONFIG_MAX_BUF_SIZE);

    pthread_create(&td_r, 0, _task_read, &is_running);
    pthread_create(&td_w, 0, _task_write, &is_running);

    while( g_eof == 0 )
        Sleep(5);

    is_running = 0;
    pthread_join(td_r, 0);
    pthread_join(td_w, 0);

    system("pause");

    return 0;
}
