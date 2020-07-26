/**
 * RS transmission header file
 *
 * Defines the routine for sending RS-bus data to the PC
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

#ifndef FILE_RS_PROTO_H
#define FILE_RS_PROTO_H

#include <stdint.h>

/**
 * Get received RS-bus data from the buffer if available, and send it to the PC
 * over the Communication protocol.
 *
 * @returns non-zero when a frame was sent, 0 otherwise.
 *
 * RS-bus frames have the following format:
 *
 * Normal frames: two bytes, address and a byte of data
 *
 * Address is the address of the responder on the RS-bus. High bit always
 * cleared (address range 0-127, RS-bus uses 1-128)
 *
 * Data format (bits): P T1 T0 N D3 D2 D1 D0
 * P      : bit parity, odd
 * T1 T0  : responder type:
 *          0 0 : switching receiver, no responder (is this useful?)
 *          0 1 : switching receiver with responder
 *          1 0 : stand-alone responder
 *          1 1 : reserved
 * N      : Nibble bit, 0 = lower nibble, 1 = higher nibble
 * D3..D0 : input pin state, 0 = passive, 1 = active
 *          N=0 : D3..D0 = E4..E1
 *          N=1 : D3..D0 = E8..E5
 *
 * -------------------------------
 * 
 * Special frames all start with a byte with the high bit set (to differentiate
 * them from normal frames):
 *
 * One byte frame:
 * 0x83 (RS_ADDR_ERR)     Addressing error: we observed more than 130 address
 *                        pulses. This message is sent only once in the lifetime
 *                        of the monitor process.
 *
 * Two byte frames:
 * 0x80 (RS_ZERO_ADDR) data
 *      Databyte sent after the address pulse for address 0; this is not a
 *      regular address.
 *
 * 0x81 (RS_HIGH_ADDR) data
 *      Databyte sent "from" address 129 or higher; not a regular address.
 *
 * 0x82 (RS_FRAME_ERR) address
 *      The specified (0-based) address sent data with a framing error (low
 *      endbit). Data is not included, just the 0-based address. Two special
 *      addresses exist:
 *      0xFF  The non-regular address 0 (cf. RS_ZERO_ADDR)
 *      0x80  The non-regular address 129 or higher (cf. RS_HIGH_ADDR)
 */
extern int8_t rs_send();

#endif // ndef FILE_RS_PROTO_H
