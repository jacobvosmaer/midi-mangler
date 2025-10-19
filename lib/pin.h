#ifndef PIN_H
#define PIN_H

#include <stdint.h>

typedef struct pin {
	volatile uint8_t *const port;
	volatile uint8_t *const ddr;
	const uint8_t bit;
} pin;

void pin_init(pin *p);
void pin_on(pin *p);
void pin_off(pin *p);
void pin_set(pin *p, uint8_t x);

#endif
