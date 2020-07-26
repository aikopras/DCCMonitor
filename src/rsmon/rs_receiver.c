/**
 * RS-bus reception routines
 *
 * This file contains the routines for reception of RS-bus data from the
 * hardware. Interrupt routines trigger on the input pins and timer interrupts
 * to decode address pulses from the command station and databytes from the
 * responders.
 *
 * INT0: Address pulses from the command station
 * INT1/PD3: Data sent by responders
 * TIMER0: times data reception from responder
 * TIMER1 is used as a "real time clock"
 *
 * The TIMER1_OVF interrupt is defined here; should it be needed for other
 * purposes as well, some method to integrate the purposes has to be written.
 * 
 * This file is part of DCC Monitor.
 *
 * Copyright 2008 Peter Lebbing <peter@digitalbrains.com>
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

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "rsmon.h"
#include "rs_receiver.h"
#include "../common/global.h"
#include "../common/timer.h"

/** 
 * Timer definitions for receiving bytes from the RS-bus responders.
 * 
 * TIMER0 is used to clock in the data at 4800 bps.
 */
// Number of clockticks in one databit
#define RS_TICKSPERBIT (div_round (F_CPU, 4800))
// Prescaler value for TIMER0: big enough to count one bitperiod
#define RS_TIMER0_PRESCALE (timer0_prescale_bits (RS_TICKSPERBIT))
// Period between two samples of the same databit
#define RS_TIMER0_SAMPLEPERIOD (timer0_period (div_round (F_CPU, 4800UL * 8UL), RS_TICKSPERBIT))
// Period between two databits
#define RS_TIMER0_BITPERIOD (timer0_period (RS_TICKSPERBIT, RS_TICKSPERBIT))
// Period between start of startbit and first sample of startbit
#define RS_TIMER0_STARTBIT (timer0_period (div_round (F_CPU, 4800 * 2), RS_TICKSPERBIT) \
    - RS_TIMER0_SAMPLEPERIOD)

/**
 * RS-bus reception buffer structure
 *
 * It's a circular buffer with three elements for every reception occurence:
 *
 * status: Status code associated with reception
 * addr:   Address from which data was received. Range 0-129, valid address
 *         range 1-128, 129 = any address >= 129.
 * data:   Received data
 *
 * Not all statuses have associated addresses or data (see rs_get_status()).
 *
 * One buffer position is wasted on detecting the difference between buffer
 * full/empty.
 *
 * overflow is non-zero when the buffer overflowed (set by interrupt routine,
 * cleared by rs_overflow_status() ).
 */
static struct {
  volatile uint8_t status[RS_BUFSIZE];
  volatile uint8_t addr[RS_BUFSIZE];
  volatile uint8_t data[RS_BUFSIZE];
  volatile uint8_t head, tail;
  volatile uint8_t overflow;
} rs_buf;

/**
 * Sample counter for supersampling of received RS-bus data
 *
 * Needs to be initialized to zero before starting the sampling interrupt
 * routine.
 *
 * Only used inside interrupt routines.
 */
static uint8_t sample_count;

/**
 * The current address as counted from the address pulses
 *
 * Range: see rs_buf.addr.
 *
 * Only used inside interrupt routines.
 */
static uint8_t rs_addr;

/**
 * Finite state machine for the phases of reception
 *
 * Only used inside interrupt routines.
 */
static uint8_t rs_state;
#define RS_IDLE 0 // Start state; no RS-bus activity
#define RS_ADDR 1 // We expect an address pulse
#define RS_STARTBIT 2 // We expect a startbit
#define RS_IN_BYTE 3 // We expect another bit for the received byte
#define RS_STOPBIT 4 // We expect a stopbit
#define RS_TIMER_OVF 5 // RS_ADDR, but real time clock has overflowed

/**
 * Initialise RS-bus monitoring process.
 *
 * Sets I/O-pin functions and interrupts.
 *
 * Should be called once, before any other RS-bus reception routines.
 *
 * It is assumed the uC is in it's default settings with regard to periphery.
 * The DDR register is only explicitly programmed to reduce the chance of a
 * shortcut when a programming error is made.
 */
void rs_init() {
  // INT0 and INT1: disable pullups, set as input
  DDRD &= ~(1 << 2);
  DDRD &= ~(1 << 3);
  PORTD &= ~(1 << 2);
  PORTD &= ~(1 << 3);

  // Rising edge on INT1 generates interrupt
  // Rising edge on INT1 generates interrupt
  MCUCR |= _BV(ISC00);
  MCUCR |= _BV(ISC01);
  MCUCR |= _BV(ISC10);
  MCUCR |= _BV(ISC11);

// Original settings below
  // Falling edge on INT0 generates interrupt
  // Rising edge on INT1 generates interrupt
//  MCUCR &= ~(_BV(ISC00));
//  MCUCR |= _BV(ISC10);
//  MCUCR |= _BV(ISC11);
//  MCUCR |= _BV(ISC01);

  // Clear INT0 interrupt flag
  GIFR = _BV(INTF0); 
  
  GICR |= _BV(INT0); // Enable INT0
  TIMSK |= _BV(OCIE0); // Enable TIMER0 Compare Match interrupt
  TIMSK |= _BV(TOIE1); // Enable TIMER1 Overflow Interrupt
}

/**
 * monitor_init() is called by common/main.c to initialize the monitor running
 * on this board. It is aliased to rs_init().
 */
void monitor_init() __attribute__ ((alias("rs_init")));

/**
 * Report and clear whether a buffer overflow occured
 */
int8_t rs_overflow_status () {
  if (rs_buf.overflow) {
    // Clear overflow condition
    rs_buf.overflow = 0;
    return -1;
  }

  return 0;
}

/**
 * Get the status of the next received byte on the RS-bus
 *
 * For some statuses, address and/or data are not defined.
 *
 * Possible statuses:
 * RS_OKAY          Byte received okay.
 * RS_FRAME_ERR     Databyte had a framing error (zero endbit). Data undefined.
 * RS_ADDR_ERR      We received more than the expected 130 address pulses.
 *                  Reported only once. Address and data undefined.
 * 
 * Returns 0 when there is no next byte in the buffer, a statusbyte otherwise.
 */
uint8_t rs_get_status () {
  uint8_t temp_tail;

  temp_tail = rs_buf.tail;
  if (rs_buf.head == temp_tail) {
    return 0;
  }
  return rs_buf.status[temp_tail];
}

/**
 * Get the address that sent the next received byte on the RS-bus
 *
 * May be undefined, dependent on rs_get_status().
 */
uint8_t rs_get_addr () {
  uint8_t addr;

  addr = rs_buf.addr[rs_buf.tail];
  return addr;
}

/**
 * Get the next received byte and advance the buffer pointer to the next byte
 *
 * May be undefined dependent on rs_get_status(), but still needs to be called
 * to get to the next byte.
 *
 * After calling this routine, rs_get_status() and rs_get_address() give
 * information relating to the next byte. Status and address of this byte is
 * lost.
 */
uint8_t rs_get_data () {
  uint8_t temp_tail;
  uint8_t data;

  temp_tail = rs_buf.tail;
  data = rs_buf.data[temp_tail];
  circ_buf_incr_ptr (&temp_tail, RS_BUFSIZE);
  rs_buf.tail = temp_tail;
  return data;
}

/**
 * This routine gets called when TIMER1 overflows; if we are in a state where
 * we expect address pulses, we need to keep track of the overflows to prevent
 * misinterpreting the elapsed time since the last pulse.
 */
ISR(TIMER1_OVF_vect) {
  if (rs_state == RS_ADDR) {
    // Change to state RS_TIMER_OVF to keep track of overflow
    rs_state = RS_TIMER_OVF;
  } else if (rs_state == RS_TIMER_OVF) {
    // Timer overflowed twice; this means we haven't seen address pulses all
    // that time, so the bus is idle.
    rs_state = RS_IDLE;
  }
}

/**
 * This interrupt gets called when the INT0 pin has a falling edge; this 
 * signifies the command station sending an address pulse.
 *
 * INT1 is enabled again to allow a single databyte to be clocked in.
 */
ISR(INT0_vect) {
  static uint16_t last_pulse_time; // "Real" time of last address pulse
  static uint8_t reported_addr_err; // Did we report an addressing error yet?
  uint16_t now, passed;

  // Get "real" time (done early for accuracy)
  now = TCNT1;

  // Enable INT1 to clock in one databyte
  GIFR = _BV(INTF1); // Clear INT1 interrupt flag
  GICR |= _BV(INT1); // Enable interrupt on new falling edge

  if (rs_state == RS_IDLE) {
    // First pulse after idle time: first address pulse
    rs_addr = 0;
    rs_state = RS_ADDR; // New state
    last_pulse_time = now; // Store this pulse time
    return;

  } else if (rs_state == RS_TIMER_OVF) {
    // We now have a pulse; we can discard the fact the timer overflowed *once*,
    // and use the fact that unsigned integer arithmetic wraps round
    rs_state = RS_ADDR;

  } else if (rs_state != RS_ADDR) {
    // We were receiving a byte, but the command station cut it off. Abort
    // receival and continue in address pulse state
    rs_state = RS_ADDR;
    TCCR0 = 0; // Disable TIMER0
    TIFR = _BV(OCF0); // Clear any pending TIMER0 interrupt
  }

  // Difference between last pulse time and now
  passed = now - last_pulse_time;

  // Check if 5 ms has passed
  if (passed > rtc_period (5 mseconds)) {
    // Yes, this is longer than the address pulses, so it's the waiting period
    // before the first address pulse. Reset address.
    rs_addr = 0;

  // Check if it it wasn't a very short pulse.
  // Ignore it if it's too short; it was probably some line instability.
  } else if (passed >= 2) {
    // Pulse seems okay
    // The last rising edge was short ago, it's an address pulse

    // Range-checking
    if (rs_addr == 129) {
      // Too many pulses, report this to the PC *once*
      // Note that rs_addr will never increase past 129
      
      if (!reported_addr_err) {
        // Report
        uint8_t temp_head;

        reported_addr_err = 1; // Report just this time
        temp_head = rs_buf.head;
        rs_buf.status[temp_head] = RS_ADDR_ERR;
        // Note: rs_buf.addr and rs_buf.data are don't-care
        circ_buf_incr_ptr (&temp_head, RS_BUFSIZE);
        rs_buf.head = temp_head;
      }

    } else {
      rs_addr++;
    }
  } 

  // Store this pulse time
  last_pulse_time = now;
}

/**
 * This interrupt routine gets called when the INT1/PD3 pin has a rising edge;
 * this singifies a start-bit sent by a responder.
 *
 * After the startbit, this interrupt should be disabled until we want to
 * detect a new startbit (so it doesn't trigger on databits).
 */
ISR(INT1_vect) {
  // Check the start bit (= TIMER0 interrupt) after half a bit-period
  // Timer set early for accuracy
  TCNT0 = 0;
  OCR0 = RS_TIMER0_STARTBIT - 1;
  TCCR0 = RS_TIMER0_PRESCALE; // Normal mode, calculated prescaler, start! 
  // Set state
  rs_state = RS_STARTBIT;
  // Init sample counter
  sample_count = 0;
  // Disable this interrupt (it will be re-enabled by another routine at the
  // correct time)
  GICR &= ~(_BV(INT1));
}

/**
 * This interrupt routine gets called when TIMER0 overflows; this means we
 * should take another sample of a databit.
 *
 * It tries to sample each bit three times and takes the majority vote as the
 * bitvalue.
 *
 * The samples are evenly spaced around the center of the bitperiod, each 1/8th
 * of a bitperiod from eachother.
 *
 * Because there is so little time between the samples, we account for the
 * possibility that this interrupt handler is delayed so much that it is too
 * late to take a good sample and schedule the next sample. 
 *
 * During the first and second sample run, just before we write the next sample
 * moment in OCR0, we first check if the current Timer0 value is still smaller
 * than the new OCR0 value minus one. This ensures that the next sample moment
 * will trigger the interrupt. Any later and we might not trigger an interrupt,
 * ruining the timing.
 *
 * If we cannot schedule this next sample moment, we take the first sample as
 * the bitvalue and schedule the next bit. 
 *
 * During the third sample run, we assume we can schedule the first sample of
 * the next bit: it's much further away. However, we do check if this third
 * sample was taken within a reasonable time after it should ideally have been
 * taken. If not, we discard it and use the value of the second sample (which,
 * as we got to here, was taken at pretty much the center of the bitperiod).
 *
 * When all databits have been sampled, rs_buf contains the complete reception.
 */
ISR(TIMER0_COMP_vect) {
  static uint8_t recv; // Current byte received
  static uint8_t samples; // The samples taken so far, LSB = most recent
  uint8_t curr_sample; // Current sample on input pin
  uint8_t bit_recv; // Current filtered bitvalue (inverted)
  /*
   * Timer0 ticks since OCF0 interrupt and the intended write of the new OCR0
   * value.
   *
   * GCC is a bit anal about computations inside comparisons. Using this
   * variable, and the one below, makes it use 8-bit arithmetic instead of 16-bit.
   */
  uint8_t latency;
  uint8_t latency_max; // Max latency; simply the constant RS_TIMER0_SAMPLEPERIOD - 1.
  uint8_t temp_head, temp_new_head, temp_recv, temp_ocr0;

  // First get the value of the current bit (it's inverted)
  curr_sample = PIND & _BV(PD3); 

  latency_max = RS_TIMER0_SAMPLEPERIOD - 1;
  temp_ocr0 = OCR0;

  switch (sample_count) {
    case 0:
      // Are we in time to start sample 2?
      latency = TCNT0 - temp_ocr0;
      if (latency < latency_max) {
        // Yes, proceed normally
        
        // Set output compare for next sample
        OCR0 = temp_ocr0 + RS_TIMER0_SAMPLEPERIOD; 
        // Count this sample
        sample_count = 1;
        if (curr_sample) {
          samples = 1;
        } else {
          samples = 0;
        }
        return;
      }

      // We are too late for sample 2. We do not sample this bit at all anymore,
      // since a majority vote on 2 samples is nonsense.
      // Progress right to the first sample of the next bit
      OCR0 = temp_ocr0 + RS_TIMER0_BITPERIOD;
      // Reset sample counter
      sample_count = 0;
      // Parse the current bit
      bit_recv = curr_sample;
      break;

    case 1:
      // Are we in time to start sample 3?
      latency = TCNT0 - temp_ocr0;
      if (latency < latency_max) {
        // Yes, proceed normally

        // Set output compare for next sample
        OCR0 = temp_ocr0 + RS_TIMER0_SAMPLEPERIOD;

        // Count this sample
        if (curr_sample) {
          samples = (samples << 1) + 1;
        } else {
          samples <<= 1;
        }

        sample_count = 2;
        
        return;
      }

      // We are too late for sample 3. Take first sample as the bit received
      // (more appropriate than taking this sample)

      // Progress right to the first sample of the next bit
      OCR0 = temp_ocr0 + RS_TIMER0_BITPERIOD - RS_TIMER0_SAMPLEPERIOD;

      // Reset sample counter
      sample_count = 0;
      // Parse the current bit, taken from the value of the first sample
      bit_recv = samples;
      break;

    case 2:
    default: // Only case left, optimization
      // Set next sample to be at the first sample of the next bit
      OCR0 = temp_ocr0 + RS_TIMER0_BITPERIOD - 2 * RS_TIMER0_SAMPLEPERIOD;

      // Reset sample counter
      sample_count = 0;

      // Did we get this sample in time?
      // If our sample was very late, we shouldn't use its result
      latency = TCNT0 - temp_ocr0;
      if (latency < latency_max) {
        // Yes, proceed normally

        // Majority vote on the bitvalue
        if (curr_sample) {
          // This sample was higg
          if (samples) {
            // At least two samples were high, take it as high
            bit_recv = 1;
          } else {
            // Only this sample was high, take it as low
            bit_recv = 0;
          }

        } else {
          // This sample was low
          if (samples == 3) {
            // The other two were high, take it as high
            bit_recv = 1;
          } else {
            // At least two were low, take it as low
            bit_recv = 0;
          }
        }
      
      } else {
        // We didn't get the sample in time, take the second sample as the
        // current bit (it's the best timed sample)

        bit_recv = (samples & 1);
      }
  }

  switch (rs_state) {
    case RS_STARTBIT:
      // The current bitvalue should be 0, a start bit
      if (bit_recv) {
        // Correct

        // Set address
        rs_buf.addr[rs_buf.head] = rs_addr;

        // Next state, prepare recv
        rs_state = RS_IN_BYTE;
        recv = 1; // Bit used to detect fully shifted in byte
      } 
      else {
        // Incorrect; back to address pulse state
        // (and re-enable startbit interrupt; this was a glitch)
        rs_state = RS_ADDR;
        TCCR0 = 0; // Disable TIMER0
        TIFR = _BV(OCF0); // Clear any pending TIMER0 interrupt
        GIFR = _BV(INTF1); // Clear INT1 interrupt flag
        GICR |= _BV(INT1); // Enable interrupt on new rising edge
      }
      return;
      
    case RS_IN_BYTE:
      // The current bitvalue is another databit
      
      temp_recv = recv;
      if (temp_recv & (1 << 7)) {
        // This is the last databit, next is the stopbit
        rs_state = RS_STOPBIT;
      }

      // Shift in the bit
      temp_recv <<= 1;
      if (!bit_recv) {
        // We received a one, set it
        temp_recv |= 1;
      }
      
      recv = temp_recv;
      return;

    case RS_STOPBIT:
    default: // Couldn't be in a different state; optimization
      // This should be a stopbit

      temp_head = rs_buf.head;

      // Increase and check head pointer
      temp_new_head = temp_head;
      circ_buf_incr_ptr (&temp_new_head, RS_BUFSIZE);
      if (temp_new_head == rs_buf.tail) {
        // Overflow
        rs_buf.overflow = 1;

      } else {

        if (!bit_recv) {
          // Stopbit correct
          rs_buf.status[temp_head] = RS_OKAY;
          // Put the received byte in the buffer
          rs_buf.data[temp_head] = recv;

        } else {
          // Stopbit incorrect
          rs_buf.status[temp_head] = RS_FRAME_ERR;
        }
        
        // Push out received status, address and byte
        rs_buf.head = temp_new_head;
      }
      
      /* Back to address pulse state
       * INT1 is left disabled to prevent clocking in more data in one address
       * space. This prevents an endless stream of framing errors when the
       * RS-bus is powered off.
       */
      rs_state = RS_ADDR;
      TCCR0 = 0; // Disable TIMER0
      TIFR = _BV(OCF0); // Clear any pending TIMER0 interrupt
      return;
  }
}
