#include <stdio.h>
#include <stdlib.h>
#include "threadpool.h"

threadpool* create_threadpool(int num_threads_in_pool)
{
    if(num_threads_in_pool>MAXT_IN_POOL||num_threads_in_pool<1)
    {
        perror("number of threads ilegal");
        exit(1);
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
        if(pthread_create(&(tpool->threads[i]),NULL,&do_work,tpool)) // If the system couldnt init the spesific process
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

        pthread_mutex_lock(&(tpool->qlock)); //lock mutex

        if(tpool->shutdown==1)
        {
            pthread_mutex_unlock(&(tpool->qlock));
            return NULL;
        }
//
//    while(!tpool->qsize) {
//        if(tpool->shutdown==1)
//        {
//            pthread_mutex_unlock(&(tpool->qlock));
//            return NULL;
//        }
        // pthread_mutex_unlock(&(tpool->qlock));
        if(tpool->qsize==0)
            pthread_cond_wait(&(tpool->q_not_empty), &(tpool->qlock));
        if (tpool->shutdown == 1) {
            pthread_mutex_unlock(&(tpool->qlock));
            return NULL;
        }
       // printf("IM HERE AT 69 DEADPOOL");
//}     // After wake up and locking mutex again start to pull work from queue.
        work_t *first=tpool->qhead;

        tpool->qsize--;
        if(tpool->qsize==0)
        {
            tpool->qtail=NULL;
        }

        tpool->qhead=first->next;

        pthread_mutex_unlock(&(tpool->qlock));
        (first->routine)(first->arg);
        //free(first);

        if(tpool->qsize==0 && tpool->dont_accept==1)
        {
            pthread_cond_signal(&(tpool->q_empty)); //TODO when this happens the main need to stop gettin new requests.
        }
//pthread_mutex_unlock(&(tpool->qlock));

    }

}

/**
* dispatch enter a "job" of type work_t into the queue.
* when an available thread takes a job from the queue, it will
* call the function "dispatch_to_here" with argument "arg".
* this function should:
* 1. create and init work_t element
* 2. lock the mutex
* 3. add the work_t element to the queue
* 4. unlock mutex
*
*/
void dispatch(threadpool* from_me, dispatch_fn dispatch_to_here, void *arg)
{

    work_t *work=(work_t*)malloc(sizeof(work_t));

    work->arg=arg;
    work->routine=dispatch_to_here;
    work->next=NULL;

   // pthread_mutex_lock(&(from_me->qlock)); // Lock so no can do any thing to the queue when we add new job.
    if(from_me->dont_accept==1)
    {
        free(work);
        return;
    }


    if(from_me->qsize==0)
    {
        from_me->qhead=work;
    }
    else
    {
        from_me->qtail->next=work;
    }

    from_me->qtail=work;

    from_me->qsize++;
  //  pthread_mutex_unlock(&(from_me->qlock));
    pthread_cond_signal(&(from_me->q_not_empty));
}


void destroy_threadpool(threadpool* destroyme)
{
    void *curr;

    destroyme->dont_accept=1;

    while(destroyme->qsize) { // TODO: Need to check if the condition wait waits for all the threads to finish.
        pthread_cond_wait(&(destroyme->q_empty), &(destroyme->qlock));
    }

    destroyme->shutdown = 1;

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
    return;
}

int func(void* toPrint1)
{
    int* toPrint = (int*)toPrint1;
    printf("Thread: %lu, print --> %d\n", pthread_self(), *toPrint+1);
    return 1;
}

/*int main(int argc, char const *argv[])
{

if (argc < 3)
{
printf("put 2 arguments!\n");
exit(1);
}

int jobs =atoi(argv[1]);
int threads =atoi(argv[2]);
int* arr = (int*)malloc(jobs*sizeof(int));
for (int i = 0; i < jobs; i++)
arr[i] = i;

threadpool* tp = create_threadpool(threads);
for (int i = 0; i < jobs; i++)
{
dispatch(tp, func, &arr[i]);

}

destroy_threadpool(tp);
free(arr);
return (0);
}*/