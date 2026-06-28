#include "queue.h"
#define nelem(x) (int)(sizeof(x) / sizeof(*(x)))
#define VOL(type, addr) *(volatile type *)(addr)
void push(struct queue *q, int x) {
	VOL(int, q->buf + (q->head % nelem(q->buf))) = x;
	VOL(uint8_t, &q->head) = q->head + 1;
}
int pop(struct queue *q, int *x) {
	int notempty = VOL(uint8_t, &q->head) != q->tail;
	*x = VOL(int, q->buf + (q->tail % nelem(q->buf)));
	q->tail += notempty;
	return notempty;
}
