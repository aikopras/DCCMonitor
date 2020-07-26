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

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "global.h"
#include "test_comm_proto.h"
#include "test_comm_forward.h"
#include "test_dispatch.h"

/**
 * State bits in TEST_STATE_VAR (additional to those defined in
 * test_dispatch.h).
 */
#define COMM_TEST_RUNNING (1 << 3)
#define FORWARD_TEST_RUNNING (1 << 4)

/**
 * Storage for state bits above and in test_dispatch.h.
 */
#define TEST_STATE_VAR global_var

/**
 * Toggle tests on and off.
 *
 * Call with one of RUN_COMM_TEST, RUN_FORWARD_TEST or TEST_CONTINUOUSLY. The
 * first call starts the specified action, a subsequent call stops it.
 */
void test_toggle (const uint8_t toggles) {
  TEST_STATE_VAR ^= toggles;
}

/**
 * Send a test frame, if appropriate.
 *
 * Returns immediately when there is no test requested to be run.
 *
 * Returns non-zero if a frame was sent, 0 otherwise.
 */
int8_t test_send () {
  uint8_t any_tests_needed;
  uint8_t temp_test_state;

  temp_test_state = TEST_STATE_VAR;

  // Optimisation: if no tests need to be done, return immediately. This matches
  // most of the time.
  any_tests_needed = temp_test_state;
  any_tests_needed &= RUN_COMM_TEST | RUN_FORWARD_TEST | COMM_TEST_RUNNING | FORWARD_TEST_RUNNING;

  if (!any_tests_needed) {
    return 0;
  };

  if (temp_test_state & COMM_TEST_RUNNING) {
    // Do a communication test frame

    if (do_comm_test (15)) {
      // Test ended
      temp_test_state &= ~COMM_TEST_RUNNING;
      
      // Turn LED 0 off for visual confirmation
      led_off (0);

      // If we want the "Communication forward test" to be run, we start it
      // here so they alternate
      if (temp_test_state & RUN_FORWARD_TEST) {
        start_forward_test (15);
        temp_test_state |= FORWARD_TEST_RUNNING;

        // Turn LED 1 on for visual confirmation
        led_on (1);
        
        if (!(temp_test_state & TEST_CONTINUOUSLY)) {
          // Don't run another one
          temp_test_state &= ~RUN_FORWARD_TEST;
        }
      }
    }

    TEST_STATE_VAR = temp_test_state;
    return 1;
  }

  if (temp_test_state & FORWARD_TEST_RUNNING) {
    // Do a forward test frame

    if (do_forward_test (15)) {
      // Test ended
      temp_test_state &= ~FORWARD_TEST_RUNNING;

      // Turn LED 1 off for visual confirmation
      led_off (1);

      // If we want the "Communication protocol test" to be run, we start it
      // here so they alternate
      if (temp_test_state & RUN_COMM_TEST) {
        start_comm_test (15);
        temp_test_state |= COMM_TEST_RUNNING;


        // Turn LED 0 on for a visual confirmation
        led_on (0);

        if (!(temp_test_state & TEST_CONTINUOUSLY)) {
          // Don't run another one
          temp_test_state &= ~RUN_COMM_TEST;
        }
      }
    }

    TEST_STATE_VAR = temp_test_state;
    return 1;
  }
    
  if (temp_test_state & RUN_COMM_TEST) { 
    // Start a communication test
    start_comm_test (15);
    temp_test_state |= COMM_TEST_RUNNING;

    // Turn LED 0 on for a visual confirmation
    led_on (0);

    if (!(temp_test_state & TEST_CONTINUOUSLY)) {
      // Don't run another one
      temp_test_state &= ~RUN_COMM_TEST;
    }

    TEST_STATE_VAR = temp_test_state;
    return 1;
  } 
  
  // When we got here, apparently RUN_FORWARD_TEST was 1, because the others
  // from the optimisation test were not. No need to check.

  // Start a "Communication forward test"
 
  start_forward_test (15);
  temp_test_state |= FORWARD_TEST_RUNNING;

  // Turn LED 1 on for a visual confirmation
  led_on (1);

  if (!(temp_test_state & TEST_CONTINUOUSLY)) {
    // Don't run another one
    temp_test_state &= ~RUN_FORWARD_TEST;
  }
  
  TEST_STATE_VAR = temp_test_state;
  return 1;
}
