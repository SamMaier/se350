#ifndef PQ_H
#define PQ_H

#include "k_rtx.h"

typedef struct {
    PCB* front[NUM_PRIORITIES];
    PCB* back[NUM_PRIORITIES];
} PQ;

int pq_is_priority_empty(const PQ* pq, const int priority);
void pq_push(PQ* pq, PCB* proc);
void pq_push_front(PQ* pq, PCB* proc);
PCB* pq_pop_front(PQ* pq, const int priority);
PCB* pq_pop_PCB(PQ* pq, const PCB* proc);
PCB* pq_pop(PQ* pq);

#endif
