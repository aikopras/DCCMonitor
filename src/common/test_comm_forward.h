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

#ifndef FILE_TEST_COMM_FORWARD_H
#define FILE_TEST_COMM_FORWARD_H

#include <stdint.h>

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
extern void start_forward_test (const uint8_t test_proto);

/**
 * Send next frame in test for communication forwarding 
 *
 * Argument test_proto: protocol number to use, range 1-15, no checking on range
 * done!
 *
 * @returns non-zero when test has ended, 0 when more frames need to be sent
 */
extern int8_t do_forward_test (const uint8_t test_proto);

#endif // ndef FILE_TEST_COMM_FORWARD_H
