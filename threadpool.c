#include <stdio.h>
#include <stdlib.h>
#include "threadpool.h"

threadpool* create_thredpool(int num_threads_in_pool)
{
    if(num_threads_in_pool>MAXT_IN_POOL||num_threads_in_pool<1)
    {
        printf("number of threads ilegal");
        return NULL;
    }
    threadpool *tpool = (threadpool*)malloc(sizeof(threadpool));
    tpool->num_threads=num_threads_in_pool;
    tpool->qsize=0;
    tpool->threads=(pthread_t*)malloc(sizeof(pthread_t)*tpool->num_threads);
    tpool->qtail=NULL;
    tpool->qhead=NULL;
    tpool->shutdown=0;
    tpool->dont_accept=0;
    pthread_cond_init(&(tpool->q_empty),NULL);
    pthread_cond_init(&(tpool->q_not_empty),NULL);
    pthread_mutex_init(&(tpool->qlock),NULL);

    for(int i=0;i<tpool->num_threads;i++)
    {
        if(pthread_create(&(tpool->threads[i]),NULL,do_work,tpool))
        {
            perror("error initializing thread\n");
            pthread_mutex_destroy(&(tpool->qlock));
            pthread_cond_destroy(&(tpool->q_empty));
            pthread_cond_destroy(&(tpool->q_not_empty));
            free(tpool->threads);
            free(tpool);
            return NULL;
        }
    }
    return tpool;

}

void* do_work(void* p)
{
threadpool *tpool=(threadpool*)p;
    while(1)
    {
        pthread_mutex_lock(&(tpool->qlock));

        if(tpool->shutdown==1)
        {
         pthread_mutex_unlock(&(tpool->qlock));
         return NULL;
        }

       while(!tpool->qsize)
       {
           pthread_cond_wait(&(tpool->q_not_empty),&(tpool->qlock));
            if(tpool->shutdown)
            {
                pthread_mutex_unlock(&(tpool->qlock));
                return NULL;
            }
       }
    }
}

void destroy_threadpool(threadpool* destroyme)
{
    void *curr;

    destroyme->dont_accept=1;

    while(destroyme->qsize)
    {
        pthread_cond_wait(&(destroyme->q_not_empty),&(destroyme->qlock));
        destroyme->shutdown=1;

        pthread_cond_broadcast(&(destroyme->q_not_empty));

        pthread_mutex_unlock(&(destroyme->qlock));


        for (int i = 0; i <destroyme->num_threads ; ++i)
        {
        pthread_join(destroyme->threads[i],&curr);
        }

        pthread_mutex_destroy(&(destroyme->qlock));
        pthread_cond_destroy(&(destroyme->q_empty));
        pthread_cond_destroy(&(destroyme->q_not_empty));
        free(destroyme->threads);
        free(destroyme);
    }


}

int main() {
    printf("Hello World!\n");
    return 0;
}

