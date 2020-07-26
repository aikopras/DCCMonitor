/**
 * Global definitions for the RS-bus monitor of the DCC Monitor project
 *
 * This is typically the file to tinker with when you want to change settings.
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

#ifndef FILE_RSMON_H 
#define FILE_RSMON_H 

/**
 * Size of buffer to hold RS-bus data.
 *
 * Denotes the total number of RS datapackets that can be held. A datapacket
 * consists of status, address and nibble data.
 *
 * Maximum of 256 (8-bit pointers)
 * Powers of 2 result in more optimal code.
 */
#define RS_BUFSIZE 16

#define RS_PROTO 2 // RS-bus protocol number for Communication protocol

/**
 * Status codes for rs_get_status() and error codes sent to the PC
 *
 * These are defined here because of the overlap.
 *
 * See rs_send()
 */
// Status codes returned by rs_get_status()
#define RS_OKAY 1
// Codes used for both rs_get_status() and error codes sent to PC
// High bit needs to be set
#define RS_FRAME_ERR 0x82
#define RS_ADDR_ERR 0x83

// Error codes sent to PC
// High bit needs to be set
#define RS_ZERO_ADDR 0x80
#define RS_ADDR_OVF 0x81

/**
 * Because the RS monitoring uses two interrupt lines, changing the pins used on
 * the uC for reception is quite more involved than just changing a definition
 * in a header file. Consequently, there is no provision here comparable to
 * DCC_INPUT_PORT in the DCC monitor header file.
 */

#endif // ndef FILE_RSMON_H 
