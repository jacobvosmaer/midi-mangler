/* MIDI fun box */
#include "pinin.h"
#include "uart.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#define nelem(x) (int)(sizeof(x) / sizeof(*(x)))
#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))
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
struct queue {
	uint8_t buf[16];
	uint8_t head, tail;
};
void push(struct queue *q, uint8_t x) { q->buf[q->head++ % nelem(q->buf)] = x; }
uint8_t pop(struct queue *q, uint8_t *x) {
	if (q->head == q->tail)
		return 0;
	*x = q->buf[q->tail++ % nelem(q->buf)];
	return 1;
}
struct queue clock, midi;
#define prescale 64
/* MIDI clock is 24 PPQN. the formula below calculates the number of prescaled
 * timer ticks that corresponds to 1 MIDI clock pulse for the given tempo in BPM
 */
int bpmticks(int n) { return (F_CPU / prescale / 2 * 5) / n; }
#define MIDI_CLOCK 0xf8
ISR(TIMER1_COMPA_vect) { push(&clock, MIDI_CLOCK); }
int main(void) {
	uint8_t time = 0;
	int bpm = 120;
	uart_init();
	TCCR0B = 1 << CS02 | 1 << CS00;
	encoder_init();
	TCCR1A = 0;
	TCCR1B = 1 << WGM12 | 1 << CS11 | 1 << CS10;
	TIMSK1 = 1 << OCIE1A;
	OCR1A = bpmticks(bpm);
	sei();
	while (1) {
		uint8_t delta = TCNT0 - time, incoming;
		int dir = encoder_debounce(delta);
		time += delta;
		if (dir) {
			bpm += dir;
			bpm = max(min(bpm, 300), 30);
			OCR1A = bpmticks(bpm);
		}
		if (uart_rx(&incoming) && incoming != MIDI_CLOCK)
			push(&midi, incoming);
		if (uart_tx_ready()) {
			uint8_t out;
			if (pop(&clock, &out) || pop(&midi, &out))
				uart_tx(out);
		}
	}
}
