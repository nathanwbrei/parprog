#include "emsim.h"
#include <pthread.h>

/*********************
 * Conveniences
 *********************/

typedef int bool;
#define false 0
#define true 1

/*********************
 * Nonblocking synchronized queue
 *********************/


typedef struct {
    team_t * team1;
    team_t * team2;
    bool final;
// Final-specific
    int gameNo;
    int numGames;
// Group-specific
    int groupNo;
    int i;
    int j;
} queue_inner_t;


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

/*********************
 * Convenience functions for condition variables
 *********************/

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

/*********************
 * Worker communication
 *********************/


typedef enum {STANDBY, PROCESS, TERMINATE} command_t;
typedef enum {IDLE, WORKING, FINISHED} response_t;

// The type signature for the worker function
typedef struct {
    // Specific to a single worker
    int id;
    response_t response;
    
    // Common to all workers
    queue_t * queue;
    team_t** teams;
    team_t** successors;
    pthread_mutex_t * globalStateMutex;  // Synchronizes access to teams and successors
    condvar_t * masterToWorker;
    condvar_t * workerToMaster;  // TODO: Use semaphore instead
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

// TODO: Want a semaphore instead

// Worker function terminates on empty queue for now
void * workerTask(void * workerArg) {
    
    worker_arg_t * arg = (worker_arg_t *) workerArg;
    queue_inner_t item;
    arg->response = IDLE;
    command_t receivedCommand = STANDBY;
    
    while(receivedCommand != TERMINATE) {
        
        pthread_mutex_lock(&arg->masterToWorker->mutex);
        pthread_cond_wait(&arg->masterToWorker->cond, &arg->masterToWorker->mutex);
        receivedCommand = *arg->command;
        pthread_mutex_unlock(&arg->masterToWorker->mutex);
        
        if (receivedCommand == STANDBY) {
            printf("WORKER %d going IDLE\n", arg->id);
            pthread_mutex_lock(&arg->workerToMaster->mutex);
            arg->response = IDLE;
            pthread_cond_signal(&arg->workerToMaster->cond);
            pthread_mutex_unlock(&arg->workerToMaster->mutex);
        }
        else if (receivedCommand == PROCESS) {
            arg->response = WORKING;
            while (pop(arg->queue, &item) == 0) {
                printf("WORKER %d processing some game\n", arg->id);
                int goals1, goals2;

                if (!item.final) {
                    
                    // Play group game (unsynchronized)
                    playGroupMatch(item.groupNo, item.team1, item.team2, &goals1, &goals2);

                    // Update global state (synchronized)
                    pthread_mutex_lock(arg->globalStateMutex);
                    
                    teams[item.i].goals += goals1 - goals2;
                    teams[item.j].goals += goals2 - goals1;
                    if (goals1 > goals2)
                        teams[item.i].points += 3;
                    else if (goals1 < goals2)
                        teams[item.j].points += 3;
                    else {
                        teams[item.i].points += 1;
                        teams[item.j].points += 1;
                    }

                    pthread_mutex_unlock(arg->globalStateMutex);

                    printf("WORKER %d played a group game\n");
                }
                else {
                    
                    // Play finals game (unsynchronized)
                    playFinalMatch(item.numGames, item.gameNo, item.team1, item.team2, &goals1, &goals2);
                    
                    // Update global state (synchronized)
                    pthread_mutex_lock(arg->globalStateMutex);
                    if (goals1 > goals2)
                        successors[item.gameNo] = team1;
                    else if (goals1 < goals2)
                        successors[item.gameNo] = team2;
                    else {
                        playPenalty(item.team1, item.team2, &goals1, &goals2);
                        if (goals1 > goals2)
                            successors[item.gameNo] = team1;
                        else
                            successors[item.gameNo] = team2;
                    }

                    pthread_mutex_unlock(arg->globalStateMutex);

                    printf("WORKER %d played a finals game\n");
                    
                }
            }
            
            printf("WORKER %d finished\n", arg->id);
            pthread_mutex_lock(&arg->workerToMaster->mutex);
            arg->response = FINISHED;
            pthread_cond_signal(&arg->workerToMaster->cond);
            pthread_mutex_unlock(&arg->workerToMaster->mutex);
        }
    }
    printf("WORKER %d terminated.\n", arg->id);
    return 0;
}


/*
 * Global variables
 */
static queue_t * queue;
static pthread_t * threads;
static worker_arg_t * args;
static pthread_mutex_t globalStateMutex;
static condvar_t workerToMaster;
static condvar_t masterToWorker;
static command_t command = STANDBY;


/*
 * Entry points
 */

void initializeThreads(int numThreads) {
    
    // TODO: malloc() queue, threads, args
    
    init_queue(queue, 20); // TODO: Malloc this
    pthread_mutex_init(&globalStateMutex, NULL); // TODO: Set as joinable or something
    condvar_init(&workerToMaster, true); // TODO: Better pred impl
    condvar_init(&masterToWorker, false);
    
    // Fire up workers
    for (int i=0; i<numThreads; i++) {
        args[i].id = i;
        args[i].queue = &queue;
        args[i].finalSum = &sum;
        args[i].globalStateMutex = &globalStateMutex;
        args[i].workerToMaster = &workerToMaster;
        args[i].masterToWorker = &masterToWorker;
        args[i].command = &command;
        pthread_create(&threads[i], NULL, &workerTask, args+i);
    }
}

void destroyThreads(int numThreads) {
    for (int i=0; i<NUMTHREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    pthread_mutex_destroy(&globalStateMutex);
    condvar_destroy(&masterToWorker);
    condvar_destroy(&workerToMaster);
    // Free queue, threads, args

}

void playGroups(team_t* teams, int numWorker)
{
    static const int cNumTeamsPerGroup = NUMTEAMS / NUMGROUPS;
    int g, i, j;
    
    // Populate queue
    for (g = 0; g < NUMGROUPS; ++g) {
        for (i =  g * cNumTeamsPerGroup; i < (g+1) * cNumTeamsPerGroup; ++i) {
            for (j = (g+1) * cNumTeamsPerGroup - 1; j > i; --j) {
                
                // team i plays against team j in group g
                queue_inner_t item;
                item.team1 = teams + i;
                item.team2 = teams + j;
                item.i = i;
                item.j = j;
                item.final = false;
                item.groupNo = g;
            
                push(queue, item);
            }
        }
    }
    
    // Announce populated queue
    pthread_mutex_lock(&masterToWorker.mutex);
    command = PROCESS;
    pthread_cond_broadcast(&masterToWorker.cond);
    printf("MASTER: Sent signal PROCESS to workers\n");
    pthread_mutex_unlock(&masterToWorker.mutex);
    
    // Block until queue emptied again
    pthread_mutex_lock(&workerToMaster.mutex);
    while (!allWorkersAck(FINISHED, args, NUMTHREADS)) {
        pthread_cond_wait(&workerToMaster.cond, &workerToMaster.mutex);
    }
    pthread_mutex_unlock(&workerToMaster.mutex);
    printf("MASTER: All workers report FINISHED.\n");

}


// TODO: Does this get called once, or multiple times?
void playFinalRound(int numGames, team_t** teams, team_t** successors, int numWorker) {
    team_t* team1;
    team_t* team2;
    int i, goals1 = 0, goals2 = 0;

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
        pthread_cond_wait(&workerToMaster.cond, &workerToMaster.mutex);
    }
    pthread_mutex_unlock(&workerToMaster.mutex);
    printf("MASTER: All workers report IDLE.\n");

    
    // Push necessary games onto queue
    for (i = 0; i < numGames; ++i) {

        queue_inner_t item;
        item.team1 = teams[i*2];
        item.team2 = teams[i*2+1];
        item.final = true;
        item.gameNo = i;
        item.numGames = numGames;
        
        push(queue, item);
    }
    
    // Announce populated queue
    pthread_mutex_lock(&masterToWorker.mutex);
    command = PROCESS;
    pthread_cond_broadcast(&masterToWorker.cond);
    printf("MASTER: Sent signal PROCESS to workers\n");
    pthread_mutex_unlock(&masterToWorker.mutex);
    
    // Block until queue emptied again
    pthread_mutex_lock(&workerToMaster.mutex);
    while (!allWorkersAck(FINISHED, args, NUMTHREADS)) {
        pthread_cond_wait(&workerToMaster.cond, &workerToMaster.mutex);
    }
    pthread_mutex_unlock(&workerToMaster.mutex);
    printf("MASTER: All workers report FINISHED.\n");

    
    // If this is the last game, shut everything down
    // Announce termination
    pthread_mutex_lock(&masterToWorker.mutex);
    command = TERMINATE;
    pthread_cond_broadcast(&masterToWorker.cond);
    pthread_mutex_unlock(&masterToWorker.mutex);
    
    destroyThreads(numThreads); // TODO: Does numThreads change? If so, fuck the people who designed this assignment


} 
