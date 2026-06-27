/* MIDI fun box */
#include "pinin.h"
#include "uart.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#define nelem(x) (int)(sizeof(x) / sizeof(*(x)))
#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))
#define prescale 64
struct {
	struct debouncer debouncer[2];
} encoder = {{DEBOUNCER(D, 0), DEBOUNCER(D, 1)}};
void encoder_init(void) {
	uint8_t i;
	for (i = 0; i < nelem(encoder.debouncer); i++)
		debouncer_init(encoder.debouncer + i);
}
int encoder_debounce(uint8_t delta) {
	uint16_t x[nelem(encoder.debouncer)] = {0};
	uint8_t i;
	if (delta)
		for (i = 0; i < nelem(encoder.debouncer); i++)
			x[i] = debouncer_update(encoder.debouncer + i);
	return (x[0] == 0x7fff && x[1] == 0) - (x[0] == 0x8000 && x[1] == 0);
}
/* Run Timer0 at F_CPU/1024 Hz, Timer1 at F_CPU/64 Hz */
void timer_init(void) { TCCR0B = 1 << CS02 | 1 << CS00; }
uint8_t outbox;
int bpm = 120;
int bpmticks(int n) { return (F_CPU / prescale / 2 * 5) / n; }
ISR(TIMER1_COMPA_vect) { outbox = 0xf8; }
int main(void) {
	uint8_t time = 0;
	uart_init();
	timer_init();
	encoder_init();
	TCCR1A = 0;
	TCCR1B = 1 << WGM12 | 1 << CS11 | 1 << CS10;
	TIMSK1 = 1 << OCIE1A;
	OCR1A = bpmticks(bpm);
	sei();
	while (1) {
		uint8_t delta = TCNT0 - time;
		int dir = encoder_debounce(delta);
		time += delta;
		if (dir) {
			bpm += dir;
			bpm = max(min(bpm, 30), 300);
			OCR1A = bpmticks(bpm);
		}
		if (outbox) {
			uint8_t x = outbox;
			outbox = 0;
			uart_tx(x);
		}
	}
}
