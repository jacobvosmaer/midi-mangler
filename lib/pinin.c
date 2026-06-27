/* pinin: GPIO input pin abstraction */
#include "pinin.h"
#define nelem(x) (int)(sizeof(x) / sizeof(*(x)))
void pinin_init(struct pinin *p) {
	*(p->ddr) &= ~p->mask;
	*(p->port) |= p->mask;
}
uint8_t pinin_read(struct pinin *p) { return !!(*(p->pin) & p->mask); }
void debouncer_init(struct debouncer *db) { pinin_init(&db->pin); }
uint16_t debouncer_update(struct debouncer *db) {
	return db->history = (db->history << 1) | pinin_read(&db->pin);
}
void encoder_init(struct encoder *enc) {
	uint8_t i;
	for (i = 0; i < nelem(enc->debouncer); i++)
		debouncer_init(enc->debouncer + i);
}
int encoder_debounce(struct encoder *enc, uint8_t delta) {
	uint16_t x[nelem(enc->debouncer)] = {0};
	uint8_t i;
	if (delta)
		for (i = 0; i < nelem(enc->debouncer); i++)
			x[i] = debouncer_update(enc->debouncer + i);
	return (x[0] == 0x7fff && x[1] == 0) - (x[0] == 0x8000 && x[1] == 0);
}
