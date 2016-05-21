#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef int bool;
#define false 0
#define true 1

typedef int queue_inner_t;

typedef struct {
    queue_inner_t* buffer;
    int capacity;
    int front;
    int back; // Also add necessary mutexes
    bool empty;
} queue_t;

int init_queue(queue_t * queue, int capacity) {
    queue->buffer = malloc((size_t)(capacity * sizeof(queue_inner_t)));
    if (queue->buffer == NULL) {
        return -1;
    }
    queue->capacity = capacity;
    queue->front = 0;
    queue->back = 0;
    queue->empty = true;
    return 0;
}

int delete_queue(queue_t * queue) {
    free(queue->buffer);
    return 0;
}

int push(queue_t * queue, queue_inner_t item) {
    /* Returns: 0  if push successful
     *          -1 if queue is full
     */
    if ((queue->front == queue->back) && !queue->empty) {
        printf("Push failed!\n");
        return -1;
    }
    int position = queue->back; 
    
    printf("Pushing %d to position %d\n", item, position);
    
    queue->buffer[position] = item;
    queue->back = (position+1) % queue->capacity;
    queue->empty = false;
    return 0;
}

int pop(queue_t * queue, queue_inner_t * item) {

    if ((queue->front == queue->back) && queue->empty) {
        printf("Pop failed!\n");
        return -1;
    }
    int position = queue->front;
    *item = queue->buffer[position];
    printf("Popping %d from position %d\n", (int)*item, position);
    queue->front = (position + 1) % queue->capacity;
    if (queue->front == queue->back) {
        queue->empty = true;
    }
    return 0;
}


int main(int argc, char *argv[])
{
    queue_t queue;
    init_queue(&queue, (queue_inner_t)4);

    push(&queue, (queue_inner_t)22);
    push(&queue, (queue_inner_t)27);
    push(&queue, (queue_inner_t)33);
    push(&queue, (queue_inner_t)49);
    push(&queue, (queue_inner_t)2);  // This should fail
    push(&queue, (queue_inner_t)2);  // This should fail

    queue_inner_t result;
    pop(&queue, &result);  // Should pop 22
    pop(&queue, &result);  // Should pop 27
    pop(&queue, &result);  // Should pop 33
    pop(&queue, &result);  // Should pop 49
    pop(&queue, &result);  // Should fail
    pop(&queue, &result);  // Should fail

    push(&queue, (queue_inner_t)10002);  // This should succeed!
    pop(&queue, &result);  // Should pop 10002
    pop(&queue, &result);  // Should fail

    return 0;
}
