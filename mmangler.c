/* velocity mapper */
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

struct {
  volatile uint8_t *const port, *const ddr, *const pin;
  uint8_t bit[2], debounce[2], elapsed;
} encoder = {&PORTD, &DDRD, &PIND, {PORTD0, PORTD1}, {0}, 0};

void encoder_init(void) {
  uint8_t mask = (1 << encoder.bit[0]) | (1 << encoder.bit[1]);
  *encoder.ddr &= ~mask;
  *encoder.port |= mask;
}

int encoder_debounce(uint8_t delta) {
  int i, a, b;

  encoder.elapsed += delta;
  if (encoder.elapsed < 16)
    return 0;
  encoder.elapsed = 0;

  for (i = 0; i < nelem(encoder.debounce); i++)
    encoder.debounce[i] =
        (encoder.debounce[i] << 1) | !!(*encoder.pin & (1 << encoder.bit[i]));

  a = encoder.debounce[0] & 3;
  b = encoder.debounce[1] & 3;
  return (a == 1 && b == 0) - (a == 2 && b == 0);
}

struct {
  volatile uint8_t *const port, *const ddr, *const pin;
  uint8_t bit, debounce;
} button = {&PORTD, &DDRD, &PIND, 1 << PORTD4, 0};

void button_init(void) {
  *button.ddr &= ~button.bit;
  *button.port |= button.bit;
}

int button_debounce(uint8_t delta) {
  if (!delta)
    return 0;
  button.debounce = (button.debounce << 1) | !!(*button.pin & button.bit);
  /* The GPIO is held high by a pull-up resistor and the button shorts it to
   * ground. The button is pressed when the GPIO reads 0. */
  return button.debounce == 0x80;
}

/* Run Timer0 at F_CPU/1024 Hz */
void timer_init(void) { TCCR0B = 1 << CS02 | 1 << CS00; }

struct {
  uint8_t note, noteon;
} velocity = {36, 0};
struct {
  int8_t tune[128];
  uint8_t lastnote;
} microtune;
#define MTUNESHIFT 5
struct {
  uint8_t shift;
} compress = {64};

enum mode { ECHO, VELOCITY, MICROTUNE, COMPRESS, NMODE };
int main(void) {
  midi_parser mp = {0};
  uint8_t time = 0, midi_byte;
  enum mode mode = COMPRESS;
  uart_init();
  timer_init();
  encoder_init();
  button_init();

  while (1) {
    midi_message msg;
    uint8_t delta = TCNT0 - time;
    int dir = encoder_debounce(delta);
    mode = (mode + button_debounce(delta)) % NMODE;
    time += delta;
    if (mode == ECHO) {
      if (uart_rx(&midi_byte))
        uart_tx(midi_byte);
    } else if (mode == VELOCITY) {
      if (dir) {
        velocity.note = (velocity.note + dir) & 127;
        uart_tx(MIDI_NOTE_ON);
        uart_tx(velocity.note);
        uart_tx(64);
        uart_tx(velocity.note);
        uart_tx(0);
      }
      if (!uart_rx(&midi_byte))
        continue;
      if (msg = midi_read(&mp, midi_byte), msg.status) {
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
    } else if (mode == MICROTUNE) {
      if (dir) {
        int8_t *t = microtune.tune + microtune.lastnote;
        *t += dir;
        *t += (*t == -(1 << MTUNESHIFT)) - (*t == (1 << MTUNESHIFT));
      }
      if (!uart_rx(&midi_byte))
        continue;
      if (msg = midi_read(&mp, midi_byte), msg.status) {
        msg.status &= 0xf0;
        if (msg.status == MIDI_NOTE_OFF ||
            (msg.status == MIDI_NOTE_ON && !msg.data[1])) {
          uart_tx(MIDI_NOTE_OFF);
          uart_tx(msg.data[0]);
          uart_tx(0);
        } else if (msg.status == MIDI_NOTE_ON) {
          uint16_t pitchbend = 8192 + ((int16_t)microtune.tune[msg.data[0]]
                                       << (13 - MTUNESHIFT));
          microtune.lastnote = msg.data[0];
          uart_tx(MIDI_PITCH_BEND);
          uart_tx(pitchbend & 127);
          uart_tx(pitchbend >> 7);
          uart_tx(MIDI_NOTE_ON);
          uart_tx(msg.data[0]);
          uart_tx(msg.data[1]);
        }
      }
    } else if (mode == COMPRESS) {
      uint8_t note;
      int16_t bendrange = 2, steps = 6;
      compress.shift = (compress.shift + dir) & 127;
      if (!uart_rx(&midi_byte))
        continue;
      msg = midi_read(&mp, midi_byte);
      msg.status &= 0xf0;
      note = msg.data[0] / steps + compress.shift;
      if (msg.status == MIDI_NOTE_OFF ||
          (msg.status == MIDI_NOTE_ON && !msg.data[1])) {
        uart_tx(msg.status);
        uart_tx(note);
        uart_tx(msg.data[1]);
      } else if (msg.status == MIDI_NOTE_ON) {
        uint16_t bend = 8192 + ((int16_t)msg.data[0] % steps - steps / 2) *
                                   8192 / (steps * bendrange);
        uart_tx(MIDI_PITCH_BEND);
        uart_tx(bend & 127);
        uart_tx(bend >> 7);
        uart_tx(MIDI_NOTE_ON);
        uart_tx(note);
        uart_tx(msg.data[1]);
      }
    }
  }
}
