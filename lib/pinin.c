/* pinin: GPIO input pin abstraction */
#include "pinin.h"
#include <avr/io.h>
void pinin_init(struct pinin *p) {
	*(p->ddr) &= ~p->mask;
	*(p->port) |= p->mask;
}
uint8_t pinin_read(struct pinin *p) { return !!(*(p->pin) & p->mask); }
void debouncer_init(struct debouncer *db) { pinin_init(&db->pin); }
uint16_t debouncer_update(struct debouncer *db) {
	return db->history = (db->history << 1) | pinin_read(&db->pin);
}
