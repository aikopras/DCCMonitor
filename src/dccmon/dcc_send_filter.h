/**
 * Variant on dcc_proto.c that implements filtering out Accessory Decoder
 * packets.
 *
 * This file overrides the default handle_keys() so key 2 toggles the filter.
 * When on, the filter only sends DCC packets addressed to an Accessory Decoder
 * to the PC, and discards the other packets received from the DCC bus.
 * 
 * For integration with automated tests, the monitor_init() alias in
 * dcc_receiver.c is only aliased weakly to dcc_init(). If automated tests are
 * included, this file has a stronger binding to monitor_init() that calls
 * dcc_init(), but also calls the automated test function that would otherwise
 * be switched on and off with key 2.
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

#ifndef FILE_DCC_SEND_FILTER_H
#define FILE_DCC_SEND_FILTER_H

#include <stdint.h>

/**
 * Get received DCC data from the buffer if available, and send it to the PC
 * over the Communication protocol.
 *
 * Only one frame is sent to the PC, even if more are available.
 *
 * If filtering is enabled, send only Accessory Decoder packets.
 *
 * Also checks for and reports overflows. If an overflow report is sent to the
 * PC, a data frame is sent as well to relieve pressure on the buffer.
 *
 * @returns non-zero when a frame was sent, 0 otherwise.
 */
extern int8_t dcc_send_filter ();

#endif // ndef FILE_DCC_SEND_FILTER_H
