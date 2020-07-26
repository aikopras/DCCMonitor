/**
 * Inline functions for easier working with timers
 *
 * These functions are convenience functions for working with timers in the
 * ATmega162. They are written with static in- and output in mind, resulting in
 * compile-time generated values.
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

#ifndef FILE_TIMER_H
#define FILE_TIMER_H

#include <stdint.h>
#include <avr/io.h>
#include "global.h"

/**
 * Rounded integer division instead of the default truncated division
 */
static inline uint64_t div_round (uint64_t dividend, uint64_t divisor) {
  return ((dividend + divisor / 2) / divisor);
}

/**
 * Integer division rounding up
 */
static inline uint64_t div_round_up (uint64_t dividend, uint64_t divisor) {
  return ((dividend + divisor - 1) / divisor);
}

/**
 * Prescaler selection for TIMER0
 *
 * The lowest prescaler setting is chosen where the value 'scale' is still
 * representible in the 8-bit timer.
 */
static inline uint16_t timer0_prescale (uint32_t scale) {
  if (scale < UINT8_MAX) {
    return 1;
  } else if ((scale / 8) < UINT8_MAX) {
    return 8;
  } else if ((scale / 64) < UINT8_MAX) {
    return 64;
  } else if ((scale / 256) < UINT8_MAX) {
    return 256;
  } else {
    return 1024;
  }
}

/**
 * Prescaler selection for TIMER0, bit representation in TCCR0 register
 *
 * Returns the value of the three Clock Select bits.
 * 
 * The lowest prescaler setting is chosen where the value 'scale' is still
 * representible in the 8-bit timer.
 */
static inline uint8_t timer0_prescale_bits (uint32_t scale) {
  if (scale < UINT8_MAX) {
    return _BV(CS00);
  } else if ((scale / 8) < UINT8_MAX) {
    return _BV(CS01);
  } else if ((scale / 64) < UINT8_MAX) {
    return _BV(CS01) | _BV(CS00);
  } else if ((scale / 256) < UINT8_MAX) {
    return _BV(CS02);
  } else {
    return _BV(CS02) | _BV(CS00); // 1024
  }
}

/**
 * Returns the TIMER0 counter value corresponding to the given number of
 * clockticks
 *
 * Argument scale should be the same value as used when 
 * timer0_prescale{,_bits} () was called, for determining the prescaler value.
 */
static inline uint8_t timer0_period (uint32_t ticks, uint32_t scale) {
  return div_round (ticks, timer0_prescale (scale));
}

/**
 * Prescaler selection for TIMER1
 *
 * The lowest prescaler setting is chosen where the value 'scale' is still
 * representible in the 16-bit timer.
 */
static inline uint16_t timer1_prescale (uint32_t scale) {
  if (scale < UINT16_MAX) {
    return 1;
  } else if ((scale / 8) < UINT16_MAX) {
    return 8;
  } else if ((scale / 64) < UINT16_MAX) {
    return 64;
  } else if ((scale / 256) < UINT16_MAX) {
    return 256;
  } else {
    return 1024;
  }
}

/**
 * Prescaler selection for TIMER1, bit representation in TCCR1B register
 *
 * Returns the value of the three Clock Select bits.
 * 
 * The lowest prescaler setting is chosen where the value 'scale' is still
 * representible in the 16-bit timer.
 */
static inline uint8_t timer1_prescale_bits (uint32_t scale) {
  if (scale < UINT16_MAX) {
    return _BV(CS10);
  } else if ((scale / 8) < UINT16_MAX) {
    return _BV(CS11);
  } else if ((scale / 64) < UINT16_MAX) {
    return _BV(CS11) | _BV(CS10);
  } else if ((scale / 256) < UINT16_MAX) {
    return _BV(CS12);
  } else {
    return _BV(CS12) | _BV(CS10); // 1024
  }
}

/**
 * Returns the TIMER1 counter value corresponding to the given number of
 * clockticks
 *
 * Argument scale should be the same value as used when 
 * timer1_prescale{,_bits} () was called, for determining the prescaler value.
 */
static inline uint16_t timer1_period (uint32_t ticks, uint32_t scale) {
  return div_round (ticks, timer1_prescale (scale));
}

/**
 * Prescaler selection for TIMER2
 *
 * The lowest prescaler setting is chosen where the value 'scale' is still
 * representible in the 8-bit timer.
 */
static inline uint16_t timer2_prescale (uint32_t ticks) {
  if (ticks < UINT8_MAX) {
    return 1;
  } else if ((ticks / 8) < UINT8_MAX) {
    return 8;
  } else if ((ticks / 32) < UINT8_MAX) {
    return 32;
  } else if ((ticks / 64) < UINT8_MAX) {
    return 64;
  } else if ((ticks / 128) < UINT8_MAX) {
    return 128;
  } else if ((ticks / 256) < UINT8_MAX) {
    return 256;
  } else {
    return 1024;
  }
}

/**
 * Prescaler selection for TIMER2, bit representation in TCCR2 register
 *
 * Returns the value of the three Clock Select bits.
 * 
 * The lowest prescaler setting is chosen where the value 'scale' is still
 * representible in the 8-bit timer.
 */
static inline uint8_t timer2_prescale_bits (uint32_t ticks) {
  if (ticks < UINT8_MAX) {
    return _BV(CS20);
  } else if ((ticks / 8) < UINT8_MAX) {
    return _BV(CS21);
  } else if ((ticks / 32) < UINT8_MAX) {
    return _BV(CS21) | _BV(CS20);
  } else if ((ticks / 64) < UINT8_MAX) {
    return _BV(CS22);
  } else if ((ticks / 128) < UINT8_MAX) {
    return _BV(CS22) | _BV(CS20);
  } else if ((ticks / 256) < UINT8_MAX) {
    return _BV(CS22) | _BV(CS21);
  } else {
    return _BV(CS22) | _BV(CS21) | _BV(CS20); // 1024
  }
}

/**
 * Returns the TIMER2 counter value corresponding to the given number of
 * clockticks
 *
 * Argument scale should be the same value as used when 
 * timer2_prescale{,_bits} () was called, for determining the prescaler value.
 */
static inline uint8_t timer2_period (uint32_t ticks, uint32_t scale) {
  return div_round (ticks, timer2_prescale (scale));
}

/**
 * Prescaler selection for TIMER3
 *
 * The lowest prescaler setting is chosen where the value 'scale' is still
 * representible in the 16-bit timer.
 */
static inline uint16_t timer3_prescale (uint32_t ticks) {
  if (ticks < UINT16_MAX) {
    return 1;
  } else if ((ticks / 8) < UINT16_MAX) {
    return 8;
  } else if ((ticks / 16) < UINT16_MAX) {
    return 16;
  } else if ((ticks / 32) < UINT16_MAX) {
    return 32;
  } else if ((ticks / 64) < UINT16_MAX) {
    return 64;
  } else if ((ticks / 256) < UINT16_MAX) {
    return 256;
  } else {
    return 1024;
  }
}

/**
 * Prescaler selection for TIMER3, bit representation in TCCR3B register
 *
 * Returns the value of the three Clock Select bits.
 * 
 * The lowest prescaler setting is chosen where the value 'scale' is still
 * representible in the 16-bit timer.
 */
static inline uint16_t timer3_prescale_bits (uint32_t ticks) {
  if (ticks < UINT16_MAX) {
    return _BV(CS30);
  } else if ((ticks / 8) < UINT16_MAX) {
    return _BV(CS31);
  } else if ((ticks / 16) < UINT16_MAX) {
    return _BV(CS32) | _BV(CS31);
  } else if ((ticks / 32) < UINT16_MAX) {
    return _BV(CS32) | _BV(CS31) | _BV(CS30);
  } else if ((ticks / 64) < UINT16_MAX) {
    return _BV(CS31) | _BV(CS30);
  } else if ((ticks / 256) < UINT16_MAX) {
    return _BV(CS32);
  } else {
    return _BV(CS32) | _BV(CS31);
  }
}

/**
 * Returns the TIMER3 counter value corresponding to the given number of
 * clockticks
 *
 * Argument scale should be the same value as used when 
 * timer3_prescale{,_bits} () was called, for determining the prescaler value.
 */
static inline uint16_t timer3_period (uint32_t ticks, uint32_t scale) {
  return div_round (ticks, timer3_prescale (scale));
}

/**
 * Returns the 16-bit counter value corresponding to the given number of
 * "microticks" for a timer running at /1024 prescaler
 *
 * 1 million microticks is 1 clocktick.
 *
 * The function is geared towards usage with the useconds, mseconds and seconds
 * macro's defined below, for computing elapsed time between events.
 *
 * Microticks are chosen to get a proper result with integer arithmetic, without
 * unnecessary rounding errors.
 */
static inline uint16_t rtc_period (uint64_t uticks) {
  return div_round (uticks, 1024000000UL);
}

/**
 * Returns the 16-bit counter value corresponding to at least the given number
 * of "microticks" for a timer running at /1024 prescaler
 *
 * 1 million microticks is 1 clocktick.
 *
 * The function is geared towards usage with the useconds, mseconds and seconds
 * macro's defined below, for computing elapsed time between events.
 *
 * This function is useful when you want to guarantee that at least that number
 * of clockticks have passed, instead of the closest match as rtc_period() does.
 *
 * Microticks are chosen to get a proper result with integer arithmetic, without
 * unnecessary rounding errors.
 */
static inline uint16_t rtc_period_least (uint64_t uticks) {
  return div_round_up (uticks, 1024000000UL);
}


/**
 * Number of microticks in a microsecond
 *
 * Usage example: rtc_period (400 useconds)
 */
#define useconds * 1ULL * F_CPU
/**
 * Number of microticks in a millisecond
 *
 * Usage example: rtc_period (10 mseconds)
 */
#define mseconds * 1000ULL useconds
/**
 * Number of microticks in a second
 *
 * Usage example: rtc_period (3 seconds)
 */
#define seconds * 1000ULL mseconds

#endif // ndef FILE_TIMER_H
