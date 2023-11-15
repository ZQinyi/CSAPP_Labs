#include "csapp.h"
#include "sbuf.h"

void sbuf_init(sbuf_t *sp, int n) {
    // sp->buf = Calloc(n, sizeof(int)); 
    sp->n = n;
    sp->front = sp->rear = 0;
    Sem_init(&sp->mutex, 0, 1);   // Binary semaphore for locking
    Sem_init(&sp->slots, 0, n);   // Initially, buf has n empty slots
    Sem_init(&sp->items, 0, 0);   // Initially, buf has zero data items
}

void sbuf_deinit(sbuf_t *sp) {
    //Free(sp->buf);
    sem_destroy(&sp->mutex);
    sem_destroy(&sp->slots);
    sem_destroy(&sp->items);
}

void sbuf_insert(sbuf_t *sp, int item) {
    P(&sp->slots);           // Wait for available slot
    P(&sp->mutex);           // Lock the buffer
    sp->buf[(++sp->rear)%(sp->n)] = item; // Insert the item
    V(&sp->mutex);           // Unlock the buffer
    V(&sp->items);           // Announce available item
}

int sbuf_remove(sbuf_t *sp) {
    P(&sp->items);           // Wait for available item
    P(&sp->mutex);           // Lock the buffer
    int item = sp->buf[(++sp->front)%(sp->n)]; // Remove the item
    V(&sp->mutex);           // Unlock the buffer
    V(&sp->slots);           // Announce available slot
    return item;
}