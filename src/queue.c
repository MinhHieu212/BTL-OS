#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
        if (q == NULL) return 1;
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
        /* TODO: put a new process to queue [q] */

        // BEGIN MY CODE
        if((q->size) == MAX_QUEUE_SIZE) return;
        (q->proc)[q->size] = proc;
        (q->size)++;
        // END MY CODE
}

struct pcb_t * dequeue(struct queue_t * q) {
        /* TODO: return a pcb whose prioprity is the highest
         * in the queue [q] and remember to remove it from q
         * */

        // BEGIN MY CODE
        struct pcb_t * proc = q->proc[0];   // take a pcb whose prioprity is the highest.
        int i;
        for(i = 0; i < q->size - 1 ; i++) {
                (q->proc)[i] = (q->proc)[i + 1];
        }
        (q->proc)[i] = NULL;   // remoive it from queue.
        (q->size)--;
        // END MY CODE
        
	return proc;
}

