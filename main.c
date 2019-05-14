#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include "rbuf.h"

#include <windows.h>


#define err(str, ...)       do{ printf("[%s:%d] " str, __func__, __LINE__, ## __VA_ARGS__); while(1);}while(0)


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
            int             timeout = 0;
            int             rval = 0;
            unsigned char   data[200] = {0};
            unsigned long   len = rand() % sizeof(data);

            len = (len) ? len : 1;
            while( (rval = rbuf_pop(&g_rbuf, data, &len)) )
            {
                Sleep(1);
                if( g_eof && timeout++ > 1000 )
                    break;
            }

            if( len )
            {
                printf("pop %ld\n", len);
                fwrite((void*)data, 1, len, fout);
            }
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
//    char    *pFilename = "test.jpg";
    char    *pFilename = "minion.jpg";
    FILE    *fin = 0;

    do {
        if( !(fin = fopen(pFilename, "rb")) )
        {
            err("open %s fail \n", pFilename);
            break;
        }

        while( *pIs_running )
        {
            int             rval = 0;
            unsigned char   raw[200] = {0};
            int             len = rand() % sizeof(raw);

            len = (len) ? len : 1;
            len = fread(raw, 1, len, fin);
            if( len == 0 )
            {
                g_eof = 1;
                break;
            }

            while( (rval = rbuf_push(&g_rbuf, raw, len)) )
                Sleep(1);

            printf("push %d\n", len);
        }
    } while(0);

    if( fin )   fclose(fin);
    pthread_exit(0);
    return 0;
}

unsigned char   g_buf[256] = {0};

int main()
{
    int             is_running = 1;
    pthread_t       td_r, td_w;

    srand(time(0));

    rbuf_init(&g_rbuf, g_buf, sizeof(g_buf));

    pthread_create(&td_r, 0, _task_read, &is_running);
    pthread_create(&td_w, 0, _task_write, &is_running);

    while( g_eof == 0 )
        Sleep(5);

    is_running = 0;
    pthread_join(td_r, 0);
    pthread_join(td_w, 0);

    return 0;
}
