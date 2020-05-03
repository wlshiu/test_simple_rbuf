#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>

#include "pthread.h"
#include "rbuf_opt.h"

#define _FOURCC(a, b, c, d)         (((d) << 24) | ((c) <<16) | ((b) << 8) | (a))
#define err(st, args...)            do{printf("%s[%d] " st, __func__, __LINE__, ##args); while(1);}while(0)

#define MAX_BUF_SIZE            (10 <<10)

#define CMD_EOS                 _FOURCC('E', 'O', 'S', ' ')

static unsigned char    g_rbuf[MAX_BUF_SIZE] = {0};
static rb_operator_t    g_hRB = {0};

static void*
_rb_write(void *argv)
{
    int             sleep_ms = 1;
    FILE            *fin = 0;
    unsigned char   buf[512] = {0};

    if( !(fin = fopen("./test.jpg", "rb")) )
    {
        err("open '%s' fail\n", "./test.jpg");
        return 0;
    }

    while(1)
    {
        rb_err_t        rb_err = RB_ERR_OK;
        int             data_size = (rand() >> 10) & 0x1FF;

        if( fin && feof(fin) )
        {
            fclose(fin);
            fin = 0;

            memset(buf, 0xFF, sizeof(buf));
            rb_opt_update_w(&g_hRB, buf, 8); // It MUST send more than 8 bytes data
        }

        if( fin )
        {
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
        }

        sleep_ms = (rand() >> 15) & 0xF;
        sleep_ms = (sleep_ms) ? sleep_ms : 1;
//        Sleep(sleep_ms);
    }

    pthread_exit(0);
    return NULL;
}

static int
_get_data_size(
    rb_data_info_t  *pInfo)
{
    static int      end_flag = 0;
    int             rval = 0;
    unsigned char   *pTmp = pInfo->r_ptr + (pInfo->data_size - 4);

    if( end_flag && pInfo->read_idx == RB_READ_TYPE_FETCH )
    {
        end_flag = 0;
        pInfo->is_dummy = 1;
        return CMD_EOS;
    }

    if( pTmp[0] == 0xFF && pTmp[1] == 0xFF && pTmp[2] == 0xFF && pTmp[3] == 0xFF )
    {
        pInfo->data_size = pInfo->data_size - 4;
        end_flag = 1;
    }

    pInfo->data_size &= ~(0x3);
    printf("data size = %d\n", pInfo->data_size);
    pInfo->is_dummy = 0;
    return rval;
}

static void*
_rb_read(void *argv)
{
    int             sleep_ms = 1;
    FILE            *fout = 0;

    if( !(fout = fopen("dump.jpg", "wb")) )
    {
        err("open '%s' fail \n", "dump.jpg");
        return 0;
    }

    while(1)
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
            case CMD_EOS:
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


        sleep_ms = (rand() >> 15) & 0xF;
        sleep_ms = (sleep_ms) ? sleep_ms : 1;
        Sleep(2);

        data_size = 0;
        rb_opt_update_r(&g_hRB, RB_READ_TYPE_REMOVE, &pData, &data_size, _get_data_size);
    }

    pthread_exit(0);
    return NULL;
}


int main()
{
    pthread_t   t1, t2;

    rb_opt_init(&g_hRB, g_rbuf, MAX_BUF_SIZE);

    srand(time(NULL));

    pthread_create(&t1, 0, _rb_write, 0);
    pthread_create(&t2, 0, _rb_read, 0);

    while(1)        Sleep(1000);

    return 0;
}
