#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

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
    queue->front = (position + 1) % queue->capacity;
    if (queue->front == queue->back) {
        queue->empty = true;
    }
    pthread_mutex_unlock(&(queue->mutex));
    return 0;
}

/* Convenience functions for condition variables */

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool pred;
} condvar_t;

int condvar_init(condvar_t * condvar, bool pred) {
    //TODO: Handle errors
    pthread_mutex_init(&condvar->mutex, NULL);
    pthread_cond_init(&condvar->cond, NULL);
    condvar->pred = pred;
    return 0;
}

int condvar_destroy(condvar_t * condvar) {
    //TODO: Handle errors
    pthread_mutex_destroy(&condvar->mutex);
    pthread_cond_destroy(&condvar->cond);
    return 0;
}

typedef enum {STANDBY, PROCESS, TERMINATE} command_t;
typedef enum {IDLE, WORKING, FINISHED} response_t;

// The type signature for the worker function
typedef struct {
    // Specific to a single worker
    int id;
    response_t response;

    // Common to all workers
    queue_t * queue;
    int * finalSum;
    pthread_mutex_t * globalStateMutex;  // Synchronizes access to big ugly global state variable
    condvar_t * masterToWorker;
    condvar_t * workerToMaster;
    command_t * command;
} worker_arg_t;
  
// Convenience function to check that all workers have finished before
// proceeding. Want to attach this to a condvar, rather than polling
bool allWorkersAck(response_t ack, worker_arg_t * workerArgs, int numWorkers) {
    bool finished = true;
    for (int i=0; i<numWorkers; i++) {
        finished &= (workerArgs[i].response == ack);
    }
    return finished;
}

// Worker function terminates on empty queue for now
void * workerTask(void * workerArg) {
    
    worker_arg_t * arg = (worker_arg_t *) workerArg;
    queue_inner_t item;
    arg->response = IDLE;
    command_t receivedCommand = STANDBY;

    while(receivedCommand != TERMINATE) {

        printf("WORKER %d waiting for signal.\n", arg->id);
        pthread_mutex_lock(&arg->masterToWorker->mutex);
        pthread_cond_wait(&arg->masterToWorker->cond, &arg->masterToWorker->mutex);
        receivedCommand = *arg->command;
        printf("WORKER %d received signal %d!!!\n", arg->id, receivedCommand);
        pthread_mutex_unlock(&arg->masterToWorker->mutex);
        if (receivedCommand == STANDBY) {
            printf("WORKER %d signaling IDLE\n", arg->id);
            pthread_mutex_lock(&arg->workerToMaster->mutex);
            arg->response = IDLE;
            pthread_cond_signal(&arg->workerToMaster->cond);
            pthread_mutex_unlock(&arg->workerToMaster->mutex);
        }
        else if (receivedCommand == PROCESS) {
            arg->response = WORKING;
            while (pop(arg->queue, &item) == 0) {
                printf("WORKER %d processing %d\n", arg->id, (int) item);

                usleep(100000);
                // Update global state
                pthread_mutex_lock(arg->globalStateMutex);
                *(arg->finalSum) += (int)item;
                printf("WORKER %d updated sum= %d\n", arg->id, *(arg->finalSum) );
                pthread_mutex_unlock(arg->globalStateMutex);
            } 

            printf("WORKER %d signaling FINISHED\n", arg->id);
            pthread_mutex_lock(&arg->workerToMaster->mutex);
            arg->response = FINISHED;
            pthread_cond_signal(&arg->workerToMaster->cond);
            pthread_mutex_unlock(&arg->workerToMaster->mutex);
        }
    }
    printf("WORKER %d terminated.\n", arg->id);
    return 0;
}

#define NUMTHREADS 7
int main(int argc, char *argv[])
{
    queue_t queue;
    init_queue(&queue, 20);

    command_t command = STANDBY;

    int sum = 0;
    pthread_mutex_t globalStateMutex;  // Global state mutex
    pthread_mutex_init(&globalStateMutex, NULL);

    condvar_t workerToMaster;
    condvar_init(&workerToMaster, true);
    condvar_t masterToWorker;
    condvar_init(&masterToWorker, false);

    pthread_t threads[NUMTHREADS];  // Handles to worker threads
    worker_arg_t args[NUMTHREADS];  // Arguments to worker threads

    // Fire up workers
    for (int i=0; i<NUMTHREADS; i++) {
        args[i].id = i;
        args[i].queue = &queue;
        args[i].finalSum = &sum;
        args[i].globalStateMutex = &globalStateMutex;
        args[i].workerToMaster = &workerToMaster;
        args[i].masterToWorker = &masterToWorker;
        args[i].command = &command;
        pthread_create(&threads[i], NULL, &workerTask, args+i);
    }

    // Populate queue
    push(&queue, (queue_inner_t) 22);
    push(&queue, (queue_inner_t) 27);
    push(&queue, (queue_inner_t) 33);
    push(&queue, (queue_inner_t) 49);

    // Announce populated queue
    pthread_mutex_lock(&masterToWorker.mutex);
    command = PROCESS;
    pthread_cond_broadcast(&masterToWorker.cond);
    printf("MASTER: Sent signal PROCESS to workers\n");
    pthread_mutex_unlock(&masterToWorker.mutex);

    printf("MASTER: Waiting for responses from workers\n");
    // Block until queue emptied again
    pthread_mutex_lock(&workerToMaster.mutex);
    while (!allWorkersAck(FINISHED, args, NUMTHREADS)) {
        printf("MASTER: About to cond_wait\n");
        pthread_cond_wait(&workerToMaster.cond, &workerToMaster.mutex);
        printf("MASTER: Done with cond_wait\n");
        printf("MASTER: Received signal from worker.\n");
    }
    pthread_mutex_unlock(&workerToMaster.mutex);
    printf("MASTER: All workers report FINISHED.\n");

    // Announce Reset
    pthread_mutex_lock(&masterToWorker.mutex);
    command = STANDBY;
    pthread_cond_broadcast(&masterToWorker.cond);
    printf("MASTER: Sent signal STANDBY to workers\n");
    pthread_mutex_unlock(&masterToWorker.mutex);

    printf("MASTER: Waiting for responses from workers\n");
    // Block until workers ACK 
    pthread_mutex_lock(&workerToMaster.mutex);
    while (!allWorkersAck(IDLE, args, NUMTHREADS)) {
        printf("MASTER: About to cond_wait\n");
        pthread_cond_wait(&workerToMaster.cond, &workerToMaster.mutex);
        printf("MASTER: Done wiht cond_wait\n");
        printf("MASTER: Received signal from worker.\n");
    }
    pthread_mutex_unlock(&workerToMaster.mutex);
    printf("MASTER: All workers report IDLE.\n");
    // Populate queue
    push(&queue, (queue_inner_t) 1400);
    push(&queue, (queue_inner_t) 2300);
    push(&queue, (queue_inner_t) 3200);
    push(&queue, (queue_inner_t) 4100);
    
    // Announce populated queue
    pthread_mutex_lock(&masterToWorker.mutex);
    command = PROCESS;
    pthread_cond_broadcast(&masterToWorker.cond);
    printf("MASTER: Sent signal PROCESS to workers\n");
    pthread_mutex_unlock(&masterToWorker.mutex);

    printf("MASTER: Waiting for responses from workers\n");
    // Block until queue emptied again
    pthread_mutex_lock(&workerToMaster.mutex);
    while (!allWorkersAck(FINISHED, args, NUMTHREADS)) {
        pthread_cond_wait(&workerToMaster.cond, &workerToMaster.mutex);
        printf("MASTER: Received signal from worker.\n");
    }
    pthread_mutex_unlock(&workerToMaster.mutex);
    printf("MASTER: All workers report FINISHED.\n");

    // Announce termination
    pthread_mutex_lock(&masterToWorker.mutex);
    command = TERMINATE;
    pthread_cond_broadcast(&masterToWorker.cond);
    pthread_mutex_unlock(&masterToWorker.mutex);

    for (int i=0; i<NUMTHREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&globalStateMutex);
    condvar_destroy(&masterToWorker);
    condvar_destroy(&workerToMaster);

    printf("Final sum: %d\n", sum);

    return 0;
}



