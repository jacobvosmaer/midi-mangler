#include "pin.h"

void pin_init(pin *p) { *(p->ddr) |= 1 << (p->bit); }
void pin_on(pin *p) { *(p->port) |= 1 << (p->bit); }
void pin_off(pin *p) { *(p->port) &= ~(1 << (p->bit)); }
void pin_set(pin *p, uint8_t x) {
	if (x)
		pin_on(p);
	else
		pin_off(p);
}
