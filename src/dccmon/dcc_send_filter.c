/**
 * Variant on dcc_proto.c that implements filtering out Accessory Decoder
 * packets.
 *
 * This file overrides the default handle_keys() so key 2 toggles the filter.
 * When on, the filter only sends DCC packets addressed to an Accessory Decoder
 * to the PC, and discards the other packets received from the DCC bus.
 * 
 * For integration with automated tests, the monitor_init() alias in
 * dcc_receiver.c is only aliased weakly to dcc_init(). If automated tests are
 * included, this file has a stronger binding to monitor_init() that calls
 * dcc_init(), but also calls the automated test function that would otherwise
 * be switched on and off with key 2.
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

#include <stdint.h>
#include "../common/global.h"
#include "../common/comm_proto.h"
#include "../common/test_dispatch.h"
#include "../common/keys.h"
#include "dcc_receiver.h"
#include "dccmon.h"
#include "dcc_send_filter.h"

// This bit in this variable is 1 when we filter on Accessory Decoder packets
#define FILTER_STATE_VAR global_var
#define FILTER_STATE_BIT (1 << 5)

#ifdef INCLUDE_TESTS
// When we have tests included, we override monitor_init() to also flag tests to
// be run continuously, since we have only two keys left for test functions, and
// thus miss a key to toggle continuous testing.
void monitor_init () {
  // Initialise DCC
  dcc_init();
  // And set tests to run continuously
  test_toggle (TEST_CONTINUOUSLY);
}
#endif

/**
 * Handle keypresses
 *
 * This function is called every 10 milliseconds by the main loop.
 *
 * Keypresses are obtained from keys_update(). Pressing key 0 or 1 start and
 * stop automated tests. Key 2 starts and stops filtering on Accessory Decoder
 * packets from the DCC bus.
 *
 * Returns non-zero when a frame has been sent to the PC, zero otherwise.
 */
int8_t handle_keys () {
  uint8_t key_change, keys_pressed;
  int8_t retval;

  retval = 0;

  // Get possible keypresses
  key_change = keys_update();
  if (key_change) {
    // A key was pressed or released

    // Which keys have just been pressed?
    keys_pressed = key_change & keys_get_state ();

    if (keys_pressed & (1 << 2)) {
      // Toggle filtering state
      
      if (!(FILTER_STATE_VAR & FILTER_STATE_BIT)) {
        // Start filtering
        
        FILTER_STATE_VAR |= FILTER_STATE_BIT;
        
        // Visual confirmation
        led_on (2);

        // Send management message for indication to user
        comm_start_frame (MANAG_PROTO);
        comm_send_byte (MANAG_DCC_OOB); // DCC out-of-band data
        comm_send_byte (MANAG_DCC_ACC_FILTER); // Accessory Decoder filter on
        comm_end_frame();
        
        retval = 1;

      } else {
        // Stop filtering
        
        FILTER_STATE_VAR &= ~FILTER_STATE_BIT;

        // Visual confirmation
        led_off (2);
        
        // Send management message for indication to user
        comm_start_frame (MANAG_PROTO);
        comm_send_byte (MANAG_DCC_OOB); // DCC out-of-band data
        comm_send_byte (MANAG_DCC_NO_ACC_FILTER); // Accessory Decoder filter on
        comm_end_frame();

        retval = 1;
      }
    }

    // When INCLUDE_TESTS is defined, keys start and stop tests.
#ifdef INCLUDE_TESTS
    if (keys_pressed & (1 << 0)) {
      // Key 0 was pressed; change "Communication protocol test" state
      test_toggle (RUN_COMM_TEST);
    }

    if (keys_pressed & (1 << 1)) {
      // Key 1 was pressed; change "Communication forward test" state
      test_toggle (RUN_FORWARD_TEST);
    }
#endif
  }

  return retval;
}


/**
 * Get received DCC data from the buffer if available, and send it to the PC
 * over the Communication protocol.
 *
 * Only one frame is sent to the PC, even if more are available.
 *
 * If filtering is enabled, send only Accessory Decoder packets.
 *
 * Also checks for and reports overflows. If an overflow report is sent to the
 * PC, a data frame is sent as well to relieve pressure on the buffer.
 *
 * @returns non-zero when a frame was sent, 0 otherwise.
 */
int8_t dcc_send_filter () {
  int8_t retval;
  uint8_t dcc_length;
  // Address partition of packet; this is the first byte in the packet
  uint8_t dcc_addr_part; 

  retval = 0;

  // Check for overflow on DCC bus
  if (dcc_overflow_status()) {
    // Overflow
    comm_start_frame (MANAG_PROTO); // Management protocol
    comm_send_byte (MANAG_BUS_OVF); // Overflow of monitored bus
    comm_end_frame ();
    retval = 1;
  }
    
  if (!dcc_would_block()) {
    // We have a DCC packet to send
    
    dcc_length = dcc_get();

    if (!(FILTER_STATE_VAR & FILTER_STATE_BIT)) {
      // Unfiltered output
      // Send the frame

      comm_start_frame (DCC_PROTO);
      for (; dcc_length > 0; dcc_length--) {
        comm_send_byte (dcc_get());
      }
      comm_end_frame ();
      
      // We sent a frame
      return 1;
    }

    // Filter on accessory decoder packets
    

    // Note that dcc_length is necessarily always minimally 1. There is no
    // waveform thinkable that would not clock in a single databyte. So
    // checking the length is redundant for that one byte.

    dcc_addr_part = dcc_get();

    if ((dcc_addr_part & 0xC0) == 0x80) {
      /* The first address byte has the form:
       * 10XXXXXX
       * That's the accessory decoder space
       * Send packet
       */

      comm_start_frame (DCC_PROTO);
      
      comm_send_byte (dcc_addr_part); // Already read that
      dcc_length--;

      // Send the rest of the packet
      for (; dcc_length > 0; dcc_length--) {
        comm_send_byte (dcc_get());
      }
      comm_end_frame();

      // We sent a frame
      return 1;
    
    } else {
      // Not an accessory decoder packet, read rest of packet and discard

      dcc_length--; // Already read one byte
      for (; dcc_length > 0; dcc_length--) {
        dcc_get();
      }
    }
  }

  return retval;
}

/**
 * monitor_send() is called by common/main.c to send out monitored data. It is
 * aliased to dcc_send_filter().
 */
int8_t monitor_send () __attribute__ ((alias("dcc_send_filter")));
