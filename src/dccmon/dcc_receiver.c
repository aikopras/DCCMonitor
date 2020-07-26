/**
 * DCC reception routines
 *
 * This file contains buffer access routines and the real workhorse, an
 * interrupt handler sampling the DCC input pin at approx. 100 kHz and
 * filtering and interpreting the DCC frames. DCC frames are made available
 * through a circular buffer.
 *
 * This file is part of DCC Monitor.
 *
 * Copyright 2007, 2008 Peter Lebbing <peter@digitalbrains.com>
 *
 * The interrupt routine is based on the filtering sampling method described on
 * http://www.opendcc.de/ -> Decoder -> Software, copyright 2006,2007 Wolfgang 
 * Kufer <kufer@gmx.de>.
 *
 * DCC Monitor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * DCC Monitor is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DCC Monitor.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdbool.h>
#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "../common/global.h"
#include "../common/timer.h"
#include "dcc_receiver.h"
#include "dccmon.h"

static struct {
  volatile uint8_t buf[DCC_BUFSIZE];
  volatile uint8_t head, tail;
} dcc_buf; // Circular buffer holding DCC data

#define DCC_OVERFLOW_VAR global_prot_var
#define DCC_OVERFLOW_BIT (1 << 1)

/**
 * Check if buffer has data available
 */
uint8_t dcc_would_block () {
  if (dcc_buf.head == dcc_buf.tail) {
    // Yes, it would block
    return true;
  }
  return false;
}

/**
 * Report and clear whether an overflow has occured
 */
uint8_t dcc_overflow_status () {
  // Try to avoid critical section
  if (!(DCC_OVERFLOW_VAR & DCC_OVERFLOW_BIT)) {
    // No overflow
    return 0;
  }

  // Overflow; need to clear bit in cricital section
  cli();
  DCC_OVERFLOW_VAR &= ~DCC_OVERFLOW_BIT;
  sei(); // End of critical section

  return 1;
}

/**
 * Return a byte of data from the circular buffer, blocking for new data if 
 * necessary.
 */
uint8_t dcc_get () {
  uint8_t temp, c;

  temp = dcc_buf.tail;
  while (dcc_buf.head == temp) {} // Buffer is empty

  c = dcc_buf.buf[temp];
  circ_buf_incr_ptr (&temp, DCC_BUFSIZE);
  dcc_buf.tail = temp;
  return c;
}

/**
 * TICKS_PER_SAMPLE: The number of clockticks that comes closes to a 10 uS
 * period. We sample the DCC input pin at 100 kHz, or equivalently, every
 * TICKS_PER_SAMPLE clockticks.
 */
#define TICKS_PER_SAMPLE (div_round (F_CPU, 100000UL))

/*
 * PULSE_DISCRIMINATOR: DCC Pulses which last this many samples or more are
 * interpreted as (half) a 0-bit. Shorter as (half) a 1-bit.
 * 
 * It is calculated to have the threshold be as close to 77 uS as possible.
 *
 * Since we try to sample at 100 kHz, it is most likely 8, but with certain
 * clock frequencies I suspect it might be 7.
 */
#define PULSE_DISCRIMINATOR (div_round (77ULL * F_CPU, TICKS_PER_SAMPLE * 1000000ULL))

/**
 * Initialise DCC receiver
 *
 * Sets the I/O-pin correctly and starts the interrupt-driven sampler. 
 *
 * It is assumed the uC is in it's default settings with regard to periphery.
 * The DDR register is only explicitly programmed to reduce the chance of a
 * shortcut when a programming error is made.
 */
void dcc_init () {
  // Set DCC input pin to be input
  DCC_INPUT_DDR &= ~(_BV(DCC_INPUT_PIN));
  // Disable pullup
  DCC_INPUT_PORT &= ~(_BV(DCC_INPUT_PIN));
  // Configure Timer0 to generate an interrupt every 10 uS
  OCR0 = timer0_period (TICKS_PER_SAMPLE, TICKS_PER_SAMPLE) - 1;
  TIMSK |= _BV(OCIE0); // Enable Compare Match interrupt
  TCCR0 = _BV(WGM01) | timer0_prescale_bits (TICKS_PER_SAMPLE); // CTC mode, start!
}

/**
 * monitor_init() is called by common/main.c to initialize the monitor running
 * on this board. It is aliased weakly to dcc_init(). dcc_send_filter.c
 * overrides this with it's own function if needed.
 */
void monitor_init() __attribute__ ((weak,alias("dcc_init")));

/**
 * Sample the DCC input pin and parse incoming DCC data, placing it in the 
 * buffer.
 * This gets called approx. every 10 uS
 * 
 * A simple software low-pass filter is used to filter jitter.
 * Because of the filtering, the duration of a high or low signal can be 
 * skewed in the case of noise.
 * Filtered durations up to the sample closest to 77 uS get interpreted as
 * (half) a 1, longer durations get interpreted as (half) a 0.
 *
 * With a clock frequency of 11.0592 MHz, this means filtered durations up to
 * 80.30 uS get interpreted as (half) a 1. This is 8 samples.
 *
 * This routine is basically a finite state machine with the state
 * recording the position in the DCC signal.
 *
 * A local buffer pointer (write_pos) is used that points to the location
 * to write the next DCC databyte. Only when a full frame is received, is
 * the buffer head pointer updated to push the frame out to the listener.
 * The length of the packet is recorded before the data bytes.
 */
ISR(TIMER0_COMP_vect) {
  static uint8_t hi_count; // Number of 1-bits in "pulses"
  static uint8_t pulses; // Shiftregister with latest unfiltered pinstates
  static uint8_t pulse_duration; // Duration of current filtered signal
  static uint8_t byte_store; // Counting bits in preamble and storing databyte during reception
  static uint8_t write_pos; // Location in buf to write databyte
  static uint8_t state; // Current reception state
  #define IN_FIRST_HALF (1 << 0) // First half of a DCC bit expected
  #define DCC_VALUE (1 << 1) // Value of the first half bit
  #define PREAMBLE (1 << 2) // Preamble bits expected
  #define LEAD0 (1 << 3) // Leading 0 expected (follows preamble)
  #define IN_BYTE (1 << 4) // Reading a byte
  #define TRAILER (1 << 5) // Trailer bit expected (follows a byte)

  // Shift in current puls
  if (DCC_INPUT_PORT & _BV(DCC_INPUT_PIN)) {
    pulses = (pulses <<1) + 1;
  } else {
    pulses <<= 1;
  }

  // Adjust hi_count and check filtered state change in one go
  if ((pulses & (1<<6) && !(pulses & 1) && --hi_count == 2) // high-to-low transition
      || (!(pulses & (1<<6)) && (pulses & 1) && ++hi_count == 3)) { // low-to-high transition

    if (pulse_duration < PULSE_DISCRIMINATOR - 1) { 
      // Duration < 80.30 uS, half of a 1-bit
      // Note: pulse_duration didn't increase this particular interrupt, because we will erase
      // it anyway.
      pulse_duration = 0;
      if (state & IN_FIRST_HALF) { 
        // We accept this first half and wait for the second
        state = (state & (~(IN_FIRST_HALF))) | DCC_VALUE;
        return;
      } else {
        // Second half
        if (!(state & DCC_VALUE)) {
          // First half was 0, second 1; does not make a valid DCC bit, restart everything
          // Take this half 1-bit as a first half
          state = PREAMBLE | DCC_VALUE;
          byte_store = 0;
          return;
        }
      }
    } else {
      // Duration >= 80.30 uS, half of a 0-bit
      
      pulse_duration = 0;
      if (state & IN_FIRST_HALF) { 
        // We accept this first half and wait for the second
        state &= ~(IN_FIRST_HALF | DCC_VALUE);
        return;
      } else {
        // Second half
        if (state & DCC_VALUE) {
          // First half was 1, second 0; does not make a valid DCC bit
          if (state & LEAD0) {
            /* 
             * This is a special case: so far, we had only 1's, but if we caught
             * what was the second half of such a 1 as the first half accidentally,
             * we end up here even though the waveform is acceptable. Next is the
             * second half of the leading 0.
             */
            state = LEAD0;
            return;
          } else {
            // Restart everything, taking this half 1-bit as a first half
            state = PREAMBLE;
            byte_store = 0;
            return;
          }
        }
      }
    }

    // When we got this far, we received a valid bit, whose value is in
    // state & DCC_VALUE

    if (state & PREAMBLE) {
      // We're in the preamble
      if (state & DCC_VALUE) {
        // We received a 1
        if (byte_store >= 9) {
          // We've received 10 preamble bits
          // Wait for a leading 0
          state = LEAD0 | IN_FIRST_HALF;
          return;
        } else {
          // More preamble
          byte_store++; // Count 1-bits here
          state = PREAMBLE | IN_FIRST_HALF;
          return;
        }
      } else {
        // We received a premature 0, restart everything
        // The last received half bit was a 0, take that in account
        state = PREAMBLE;
        byte_store = 0;
        return;
      }
    } else if (state & LEAD0) {
      if (state & DCC_VALUE) {
        // We received a 1, the preamble hasn't finished yet
        state = LEAD0 | IN_FIRST_HALF;
        return;
      } else {
        // We received a 0, next should be a byte
        uint8_t temp;
        
        temp = dcc_buf.head;

        // Initialise packet length
        dcc_buf.buf[temp] = 0;
        // Set write_pos to the byte following the length
        circ_buf_incr_ptr (&temp, DCC_BUFSIZE);
        write_pos = temp;
        state = IN_BYTE | IN_FIRST_HALF;
        byte_store = 1; // This bit is used to see when we received a full byte
        return;
      }
    } else if (state & IN_BYTE) {
      // We're receiving a byte
      uint8_t temp_data, temp_pos;

      if (byte_store & (1<<7)) {
        // Last bit in the byte
        // Shift in a bit
        temp_data = byte_store << 1;
        if (state & DCC_VALUE) {
          temp_data |= 1;
        }
        
        // Check whether we have space to store it in the buffer
        temp_pos = write_pos;
        if (write_pos == dcc_buf.tail) {
          // Overflow! Discard this packet
          DCC_OVERFLOW_VAR |= DCC_OVERFLOW_BIT;
          state = PREAMBLE | IN_FIRST_HALF; // Not that IN_FIRST_HALF matters...
          byte_store = 0;
          return;
        }

        // Store it in the buffer
        dcc_buf.buf[temp_pos] = temp_data;

        // Increase writing position in buffer
        circ_buf_incr_ptr (&temp_pos, DCC_BUFSIZE);
        write_pos = temp_pos;
        
        // Increase packet length
        dcc_buf.buf[dcc_buf.head]++;

        // Next is the trailer bit
        state = TRAILER | IN_FIRST_HALF;
        return;

      } else {
        // Not the last bit
        // Shift in a bit
        temp_data = byte_store << 1;
        if (state & DCC_VALUE) {
          temp_data |= 1;
        }
        byte_store = temp_data;
        state = IN_BYTE | IN_FIRST_HALF;
        return;
      }

    } else { // if state & TRAILER (implicit, it is none of the other states)
      // This is the trailer bit
      if (state & DCC_VALUE) {
        // This is the end of the DCC message
        // During byte reception, we allowed the buffer to go completely full
        // This is not allowed now, because that state is indiscernable from completely empty

        if (write_pos == dcc_buf.tail) {
          // Overflow!
          DCC_OVERFLOW_VAR |= DCC_OVERFLOW_BIT;
        } else {
          // Accept the message in the buffer
          dcc_buf.head = write_pos;
        }

        state = PREAMBLE | IN_FIRST_HALF;
        byte_store = 0;
        return;

      } else {
        // Another byte to come
        state = IN_BYTE | IN_FIRST_HALF;
        byte_store = 1; // This bit is used to see when we've got a full byte
        return;
      }
    }

  } else {
    // Nothing changes, only adjust duration.
    // Stop counting once it qualifies as a 0-bit to prevent overflow.
    if (pulse_duration < PULSE_DISCRIMINATOR - 1) {
      pulse_duration++;
    }
  }
}
