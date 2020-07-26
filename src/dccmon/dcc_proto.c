/**
 * The routine that gets the DCC data from the buffer and wraps it in the
 * correct Communication protocol frame.
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

#include <stdint.h>
#include "../common/global.h"
#include "../common/comm_proto.h"
#include "dcc_receiver.h"
#include "dccmon.h"
#include "dcc_proto.h"

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
int8_t dcc_send() {
  int8_t retval;
  uint8_t dcc_length;

  retval = 0;

  // Check for overflow on DCC bus
  if (dcc_overflow_status()) {
    // Overflow
    comm_start_frame (MANAG_PROTO); // Management protocol
    comm_send_byte (MANAG_BUS_OVF); // Overflow of monitored bus
    comm_end_frame ();
    retval = 1;
  }
    
  if (!dcc_would_block()) {
    // We have a DCC packet to send
    
    dcc_length = dcc_get();

    // Send the frame
    comm_start_frame (DCC_PROTO); // DCC protocol
    for (; dcc_length > 0; dcc_length--) {
      comm_send_byte (dcc_get());
    }
    comm_end_frame ();
    
    retval = 1;
  }

  return retval;
}

/**
 * monitor_send() is called by common/main.c to send out monitored data. It is
 * aliased to dcc_send().
 */
int8_t monitor_send () __attribute__ ((alias("dcc_send")));
