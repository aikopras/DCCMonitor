/**
 * UART routines
 *
 * These routines allow I/O to be done through the UARTs, using interrupt
 * routines for the I/O paths that are heavily used in the DCC Monitor.
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

#include <avr/io.h> 
#include <avr/interrupt.h>
#include <stdint.h>
#include "global.h"
#include "uart.h"

/**
 * UART0 reception circular buffer
 */
static struct {
  volatile uint8_t buf[UART0_TX_BUFSIZE];
  volatile uint8_t head, tail;
} uart0_tx_buffer;

/*
 * Initialise UARTs, and enable
 */
void uart_init () {
  /* The following settings are default and thus not set explicitly:
   * asynchronous operation, 8N1 frameformat, 
   * U2X = 0 (no Double Speed Asynchronous Communication)
   */
  // Set baudrate
  UBRR0H = ((F_CPU / (16UL * UART_BAUD)) - 1) >> 8;
  UBRR0L = ((F_CPU / (16UL * UART_BAUD)) - 1) & 0xff;
  UBRR1H = ((F_CPU / (16UL * UART_BAUD)) - 1) >> 8;
  UBRR1L = ((F_CPU / (16UL * UART_BAUD)) - 1) & 0xff;

  // Enable UARTs, UART1 RX complete interrupt
  UCSR0B = _BV(RXEN0) | _BV(TXEN0);
  UCSR1B = _BV(RXCIE1) | _BV(RXEN1) | _BV(TXEN1);
}

/**
 * Transmit a byte through UART 0.
 *
 * This is a blocking routine. If the buffer is full, it will busy-wait.
 *
 * Instead of using a bit to store the status of the buffer, the USART
 * Data Register Empty Interrupt Enable is used: when the buffer is emptied
 * by the transmit interrupt handler, it disables itself, so we can use that
 * for status.
 */
void uart0_put (const uint8_t c) {
  uint8_t temp_head;

  temp_head = uart0_tx_buffer.head;
  while (temp_head == uart0_tx_buffer.tail && bit_is_set (UCSR0B, UDRIE0)) {
    // Buffer is full
  }

  uart0_tx_buffer.buf[temp_head] = c; // Place byte in buffer
  
  // Increase buffer head
  circ_buf_incr_ptr(&temp_head, UART0_TX_BUFSIZE);

  uart0_tx_buffer.head = temp_head;

  // Clear TXC flag (used for Idle Frame management in comm_proto.c)
  UCSR0A |= _BV(TXC0);
  // Enable transmit interrupt
  UCSR0B |= _BV(UDRIE0);
}

/**
 * Interrupt handler for transmitting data through UART 0 (UDR empty interrupt)
 *
 * This handler should only be active when there is data in the buffer, so we 
 * don't test that.
 */
ISR(USART0_UDRE_vect) {
  uint8_t temp_tail;

  temp_tail = uart0_tx_buffer.tail;
  UDR0 = uart0_tx_buffer.buf[temp_tail]; // Get byte from buffer
  circ_buf_incr_ptr(&temp_tail, UART0_TX_BUFSIZE); // Increase pointer

  if (temp_tail == uart0_tx_buffer.head) {
    // Buffer empty, disable this interrupt
    UCSR0B &= ~(_BV(UDRIE0));
  }
  uart0_tx_buffer.tail = temp_tail;
}

/**
 * Receive a byte through UART 0.
 *
 * No buffering (the ATmega162 has a hardware FIFO of 3 bytes)
 *
 * @return -1 when receive buffer is empty
 */
int16_t uart0_get () {
  uint8_t c;

  if (bit_is_clear (UCSR0A, RXC0)) {
    // No data to be read
    return -1;
  }
  c = UDR0; // Get databyte
  return c;
}
