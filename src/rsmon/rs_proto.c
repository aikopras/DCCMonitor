/**
 * The routine that gets the RS-bus data from the buffer and wraps it in the
 * correct Communication protocol frame.
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

#include <stdint.h>
#include "../common/global.h"
#include "../common/comm_proto.h"
#include "rs_receiver.h"
#include "rsmon.h"
#include "rs_proto.h"

/**
 * Get received RS-bus data from the buffer if available, and send it to the PC
 * over the Communication protocol.
 *
 * Only one frame is sent to the PC, even if more are available.
 *
 * Also checks for and reports overflows. If an overflow report is sent to the
 * PC, a data frame is sent as well to relieve pressure on the buffer.
 *
 * @returns non-zero when a frame was sent, 0 otherwise.
 */
int8_t rs_send () {
  uint8_t rs_status, rs_addr;
  int8_t retval;

  retval = 0;

  if (rs_overflow_status()) {
    // Overflow occured
    comm_start_frame (MANAG_PROTO); // Management protocol
    comm_send_byte (MANAG_BUS_OVF); // Overflow on monitored bus
    comm_end_frame ();
    retval = 1;
  }
  
  if ((rs_status = rs_get_status()) != 0) {
    // We've got an RS packet

    comm_start_frame (RS_PROTO); // Start RS-bus frame

    switch (rs_status) {
      case RS_OKAY:
        // Send address and data
        rs_addr = rs_get_addr();

        // Make address 0-based and range-check
        rs_addr--;
        if (rs_addr & (1 << 7)) {
          // Out of range

          if (rs_addr == 0xFF) {
            // Address was 0, send error code
            comm_send_byte (RS_ZERO_ADDR);

          } else {
            // Address was >128, send error code
            comm_send_byte (RS_ADDR_OVF);
          }

        } else {
          // Address in range, send address
          comm_send_byte (rs_addr);
        }

        // Send data
        comm_send_byte (rs_get_data());
        break;

      case RS_FRAME_ERR:
        // Framing error, send error code and address
        comm_send_byte (RS_FRAME_ERR);
        rs_addr = rs_get_addr();
        rs_addr--;
        comm_send_byte (rs_addr);
        // We still need to call rs_get_data() to get the next contents of the
        // buffer next time
        rs_get_data();
        break;

      case RS_ADDR_ERR:
      default: // No other codes exist; optimisation
        // We counted too many address pulses, report
        // (this error is sent only once in the lifetime of the program)
        comm_send_byte (RS_ADDR_ERR);
        // We still need to call rs_get_data() to get the next contents of the
        // buffer next time
        rs_get_data();
        break;
    }

    // End the RS protocol frame
    comm_end_frame();
    retval = 1;
  }

  return retval;
}

/**
 * monitor_send() is called by common/main.c to send out monitored data. It is
 * aliased to rs_send().
 */
int8_t monitor_send () __attribute__ ((alias("rs_send")));
