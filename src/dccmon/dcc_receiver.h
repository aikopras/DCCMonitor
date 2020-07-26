/**
 * DCC receiver header file
 *
 * Defines routines for interfacing to the DCC reception mechanism.
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

#ifndef FILE_DCC_RECEIVER_H
#define FILE_DCC_RECEIVER_H

#include <stdint.h>

/**
 * Initialise DCC receiver
 *
 * Sets the I/O-pin correctly and starts the interrupt-driven sampler. 
 *
 * It is assumed the uC is in it's default settings with regard to periphery.
 * The DDR register is only explicitly programmed to reduce the chance of a
 * shortcut when a programming error is made.
 */
extern void dcc_init();

/**
 * Check if buffer has data available.
 * Returns true when there is no data available and a call to dcc_get would
 * block waiting for data. Returns false otherwise.
 */
extern uint8_t dcc_would_block ();

/**
 * Check and clear overflow indication.
 * Returns true when an overflow has occured since the last time this routine
 * was called.
 */
extern uint8_t dcc_overflow_status ();

/**
 * Get a byte of data from the buffer.
 * If needed, block waiting for new data.
 * Data returned is a byte indicating the length of the next DCC frame,
 * followed by that many bytes of DCC data. When the length is available,
 * the whole frame is available without blocking.
 */
extern unsigned char dcc_get ();

#endif // ndef FILE_DCC_RECEIVER_H
