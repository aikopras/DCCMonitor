/**
 * Definitions for routines for debouncing the keys and interpreting the result
 *
 * The current state of the keys connected to the board is checked periodically.
 * After a key has been in the same state for 4 consecutive polls, and that
 * state is different then before, it is reported to have changed.
 *
 * Keys should be connected to one I/O-port on consecutive pins. Keys should
 * connect to ground on key press, and have an open circuit when not pressed
 * down.
 *
 * If for instance 4 keys are connected to PD3-6, the correct values for the
 * constants below are:
 * KEYS_PORT PIND
 * KEYS_DDR DDRD
 * KEYS_SHIFT 3
 * KEYS_COUNT 4
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

#ifndef FILE_KEYS_H
#define FILE_KEYS_H
#include <stdint.h>

/**
 * Port to which the keys are connected.
 */
#define KEYS_PORT PORTC
/**
 * PIN register for KEYS_PORT
 */
#define KEYS_PIN PINC
/**
 * DDR register for KEYS_PORT
 */
#define KEYS_DDR DDRC
/**
 * Pin number (0-based) to which the lowest numbered key is connected. Used to
 * shift the keys to be from bit 0 and upwards.
 */
#define KEYS_SHIFT 0
/**
 * Number of keys connected
 */
#define KEYS_COUNT 3
/**
 * Bitmask with 1's for every bit on the port that is connected to a key.
 * (automatically calculated, no need to change).
 */
#define KEYS_MASK (((1 << KEYS_COUNT) - 1) << KEYS_SHIFT)

/**
 * Initialise keys
 *
 * Sets the I/O-pins correctly
 */
extern void keys_init ();

/**
 * Poll state of the keys and report differences
 *
 * Should be called periodically; 10 milliseconds is a good period. The timing
 * does not have to be very precise.
 *
 * Returns a bitfield with a set bit if a key changed and a cleared bit if it is
 * still the same. Bit 0 is key 0, bit 1 is key 1, etcetera.
 */
extern uint8_t keys_update ();

/**
 * Get current, debounced state of the keys
 *
 * Returns a bitfield with a set bit if a key is pressed down, and a cleared bit
 * if it is not pressed down. Bit 0 is key 0, bit 1 is key 1, etcetera.
 */
extern uint8_t keys_get_state ();

#endif // ndef FILE_KEYS_H
