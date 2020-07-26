/** 
 * This is the main loop for the firmware of the DCC Monitor project.
 *
 * It initialises the monitor and sends any data received by the monitor.
 *
 * Daisy-chained frames are forwarded.
 *
 * After powerup or a reset, the hardware is initialised and a "Hello"
 * management protocol message is sent.
 *
 * If INCLUDE_TESTS is defined, the keys on the board can be used to start and
 * stop automated tests. Key 0 starts and stops the "Communication protocol
 * test", key 1 starts and stops the "Communication forward test" and key 2
 * toggles between continuous testing and one-shot testing.
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
 * You should have received a copy of the GNU General Public License along with
 * DCC Monitor.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "global.h"
#include "uart.h"
#include "comm_proto.h"
#include "test_dispatch.h"
#include "timer.h"
#include "keys.h"

// Global miscellaneous variables; see global.h
volatile uint8_t global_prot_var;
uint8_t global_var;

// Storage for test functions; see global.h
uint8_t global_test_var[GLOBAL_TEST_VAR_SIZE];

/*
 * Optimisation part
 *
 * Short version: main() is unused, main_loop() takes it's place, but main_loop
 * can never return, it should be an infinite loop.
 *
 * Long version:
 *
 * Normally, the startup code calls main, and if main returns disregards the
 * return code and goes into an infinite loop.
 *
 * We do not return.
 *
 * The normal code flow is shortcircuited: before the call to main in section
 * .init9, we jump into our function main_loop() in section .init8. main_loop()
 * never returns, so the code in sections .init9 and .finiX are never reached.
 *
 * The function main() is still defined to keep the linker happy.
 *
 * The most important reason is that we want to define "noreturn" on our main
 * loop, since this triggers the optimiser to make the function much more
 * efficient, mainly in stack usage. A bug in GCC causes a warning to be emitted
 * if main() is declared "noreturn" and it is running in C99 or GNU99
 * compatibility mode.  This construction avoids that warning, so it doesn't
 * confuse developers.
 *
 * As an added bonus, it saves two bytes of SRAM on the stack: main() is CALLed,
 * pushing the return address on the stack. This is avoided.
 */

static void jump_main_loop() __attribute__ ((naked, section (".init8"), used));

static void jump_main_loop() {
  asm volatile ("jmp main_loop");
}

int main() {}

// main_loop() never returns, tell GCC so it can optimise with it
// It is only called from inline assembly, so mark it used to avoid it being
// removed.
static void main_loop () __attribute__ ((noreturn, used));

/*
 * End of optimisation part
 */

/**
 * Default key handler
 *
 * If tests are included, keys start and stop tests. If tests are not included,
 * keys are not used at all.
 *
 * This is a weakly bound symbol; you can override it with a handle_keys()
 * function of your own. main_loop() calls handle_keys() every 10 milliseconds.
 *
 * Always returns zero meaning no frame was sent from this function.
 */
int8_t handle_keys () __attribute__ ((weak));
int8_t handle_keys () {
  // When INCLUDE_TESTS is defined, keys start and stop tests. Otherwise, this
  // function does nothing.
#ifdef INCLUDE_TESTS
  uint8_t key_change, keys_pressed;

  // Get possible keypresses
  key_change = keys_update();
  if (key_change) {
    // A key was pressed or released

    // Which keys have just been pressed?
    keys_pressed = key_change & keys_get_state ();

    if (keys_pressed & (1 << 0)) {
      // Key 0 was pressed; change "Communication protocol test" state
      test_toggle (RUN_COMM_TEST);
    }

    if (keys_pressed & (1 << 1)) {
      // Key 1 was pressed; change "Communication forward test" state
      test_toggle (RUN_FORWARD_TEST);
    }

    if (keys_pressed & (1 << 2)) {
      // Key 2 was pressed; change "Continuously test" state

      test_toggle (TEST_CONTINUOUSLY);
    }
  }
#endif

  return 0;
}

/**
 * Main loop
 *
 * Initialises the hardware, calls the initialisation functions for subsystems,
 * sends a "Hello" and manages the data flow between the subsystems.
 *
 * It never returns.
 */
static void main_loop () {
  uint8_t active, last_active; // For idle checking
  uint16_t now; // Current "real" time
  /**
   * Last time we paused the outgoing serial stream for synchronization
   * purposes.
   * It's the higher byte of the 16-bit RTC at that time.
   *
   * This means that the period of 2 seconds we check for should be a sensible
   * period to compare against on the high byte of the RTC. This should not be a
   * problem.
   */
  uint8_t last_pause;
  uint8_t pause_diff; // Difference between now and last_pause
  uint8_t last_centisec_time; // The last time 10 msec had passed
  uint8_t centisec_time_diff; // Time passed since last_centisec_time

  // Pullups enabled (unconnected pins; defined level -> saves power)
  // init_monitor() below will change some of this later.
	PORTA = 255;
  PORTB = 255;
  PORTC = 255;
  PORTD = 255;

  // Init Real Time Clock
  TCCR1B = _BV(CS12) | _BV (CS10); // Prescaler /1024, start in normal mode

	sei(); // Enable interrupts

	leds_init();
  keys_init();
  uart_init();
  monitor_init();
  
  // Send hello
  comm_start_frame (MANAG_PROTO); // Management protocol
  comm_send_byte (MANAG_HELLO); // Management hello message
  comm_end_frame ();

  last_active = 1; // We sent the hello message

  // Get "real" time
  cli(); // Start of critical section
  now = TCNT1;
  sei(); // End of critical section

  last_pause = now >> 8; // Just an initialisation value

  last_centisec_time = (uint8_t) now; // Just an initialisation value
  
  for (;;) {
    active = 0; // Initialise to inactive

    /* We check the keys on the board every 10 milliseconds, and send maximally
     * one test frame every 10 milliseconds.
     * To save storage and computation time, we only work on the lower 8 bits of
     * the "Real" Time Clock. This means the loop has to run often enough to
     * have centisec_time_diff be bigger than rtc_period (10 mseconds) before
     * centisec_time_diff wraps back to 0. This is a very loose constraint, and
     * I don't foresee any problems, but it should be noted.
     */
    // Is it time yet?
    centisec_time_diff = now - last_centisec_time; // 8-bit arithmetic!
    if (centisec_time_diff > rtc_period (10 mseconds)) {
      // Yes, another 10 msec have passed
      
      active |= handle_keys();
#ifdef INCLUDE_TESTS
      active |= test_send();
#endif

      // Update timestamp
      last_centisec_time = (uint8_t) now;
    }

    // Forward daisy-chained frame, record activity
    active |= comm_forward(); 

    // Send data from the monitor, record activity
    active |= monitor_send();

    // Get "real time"
    cli(); // Start of critical section
    now = TCNT1;
    sei(); // End of critical section

    if (!active && last_active) {
      // Last time we sent a packet, now we didn't
      // Initialise idle timer

      comm_start_idle_timer ();
    
    } else if (!active && !last_active) {
      // We're still idle

      comm_check_idle_timer (now);
    }

    // We insert an 8-bitperiod pause on the outgoing serial stream about
    // every 2 seconds. This pause allows resynchronization of startbits on sender
    // and receiver if they somehow got out-of-sync.
    pause_diff = (now >> 8) - last_pause;
    if (pause_diff > (rtc_period (2 seconds) >> 8)) {
      // 2 seconds passed since last pause, do it again

      comm_sync_pause();

      // We paused now
      last_pause = (now >> 8);
    }

    // Set last_active for next run
    last_active = active;
  }	
}
