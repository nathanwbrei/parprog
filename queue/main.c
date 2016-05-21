#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef int bool;
#define false 0
#define true 1

typedef int queue_inner_t;

typedef struct {
    queue_inner_t* buffer;
    pthread_mutex_t mutex;
    int capacity;
    int front;
    int back;
    bool empty;
} queue_t;

int init_queue(queue_t * queue, int capacity) {
    queue->buffer = malloc((size_t)(capacity * sizeof(queue_inner_t)));
    if (queue->buffer == NULL) {
        return -1;
    }
    pthread_mutex_init(&(queue->mutex), NULL);
    queue->capacity = capacity;
    queue->front = 0;
    queue->back = 0;
    queue->empty = true;
    return 0;
}

int delete_queue(queue_t * queue) {
    free(queue->buffer);
    pthread_mutex_destroy(&(queue->mutex));
    return 0;
}

int push(queue_t * queue, queue_inner_t item) {
    /* Returns: 0  if push successful
     *          -1 if queue is full
     */
    pthread_mutex_lock(&(queue->mutex));
    if ((queue->front == queue->back) && !queue->empty) {
        printf("Push failed!\n");
        pthread_mutex_unlock(&(queue->mutex));
        return -1;
    }
    int position = queue->back; 
    
    printf("Pushing %d to position %d\n", item, position);
    
    queue->buffer[position] = item;
    queue->back = (position+1) % queue->capacity;
    queue->empty = false;
    pthread_mutex_unlock(&(queue->mutex));
    return 0;
}

int pop(queue_t * queue, queue_inner_t * item) {

    pthread_mutex_lock(&(queue->mutex));
    if ((queue->front == queue->back) && queue->empty) {
        printf("Pop failed!\n");
        pthread_mutex_unlock(&(queue->mutex));
        return -1;
    }
    int position = queue->front;
    *item = queue->buffer[position];
    printf("Popping %d from position %d\n", (int)*item, position);
    queue->front = (position + 1) % queue->capacity;
    if (queue->front == queue->back) {
        queue->empty = true;
    }
    pthread_mutex_unlock(&(queue->mutex));
    return 0;
}


// The type signature for the worker function
typedef struct {
    int id;
    queue_t * queue;
    int * finalSum;
    pthread_mutex_t * globalStateMutex;
} worker_arg_t;
    

// Worker function terminates on empty queue for now
void * workerTask(void * workerArg) {
    
    worker_arg_t * arg = (worker_arg_t *) workerArg;
    queue_inner_t item;

    while(!arg->queue->empty) {

        if ( pop(arg->queue, &item) == 0) {
            printf("thread %d just popped %d\n", arg->id, (int) item);

            // Update global state
            pthread_mutex_lock(arg->globalStateMutex);
            *(arg->finalSum) += (int)item;
            printf("thread %d just updated sum= %d\n", arg->id, *(arg->finalSum) );
            pthread_mutex_unlock(arg->globalStateMutex);
        } else {
            printf("thread %d just failed at popping.\n", arg->id);
        }
    }
    return 0;
}

/*
 * Simple producer-consumer task.
 *
 * The main thread populates a queue with integers. 
 * It then launches some threads which negotiate for an item off the
 * queue, do some work on the item, and add the results to a global
 * sum. The global sum is protected by a mutex.
 * Main adds a bunch of items, then blocks until queue empties.
 * Workers terminate on empty queue, for now.
 */

int main(int argc, char *argv[])
{
    queue_t queue;
    init_queue(&queue, (queue_inner_t)4);

    push(&queue, (queue_inner_t)22);
    push(&queue, (queue_inner_t)27);
    push(&queue, (queue_inner_t)33);
    push(&queue, (queue_inner_t)49);

    int sum = 0;
   

    pthread_t threads[3];  // Handles to worker threads
    worker_arg_t args[3];  // Arguments to worker threads
    pthread_mutex_t globalStateMutex;  // Global state mutex
    pthread_mutex_init(&globalStateMutex, NULL);

    for (int i=0; i<3; i++) {
        args[i].id = i;
        args[i].queue = &queue;
        args[i].finalSum = &sum;
        args[i].globalStateMutex = &globalStateMutex;
        pthread_create(&threads[i], NULL, &workerTask, args+i);
    }


    for (int i=0; i<3; i++) {
        pthread_join(threads[i],NULL);
    } 
    pthread_mutex_destroy(&globalStateMutex);
    printf("Final sum: %d\n", sum);

    return 0;
}
