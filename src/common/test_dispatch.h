/** 
 * Routines handling the running of automated tests.
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
 * You should have received a copy of the GNU General Public License along with
 * DCC Monitor.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FILE_TEST_DISPATCH_H
#define FILE_TEST_DISPATCH_H

/**
 * Bits used as state bits in test_dispatch.c and used as argument to
 * test_toggle().
 */
#define RUN_COMM_TEST (1 << 0)
#define RUN_FORWARD_TEST (1 << 1)
#define TEST_CONTINUOUSLY (1 << 2)

/**
 * Toggle tests on and off.
 *
 * Call with one of RUN_COMM_TEST, RUN_FORWARD_TEST or TEST_CONTINUOUSLY. The
 * first call starts the specified action, a subsequent call stops it.
 */
extern void test_toggle (const uint8_t toggles);

/**
 * Send a test frame, if appropriate.
 *
 * Returns immediately when there is no test requested to be run.
 *
 * Returns non-zero if a frame was sent, 0 otherwise.
 */
extern int8_t test_send ();

#endif // ndef FILE_TEST_DISPATCH_H
