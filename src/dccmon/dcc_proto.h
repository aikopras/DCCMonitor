/**
 * DCC transmission header file
 *
 * Defines the routine for sending DCC data to the PC
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
#ifndef FILE_DCC_PROTO_H
#define FILE_DCC_PROTO_H

#include <stdint.h>

/**
 * Get received DCC data from the buffer if available, and send it to the PC
 * over the Communication protocol.
 *
 * Only one frame is sent to the PC, even if more are available.
 *
 * Also checks for and reports overflows. If an overflow report is sent to the
 * PC, a data frame is sent as well to relieve pressure on the buffer.
 *
 * @returns non-zero when a frame was sent, 0 otherwise.
 */
extern int8_t dcc_send();

#endif // ndef FILE_DCC_PROTO_H
