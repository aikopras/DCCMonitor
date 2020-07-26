/**
 * Header file for the automated test suite for the Communication protocol
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

#ifndef FILE_TEST_COMM_PROTO_H
#define FILE_TEST_COMM_PROTO_H

#include <stdint.h>

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
extern void start_comm_test (const uint8_t test_proto);

/**
 * Send next frame in test for communication protocol
 *
 * Argument test_proto: protocol number to use, range 1-15, no checking on range
 * done!
 *
 * @returns non-zero when test has ended, 0 when more frames need to be sent
 */
extern int8_t do_comm_test (const uint8_t test_proto);

#endif // ndef FILE_TEST_COMM_PROTO_H
