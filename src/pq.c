#include "pq.h"

/* check if a given priority has no processes */
int pq_is_priority_empty(const PQ* pq, const int priority) {
    /* return true if priority is out of bounds */
    if (priority < HIGH || priority > HIDDEN) return 1;
    return pq->front[priority] == NULL;
}

/* push a given process onto the priority queue */
void pq_push(PQ* pq, PCB* proc) {
    int priority = proc->m_priority;
    if (pq_is_priority_empty(pq, priority)) {
        /* if queue is empty, set both the front and back to proc */
        pq->front[priority] = proc;
        pq->back[priority] = proc;
    } else {
        /* if queue is not empty, add proc to the back of the queue */
        pq->back[priority]->mp_next = proc;
        pq->back[priority] = proc;
    }
}

/* push a given process onto the front of the priority queue */
void pq_push_front(PQ* pq, PCB* proc) {
    int priority = proc->m_priority;
    if (pq_is_priority_empty(pq, priority)) {
        /* if queue is empty, set both the front and back to proc */
        pq->front[priority] = proc;
        pq->back[priority] = proc;
    } else {
        /* if queue is not empty, add proc to the front of the queue */
        proc->mp_next = pq->front[priority];
        pq->front[priority] = proc;
    }
}

/* get the next process of a given priority. Only used internally */
PCB* pq_pop_front(PQ* pq, const int priority) {
    PCB* front_proc;

    /* if our queue is empty, return a NULL pointer */
    if (pq_is_priority_empty(pq, priority)) return NULL;

    front_proc = pq->front[priority];
    pq->front[priority] = front_proc->mp_next;

    /* if queue only had 1 proc, it is now empty */
    if (pq->front[priority] == NULL) pq->back[priority] = NULL;

    front_proc->mp_next = NULL;
    return front_proc;
}

/* pop a specific process */
PCB* pq_pop_PCB(PQ* pq, const PCB* proc) {
    PCB* found_proc;
    PCB* temp_proc = pq->front[proc->m_priority];

    /* proc is at the front of the queue */
    if (temp_proc == proc) return pq_pop_front(pq, proc->m_priority);

    /* traverse our queue till we find proc */
    while (temp_proc != NULL && temp_proc->mp_next != NULL && temp_proc->mp_next != proc) {
        temp_proc = temp_proc->mp_next;
    }

    /* proc does not exist */
    if (temp_proc->mp_next == NULL) return NULL;

    /* remove and return temp_proc->mp_next */
    found_proc = temp_proc->mp_next;
    temp_proc->mp_next = found_proc->mp_next;
    found_proc->mp_next = NULL;
    return found_proc;
}

/* pop the first, highest-priority process */
PCB* pq_pop(PQ* pq) {
    int priority;
    PCB* proc = NULL;

    for (priority = HIGH; priority <= HIDDEN; priority++) {
        proc = pq->front[priority];
        if (proc != NULL) return pq_pop_front(pq, priority);
    }

    return NULL; // impossible - should return NULL process first
}
