/**
 * Automated test suite for the forwarding routines of the Communication protocol
 *
 * This suite tests the correctness of the implementation of the Communication
 * protocol forwarding routines in the board this board is connected to. It
 * should only be run by a board that is not directly connected to the PC,
 * because it sends a bogus frame from address 7 which would otherwise be
 * received by the PC. The upstream board should detect this as a "Chain Too
 * Long" and drop the bogus frame.
 *
 * This test only tests aspects of the communication protocol that are not
 * already checked by the "Communication protocol" test. That test should also
 * be run to check the forwarding of the communication protocol fully.
 *
 * It only consists of 2 tests: the "Chain Too Long" test described above, and a
 * test that tries to exactly fill the reception buffer of the upstream board,
 * to detect it correctly differentiates between an impossibly long packet and a
 * simple software buffer overflow.
 *
 * If traffic on the communication lines is high, this latter test will generate
 * a "Soft Overflow" nevertheless, so traffic should be low during the test. If
 * there still is a packet in the reception buffer before this test packet, it
 * will not trigger the test in comm_proto.c that would mark it as an impossibly
 * long packet.
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
#include "global.h"
#include "comm_proto.h"
#include "uart.h"
#include "test_comm_forward.h"

/**
 * State variable for the test. Stored in the global variable global_test_var,
 * see global.h
 *
 * Size of test storage: 1 byte.
 */
#define FORWARD_TEST_NUM (*(uint8_t *) ((void *) global_test_var))

/**
 * Start a test for the communication forwarding routines
 *
 * Call do_forward_test() to send the next test frame.
 *
 * A "Start forward test" management frame is sent to the PC.
 *
 * Argument test_proto: protocol number to use, range 1-15, no checking on range
 * done!
 */
void start_forward_test (const uint8_t test_proto) {
  // Set next test number to test 0
  FORWARD_TEST_NUM = 0;
  // Announce to the PC that a test will be run, with temporary protocol number
  // test_proto.
  comm_start_frame (MANAG_PROTO); // Management protocol
  comm_send_byte (MANAG_TEST); // Test functions
  comm_send_byte (MANAG_TEST_FORWARD); // "Communication forward" test
  comm_send_byte (test_proto); // Start test with protocol number used
  comm_end_frame ();
}

/**
 * Send next frame in test for communication forwarding 
 *
 * Argument test_proto: protocol number to use, range 1-15, no checking on range
 * done!
 *
 * @returns non-zero when test has ended, 0 when more frames need to be sent
 */
int8_t do_forward_test (const uint8_t test_proto) {
  uint8_t test_num;

  test_num = FORWARD_TEST_NUM;

  switch (test_num) {
    case 0:
      // Test if next board correctly generates "Chain too long" by sending it a
      // frame that appears to come from address 7.
      uart0_put (0xF0); // Frame start, address 7, proto 0
      uart0_put (0x70); // Correct parity byte
      break;

    case 1:
      // Test that next board correctly detects we overflow the input buffer
      // with one single frame. Obviously this depends on the input buffers
      // being equally big in this and the next board.
      uart0_put (test_proto | (1 << 7)); // Frame start byte
      for (uint8_t cnt = 0; cnt < UART1_RX_BUFSIZE - 2; cnt++) {
        uart0_put (0x00); // Just filler
      }
      uart0_put (test_proto); // Correct parity byte
      break;
    
    case 2:
    default: // Only case left, optimisation
      // We're done, send a management protocol frame indicating so
      comm_start_frame (MANAG_PROTO); // Management protocol
      comm_send_byte (MANAG_TEST); // Test functions
      comm_send_byte (MANAG_TEST_FORWARD); // "Communication forward" test
      comm_send_byte (0); // End test
      comm_end_frame ();
      return -1;
  }

  // Set next test number
  test_num++;
  FORWARD_TEST_NUM = test_num;
  return 0;
}     
