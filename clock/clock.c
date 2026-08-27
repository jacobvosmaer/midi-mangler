/* MIDI clock merge box */
#include "encoder.h"
#include "uart.h"
#include <avr/io.h>
#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))
/* MIDI clock is 24 PPQN. the formula below calculates the number of prescaled
 * timer ticks that corresponds to 1 MIDI clock pulse for the given tempo in BPM
 */
#define fTimer1 (F_CPU / 64)
int bpmticks(int n) { return (fTimer1 / 2 * 5) / n; }
#define MIDI_CLOCK 0xf8
int main(void) {
	uint8_t tick = 0;
	int bpm = 131, newbpm = bpm;
	struct encoder encoder;
	uart_init();
	TCCR1B = 1 << CS11 | 1 << CS10; /* Timer1 prescaler 64 */
	TCCR1B |= 1 << WGM12;		/* CTC mode */
	OCR1A = bpmticks(bpm);
	encoderinit(&encoder);
	while (1) {
		newbpm += encoderupdate(&encoder);
		newbpm = max(min(newbpm, 300), 30);
		if (TIFR1 & 1 << OCF1A) {
			TIFR1 |= 1 << OCF1A;
			tick = 1;
			if (bpm != newbpm) {
				bpm = newbpm;
				OCR1A = bpmticks(bpm);
			}
		}
		if (uart_tx_ready()) {
			uint8_t b;
			if (tick) {
				tick = 0;
				uart_tx(MIDI_CLOCK);
			} else if (uart_rx(&b) && b != MIDI_CLOCK) {
				uart_tx(b);
			}
		}
	}
}
