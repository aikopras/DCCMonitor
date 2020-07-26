/**
 * UART routine definitions
 *
 * Provides the definitions for doing I/O through the UARTs.
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

#ifndef FILE_UART_H
#define FILE_UART_H
#include <stdint.h>

/**
 * Initialise and enable UARTs
 *
 * Precondition: global interrupts disabled.
 */
extern void uart_init ();

/**
 * Transmit a byte through UART 0.
 *
 * This is a blocking routine. If the buffer is full, it will busy-wait.
 */
extern void uart0_put (const uint8_t c);

/**
 * Receive a byte through UART 0.
 * @return -1 when receive buffer is empty
 */
extern int16_t uart0_get ();

#endif // ndef FILE_UART_H
