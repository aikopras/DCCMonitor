/**
 * Global definitions for the DCC Monitor project
 *
 * This is typically the file to tinker with when you want to change settings
 *
 * This file is part of DCC Monitor.
 *
 * Copyright 2007, 2008 Peter Lebbing <peter@digitalbrains.com>
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

#ifndef FILE_GLOBAL_H
#define FILE_GLOBAL_H
#include <avr/io.h>

/**
 * CPU clock frequency (Hertz)
 *
 * If you change the frequency, you should check all timers and related concepts
 * to see if rounding errors don't cause inacceptable deviations. Not a
 * completely trivial task!
 */
#define F_CPU 11059200UL

// UART baudrate (Hertz == bps)
#define UART_BAUD 57600

/**
 * UART0 transmit buffer size
 *
 * Used for sending out the communication protocol
 * Maximum of 256 (pointers are 8-bit)
 * A power of 2 results in more optimal code
 */
#define UART0_TX_BUFSIZE 16

/**
 * UART1 receive buffer size
 *
 * Used for receiving the communication protocol from a daisy-chained board.
 * The absolute lower boundary is the size of the largest frame (on the wire)
 * plus the frame start byte of the next frame.
 *
 * In reality it should be bigger, because a maximally sized frame might not get
 * sent out immediately on reception. 
 *
 * Maximum of 256 (pointers are 8-bit)
 * A power of 2 results in more optimal code
 */
#define UART1_RX_BUFSIZE 32

/**
 * Maximum number of databytes in a frame
 *
 * Frames with 12 databytes are 16 bytes long on the wire
 */
#define MAX_FRAME_SIZE 12

/**
 * Definitions for using the LEDs on the board
 *
 * The LEDs should be connected to consecutive pins on one port. The following
 * definitions give the location of the LEDs.
 *
 * F.e., if LEDs are connected to PD3-PD7:
 * LEDS_PORT PORTD
 * LEDS_DDR DDRD
 * LEDS_SHIFT 3
 * LEDS_COUNT 5
 */

/**
 * The port to which the LEDs are connected
 */
#define LEDS_PORT PORTC
/**
 * The DDR register for LEDS_PORT
 */
#define LEDS_DDR DDRC
/**
 * The lowest port pin number connected to a LED
 */
#define LEDS_SHIFT 3
/**
 * The number of LEDs connected
 */
#define LEDS_COUNT 3
/**
 * Bitmask with 1's for every bit on the port that is connected to a LED.
 * (automatically calculated, no need to change).
 */
#define LEDS_MASK (((1 << LEDS_COUNT) - 1) << LEDS_SHIFT)

/**
 * Macro to initialise the port for the LEDs.
 *
 * It turns off the LEDs and sets the port pins to be output.
 */
#define leds_init() do { LEDS_PORT &= ~LEDS_MASK; LEDS_DDR |= LEDS_MASK; } while (0)

/**
 * Macro to turn a LED on. Arg: number of the LED, 0-based.
 */
#define led_on(x) (LEDS_PORT |= (1 << x) << LEDS_SHIFT)

/**
 * Macro to turn a LED off. Arg: number of the LED, 0-based.
 */
#define led_off(x) (LEDS_PORT &= ~((1 << x) << LEDS_SHIFT))

/**
 * Macro to toggle a LED. Arg: number of the LED, 0-based.
 */
#define led_toggle(x) (LEDS_PORT ^= (1 << x) << LEDS_SHIFT)

/**
 * Communication protocol definitions
 */
#define MANAG_PROTO 0 // Management protocol has number 0
// First bytes
#define MANAG_HELLO 0 // Management proto hello message
#define MANAG_BUS_OVF 1 // Management protocol "Overflow on monitored bus" message
#define MANAG_TEST  6 // Management protocol test functions
#define MANAG_DCC_OOB 7 // Management protocol DCC out-of-band data

// MANAG_TEST second bytes
#define MANAG_TEST_COMM 0 // Management protocol "Communication protocol" test
#define MANAG_TEST_FORWARD 1 // Management protocol "Communication forward" test

// MANAG_DCC_OOB second bytes
#define MANAG_DCC_NO_ACC_FILTER 0 // No longer filtering on Accessory Decoders
#define MANAG_DCC_ACC_FILTER 1 // Filtering on Accessory Decoders

/**
 * Globally available variable for miscellaneous purpose
 *
 * It is used in interrupt contexts, so access should be
 * atomic.
 *
 * Current uses:
 * bit 0: comm_proto.c: CHAIN_OVERFLOW_BIT
 * bit 1: dccmon/dcc_receiver.c: DCC_OVERFLOW_BIT
 */
extern volatile uint8_t global_prot_var;

/**
 * Globally available variable for miscellaneous purpose
 *
 * Not used in interrupt contexts.
 *
 * Current uses:
 * bit 0: test_dispatch.h: RUN_COMM_TEST
 * bit 1: test_dispatch.h: RUN_FORWARD_TEST
 * bit 2: test_dispatch.h: TEST_CONTINUOUSLY
 * bit 3: test_dispatch.c: COMM_TEST_RUNNING
 * bit 4: test_dispatch.c: FORWARD_TEST_RUNNING
 * bit 5: dccmon/dcc_send_filter.c: FILTER_STATE_BIT
 */
extern uint8_t global_var;

/**
 * Storage for test functions (to keep counters etc)
 *
 * There is always maximally one test running, so tests share it.
 */
extern uint8_t global_test_var[];

/**
 * The size of the global test var array, in bytes.
 * 
 * Should be big enough to hold the biggest test.
 */
#define GLOBAL_TEST_VAR_SIZE 2

/**
 * Initialize monitor
 *
 * This function should be implemented by the specific protocol monitor the
 * firmware is being built for.
 */
extern void monitor_init();

/**
 * Send data from monitor
 *
 * This function should be implemented by the specific protocol monitor the
 * firmware is being built for. It should send out data collected by the monitor
 * via the Communication Protocol.
 *
 * Returns non-zero when a frame has been sent out, to record activity of the
 * Communication Protocol.
 */
extern int8_t monitor_send();

/**
 * Key handler
 *
 * This function is called every 10 milliseconds by the main loop. It can be
 * used to filter key input with keys_update() and take action depending on the
 * keypresses.
 *
 * It can be implemented by the specific protocol monitor the firmware is being
 * built for. If not, a sane default is used.
 *
 * Should return non-zero when a frame has been sent by this function, zero if
 * no frame was sent.
 */
extern int8_t handle_keys();

/**
 * Inline function to do circular buffer arithmetic.
 *
 * Compiler optimises out the test for buffer size (provided it's constant,
 * obviously).
 */
static inline void circ_buf_incr_ptr (uint8_t* pointer, const uint8_t bufsize) {
  if (bufsize & (bufsize - 1)) {
    // bufsize is not a power of 2; use simple increase
    if (*pointer >= bufsize - 1) {
      *pointer = 0;
    } else { \
      (*pointer)++;
    }
  } else {
    // bufsize is a power of 2; use optimized increase
    *pointer = (*pointer + 1) & (bufsize - 1);
  }
}

/**
 * Inline function for appending the byte in argument c to a circular buffer with overflow check.
 *
 * Returns non-zero when the buffer is full.
 */
static inline int8_t append_circ_buf (volatile uint8_t buf[], const uint8_t bufsize, const uint8_t c, uint8_t* head, const uint8_t tail) {
  if (*head == tail) {
    // Buffer full
    return -1;
  }
  buf[*head] = c;
  circ_buf_incr_ptr (head, bufsize);
  return 0;
}

#endif // ndef FILE_GLOBAL_H
