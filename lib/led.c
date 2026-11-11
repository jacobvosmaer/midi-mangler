/* Sparkfun Pro Micro LED's */
#include "led.h"
#include <avr/io.h>
#define LED(name, port, bit)                                                   \
	void led##name(uint8_t on) {                                           \
		enum { mask = 1 << (bit) };                                    \
		DDR##port |= mask;                                             \
		if (on)                                                        \
			PORT##port &= ~mask;                                   \
		else                                                           \
			PORT##port |= mask;                                    \
	}
LED(green, D, 5)
LED(yellow, B, 0)
void ledinit(void) {
	ledgreen(0);
	ledyellow(0);
}
