#ifndef NRF52840_UART_H
#define NRF52840_UART_H
/****************************************************************************
 *
 * File:          nrf52840_uart.h
 * Author(s):     Georgi Valkov (GV)
 * Project:       UART miniport driver for nrf52840
 * Description:   hadrware specific definitions and types
 *
 * Copyright (C) 2020 Georgi Valkov
 *                      Sedma str. 23, 4491 Zlokuchene, Bulgaria
 *                      e-mail: gvalkov@abv.bg
 *
 ****************************************************************************/

/* Include files ************************************************************/

#ifdef RX
#undef RX
#endif

#include <os_layer.h>
#include <arch/cm4/nrf52840/nrf52840.h>
#include <arch/cm4/nrf52840/nrf52840_bitfields.h>



/* Macros *******************************************************************/

#ifndef NRF52840_USE_DMA
#define NRF52840_USE_DMA 1
#endif // !NRF52840_USE_DMA

// array size
#define NRF52840_UART_COUNT(a) (sizeof((a)) / sizeof(*(a)))

#ifndef NRF52840_UART_BUFFER_SIZE
#define NRF52840_UART_BUFFER_SIZE 256
#endif // !NRF52840_UART_BUFFER_SIZE

#define NRF52840_UART_BUFFER_MASK (NRF52840_UART_BUFFER_SIZE - 1)

#if (NRF52840_UART_BUFFER_SIZE < 2) || (NRF52840_UART_BUFFER_SIZE & NRF52840_UART_BUFFER_MASK)
#error NRF52840_UART_BUFFER_SIZE must be power of 2
#endif


/* Types ********************************************************************/

// prototype of uart API type
struct uart_api;

typedef struct nrf52840_uart_state
{
	// RX and TX indexes for panic UART
	// intentionally put in the beginning of the structure to aid debugging
	uint32_t rx_a;
	uint32_t rx_b;
	uint32_t rx_B;
	uint32_t tx_A;
	uint32_t tx_a;
	uint32_t tx_b;

	uint32_t tx_ready;
	uint32_t loopback;

	union
	{
		NRF_UART_Type * reg;
		NRF_UARTE_Type * reg_dma;
	};

	union
	{
		UART_PSEL_Type pin_select;
		UARTE_PSEL_Type pin_select_dma;
	};

	// RX and TX buffers for panic UART
	uint32_t size;
	uint32_t mask;
	char rx[NRF52840_UART_BUFFER_SIZE];
	char tx[NRF52840_UART_BUFFER_SIZE];
} nrf52840_uart_state;

// the hardware library defines this
// as pointer type to uart base address
// the generic code uses void *
#define uart_reg nrf52840_uart_state *

typedef struct baudrate_map_t
{
	uint32_t baudrate;
	uint32_t baud_reg;
} baudrate_map_t;

enum NRF52840_UART_PSEL
{
	NRF52840_UART_PSEL_PIN = 0x1f,	// 0-31
	NRF52840_UART_PSEL_PORT = 0x20,	// 32
	NRF52840_UART_PSEL_MASK = NRF52840_UART_PSEL_PIN | NRF52840_UART_PSEL_PORT, // 0-64
	NRF52840_UART_PSEL_DISCONNECT = 0x80000000,
};


/* Local Prototypes *********************************************************/

/* Global Prototypes ********************************************************/

/* Function prototypes ******************************************************/

_BEGIN_CPLUSPLUS

// hardware interface
void nrf52840_uart_abort(struct uart_api * uart);
int nrf52840_uart_set_duplex(struct uart_api * uart, int enable);
int nrf52840_uart_set_baud_rate(struct uart_api * uart, uint32_t baud, uint32_t bus);
int nrf52840_uart_recv_job(struct uart_api * uart);

#if NRF52840_USE_DMA
int nrf52840_uart_init_dma(struct uart_api * uart);
void nrf52840_uart_deinit_dma(struct uart_api * uart);
int nrf52840_uart_configure_dma(struct uart_api * uart);
int nrf52840_uart_send_byte_dma(struct uart_api * uart, char c);
int nrf52840_uart_recv_byte_dma(struct uart_api * uart, char * c);
int nrf52840_uart_send_job_dma(struct uart_api * uart);
int nrf52840_uart_handler_int_dma(struct uart_api * uart, int int_state);
void __attribute__((interrupt)) nrf52840_uart_handler_raw_dma_0(void);
void __attribute__((interrupt)) nrf52840_uart_handler_raw_dma_1(void);
#else
int nrf52840_uart_init(struct uart_api * uart);
void nrf52840_uart_deinit(struct uart_api * uart);
int nrf52840_uart_configure(struct uart_api * uart);
int nrf52840_uart_send_byte(struct uart_api * uart, char c);
int nrf52840_uart_send_byte_sync(struct uart_api * uart, char c);
int nrf52840_uart_recv_byte(struct uart_api * uart, char * c);
int nrf52840_uart_send_job(struct uart_api * uart);
int nrf52840_uart_handler_int(struct uart_api * uart, int int_state);
void __attribute__((interrupt)) nrf52840_uart_handler_raw_0(void);
#endif

_END_CPLUSPLUS


/* Global variables *********************************************************/

/* End of file **************************************************************/

#endif // NRF52840_UART_H
