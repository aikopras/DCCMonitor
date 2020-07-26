/**
 * Communication protocol header file
 *
 * Provides the routines for communicating with other boards and the PC in the
 * Communication protocol.
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

#ifndef FILE_COMM_PROTO_H
#define FILE_COMM_PROTO_H

#include <stdint.h>

/**
 * Initialise idle timer for Idle Frame management
 *
 * Should be called when no more frames are available to send, followed by polls
 * of comm_check_idle_timer().
 */
extern void comm_start_idle_timer();

/**
 * Check idle timer and send Idle Frame if needed
 *
 * Should be called when no frames have been sent since start_idle_timer().
 * Argument 'now' gives the current "real" time. When the UART goes idle and
 * stays idle for long enough, this routine will send an Idle Frame once.
 *
 * Precondition: comm_start_idle_timer() has been called after the data has been
 * sent to the UART transmission routines. No data should be sent to the UART
 * after the call to comm_start_idle_timer() !
 */
extern void comm_check_idle_timer(const uint16_t now);

/**
 * Pause outgoing transmission for long enough to allow the receiver to
 * synchronize on the startbit.
 */
extern void comm_sync_pause ();

/**
 * Starts a communication protocol frame on the outgoing serial port with 
 * address 0 and protocol proto.
 */
extern void comm_start_frame (const uint8_t proto);

/**
 * Sends a byte of frame data over the outgoing serial port.
 *
 * Precondition: comm_start_frame() has been called earlier.
 */
extern void comm_send_byte (const uint8_t c);

/**
 * Ends a communication protocol frame on the outgoing serial port. 
 *
 * Precondition: comm_start_frame() has been called earlier.
 */
extern void comm_end_frame ();

/**
 * Forward an incoming frame from the daisy-chain, if available.
 *
 * Normally, only one frame is sent, either an error or a data frame. "Soft
 * overflow on incoming daisy-chain" is an exception: we always try to send a
 * packet from the buffer in combination with that error frame, because
 * apparently the pressure on the buffer is high and we should try to alleviate
 * it.
 *
 * Management frames are hardcoded for efficiency. Should the protocol be
 * changed such that the management frames:
 * - "Soft/hard overflow on incoming daisy-chain"
 * - "Chain too long"
 * - "Malformed packet"
 * are changed, this routine will need altering as well.
 *
 * @returns non-zero when a frame was sent, 0 otherwise.
 */
extern int8_t comm_forward();

#endif // ndef FILE_COMM_PROTO_H
