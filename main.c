
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include "rbuf.h"

#include <unistd.h>


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

static int
_compare(char *pIn_a, char *pIn_b)
{
    FILE    *fin = 0, *fin_1 = 0;
    long    file_size = 0;
    char    *pBuf_1 = 0, *pBuf_2 = 0;
    if( !(fin = fopen(pIn_a, "rb")) )
    {
        err("open %s fail \n", pIn_a);
    }

    fseek(fin, 0, SEEK_END);
    file_size = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    if( !(fin_1 = fopen(pIn_b, "rb")) )
    {
        err("open %s fail \n", pIn_b);
    }

    if( !(pBuf_1 = malloc(file_size)) )
    {
        err("malloc fail !\n");
    }

    if( !(pBuf_2 = malloc(file_size)) )
    {
        err("malloc fail !\n");
    }

    fread(pBuf_1, 1, file_size, fin);
    fread(pBuf_2, 1, file_size, fin_1);

    if( memcmp(pBuf_1, pBuf_2, file_size) != 0 )
    {
        err("not match \n");
    }

    if( pBuf_1 )    free(pBuf_1);
    if( pBuf_2 )    free(pBuf_2);

    if( fin )       fclose(fin);
    if( fin_1 )     fclose(fin_1);
    return 0;
}

unsigned char   g_buf[256] = {0};

int main()
{
    int             is_running = 1;
    pthread_t       td_r, td_w;

    srand(time(0));

    rbuf_init(&g_rbuf, g_buf, sizeof(g_buf));

#if 1
    if( 0 )
    {
        FILE    *fout = 0;
        char    *pBuf = 0;
        int     len = 128 << 10;
        if( !(fout = fopen("test_patt.bin", "wb")) )
        {
            err("open %s fail \n", "test_patt.bin");
        }

        if( !(pBuf = malloc(len)) )
        {
            err("malloc fail !\n");
        }
        for(int i = 0; i < len; ++i)
            pBuf[i] = i + 1;

        fwrite(pBuf, 1, len, fout);
        if( pBuf )  free(pBuf);
        if( fout )  fclose(fout);
        return;
    }

    do {
        FILE    *fin = 0, *fout = 0;
        char    *pFilename = 0;

        pFilename = "dump.jpg";
        if( !(fout = fopen(pFilename, "wb")) )
        {
            err("open %s fail \n", pFilename);
        }

//        pFilename = "test_patt.bin";
        pFilename = "minion.jpg";
        if( !(fin = fopen(pFilename, "rb")) )
        {
            err("open %s fail \n", pFilename);
        }

        while( 1 )
        {
            if( rand() & 0x1 )
            {
                // push
                static int             rval = 0;
                static unsigned char   raw[20] = {0};
                static int             len = 0;

                if( g_eof )     continue;

                if( rval == 0 )
                {
                    if( len == 0 )      len = rand() % sizeof(raw);

                    len = (len) ? len : 1;
                    len = fread(raw, 1, len, fin);
                    if( len == 0 )
                    {
                        g_eof = 1;
                    }
                }

                rval = rbuf_push(&g_rbuf, raw, len);
                if( !rval )     printf("push %d\n", len);
            }
            else
            {
                // pop
                static int      timeout = 0;
                int             rval = 0;
                unsigned char   data[20] = {0};
                unsigned long   len = rand() % sizeof(data);

                len = (len) ? len : 1;
                rval = rbuf_pop(&g_rbuf, data, &len);
                if( rval )
                {
                    if( g_eof && timeout++ > 10000 )
                        break;
                }

                if( len )
                {
                    printf("pop %ld\n", len);
                    fwrite((void*)data, 1, len, fout);
                    timeout = 0;
                }
            }
        }
        if( fout )      fclose(fout);
        if( fin )       fclose(fin);

        rbuf_deinit(&g_rbuf);

        _compare(pFilename, "dump.jpg");
    } while(0);
#else

    pthread_create(&td_w, 0, _task_write, &is_running);
    pthread_create(&td_r, 0, _task_read, &is_running);

    while( g_eof == 0 )
    {
        Sleep(5);
    }

    is_running = 0;
    pthread_join(td_r, 0);
    pthread_join(td_w, 0);
#endif // 1
    return 0;
}
