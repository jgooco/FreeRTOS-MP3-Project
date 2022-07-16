#include "uart_lab.h"
#include "FreeRTOS.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include "queue.h"
#include <stdbool.h>
#include <stdint.h>

static void uart23_power_on(void) {
  const uint32_t uart2_peripheral = (1 << 24);
  const uint32_t uart3_peripheral = (1 << 25);
  LPC_SC->PCONP |= uart2_peripheral;
  LPC_SC->PCONP |= uart3_peripheral;
}

static void baud_rate_init(uint32_t baud_rate) {
  // Disable fractional dividers
  const uint32_t divaddval = (0 << 0);
  const uint32_t mulval = (1 << 4);
  LPC_UART2->FDR = divaddval | mulval;
  LPC_UART3->FDR = divaddval | mulval;

  const uint16_t value_16bit = (96U * 1000 * 1000) / (16 * baud_rate);
  LPC_UART2->DLL = (value_16bit & 0xFF);
  LPC_UART2->DLM = (value_16bit >> 8) & 0xFF;
  LPC_UART3->DLL = (value_16bit & 0xFF);
  LPC_UART3->DLM = (value_16bit >> 8) & 0xFF;
}

static void configure_uart_pins(void) {
  LPC_IOCON->P2_8 &= ~0b111; // UART2
  LPC_IOCON->P2_9 &= ~0b111;
  LPC_IOCON->P4_28 &= ~0b111; // UART3
  LPC_IOCON->P4_29 &= ~0b111;

  LPC_IOCON->P2_8 |= 0b010; // UART2
  LPC_IOCON->P2_9 |= 0b010;
  LPC_IOCON->P4_28 |= 0b010; // UART3
  LPC_IOCON->P4_29 |= 0b010;
}

static void lcr_config(void) {
  const uint32_t lcr_word_length = (3 << 0);
  LPC_UART2->LCR = lcr_word_length;
  LPC_UART3->LCR = lcr_word_length;
}

void uart_lab__init(uart_number_e uart, uint32_t peripheral_clock, uint32_t baud_rate) {
  uart23_power_on(); // powers on peripheral
  const uint32_t dlab_bit = (1 << 7);
  LPC_UART2->LCR = dlab_bit;
  LPC_UART3->LCR = dlab_bit;
  baud_rate_init(baud_rate); // sets baud rate, with assuming peripheal_clock is 96 MHz
  LPC_UART2->LCR &= ~dlab_bit;
  LPC_UART3->LCR &= ~dlab_bit;
  configure_uart_pins();
  lcr_config();
}

static void wait_for_data(void) {
  while (1) {
    if (LPC_UART2->LSR & (1 << 0)) {
      break;
    }
    if (LPC_UART3->LSR & (1 << 0)) {
      break;
    }
  }
}

// Read the byte from RBR and actually save it to the pointer
bool uart_lab__polled_get(uart_number_e uart, char *input_byte) {
  bool receive = false;
  if (uart == UART_2) {
    wait_for_data();
    *input_byte = (char *)LPC_UART2->RBR;
    return receive = true;
  } else if (uart == UART_3) {
    wait_for_data();
    *input_byte = (char *)LPC_UART3->RBR;
    return receive = true;
  }

  // a) Check LSR for Receive Data Ready
  // b) Copy data from RBR register to input_byte
}

static void wait_to_send(void) {
  while (1) {
    if (LPC_UART2->LSR & (1 << 5)) {
      break;
    }
    if (LPC_UART3->LSR & (1 << 5)) {
      break;
    }
  }
}

bool uart_lab__polled_put(uart_number_e uart, char output_byte) {
  if (uart == UART_2) {
    wait_to_send();
    return LPC_UART2->THR = output_byte;
  } else if (uart == UART_3) {
    wait_to_send();
    return LPC_UART3->THR = output_byte;
  }
  // a) Check LSR for Transmit Hold Register Empty
  // b) Copy output_byte to THR register
}

// PART 2
static QueueHandle_t your_uart_rx_queue;

static void u2_receiver_int(void) {
  if (!(LPC_UART2->IIR & (1 << 0))) {
    if (LPC_UART2->IIR & (2 << 1)) {
      while (!(LPC_UART2->LSR & (1 << 0))) {
        ;
      }
      const char byte = LPC_UART2->RBR;
      xQueueSendFromISR(your_uart_rx_queue, &byte, NULL);
    }
  }
}

static void u3_receiver_int(void) {
  if (!(LPC_UART3->IIR & (1 << 0))) {
    if (LPC_UART3->IIR & (2 << 1)) {
      while (!(LPC_UART3->LSR & (1 << 0))) {
        ;
      }
      const char byte = LPC_UART3->RBR;
      xQueueSendFromISR(your_uart_rx_queue, &byte, NULL);
    }
  }
}

// Private function of our uart_lab.c
static void your_receive_interrupt(void) {
  // TODO: Read the IIR register to figure out why you got interrupted
  // TODO: Based on IIR status, read the LSR register to confirm if there is data to be read
  // TODO: Based on LSR status, read the RBR register and input the data to the RX Queue
  u2_receiver_int();
  u3_receiver_int();
}

// Public function to enable UART interrupt
// TODO Declare this at the header file
void uart__enable_receive_interrupt(uart_number_e uart_number) {
  // TODO: Enable UART receive interrupt by reading the LPC User manual
  // Hint: Read about the IER register
  // TODO: Create your RX queue
  // your_uart_rx_queue = xQueueCreate(...);
  // TODO: Use lpc_peripherals.h to attach your interrupt
  // lpc_peripheral__enable_interrupt(..., your_receive_interrupt);
  const uint32_t dlab_bit = (1 << 7);
  LPC_UART2->LCR &= ~dlab_bit;
  LPC_UART3->LCR &= ~dlab_bit;
  const uint32_t ier_int = ((1 << 0) | (1 << 1) | (1 << 2));
  LPC_UART2->IER = ier_int;
  LPC_UART3->IER = ier_int;
  if (uart_number == UART_2) {
    lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__UART2, your_receive_interrupt, "receive_interrupt");
    NVIC_EnableIRQ(UART2_IRQn);
    your_uart_rx_queue = xQueueCreate(16, sizeof(char));
  }
  if (uart_number == UART_3) {
    lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__UART3, your_receive_interrupt, "receive_interrupt");
    NVIC_EnableIRQ(UART3_IRQn);
    your_uart_rx_queue = xQueueCreate(16, sizeof(char));
  }
}

// Public function to get a char from the queue (this function should work without modification)
// TODO: Declare this at the header file
bool uart_lab__get_char_from_queue(char *input_byte, uint32_t timeout) {
  return xQueueReceive(your_uart_rx_queue, input_byte, timeout);
}