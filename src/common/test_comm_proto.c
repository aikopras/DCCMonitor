/**
 * Automated test suite for the Communication protocol
 *
 * This suite tests the correctness of the implementation of the Communication
 * protocol by sending a stream of frames, both correct and incorrect, and
 * checking on the receiving side if the frames are as expected.
 *
 * The results are constant to automate the process.
 *
 * The test is stateful: loss of packets causes the receiver to go out-of-sync
 * and report errors for a correct stream.
 *
 * This is a developer's tool that is only incidentally used. No fixed protocol
 * number is defined for the test; use a number that's not otherwise used on the
 * board running the test.
 *
 * Note: the tests do not include some useful test cases for daisy-chaining.
 * Use test_comm_forward.c for that.
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

#include <stdint.h>
#include "comm_proto.h"
#include "uart.h"
#include "global.h"
#include "test_comm_proto.h"

/**
 * State variables for the test. Stored in the global variable global_test_var,
 * see global.h
 *
 * Size of structure: 2 bytes.
 */
struct comm_test_var {
  /**
   * Length of next frame to send.
   * A value of MAX_FRAME_SIZE + 1 indicates special tests.
   */
  uint8_t len;
  /**
   * Bit position of high bit in next frame to send.
   * When len == MAX_FRAME_SIZE + 1, number of special test to run.
   */
  uint8_t bitpos;
};

// Store state in global_test_var
#define COMM_TEST_VAR (*(struct comm_test_var *) ((void *) global_test_var))

/**
 * Start a test for the communication protocol
 *
 * Call do_comm_test() to send the next test frame.
 *
 * This test sends packets of all sizes, and lets a 1-bit run over the 8th bit, to test
 * proper reassembly of the bytes at the receiving end. Lastly, a few special
 * tests are run to check proper error detection on the receiving side.
 *
 * Argument test_proto: protocol number to use, range 1-15, no checking on range
 * done!
 */
void start_comm_test (const uint8_t test_proto) {
  COMM_TEST_VAR.len = COMM_TEST_VAR.bitpos = 0;
  // Announce to PC that a test will be run, with temporary protocol number
  // test_proto.
  comm_start_frame (MANAG_PROTO); // Management protocol
  comm_send_byte (MANAG_TEST); // Test functions
  comm_send_byte (MANAG_TEST_COMM); // "Communication protocol" test
  comm_send_byte (test_proto); // Start test with protocol number used
  comm_end_frame ();
}

/**
 * Send next frame in test for communication protocol
 *
 * Argument test_proto: protocol number to use, range 1-15, no checking on range
 * done!
 *
 * @returns non-zero when test has ended, 0 when more frames need to be sent
 */
int8_t do_comm_test (const uint8_t test_proto) {
  uint8_t len, bitpos;

  len = COMM_TEST_VAR.len;
  bitpos = COMM_TEST_VAR.bitpos;

  if (len == MAX_FRAME_SIZE + 1) {
    // Done with regular frames, send a few specials

    switch (bitpos) {
      case 1:
        // Deliberate parity error
        // Excludes the data from the parity to test that it is included in
        // calculation
        uart0_put (test_proto | (1 << 7)); // Frame start byte
        uart0_put (0x7F);
        uart0_put (0x00);
        uart0_put (test_proto); // Parity
        break;

      case 2:
        // Deliberate parity error
        // Excludes the hi-bits portion from the parity to test that it is
        // included in calculation
        uart0_put (test_proto | (1 << 7)); // Frame start byte
        uart0_put (0x00);
        uart0_put (0x01);
        uart0_put (test_proto); // Parity
        break;

      case 3:
        // Deliberate parity error
        // Excludes the frame start byte from the parity to test that it is
        // included in calculation
        uart0_put (test_proto | (1 << 7)); // Frame start byte
        uart0_put (0x00);
        uart0_put (0x00);
        uart0_put (0x00); // Parity
        break;

      case 4:
        // Incomplete frame: just a frame start byte
        uart0_put (test_proto | (1 << 7));
        break;

      case 5:
        // Malformed packet: a 3-byte frame doesn't exist
        uart0_put (test_proto | (1 << 7)); // Frame start byte
        uart0_put (0x00);
        uart0_put (test_proto); // Parity
        break;

      case 6:
        // Impossibly big frame
        comm_start_frame (test_proto);
        for (uint8_t cnt = 0; cnt < MAX_FRAME_SIZE + 1; cnt++) {
          comm_send_byte (0x00);
        }
        comm_end_frame ();
        break;

      case 7:
      default: // Only possible state left, optimisation
        // We're done, send a management protocol frame indicating so
        comm_start_frame (MANAG_PROTO); // Management protocol
        comm_send_byte (MANAG_TEST); // Test functions
        comm_send_byte (MANAG_TEST_COMM); // "Communication protocol" test
        comm_send_byte (0); // End test
        comm_end_frame ();
        return -1;
    }
    COMM_TEST_VAR.bitpos = bitpos + 1;
    return 0;
  }

  if (bitpos == len) {
    // Next time start with a packet one byte bigger, and bitpos 1.
    COMM_TEST_VAR.len = len + 1;
    COMM_TEST_VAR.bitpos = 1;
  } else {
    // Next time use next bitpos
    COMM_TEST_VAR.bitpos = bitpos + 1;
  }

  // Send test frame
  comm_start_frame (test_proto);

  // Special case: empty frame
  if (len == 0) {
    comm_end_frame();
    return 0;
  }

  // Little optimisation: testing for zero is cheaper
  bitpos--;

  // First a stream of zeroes up to the position of bitpos
  for (; bitpos != 0; bitpos--, len--) {
    comm_send_byte (0);
  }

  // One byte with the high bit set
  comm_send_byte (0x80);
  len--;

  // And finish with a stream of zeroes

  for (; len != 0; len--) {
    comm_send_byte (0);
  }

  comm_end_frame ();
  return 0;
}
