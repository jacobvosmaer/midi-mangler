#ifndef PININ_H
#define PININ_H
#include <stdint.h>
struct pinin {
	volatile uint8_t *const port, *const ddr, *const pin;
	uint8_t mask;
};
#define PININ(port, n) {&PORT##port, &DDR##port, &PIN##port, 1 << PORT##port##n}
void pinin_init(struct pinin *p);
uint8_t pinin_read(struct pinin *p);
struct debouncer {
	struct pinin pin;
	uint16_t history;
};
#define DEBOUNCER(port, n) {PININ(port, n), 0}
void debouncer_init(struct debouncer *db);
uint16_t debouncer_update(struct debouncer *db);
struct encoder {
	struct debouncer debouncer[2];
};
#define ENCODER(port1, n1, port2, n2)                                          \
	{                                                                      \
		{                                                              \
			DEBOUNCER(port1, n1), DEBOUNCER(port2, n2)             \
		}                                                              \
	}
void encoder_init(struct encoder *enc);
int encoder_debounce(struct encoder *enc, uint8_t delta);
#endif
