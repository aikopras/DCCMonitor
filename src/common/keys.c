/**
 * Routines for debouncing the keys and interpreting the result
 *
 * The current state of the keys connected to the board is checked periodically.
 * After a key has been in the same state for 4 consecutive polls, and that
 * state is different then before, it is reported to have changed.
 *
 * The counters can be implemented in some simple boolean logic which makes the
 * routine short and fast. 4 counts can be simply implemented, more would
 * require more computation.
 *
 * Each time, the current state of the keys is compared with the last reported
 * state. Only when the state of a key is different, a counter is increased. If
 * the state is the same as the last reported state, the counter is cleared to 
 * 0.
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
#include <avr/io.h>
#include "keys.h"

/**
 * Bitfield containing the current debounced state of the keys. The bit is set
 * if a key is pressed down, and cleared if it is not pressed down. Bit 0 is key
 * 0, bit 1 is key 1, etcetera.
 */
static uint8_t key_state;

/**
 * Initialise keys
 *
 * Sets the I/O-pins correctly
 */
void keys_init () {
  // Input
  KEYS_DDR &= ~(KEYS_MASK);
  // Pullups enabled
  KEYS_PORT |= KEYS_MASK;
}

/**
 * Poll state of the keys and report differences
 *
 * Should be called periodically; 10 milliseconds is a good period. The timing
 * does not have to be very precise.
 *
 * Returns a bitfield with a set bit if a key changed and a cleared bit if it is
 * still the same. Bit 0 is key 0, bit 1 is key 1, etcetera.
 */
uint8_t keys_update () {
  /**
   * Counters for debouncing keys. Each key has a 2-bit counter. The higher bit
   * of key x is bit x in count1, the lower bit is bit x in count0.
   */
  static uint8_t count1, count0;
  /**
   * Difference between key_state and current reading of the keys. A set bit is
   * a key that is different from key_state.
   */
  uint8_t diff;
  /**
   * Keys that have changed after debouncing (return value).
   */
  uint8_t change;
  uint8_t temp_count1, temp_count0;

  /* Get the difference between filtered and unfiltered state
   * If the following expressions are combined, GCC does 16-bit arithmetic;
   * I can't find a casting combination that prevents this.
   * Sheesh.
   */
  diff = ~KEYS_PIN;
  diff &= KEYS_MASK;
  diff >>= KEYS_SHIFT;
  diff ^= key_state;

  // Little optimisation: matches most of the time
  if (diff == 0) {
    // Nothing changed, clear counters and return
    count1 = count0 = 0;
    return 0;
  }

  temp_count1 = count1;
  temp_count0 = count0;

  // Check if this is the 4th pass in which a key is different
  // If a bit is set in diff, count1 and count0, that difference is accepted as
  // the new filtered state
  change = diff & temp_count1 & temp_count0;

  // Increase all counters for set bits in diff; clear all counters for
  // cleared bits
  // Keys at the count of 4 that are still in diff overflow back to 0
  temp_count1 ^= temp_count0;
  temp_count1 &= diff;
  temp_count0 = ~temp_count0;
  temp_count0 &= diff;

  count1 = temp_count1;
  count0 = temp_count0;

  // Update key_state with possible changed keys
  key_state ^= change;

  // Return report on changes
  return change;
}

/**
 * Get current, debounced state of the keys
 *
 * Returns a bitfield with a set bit if a key is pressed down, and a cleared bit
 * if it is not pressed down. Bit 0 is key 0, bit 1 is key 1, etcetera.
 */
uint8_t keys_get_state () {
  return key_state;
}
