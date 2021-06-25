#ifndef GENERIC_UART_H
#define GENERIC_UART_H
/****************************************************************************
 *
 * File:          generic_uart.h
 * Author(s):     Georgi Valkov (GV)
 * Project:       generic UART miniport driver
 * Description:   high-level API definitions and types
 *
 * Copyright (C) 2019 Georgi Valkov
 *                      Sedma str. 23, 4491 Zlokuchene, Bulgaria
 *                      e-mail: gvalkov@abv.bg
 *
 ****************************************************************************/

/* Include files ************************************************************/

#include <os_layer.h>


/* Macros *******************************************************************/

// uart parity mode
enum UART_PARITY_E
{
	UART_PARITY_NONE = 0,
#ifndef UART_PAL_H
	UART_PARITY_ODD = 1,
	UART_PARITY_EVEN = 2,
#endif
};

// uart word size
enum UART_WORD_SIZE_E
{
	UART_WORD_SIZE_8 = 0,
	UART_WORD_SIZE_9 = 1,
	UART_WORD_SIZE_10 = 2,
	UART_WORD_SIZE_7 = 3,
};

// uart command 0: write, 1: read
enum UART_CMD_E
{
	UART_CMD_WRITE = 0,
	UART_CMD_READ = 1,
};

typedef union
{
	uint8_t value;			// 0xff

	struct
	{
		int wait_rx : 1;	// 0x01
		int wait_tx : 1;	// 0x02
		int stop : 1;		// 0x04
		int tx : 1;			// 0x08
		int : 3;			// 0x70
		int valid : 1;		// 0x80
	};
} uart_status;

typedef union
{
	uint32_t value;				// 0x ffff ffff

	struct
	{
		int int_priority : 4;	// 0x 0000 000f  interrupt priority
		int use_interrupts : 1;	// 0x 0000 0010  enable interrupts
		int use_dma : 1;        // 0x 0000 0020  enable DMA
		int idle_ends_recv : 1;	// 0x 0000 0040  allow recv to return less then requested bytes if the RX line becomes idle
		int duplex : 1;         // 0x 0000 0080  enable full duplex mode
		int parity : 2;         // 0x 0000 0300  UART_PARITY_NONE, UART_PARITY_ODD, UART_PARITY_EVEN
		int word_size : 2;      // 0x 0000 0c00  UART_WORD_SIZE_8, UART_WORD_SIZE_7 - UART_WORD_SIZE_10
		int stop_bits_2 : 1;    // 0x 0000 1000  0: 1 stop bit, 1: 2 stop bits
		int msb_first : 1;      // 0x 0000 2000  0: LSB first, 1: MSB first
		int inv_rx : 1;         // 0x 0000 4000  invert RX line polarity
		int inv_tx : 1;         // 0x 0000 8000  invert TX line polarity
		int flow_control : 1;   // 0x 0001 0000  flow control
		int : 14;               // 0x 7ffe 0000  reserved
		int enabled : 1;        // 0x 8000 0000  enabled
	};
} uart_config;

#define UART_OWN_ADDRESS_T \
	union\
	{\
		uint32_t address;\
		\
		struct\
		{\
			uint16_t address1;\
			uint16_t address2;\
		};\
	}

// the hardware library defines this
// as pointer type to UART base address
// the generic code uses void *
#ifndef uart_reg
#define uart_reg void *
#endif

#ifndef uart_task
#define uart_task int
#endif


/* Types ********************************************************************/

struct uart_api;

typedef int(*fn_uart_init)(struct uart_api * uart);
typedef void(*fn_uart_deinit)(struct uart_api * uart);
typedef void(*fn_uart_abort)(struct uart_api * uart);
typedef int(*fn_uart_set_duplex)(struct uart_api * uart, int enable);
typedef int(*fn_uart_set_baud_rate)(struct uart_api * uart, uint32_t baud, uint32_t bus);
typedef int(*fn_uart_handler_int)(struct uart_api * uart, int state);
typedef int(*fn_uart_send_byte)(struct uart_api * uart, char c);
typedef int(*fn_uart_recv_byte)(struct uart_api * uart, char * c);
typedef int(*fn_uart_send_job)(struct uart_api * uart);
typedef int(*fn_uart_recv_job)(struct uart_api * uart);
typedef int(*fn_uart_send)(struct uart_api * uart, char * msg, int count);
typedef int(*fn_uart_recv)(struct uart_api * uart, char * msg, int count);
typedef int (*fn_uart_wake_tx)(struct uart_api * uart);
typedef int (*fn_uart_wake_rx)(struct uart_api * uart);

#ifndef UART_HANDLER_TYPE_DEFINED
typedef int(*fn_uart_handler_send)(struct uart_api * uart);
typedef int(*fn_uart_handler_recv)(struct uart_api * uart);
#endif

struct uart_job
{
	char * msg;
	size_t bytes_requested;
	size_t bytes_transferred;
};

struct uart_api
{
	// interface index and base address
	uint32_t index;
	uart_reg reg;
	uart_task task_rx;
	uart_task task_tx;
	int int_number;
	void * int_prolog;
	uart_status status;
	uart_config config;
	uint32_t baud_rate;

	UART_OWN_ADDRESS_T;

	struct uart_job job_rx;
	struct uart_job job_tx;

	// interface functions
	fn_uart_init init;
	fn_uart_deinit deinit;
	fn_uart_abort abort;
	fn_uart_set_baud_rate set_baud_rate;
	fn_uart_handler_int handler_int;
	fn_uart_handler_send handler_send;
	fn_uart_handler_recv handler_recv;
	fn_uart_set_duplex set_duplex;
	fn_uart_send_byte send_byte;
	fn_uart_recv_byte recv_byte;
	fn_uart_send_job send_job;
	fn_uart_recv_job recv_job;
	fn_uart_send send;
	fn_uart_recv recv;
	fn_uart_wake_tx uart_wake_tx;
	fn_uart_wake_rx uart_wake_rx;
};


/* Local Prototypes *********************************************************/

/* Global Prototypes ********************************************************/

/* Function prototypes ******************************************************/

_BEGIN_CPLUSPLUS

int uart_send(struct uart_api * uart, char * msg, int count);
int uart_recv(struct uart_api * uart, char * msg, int count);

int uart_wake_tx(struct uart_api * uart);
int uart_wake_rx(struct uart_api * uart);

// constructor for uart_api, provided in the hardware miniport interface
int uart_create(struct uart_api * uart, uint32_t interface_index);
struct uart_api * uart_open(uint32_t interface_index);

_END_CPLUSPLUS

/* Global variables *********************************************************/

/* End of file **************************************************************/

#endif // GENERIC_UART_H
