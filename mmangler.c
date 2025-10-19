/* MIDI fun box */
#include "midi.h"
#include <avr/io.h>
#include <util/delay.h>
#define nelem(x) (int)(sizeof(x) / sizeof(*(x)))
void uart_init(void) {
  enum { midi_baud = 31250, ubrr = (F_CPU / 16 / midi_baud) - 1 };
  UBRR1H = ubrr >> 8;
  UBRR1L = ubrr & 0xff;
  UCSR1B = _BV(RXEN1) | _BV(TXEN1);
}
void uart_tx(uint8_t c) {
  while (!(UCSR1A & (1 << UDRE1)))
    ;
  UDR1 = c;
}
uint8_t uart_rx(uint8_t *c) {
  uint8_t status = UCSR1A;
  if (!(status & _BV(RXC1)))
    return 0;
  *c = UDR1;
  return !(status & (_BV(FE1) | _BV(DOR1) | _BV(UPE1)));
}
struct pinin {
  volatile uint8_t *const port, *const ddr, *const pin;
  uint8_t mask;
};
#define PININ(port, n) {&PORT##port, &DDR##port, &PIN##port, 1 << PORT##port##n}
void pinin_init(struct pinin *p) {
  *(p->ddr) &= ~p->mask;
  *(p->port) |= p->mask;
}
uint8_t pinin_read(struct pinin *p) { return !!(*(p->pin) & p->mask); }
struct debouncer {
  struct pinin pin;
  uint16_t history;
};
#define DEBOUNCER(port, n) {PININ(port, n), 0}
void debouncer_init(struct debouncer *db) { pinin_init(&db->pin); }
uint16_t debouncer_update(struct debouncer *db) {
  return db->history = (db->history << 1) | pinin_read(&db->pin);
}
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
  return (x[0] == 0x0001 && x[1] == 0) - (x[0] == 0x8000 && x[1] == 0);
}
struct {
  struct debouncer debouncer;
} button = {DEBOUNCER(D, 4)};
void button_init(void) { debouncer_init(&button.debouncer); }
int button_debounce(uint8_t delta) {
  uint16_t x = delta ? debouncer_update(&button.debouncer) : 0;
  return x == 0x8000;
}
/* Run Timer0 at F_CPU/1024 Hz */
void timer_init(void) { TCCR0B = 1 << CS02 | 1 << CS00; }
struct {
  uint8_t note, noteon;
} velocity = {36, 0};
void velocity_encoder(int dir) {
  velocity.note = (velocity.note + dir) & 127;
  uart_tx(MIDI_NOTE_ON);
  uart_tx(velocity.note);
  uart_tx(64);
  uart_tx(velocity.note);
  uart_tx(0);
}
void velocity_midi(midi_message msg) {
  msg.status &= 0xf0;
  if (msg.status == MIDI_NOTE_OFF ||
      (msg.status == MIDI_NOTE_ON && !msg.data[1])) {
    uart_tx(MIDI_NOTE_ON);
    uart_tx(velocity.note);
    uart_tx(0);
    velocity.noteon = 0;
  } else if (msg.status == MIDI_NOTE_ON) {
    int keyvel = 1, mapstart = 48;
    if (msg.data[0] >= mapstart)
      keyvel += (msg.data[0] - mapstart) * 5;
    if (keyvel > 127)
      keyvel = 127;
    uart_tx(MIDI_NOTE_ON);
    if (velocity.noteon) { /* prevent overlapping notes */
      uart_tx(velocity.note);
      uart_tx(0);
    }
    uart_tx(velocity.note);
    uart_tx(keyvel);
    velocity.noteon = 1;
  }
}
struct {
  int8_t tune[128];
  uint8_t lastnote;
} microtune;
#define MTUNESHIFT 5
void microtune_encoder(int dir) {
  int8_t *t = microtune.tune + microtune.lastnote;
  *t += dir;
  *t += (*t == -(1 << MTUNESHIFT)) - (*t == (1 << MTUNESHIFT));
}
void microtune_midi(midi_message msg) {
  msg.status &= 0xf0;
  if (msg.status == MIDI_NOTE_OFF ||
      (msg.status == MIDI_NOTE_ON && !msg.data[1])) {
    uart_tx(MIDI_NOTE_OFF);
    uart_tx(msg.data[0]);
    uart_tx(0);
  } else if (msg.status == MIDI_NOTE_ON) {
    uint16_t pitchbend =
        8192 + ((int16_t)microtune.tune[msg.data[0]] << (13 - MTUNESHIFT));
    microtune.lastnote = msg.data[0];
    uart_tx(MIDI_PITCH_BEND);
    uart_tx(pitchbend & 127);
    uart_tx(pitchbend >> 7);
    uart_tx(MIDI_NOTE_ON);
    uart_tx(msg.data[0]);
    uart_tx(msg.data[1]);
  }
}
struct {
  uint8_t held[16], shift;
} compress = {{0}, 64};
void compress_encoder(int dir) {
  uint8_t i, j;
  compress.shift = (compress.shift + dir) & 127;
  /* If the user turns the encoder while holding down a MIDI key, that key
   * will never get a note-off when it's released. To avoid that we send a
   * note-off for every held key when the encoder is turned. You would
   * think that is what MIDI "all notes off" is for but some synths ignore
   * it so we manually track the held notes. */
  for (i = 0; i < nelem(compress.held); i++) {
    if (compress.held[i])
      continue;
    for (j = 0; j < 8; j++) {
      if (compress.held[i] & (1 << j)) {
        uart_tx(MIDI_NOTE_OFF);
        uart_tx(i * 8 + j);
        uart_tx(0);
      }
    }
    compress.held[i] = 0;
  }
}
void compress_midi(midi_message msg) {
  int16_t bendrange = 2, steps = 6;
  uint8_t note = msg.data[0] / steps + compress.shift;
  msg.status &= 0xf0;
  if (msg.status == MIDI_NOTE_OFF ||
      (msg.status == MIDI_NOTE_ON && !msg.data[1])) {
    uart_tx(msg.status);
    uart_tx(note);
    uart_tx(msg.data[1]);
    compress.held[note / 8] &= ~(1 << (note % 8));
  } else if (msg.status == MIDI_NOTE_ON) {
    uint16_t bend = 8192 + ((int16_t)msg.data[0] % steps - steps / 2) * 8192 /
                               (steps * bendrange);
    uart_tx(MIDI_PITCH_BEND);
    uart_tx(bend & 127);
    uart_tx(bend >> 7);
    uart_tx(MIDI_NOTE_ON);
    uart_tx(note);
    uart_tx(msg.data[1]);
    compress.held[note / 8] |= 1 << (note % 8);
  }
}
struct {
  void (*encoder)(int dir);
  void (*midi)(midi_message msg);
} modes[] = {
    {velocity_encoder, velocity_midi},
    {microtune_encoder, microtune_midi},
    {compress_encoder, compress_midi},
};
uint8_t mode = nelem(modes); /* echo mode */
int main(void) {
  midi_parser mp = {0};
  uint8_t time = 0, midi_byte;
  uart_init();
  timer_init();
  encoder_init();
  button_init();
  while (1) {
    uint8_t delta = TCNT0 - time;
    int dir = encoder_debounce(delta);
    mode = (mode + button_debounce(delta)) % (nelem(modes) + 1);
    time += delta;
    if (mode == nelem(modes)) {
      if (uart_rx(&midi_byte))
        uart_tx(midi_byte);
    } else {
      if (dir)
        modes[mode].encoder(dir);
      if (uart_rx(&midi_byte))
        modes[mode].midi(midi_read(&mp, midi_byte));
    }
  }
}
