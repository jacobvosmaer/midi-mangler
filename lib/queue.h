#include <stdint.h>
struct queue {
	int buf[16];
	uint8_t head, tail;
};
void push(struct queue *q, int x);
int pop(struct queue *q, int *x);
