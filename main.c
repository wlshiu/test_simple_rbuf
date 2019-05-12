

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include <windows.h>
//=============================================================================
//                  Constant Definition
//=============================================================================

//=============================================================================
//                  Macro Definition
//=============================================================================
#define err(str, ...)       do{printf("[%s:%d] " str, __func__, __LINE__, ## __VA_ARGS__); while(1);}while(0)
//=============================================================================
//                  Structure Definition
//=============================================================================
typedef struct rbuf
{
    unsigned long   r_ptr;
    unsigned long   w_ptr;
    unsigned long   start_ptr;
    unsigned long   end_ptr;
} rbuf_t;
//=============================================================================
//                  Global Data Definition
//=============================================================================

//=============================================================================
//                  Private Function Definition
//=============================================================================
static int
rbuf_init(
    rbuf_t          *pHRbuf,
    unsigned long   start_ptr,
    unsigned long   buf_size)
{
    if( !pHRbuf )   return -1;

    pHRbuf->r_ptr     = start_ptr;
    pHRbuf->w_ptr     = start_ptr;
    pHRbuf->start_ptr = start_ptr;
    pHRbuf->end_ptr   = start_ptr + buf_size;
    return 0;
}

static int
rbuf_push(
    rbuf_t          *pHRbuf,
    unsigned long   pData,
    unsigned long   data_size)
{
    unsigned long    w_ptr = pHRbuf->w_ptr;

    if( (w_ptr + data_size) > pHRbuf->end_ptr )
    {
        int     remain = pHRbuf->end_ptr - w_ptr;

        if( remain > 0 )    memcpy((void*)w_ptr, (void*)pData, remain);

        pData += remain;
        memcpy((void*)pHRbuf->start_ptr, (void*)pData, data_size - remain);
        w_ptr = pHRbuf->start_ptr + data_size - remain;
    }
    else
    {
        memcpy((void*)w_ptr, (void*)pData, data_size);
        w_ptr += data_size;
    }
    pHRbuf->w_ptr = w_ptr;
    return 0;
}


static int
rbuf_pop(
    rbuf_t          *pHRbuf,
    unsigned long   *pData,
    unsigned long   *pData_size)
{
    unsigned long    w_ptr = pHRbuf->w_ptr;
    unsigned long    r_ptr = pHRbuf->r_ptr;
    // align writing index
    if( w_ptr < r_ptr )
    {
        *pData_size = pHRbuf->end_ptr - r_ptr;
        *pData      = r_ptr;

        r_ptr = pHRbuf->start_ptr;
    }
    else
    {
        *pData_size = w_ptr - r_ptr;
        *pData      = (*pData_size) ? r_ptr : 0;
        r_ptr += (*pData_size);
    }

    pHRbuf->r_ptr = r_ptr;
    return 0;
}
//=============================================================================
//                  Public Function Definition
//=============================================================================
static int      g_eof = 0;
static rbuf_t   g_rbuf = {0};

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
            unsigned long   data = 0;
            unsigned long   len = 0;

            rbuf_pop(&g_rbuf, &data, &len);
            if( data && len )
            {
                fwrite((void*)data, 1, len, fout);
            }

            Sleep(1);
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
            unsigned char   raw[200] = {0};
            int             len = sizeof(raw);

            len = fread(raw, 1, len, fin);
            if( len == 0 )
            {
                g_eof = 1;
                break;
            }

            rbuf_push(&g_rbuf, (unsigned long)raw, len);

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
    unsigned char   buf[1024] = {0};

    srand(time(0));

    rbuf_init(&g_rbuf, (unsigned long)buf, sizeof(buf));

    pthread_create(&td_r, 0, _task_read, &is_running);
    pthread_create(&td_w, 0, _task_write, &is_running);

    while( g_eof == 0 )
        Sleep(5);

    is_running = 0;
    pthread_join(td_r, 0);
    pthread_join(td_w, 0);

    return 0;
}
