/**
 * Global definitions for the DCC monitor of the DCC Monitor project
 *
 * This is typically the file to tinker with when you want to change settings.
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


#ifndef FILE_DCCMON_H
#define FILE_DCCMON_H

/**
 * DCC buffer size
 * Maximum of 256 (pointers are 8-bit)
 * A power of 2 results in more optimal code
 * It should probably be able to hold at least 2 DCC messages plus 2 length
 * bytes. That way another frame can be received while the first is transmitted,
 * plus some leeway for any delays.
 */
#define DCC_BUFSIZE 16

#define DCC_PROTO 1 // DCC protocol number for Communication protocol

// DCC input port
#define DCC_INPUT_PORT PIND
// DCC input port DDR register
#define DCC_INPUT_DDR DDRD
// DCC input pin
#define DCC_INPUT_PIN 3

#endif // ndef FILE_DCCMON_H
