#ifndef UART_MINIPORT_H
#define UART_MINIPORT_H


#if !CONFIG_NRFX_UARTE
#define USE_UART_MINIPORT 1
#endif

#if USE_UART_MINIPORT
#define UART_INTERFACE 0
#include <port/miniport/uart/nrf52840_uart.h>
#include <port/miniport/uart/generic_uart.h>
#endif


#endif /* UART_MINIPORT_H */
