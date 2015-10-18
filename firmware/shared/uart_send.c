/*--------------------------------------------------------------------------*
* UART interface implementation for ATmega
*---------------------------------------------------------------------------*
* 01-Apr-2014 ShaneG
*
* Half-duplex 8N1 serial UART in software.
*
* 1%/2% Tx/Rx timing error for 115.2kbps@8Mhz
* 2%/1% Tx/Rx timing error for 230.4kbps@8Mhz
*
* Uses only one pin on the AVR for both Tx and Rx:
*
*              D1
*  AVR ----+--|>|-----+----- Tx
*          |      10K $ R1
*          +--------(/^\)--- Rx
*               NPN E   C
*--------------------------------------------------------------------------*/
#include <avr/io.h>
#include <avr/interrupt.h>
#include "../hardware.h"
#include "uart_defs.h"
#include "softuart.h"

// Only if enabled
#ifdef UART_ENABLED

/** Initialise the UART
 */
void uartInit() {
  // Set as input and disable pullup
  DDRB  &= ~(1 << UART_RX);
  PORTB &= ~(1 << UART_RX);
#ifdef UART_TWOPIN
  // Set up TX pin
  DDRB |= (1 << UART_TX);
  PORTB |= (1 << UART_TX);
#  ifdef UART_INTERRUPT
  // Enable pin change interrupts
  PCMSK |= (1 << UART_RX);
  GIMSK |= (1 << PCIE);
#  endif /* UART_INTERRUPT */
#endif /* UART_TWOPIN */
  }

/** Write a single character
 *
 * Send a single character on the UART.
 *
 * @param ch the character to send.
 */
void uartSend(char ch) {
  // Set to output state and bring high
  PORTB |= (1 << UART_TX);
#ifdef UART_ONEPIN
  DDRB  |= (1 << UART_TX);
#endif
  cli();
  asm volatile(
    "  cbi %[uart_port], %[uart_pin]    \n\t"  // Start bit
    "  in __tmp_reg__, %[uart_port]     \n\t"  // Load output port into r0
    "  ldi r16, 3                       \n\t"  // Stop bit + idle state
    "  ldi r17, %[txdelay]              \n\t"
    "TxLoop%=:                          \n\t"
    // 8 cycle loop + delay - total = 7 + 3*txdelay
    "  mov r18, r17                     \n\t"  // Copy txdelay into working reg r18, r17 untouched
    "TxDelay%=:                         \n\t"
    // delay (3 cycle * delayCount) - 1
    "  dec r18                          \n\t"
    "  brne TxDelay%=                   \n\t"
    "  bst %[ch], 0                     \n\t"  // Store input byte bit0 to T bit
    "  bld __tmp_reg__, %[uart_pin]     \n\t"  // Load T bit to bit # of tx pin of r0
    "  lsr r16                          \n\t"
    "  ror %[ch]                        \n\t"  // Shift over input byte
    "  out %[uart_port], __tmp_reg__    \n\t"  // Load r0 to output port
    "  brne TxLoop%=                    \n\t"
    :
    : [uart_port] "I" (_SFR_IO_ADDR(PORTB)),
      [uart_pin] "I" (UART_TX),
      [txdelay] "M" (TXDELAY),
      [ch] "r" (ch)
    : "r16","r17","r18");
  sei();
#ifdef UART_ONEPIN
  // Change back to idle state
  DDRB  &= ~(1 << UART_TX);
  PORTB &= ~(1 << UART_TX);
#endif
  }

#endif /* UART_ENABLED */

