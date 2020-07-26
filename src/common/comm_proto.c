/**
 * Communication protocol routines
 *
 * Provides the routines for sending and receiving frames in the communication 
 * protocol of the DCC Monitor project.
 *
 * This file is part of DCC Monitor.
 *
 * Copyright 2007,2008 Peter Lebbing <peter@digitalbrains.com>
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
#include <avr/interrupt.h>
#include "global.h"
#include "uart.h"
#include "timer.h"
#include "comm_proto.h"

// Transmission variables
static uint8_t hi_bits; // most signnificant bits of sent bytes
static uint8_t parity; // Parity byte to be sent at end of packet (MSB is a don't-care)

// Reception variables

/**
 * Circular buffer holding received bytes from daisy-chain.
 *
 * Frame start bytes are in the range 0x80-0xEF. Instead of a frame start byte,
 * there can be a 1-byte error code in the range 0xF0-0xFF, as defined below.
 */
static struct {
  volatile uint8_t buf[UART1_RX_BUFSIZE];
  volatile uint8_t head, tail;
} uart1_rx_buffer;

/**
 * Variable and bit holding "daisy-chain overflow" flag
 * This flag is set when the buffer holding incoming communication protocol 
 * packets overflows
 */
#define CHAIN_OVERFLOW_VAR global_prot_var
#define CHAIN_OVERFLOW_BIT (1 << 0)

/**
 * Error codes passed by the interrupt routine
 * 
 * These error codes are passed in the circular buffer uart1_rx_buffer.
 *
 * Note that the correspondence of the lower nibble with management protocol
 * error codes is just for recognisability and not a requirement. In total,
 * there's room for 16 error codes in the lower nibble, the upper nibble is
 * required to be 0xF.
 */
#define RECV_ERR_MALFORMED 0xF2
#define RECV_ERR_CHAIN_LONG 0xF3
#define RECV_ERR_H_OVERFLOW 0xF5

/**
 * The "real time" we saw UART 0 become idle, or:
 * 0 - UART still active
 * 1 - Already sent an Idle Frame
 *
 * This means that "real" times 0 and 1 are mapped to 255.
 *
 * Only the most significant byte of the 16-bit RTC.
 *
 * Used for Idle Frame management: after a certain idle time, an Idle Frame is
 * sent.
 */
static uint8_t last_byte_time; 

/**
 * Initialise idle timer for Idle Frame management
 *
 * Should be called when no more frames are available to send, followed by polls
 * of check_idle_timer().
 */
void comm_start_idle_timer () {
  last_byte_time = 0;
}

/**
 * Check idle timer and send Idle Frame if needed
 *
 * Should be called when no frames have been sent since start_idle_timer().
 * Argument 'now' gives the current "real" time. When the UART goes idle and
 * stays idle for long enough, this routine will send an Idle Frame once.
 *
 * Precondition: start_idle_timer() has been called after the data has been sent
 * to the UART transmission routines. No data should be sent to the UART after
 * the call to start_idle_timer() !
 */
void comm_check_idle_timer (const uint16_t now) {
  uint8_t idle_time;
  uint8_t now_hi; // High 8-bits of now 
  uint8_t temp_last;

  now_hi = now >> 8;
  temp_last = last_byte_time;

  if (temp_last == 0) {
    // Last time we checked, the UART was still active
    
    // Check again
    if (UCSR0A & _BV(TXC0)) {
      // It's idle now
      
      // Save the idle time and return
      if (now_hi == 0 || now_hi == 1) {
        // Special values, use 255
        last_byte_time = 255;
      } else {
        last_byte_time = now_hi;
      }
    }
    return;
  } else if (temp_last == 1) {
    // We already sent an idle frame
    return;
  }

  // we use the fact that unsigned integer arithmetic wraps around nicely
  idle_time = now_hi - temp_last;

  // Did 100 ms already pass?
  if (idle_time > (rtc_period (100 mseconds) >> 8)) {
    // Yes, send idle frame

    uart0_put (0x80); 
    // Set last_byte_time to special value 1
    last_byte_time = 1;
  }
  return;
}

/**
 * Pause outgoing transmission for long enough to allow the receiver to
 * synchronize on the startbit.
 */
void comm_sync_pause () {
  uint8_t ucsr0b_save; // We need to restore the UDRIE0 bit correctly
  uint8_t pause_start; // When the pause started
  uint8_t pause_duration; // How long we've paused so far
  
  // Stop sending new bytes on the UART; save state of UDRIE0 though
  cli(); // Start of critical section
  ucsr0b_save = UCSR0B; // Save state
  UCSR0B &= ~(_BV(UDRIE0)); // Disable transmit interrupt
  sei(); // End of critical section

  // Wait until the last byte was transmitted completely
  loop_until_bit_is_set (UCSR0A, TXC0);
  
  // Start a byteperiod long pause
  pause_start = TCNT1L;

  /* The pause is computed as follows:
   * We want an 8-bitperiod pause
   * An 8-bitperiod pause is (F_CPU / UART_BAUD) * 8 clockticks long
   * rtc_period_least takes microticks as it's argument, so multiply by
   * 1,000,000.
   * Lastly, since a pause_start value of, f.e., 100 could actually be
   * 100.99, we need to add 1 so we don't take it one too low.
   *
   * The period should fit in the lower 8 bits of the RTC because of the
   * 8-bit arithmetic. This should not be a problem at all.
   *
   * Note that 16-bit arithmetic would also mean a critical section, because
   * TCNT1H would have to be read. This is unacceptable in a busy-wait loop.
   */
  do {
    pause_duration = TCNT1L - pause_start;
  } while (pause_duration < rtc_period_least (F_CPU * 9ULL * 1000000ULL / UART_BAUD) + 1);

  // Restore state of UDRIE0
  if (ucsr0b_save & _BV(UDRIE0)) {
    UCSR0B |= _BV(UDRIE0);
  }
}

/**
 * Start a frame on the outgoing serial port.
 *
 * A byte containing address 0, protocol proto and a flag bit indicating frame
 * start is sent, and variables are initialised.
 */
void comm_start_frame (const uint8_t proto) {
	parity = (proto & 15) | (1 << 7) ;// Frame start byte: address 0, proto, MSB set
	uart0_put (parity); 
	hi_bits = (1 << 7); // The bit is used to see when 7 bytes have been sent
}

/**
 * Send a byte over the outgoing serial port, stripping and remembering the
 * high bit, and sending the hi_bits when appropriate.
 */
void comm_send_byte (const uint8_t c) {
  uint8_t temp_hi, temp_par;

  // Put MSB in hi_bits and clear it
  if (c & (1 << 7)) {
    temp_hi = (hi_bits >> 1) | (1 << 7);
  } else {
    temp_hi = (hi_bits >> 1);
  }
	uart0_put (c & 127); // Send MSB-stripped byte
	temp_par = parity ^ c; // XOR parity and sent byte
  
  if (temp_hi & 1) {
    // 7 bytes have been sent, send the full hi_bits byte
    temp_hi >>= 1; // Shift out the bit used to count sent bytes
    uart0_put (temp_hi); // Send hi_bits
    temp_par ^= temp_hi; // XOR parity and sent byte
    temp_hi = (1 << 7); // The bit is used to see when 7 bytes have been sent
  }

  hi_bits = temp_hi;
  parity = temp_par;
}

/**
 * End a frame on the outgoing serial port, sending parity information and,
 * if appropriate, remaining hi_bits.
 */
void comm_end_frame () {
  uint8_t temp_par, temp_hi;

  temp_par = parity;

  // Do we still need to send a hi_bits byte?
  if (hi_bits != (1 << 7)) {
    // Yes, there's unsent data in hi_bits

    temp_hi = hi_bits;

    // Shift the bits into position
    while (!(temp_hi & 1)) {
      temp_hi >>= 1; // Keep shifting
    }
    temp_hi >>= 1; // Shift out the bit used to count sent bytes

    uart0_put (temp_hi); // Send hi_bits
    temp_par ^= temp_hi; // XOR parity and sent byte
  }

  // Send parity byte
  uart0_put (temp_par & 127); // Bit 7 is meaningless and stripped
}

/**
 * Inline function called by comm_forward() for reporting overflow.
 *
 * @returns non-zero when overflow has been reported.
 */
static inline int8_t report_overflow() __attribute ((always_inline));
static inline int8_t report_overflow() {
  // Try to avoid critical section
  if (!(CHAIN_OVERFLOW_VAR & CHAIN_OVERFLOW_BIT)) {
    // No overflow
    return 0;
  }

  // Overflow, need critical section to clear it
  cli();
  CHAIN_OVERFLOW_VAR &= ~CHAIN_OVERFLOW_BIT;
  sei(); // End of critical section

  uart0_put (0x80);
  uart0_put (0x04);
  uart0_put (0x00);
  uart0_put (0x04);
  return -1;
}

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
int8_t comm_forward() {
  uint8_t temp_head, temp_tail;
  uint8_t frame_start; // Frame start byte
  uint8_t parity_correct; // Correction to be applied to parity byte
  uint8_t old_data, new_data; // Bytes read from buffer

  // Check for a frame to transmit
  temp_head = uart1_rx_buffer.head;
  temp_tail = uart1_rx_buffer.tail;
  if (temp_head == temp_tail) {
    // No frame to transmit, we're done
    // Check for overflow and return true if that caused a packet to be sent 
    return report_overflow();
  }

  frame_start = uart1_rx_buffer.buf[temp_tail];
  circ_buf_incr_ptr (&temp_tail, UART1_RX_BUFSIZE);
  uart1_rx_buffer.tail = temp_tail;

  while (temp_tail == temp_head || ((new_data = uart1_rx_buffer.buf[temp_tail]) & (1 << 7))) {
    // This is a one-byte frame

    switch (frame_start) {

      case 0x80:
        // This is a frame idle command; skip it
        if (temp_tail == temp_head) {
          // We're done
          // Check for overflow and return true if that caused a packet to be sent  
          return report_overflow();
        }
        // Continue with the new frame
        frame_start = new_data;
        circ_buf_incr_ptr (&temp_tail, UART1_RX_BUFSIZE);
        uart1_rx_buffer.tail = temp_tail;
        break;

      case RECV_ERR_CHAIN_LONG:
        // Send the management frame "Chain too long"
        // Complete management frame: 0x80 0x03 0x00 0x03
        uart0_put (0x80);
        uart0_put (0x03);
        uart0_put (0x00);
        uart0_put (0x03);
        // Check for overflow
        report_overflow();
        return -1; // We sent a packet

      case RECV_ERR_H_OVERFLOW:
        // Send the management frame "Hard overflow on incoming daisy-chain"
        // Complete management frame: 0x80 0x05 0x00 0x05
        uart0_put (0x80);
        uart0_put (0x05);
        uart0_put (0x00);
        uart0_put (0x05);
        // Check for overflow
        report_overflow();
        return -1; // We sent a packet

      case RECV_ERR_MALFORMED:
      default:
        /*
         * Any 1-byte frame that's not one of the cases above is malformed
         * Send the management frame "Malformed packet"
         * Complete management frame: 0x80 0x02 0x00 0x02
         */
        uart0_put (0x80);
        uart0_put (0x02);
        uart0_put (0x00);
        uart0_put (0x02);
        // Check for overflow
        report_overflow();
        return -1; // We sent a packet
    }
  }

  /* It's not a 1-byte frame, and new_data holds the first databyte of the frame (or
   * the parity byte if it's an empty frame). However, tail still points at that
   * first byte.
   */
  circ_buf_incr_ptr (&temp_tail, UART1_RX_BUFSIZE);
  uart1_rx_buffer.tail = temp_tail;
  

  /* 
   * Increase frame address and send out (remember parity change)
   * Note that the interrupt handler already detected "chain too long", so the
   * increase never overflows.
   */
  parity_correct = frame_start;
  frame_start += (1 << 4);
  parity_correct ^= frame_start;
  uart0_put (frame_start);
  // This assignment helps the compiler generate better code
  // (experimentally determined)
  temp_tail = uart1_rx_buffer.tail;

  // Now output all databytes verbatim, but change the parity when we're at the
  // end
  old_data = new_data;
  while (temp_head != temp_tail && !((new_data = uart1_rx_buffer.buf[temp_tail]) & (1 << 7))) {
    // There's another byte in the frame, so the old one wasn't the parity

    circ_buf_incr_ptr (&temp_tail, UART1_RX_BUFSIZE);
    uart1_rx_buffer.tail = temp_tail;
    uart0_put (old_data); // Send databyte
    // This assignment helps the compiler generate better code
    // (experimentally determined)
    temp_tail = uart1_rx_buffer.tail;
    old_data = new_data;
  }

  // old_data now holds the parity byte; since we changed the address in the
  // frame start byte, we need to adjust it.
  uart0_put (old_data ^ parity_correct);
  // And we're done
  // Check for overflow
  report_overflow();
  return -1; // We sent a packet
}


/**
 * Inline function used by daisy-chain reception interrupt handler
 *
 * Drop current frame, emit error code and start a new frame in the buffer.
 *
 * err: error code
 * framebyte: first byte of new frame (is checked for validity)
 * write_pos: circular buffer pointer to next position to write received byte 
 * head: start of current frame in buffer
 * tail: to check for buffer overflow
 * 
 * If write_pos is don't-care, just pass some non-static local variable to
 * avoid writing to the SRAM.
 *
 * Passing a constant as "framebyte" means the check is done at compile-time
 * (obviously, passing a constant >= 0xF0 is rather silly).
 */
static inline void errcode_and_framebyte (const uint8_t, const uint8_t, uint8_t*, 
    uint8_t, const uint8_t) __attribute__ ((always_inline));
static inline void errcode_and_framebyte (const uint8_t err, const uint8_t framebyte, 
    uint8_t* write_pos, uint8_t head, const uint8_t tail) 
{
  uint8_t next_write_pos;

  // Overwrite previous frame
  next_write_pos = head;
  circ_buf_incr_ptr (&next_write_pos, UART1_RX_BUFSIZE);

  if (next_write_pos == tail) {
    /* 
     * Overflow, abandon
     *
     * No room to report the error, but still room to put the received byte at
     * the head; if it's a frame start byte, we will receive the next frame.
     * Otherwise, the rest of the frame will be discarded.
     */

    CHAIN_OVERFLOW_VAR |= CHAIN_OVERFLOW_BIT;
    
    if (framebyte >= 0xF0) {
      /* 
       * This is the start of an incoming frame with address 7. The chain is too 
       * long, but we have no room to report it. Just ignore this frame.
       * Mark the byte at the head as not starting the frame, causing the
       * interrupt routine to ignore the rest of the frame.
       */

      uart1_rx_buffer.buf[head] = 0;
      return;
    }

    uart1_rx_buffer.buf[head] = framebyte;
    *write_pos = next_write_pos;
    return;
  }

  // Put the errorcode in the buffer
  uart1_rx_buffer.buf[head] = err;
  
  if (framebyte >= 0xF0) {
    // This is the start of an incoming frame with address 7. The chain is too
    // long, report this error.
    
    // Increase the buffer pointers
    head = next_write_pos;
    circ_buf_incr_ptr (&next_write_pos, UART1_RX_BUFSIZE);

    if (next_write_pos == tail) {
      // No room to report the "chain too long" error, mark the byte at the 
      // (new) head as not starting the frame, to ignore the rest of the frame.

      CHAIN_OVERFLOW_VAR |= CHAIN_OVERFLOW_BIT;
      uart1_rx_buffer.buf[head] = 0;
      // Push out the first error code
      uart1_rx_buffer.head = head;
      return;
    }
    
    // Pass the error code for "chain too long"
    uart1_rx_buffer.buf[head] = RECV_ERR_CHAIN_LONG;

    // Mark the byte at the head as not starting the frame, causing the
    // interrupt routine to ignore the rest of the frame.
    uart1_rx_buffer.buf[next_write_pos] = 0;

    // Push out the two error codes
    uart1_rx_buffer.head = next_write_pos;
    return;
  }

  // Put the received byte at the new head; if it's a frame start byte, we will
  // receive the next frame. Otherwise, the rest of the frame will be discarded.
  uart1_rx_buffer.buf[next_write_pos] = framebyte;
  // Push out the error code
  uart1_rx_buffer.head = next_write_pos;

  circ_buf_incr_ptr (&next_write_pos, UART1_RX_BUFSIZE);
  *write_pos = next_write_pos;
}

/**
 * This interrupt routine gets called when a byte has succesfully been received
 * on UART 1, the UART connecting to a daisy-chained board.
 *
 * It does some basic error checking that's best done here, and appends the byte
 * in the buffer. Only when the start of a new frame is detected, will the
 * previous frame be made available in the buffer (by updating the head
 * pointer). This way, only complete frames are presented to the routines
 * running outside interrupt context, so they can complete processing once they
 * start it.
 *
 * Errors cause the current frame to be discarded.
 *
 * Errors detected:
 * - Byte framing error (indicated by UART)
 * - Hard overflow (indicated by UART): the UART FIFO was not read quick enough
 *   and a byte was overwritten in the FIFO
 * - Chain too long: incoming frame already had address 7, the highest
 *   daisy-chain address
 * - Frame too large (one frame filled the complete buffer)
 * - Circular buffer overflow
 *
 * All but the overflows and daisy-chain length are reported as "Malformed Packet"
 */
ISR(USART1_RXC_vect) {
  /**
   * Points to the next position to write in buffer 
   * Pointer undefined when the byte at the head of the buffer is not a
   * frame start byte!
   */
  static uint8_t write_pos;
  uint8_t recv; // Received byte
  uint8_t temp_write_pos, temp_head, temp_tail;
  // Previous frame start byte 
  uint8_t prev_frame_start;
    
  temp_head = uart1_rx_buffer.head;
  temp_tail = uart1_rx_buffer.tail;

  if (UCSR1A & _BV(FE1)) {
    // Framing error, send error code and discard the current frame
    // We still need to read the UDR1 register to discard the received byte
    recv = UDR1;

    errcode_and_framebyte (RECV_ERR_MALFORMED, 0, &temp_write_pos, temp_head, temp_tail);
    return;
  }

  if (UCSR1A & _BV(DOR1)) {
    // FIFO overflow. Send error code, discard the current frame and append the 
    // received byte.

    recv = UDR1;
    errcode_and_framebyte (RECV_ERR_H_OVERFLOW, recv, &write_pos, temp_head, temp_tail);
    return;
  }
  
  // Get received byte
  recv = UDR1;

  prev_frame_start = uart1_rx_buffer.buf[temp_head];

  if (!(prev_frame_start & (1 << 7))) {
    // We were discarding databytes
    
    if (!(recv & (1 << 7))) {
      // This is a databyte, so discard it
      return;
    }

    /* This is the start of a new frame, and we have no previous frame to
     * deliver.
     * Do not use write_pos, but use the knowledge our new frame should
     * start at the head.
     */

    if (recv >= 0xF0) {
      // This is the start of an incoming frame with address 7. The chain is too
      // long, send error code and discard the current frame.
      
      errcode_and_framebyte (RECV_ERR_CHAIN_LONG, 0, &temp_write_pos, temp_head, temp_tail);
      return;
    }
    
    // Buffer free checks are not needed, the head is guaranteed to be
    // available
    temp_write_pos = temp_head;
    uart1_rx_buffer.buf[temp_write_pos] = recv;
    circ_buf_incr_ptr (&temp_write_pos, UART1_RX_BUFSIZE);
    write_pos = temp_write_pos;
    return;
  }

  temp_write_pos = write_pos;

  if (!(recv & (1 << 7))) {
    // This is a databyte in the current frame, just append it

    if (!append_circ_buf (uart1_rx_buffer.buf, UART1_RX_BUFSIZE, recv, &temp_write_pos, temp_tail)) {
      // Okay, set write_pos and we're done
      write_pos = temp_write_pos;
      return;
    }
    
    // The buffer is full

    if (temp_head == temp_tail) {
      /*
       * Only this frame is in the buffer, yet it is full. The frame is too 
       * long. 
       * Discard the frame and send out "Malformed packet" error code.
       * We can assume that this will fit, so no checking.
       */

      temp_write_pos = temp_head;
      // Pass the error code
      uart1_rx_buffer.buf[temp_write_pos] = RECV_ERR_MALFORMED;
      circ_buf_incr_ptr (&temp_write_pos, UART1_RX_BUFSIZE);
      // Mark the byte at the head as not starting the frame, causing this routine
      // to skip the rest of the data
      uart1_rx_buffer.buf[temp_write_pos] = 0;
      // Push out the error code
      uart1_rx_buffer.head = temp_write_pos;
      return;
    }

    // Discard the current frame

    CHAIN_OVERFLOW_VAR |= CHAIN_OVERFLOW_BIT;
    // Mark the byte at the head as not starting the frame, causing this routine
    // to skip the rest of the data
    uart1_rx_buffer.buf[temp_head] = 0;
    return;
  }

  // The received character is the start of a frame.
  // There still is a previous frame to send out. 

  if (write_pos == temp_tail) {
    // The buffer is full, discard the previous frame

    if (temp_head == temp_tail) {
      /*
       * Only the previous frame is in the buffer, yet it is full. The frame is
       * too long.  
       * Discard the previous frame and send out "Malformed packet" error code.
       * We can assume that this will fit, so no checking.
       */

      temp_write_pos = temp_head;
      // Pass the error code
      uart1_rx_buffer.buf[temp_write_pos] = RECV_ERR_MALFORMED;
      circ_buf_incr_ptr (&temp_write_pos, UART1_RX_BUFSIZE);
      // Start the new frame at the head of the buffer 
      uart1_rx_buffer.buf[temp_write_pos] = recv;
      // Push out the error code
      uart1_rx_buffer.head = temp_write_pos;

      circ_buf_incr_ptr (&temp_write_pos, UART1_RX_BUFSIZE);
      write_pos = temp_write_pos;
      return;
    }

    CHAIN_OVERFLOW_VAR |= CHAIN_OVERFLOW_BIT;

    if (recv >= 0xF0) {
      // This is the start of an incoming frame with address 7. The chain is too
      // long, send error code and discard the previous and new frame.
      
      errcode_and_framebyte (RECV_ERR_CHAIN_LONG, 0, &temp_write_pos, temp_head, temp_tail);
      return;
    }

    temp_write_pos = temp_head;
    uart1_rx_buffer.buf[temp_write_pos] = recv;
    circ_buf_incr_ptr (&temp_write_pos, UART1_RX_BUFSIZE);
    write_pos = temp_write_pos;
    return;
  }
  
  // Start the new frame
  if (recv >= 0xF0) {
    // This is the start of an incoming frame with address 7. The chain is too
    // long, send error code and discard the new frame.
    
    errcode_and_framebyte (RECV_ERR_CHAIN_LONG, 0, &temp_write_pos, temp_write_pos, temp_tail);
    return;
  }

  uart1_rx_buffer.buf[temp_write_pos] = recv;
  // And push out the previous frame
  uart1_rx_buffer.head = temp_write_pos;

  circ_buf_incr_ptr (&temp_write_pos, UART1_RX_BUFSIZE);
  write_pos = temp_write_pos;
}
