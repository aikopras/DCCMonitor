/**
 * RS-bus reception routines header file
 *
 * Defines the routines for handling the reception of RS-bus data
 *
 * Note: the RS-bus reception routines define the TIMER1_OVF interrupt handler.
 * Should it be needed for other purposes as well, some method to integrate the
 * purposes has to be written.
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

#ifndef FILE_RS_RECEIVER_H
#define FILE_RS_RECEIVER_H

#include <stdint.h>

/**
 * Initialise RS-bus monitoring process.
 *
 * Sets I/O-pin functions and interrupts.
 *
 * Should be called once, before any other RS-bus reception routines.
 *
 * It is assumed the uC is in it's default settings with regard to periphery.
 * The DDR register is only explicitly programmed to reduce the chance of a
 * shortcut when a programming error is made.
 */
extern void rs_init();

/**
 * Report and clear whether a buffer overflow occured
 */
extern int8_t rs_overflow_status();

/**
 * Get the status of the next received byte on the RS-bus
 *
 * For some statuses, address and/or data are not defined.
 *
 * Possible statuses:
 * RS_OKAY          Byte received okay.
 * RS_FRAME_ERR     Databyte had a framing error (zero endbit). Data undefined.
 * RS_ADDR_ERR      We received more than the expected 130 address pulses.
 *                  Reported only once. Address and data undefined.
 * 
 * Returns 0 when there is no next byte in the buffer, a statusbyte otherwise.
 */
extern uint8_t rs_get_status();

/**
 * Get the address that sent the next received byte on the RS-bus
 *
 * May be undefined, dependent on rs_get_status().
 */
extern uint8_t rs_get_addr ();

/**
 * Get the next received byte and advance the buffer pointer to the next byte
 *
 * May be undefined dependent on rs_get_status(), but still needs to be called
 * to get to the next byte.
 *
 * After calling this routine, rs_get_status() and rs_get_address() give
 * information relating to the next byte. Status and address of this byte is
 * lost.
 */
extern uint8_t rs_get_data();

#endif // ndef FILE_RS_RECEIVER_H
