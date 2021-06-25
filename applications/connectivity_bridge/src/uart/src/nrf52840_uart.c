/****************************************************************************
 *
 * File:          nrf52840_uart.c
 * Author(s):     Georgi Valkov (GV)
 * Project:       UART miniport driver for nrf52840
 * Description:   Hardware layer
 *
 * Copyright (C) 2020 Georgi Valkov
 *                      Sedma str. 23, 4491 Zlokuchene, Bulgaria
 *                      e-mail: gvalkov@abv.bg
 *
 ****************************************************************************/

/* Include files ************************************************************/

#include <int.h>
#include <string.h>
#include <port/miniport/uart/nrf52840_uart.h>
#include <port/miniport/uart/generic_uart.h>


#if NRF52840_USE_DMA
const baudrate_map_t baudrate_map[] =
{
	{ 1200, UARTE_BAUDRATE_BAUDRATE_Baud1200 },
	{ 2400, UARTE_BAUDRATE_BAUDRATE_Baud2400 },
	{ 4800, UARTE_BAUDRATE_BAUDRATE_Baud4800 },
	{ 9600, UARTE_BAUDRATE_BAUDRATE_Baud9600 },
	{ 14400, UARTE_BAUDRATE_BAUDRATE_Baud14400 },
	{ 19200, UARTE_BAUDRATE_BAUDRATE_Baud19200 },
	{ 28800, UARTE_BAUDRATE_BAUDRATE_Baud28800 },
	{ 31250, UARTE_BAUDRATE_BAUDRATE_Baud31250 },
	{ 38400, UARTE_BAUDRATE_BAUDRATE_Baud38400 },
	{ 56000, UARTE_BAUDRATE_BAUDRATE_Baud56000 },
	{ 57600, UARTE_BAUDRATE_BAUDRATE_Baud57600 },
	{ 76800, UARTE_BAUDRATE_BAUDRATE_Baud76800 },
	{ 115200, UARTE_BAUDRATE_BAUDRATE_Baud115200 },
	{ 230400, UARTE_BAUDRATE_BAUDRATE_Baud230400 },
	{ 250000, UARTE_BAUDRATE_BAUDRATE_Baud250000 },
	{ 460800, UARTE_BAUDRATE_BAUDRATE_Baud460800 },
	{ 921600, UARTE_BAUDRATE_BAUDRATE_Baud921600 },
	{ 1000000, UARTE_BAUDRATE_BAUDRATE_Baud1M },
};
#else
const baudrate_map_t baudrate_map[] =
{
	{ 1200, UART_BAUDRATE_BAUDRATE_Baud1200 },
	{ 2400, UART_BAUDRATE_BAUDRATE_Baud2400 },
	{ 4800, UART_BAUDRATE_BAUDRATE_Baud4800 },
	{ 9600, UART_BAUDRATE_BAUDRATE_Baud9600 },
	{ 14400, UART_BAUDRATE_BAUDRATE_Baud14400 },
	{ 19200, UART_BAUDRATE_BAUDRATE_Baud19200 },
	{ 28800, UART_BAUDRATE_BAUDRATE_Baud28800 },
	{ 31250, UART_BAUDRATE_BAUDRATE_Baud31250 },
	{ 38400, UART_BAUDRATE_BAUDRATE_Baud38400 },
	{ 56000, UART_BAUDRATE_BAUDRATE_Baud56000 },
	{ 57600, UART_BAUDRATE_BAUDRATE_Baud57600 },
	{ 76800, UART_BAUDRATE_BAUDRATE_Baud76800 },
	{ 115200, UART_BAUDRATE_BAUDRATE_Baud115200 },
	{ 230400, UART_BAUDRATE_BAUDRATE_Baud230400 },
	{ 250000, UART_BAUDRATE_BAUDRATE_Baud250000 },
	{ 460800, UART_BAUDRATE_BAUDRATE_Baud460800 },
	{ 921600, UART_BAUDRATE_BAUDRATE_Baud921600 },
	{ 1000000, UART_BAUDRATE_BAUDRATE_Baud1M },
};
#endif


/* Types ********************************************************************/

// per interface configuration
typedef struct nrf52840_uart_interface
{
	union
	{
		NRF_UART_Type * reg;
		NRF_UARTE_Type * reg_dma;
	};

	uint16_t int_number;
	uint8_t use_int;
	uint8_t use_dma;

	union
	{
		UART_PSEL_Type pin_select;
		UARTE_PSEL_Type pin_select_dma;
	};

	uint32_t baud_rate;
	uint16_t enabled;
	uint16_t flow_control;
} nrf52840_uart_interface;


/* Global variables *********************************************************/

// UART states
nrf52840_uart_state nrf52840_uart_states[2];

// UART instances
struct uart_api nrf52840_uarts[2];

// UART instance links used by the interrupt handler
struct uart_api * nrf52840_links[2] = { NULL, NULL };

// list of available interfaces
nrf52840_uart_interface nrf52840_uart_interfaces[] =
{
	{
		// debug UART
		.reg_dma = NRF_UARTE0,
		.int_number = UARTE0_UART0_IRQn + 16,
		.use_int = 1,
		.use_dma = NRF52840_USE_DMA,

#ifdef DT_NODE_HAS_STATUS
		.pin_select_dma =
		{
			.CTS = DT_PROP_OR(DT_NODELABEL(uart0), cts_pin, -1),
			.RTS = DT_PROP_OR(DT_NODELABEL(uart0), rts_pin, -1),
			.RXD = DT_PROP_OR(DT_NODELABEL(uart0), rx_pin, -1),
			.TXD = DT_PROP_OR(DT_NODELABEL(uart0), tx_pin, -1),
		},
		.baud_rate = DT_PROP_OR(DT_NODELABEL(uart0), current_speed, 115200),
		.enabled = DT_NODE_HAS_STATUS(DT_NODELABEL(uart0), okay),
		.flow_control = DT_PROP_OR(DT_NODELABEL(uart0), hw_flow_control, 0),
#else
		.pin_select_dma =
		{
			.CTS = 22, // -1,
			.RTS = 15, // -1,
			.RXD = 20,
			.TXD = 5,
		},
		.baud_rate = 115200,
		.enabled = 1,
		.flow_control = 0,
#endif
	},
#if NRF52840_USE_DMA
	{
		.reg_dma = NRF_UARTE1,
		.int_number = UARTE1_IRQn + 16,
		.use_int = 1,
		.use_dma = 1,

#ifdef DT_NODE_HAS_STATUS
		.pin_select_dma =
		{
			.CTS = DT_PROP_OR(DT_NODELABEL(uart1), cts_pin, -1),
			.RTS = DT_PROP_OR(DT_NODELABEL(uart1), rts_pin, -1),
			.RXD = DT_PROP_OR(DT_NODELABEL(uart1), rx_pin, -1),
			.TXD = DT_PROP_OR(DT_NODELABEL(uart1), tx_pin, -1),
		},
		.baud_rate = DT_PROP_OR(DT_NODELABEL(uart1), current_speed, 115200),
		.enabled = DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay),
		.flow_control = DT_PROP_OR(DT_NODELABEL(uart1), hw_flow_control, 0),
#else
		.pin_select_dma =
		{
			.CTS = 22, // -1,
			.RTS = 15, // -1,
			.RXD = 20,
			.TXD = 5,
		},
		.baud_rate = 115200,
		.enabled = 1,
		.flow_control = 0,
#endif
	},
#endif
};


/* Functions ****************************************************************/

// constructor for uart_api
// configures the uart_api structure to use the hardware interface
int uart_create(struct uart_api * uart, uint32_t interface_index)
{
	if (interface_index >= NRF52840_UART_COUNT(nrf52840_uart_interfaces))
	{
		// interface index out of range
		return NRF52840_UART_COUNT(nrf52840_uart_interfaces);
	}

	nrf52840_uart_state * state = &nrf52840_uart_states[interface_index];
	nrf52840_uart_interface * uart_interface = &nrf52840_uart_interfaces[interface_index];

	if (!uart_interface->reg)
	{
		// interface not implemented
		return FAIL;
	}

	memset(uart, 0, sizeof(*uart));
	memset(state, 0, sizeof(*state));

	state->reg = uart_interface->reg;
	state->pin_select = uart_interface->pin_select;
	state->size = NRF52840_UART_BUFFER_SIZE;
	state->mask = NRF52840_UART_BUFFER_MASK;

	if (uart_interface->use_dma)
	{
#if NRF52840_USE_DMA
		uart_interface->use_int = 1;
		uart->deinit = nrf52840_uart_deinit_dma;
		uart->init = nrf52840_uart_init_dma;
		uart->handler_int = nrf52840_uart_handler_int_dma;
		uart->send_byte = nrf52840_uart_send_byte_dma;
		uart->recv_byte = nrf52840_uart_recv_byte_dma;
		uart->send_job = nrf52840_uart_send_job_dma;
		uart->recv_job = nrf52840_uart_recv_job;
#else
		return FAIL;
#endif
	}
	else
	{
#if NRF52840_USE_DMA
		return FAIL;
#else
		uart->deinit = nrf52840_uart_deinit;
		uart->init = nrf52840_uart_init;
		uart->handler_int = nrf52840_uart_handler_int;
		uart->recv_byte = nrf52840_uart_recv_byte;

		if (uart_interface->use_int)
		{
			uart->send_job = nrf52840_uart_send_job;
			uart->recv_job = nrf52840_uart_recv_job;
			uart->send_byte = nrf52840_uart_send_byte;
		}
		else
		{
			uart->send_job = NULL;
			uart->recv_job = NULL;
			uart->send_byte = nrf52840_uart_send_byte_sync;
		}
#endif
	}

	uart->index = interface_index;
	uart->reg = state;
	uart->int_number = uart_interface->int_number;
	uart->baud_rate = uart_interface->baud_rate;
	uart->int_prolog = NULL;
	uart->address = 0;
	uart->abort = nrf52840_uart_abort;
	uart->set_baud_rate = nrf52840_uart_set_baud_rate;
	uart->set_duplex = nrf52840_uart_set_duplex;
	uart->handler_send = NULL;
	uart->handler_recv = NULL;
	uart->send = uart_send;
	uart->recv = uart_recv;
	uart->uart_wake_tx = uart_wake_tx;
	uart->uart_wake_rx = uart_wake_rx;
	uart->task_rx = FAIL;
	uart->task_tx = FAIL;
	uart->status.value = 0;

	uart_config config =
	{
		.int_priority = 1,
		.use_interrupts = uart_interface->use_int ? 1 : 0,
		.use_dma = uart_interface->use_dma ? 1 : 0,
		.idle_ends_recv = 1,
		.duplex = 1,
		.parity = UART_PARITY_NONE,
		.word_size = UART_WORD_SIZE_8,
		.stop_bits_2 = 0,
		.msb_first = 0,
		.inv_rx = 0,
		.inv_tx = 0,
		.flow_control = uart_interface->flow_control,
		.enabled = uart_interface->enabled,
	};

	uart->config = config;

	return OK;
}

// constructor for uart_api
// configures the uart_api structure to use the hardware interface
// if the interface is already configured, returns the existing interface
struct uart_api * uart_open(uint32_t interface_index)
{
	if (interface_index >= NRF52840_UART_COUNT(nrf52840_uart_interfaces))
	{
		// interface index out of range
		return NULL;
	}

	struct uart_api * uart = &nrf52840_uarts[interface_index];
	nrf52840_uart_state * state = &nrf52840_uart_states[interface_index];
	nrf52840_uart_interface * uart_interface = &nrf52840_uart_interfaces[interface_index];

	if (
		(uart->reg == state) &&
		(uart->reg->reg_dma == uart_interface->reg_dma) &&
		uart->status.valid &&
		nrf52840_links[interface_index]
		)
	{
		// interface already created
		return uart;
	}

	// create interface
	if (OK == uart_create(uart, interface_index))
	{
		return uart;
	}

	return NULL;
}

// abort any active transfers
void nrf52840_uart_abort(struct uart_api * uart)
{
	uart->job_tx.msg = NULL;
	uart->job_rx.msg = NULL;
	uart->uart_wake_tx(uart);
	uart->uart_wake_rx(uart);
}

// enable or disable full-duplex modes
int nrf52840_uart_set_duplex(struct uart_api * uart, int enable)
{
	return FAIL;
}


#define ABS_DIFF(a, b) (a >= b) ? (a - b) : (b - a)

int nrf52840_uart_set_baud_rate(struct uart_api * uart, uint32_t baud, uint32_t bus)
{
#if NRF52840_USE_DMA
	NRF_UARTE_Type * reg = uart->reg->reg_dma;
#else
	NRF_UART_Type * reg = uart->reg->reg;
#endif

	if (!baud)
	{
		baud = uart->baud_rate ? uart->baud_rate : 115200;
	}

	const size_t count = sizeof(baudrate_map) / sizeof(*baudrate_map);
	size_t i = 0;
	size_t j = 0;
	uint32_t dj = ABS_DIFF(baud, baudrate_map[i].baudrate);

	for (i = 0; i < count; i++)
	{
		uint32_t di = ABS_DIFF(baud, baudrate_map[i].baudrate);

		if (dj > di)
		{
			dj = di;
			j = i;
		}
	}

	uart->baud_rate = baudrate_map[j].baudrate;
	reg->BAUDRATE = baudrate_map[j].baud_reg;

	return OK;
}

#if !NRF52840_USE_DMA
// deinit hardware interface
void nrf52840_uart_deinit(struct uart_api * uart)
{
	int_state_t int_state = int_disable_nested();
	uart_reg state = uart->reg;
	NRF_UART_Type * reg = state->reg;

	// disable UART interrupt
	int_source_disable(uart->int_number);
	reg->INTENCLR = 0xffffffff;

	uart->status.valid = 0;
	nrf52840_links[uart->index] = NULL;

	state->rx_a = 0;
	state->rx_b = 0;
	state->rx_B = 0;
	state->tx_A = 0;
	state->tx_a = 0;
	state->tx_b = 0;
	state->tx_ready = 0;

	reg->TASKS_STOPRX = UART_TASKS_STOPRX_TASKS_STOPRX_Trigger;
	reg->TASKS_STOPTX = UART_TASKS_STOPTX_TASKS_STOPTX_Trigger;
	reg->ENABLE = UART_ENABLE_ENABLE_Disabled;

	UART_PSEL_Type pin_select =
	{
		.RTS = -1,
		.TXD = -1,
		.CTS = -1,
		.RXD = -1,
	};

	reg->PSEL = pin_select;

	reg->ERRORSRC = -1;

	reg->EVENTS_CTS = UART_EVENTS_CTS_EVENTS_CTS_NotGenerated;
	reg->EVENTS_NCTS = UART_EVENTS_NCTS_EVENTS_NCTS_NotGenerated;
	reg->EVENTS_RXDRDY = UART_EVENTS_RXDRDY_EVENTS_RXDRDY_NotGenerated;
	reg->EVENTS_TXDRDY = UART_EVENTS_TXDRDY_EVENTS_TXDRDY_NotGenerated;
	reg->EVENTS_ERROR = UART_EVENTS_ERROR_EVENTS_ERROR_NotGenerated;
	reg->EVENTS_RXTO = UART_EVENTS_RXTO_EVENTS_RXTO_NotGenerated;

	uart->abort(uart);

	int_enable_nested(int_state);
}

// init hardware interface
int nrf52840_uart_init(struct uart_api * uart)
{
	if (uart->status.valid)
	{
		// already initialised
		// update the configuration
		return nrf52840_uart_configure(uart);
	}

	uart->deinit(uart);

	uart_reg state = uart->reg;
	NRF_UART_Type * reg = state->reg;
	uart_config * config = &uart->config;

	if (OK != nrf52840_uart_configure(uart))
	{
		return FAIL;
	}

	int_state_t int_state = int_disable_nested();

	// link used by the interrupt handler
	nrf52840_links[uart->index] = uart;

	if (config->use_interrupts)
	{
		if (OK == int_set_vector(uart->int_number, nrf52840_uart_handler_raw_0))
		{
			int_source_configure(uart->int_number, config->int_priority, 0);
			int_source_enable(uart->int_number);

			// 0x00000280  RXTO=2 0000  ERROR=0200  TXRDY=0080  RXRDY=0004  NCTS=0002  CTS=0001
			reg->INTENSET =
				// UART_INTENSET_RXTO_Msk |
				UART_INTENSET_ERROR_Msk |
				UART_INTENSET_TXDRDY_Msk |
				UART_INTENSET_RXDRDY_Msk |
				// UART_INTENSET_NCTS_Msk |
				// UART_INTENSET_CTS_Msk |
				0;
		}
		else
		{
			config->use_interrupts = 0;
			uart->task_rx = FAIL;
			uart->task_tx = FAIL;
		}
	}

	state->tx_ready = 1;
	uart->status.valid = 1;

	reg->SHORTS =
		(UART_SHORTS_NCTS_STOPRX_Disabled << UART_SHORTS_NCTS_STOPRX_Pos) |
		(UART_SHORTS_CTS_STARTRX_Disabled << UART_SHORTS_CTS_STARTRX_Pos) |
		0;

	reg->ENABLE = UART_ENABLE_ENABLE_Enabled;
	reg->TASKS_STARTRX = UART_TASKS_STARTRX_TASKS_STARTRX_Trigger;
	reg->TASKS_STARTTX = UART_TASKS_STARTTX_TASKS_STARTTX_Trigger;
	reg->EVENTS_CTS = UART_EVENTS_CTS_EVENTS_CTS_NotGenerated;
	reg->EVENTS_NCTS = UART_EVENTS_NCTS_EVENTS_NCTS_NotGenerated;
	reg->EVENTS_RXDRDY = UART_EVENTS_RXDRDY_EVENTS_RXDRDY_NotGenerated;
	reg->EVENTS_TXDRDY = UART_EVENTS_TXDRDY_EVENTS_TXDRDY_NotGenerated;
	reg->EVENTS_ERROR = UART_EVENTS_ERROR_EVENTS_ERROR_NotGenerated;
	reg->EVENTS_RXTO = UART_EVENTS_RXTO_EVENTS_RXTO_NotGenerated;

	int_enable_nested(int_state);

	return OK;
}

int nrf52840_uart_configure(struct uart_api * uart)
{
	if (OK != uart->set_baud_rate(uart, 0, 0))
	{
		return FAIL;
	}

	uart_reg state = uart->reg;
	NRF_UART_Type * reg = state->reg;
	uart_config * config = &uart->config;

	if (state->pin_select_dma.CTS & state->pin_select_dma.RTS & NRF52840_UART_PSEL_DISCONNECT)
	{
		// CTS and RTS not connected, disable flow control
		config->flow_control = 0;
	}

	reg->CONFIG =
		((config->stop_bits_2 ? UART_CONFIG_STOP_Two : UART_CONFIG_STOP_One) << UART_CONFIG_STOP_Pos) |
		((config->parity ? UART_CONFIG_PARITY_Included : UART_CONFIG_PARITY_Excluded) << UART_CONFIG_PARITY_Pos) |
		((config->flow_control ? UART_CONFIG_HWFC_Enabled : UART_CONFIG_HWFC_Disabled) << UART_CONFIG_HWFC_Pos) |
		0;

	reg->PSEL = state->pin_select;

	return OK;
}


// error handler
static void nrf52840_uart_error_handler(struct uart_api * uart)
{
	NRF_UART_Type * reg = uart->reg->reg;

	// clear error sources
	if (reg->ERRORSRC)
	{
		reg->ERRORSRC = -1;
		reg->TASKS_STARTRX = UART_TASKS_STARTRX_TASKS_STARTRX_Trigger;
	}

	// clear errors
	if (reg->EVENTS_ERROR)
	{
		reg->EVENTS_ERROR = UART_EVENTS_ERROR_EVENTS_ERROR_NotGenerated;
	}

	// clear CTS event
	if (reg->EVENTS_CTS)
	{
		reg->EVENTS_CTS = UART_EVENTS_CTS_EVENTS_CTS_NotGenerated;
	}

	// clear NCTS event
	if (reg->EVENTS_NCTS)
	{
		reg->EVENTS_NCTS = UART_EVENTS_NCTS_EVENTS_NCTS_NotGenerated;
	}
}

// send character to hardware
static void nrf52840_uart_send_hw(struct uart_api * uart, uint32_t c)
{
	uart_reg state = uart->reg;
	NRF_UART_Type * reg = state->reg;

	// send character
	state->tx_ready = 0;
	reg->EVENTS_TXDRDY = UART_EVENTS_TXDRDY_EVENTS_TXDRDY_NotGenerated;
	reg->TASKS_STARTTX = UART_TASKS_STARTTX_TASKS_STARTTX_Trigger;
	reg->TXD = (uint32_t)c;
}


// send byte
int nrf52840_uart_send_byte(struct uart_api * uart, char c)
{
	int b = OK;

	int_state_t int_state = int_disable_nested();
	int_state_t faultmask = int_get_faultmask();

	if (int_state || faultmask)
	{
		// interrupts were previously disabled
		// async send unavailable
		b = nrf52840_uart_send_byte_sync(uart, c);
	}
	else
	{
		// interrupts were enabled
		uart_reg state = uart->reg;
		NRF_UART_Type * reg = state->reg;

		if ((state->tx_b - state->tx_a) > state->mask)
		{
			// TX queue full
			uint32_t tx_a = state->tx_a;
			int task = task_id_get();
			uart->task_tx = task;

			int_enable_nested(int_state);

			// while queue full
			if (task == FAIL)
			{
				// we are running on bare metal
				// make sure the queue index has changed
				while (state->tx_a == tx_a);
			}
			else
			{
				task_wait();

				if (state->tx_a == tx_a)
				{
					// request aborted
					return FAIL;
				}
			}

			int_state = int_disable_nested();
		}

		if (
			(state->tx_a == state->tx_b) &&
			(reg->EVENTS_TXDRDY || state->tx_ready)
			)
		{
			// queue empty and TX ready
			// send character
			nrf52840_uart_send_hw(uart, c);
		}
		else
		{
			// queue not empty or TX busy
			// enqueue character
			state->tx[state->tx_b & state->mask] = c;
			state->tx_b++;
		}
	}

	int_enable_nested(int_state);

	return b;
}

// send byte sync
int nrf52840_uart_send_byte_sync(struct uart_api * uart, char c)
{
	nrf52840_uart_error_handler(uart);

	uart_reg state = uart->reg;
	NRF_UART_Type * reg = state->reg;

	if (uart->config.use_interrupts)
	{
		// send all enqueued characters
		while (state->tx_a != state->tx_b)
		{
			if (!state->tx_ready)
			{
				// wait until space is available in the FIFO
				while (!reg->EVENTS_TXDRDY);
			}

			// send enqueued character
			nrf52840_uart_send_hw(uart, state->tx[state->tx_a & state->mask]);
			state->tx_a++;
		}
	}

	if (!state->tx_ready)
	{
		// wait until space is available in the FIFO
		while (!reg->EVENTS_TXDRDY);
	}

	// send character
	nrf52840_uart_send_hw(uart, c);

	return OK;
}

// receive byte
int nrf52840_uart_recv_byte(struct uart_api * uart, char * c)
{
	int b = OK;
	uart_reg state = uart->reg;
	NRF_UART_Type * reg = state->reg;

	if (uart->config.use_interrupts)
	{
		int_state_t int_state = int_get_faultmask_or_primask();

		if (!int_state)
		{
			// interrupts are enabled
			int_state = int_disable_nested();

			if (state->rx_a == state->rx_b)
			{
				// RX queue empty
				int task = task_id_get();
				uart->task_rx = task;

				int_enable_nested(int_state);

				// while queue full
				if (task == FAIL)
				{
					// we are running on bare metal
					// make sure the RX queue is not empty
					while (state->rx_a == state->rx_b);
				}
				else
				{
					task_wait();

					if (state->rx_a == state->rx_b)
					{
						// request aborted
						return FAIL;
					}
				}
			}

			int_state = int_disable_nested();

			// one ore more characters are available
			*c = state->rx[state->rx_a & state->mask];
			state->rx_a++;

			int_enable_nested(int_state);

			return b;
		}
	}

	// wait until a character is available
	while (!reg->EVENTS_RXDRDY);

	reg->EVENTS_RXDRDY = UART_EVENTS_RXDRDY_EVENTS_RXDRDY_NotGenerated;
	*c = reg->RXD;

	return b;
}


int nrf52840_uart_send_job(struct uart_api * uart)
{
	// queue characters from job until the job is complete or the the queue is full
	// if the job is not complete, the interrupt handler will complete it and wake us
	uart_reg state = uart->reg;
	NRF_UART_Type * reg = state->reg;
	struct uart_job * job = &uart->job_tx;

	int_state_t int_state = int_disable_nested();

	if (!job->msg)
	{
		// the interrupt handler has already completed the job
		int_enable_nested(int_state);
		return OK;
	}

	if (!reg->EVENTS_TXDRDY && state->tx_ready)
	{
		// TX ready
		// send character
		nrf52840_uart_send_hw(uart, job->msg[job->bytes_transferred]);
		job->bytes_transferred++;
	}

	// while the the job has characters and the TX queue is not full
	while (
		(job->bytes_transferred < job->bytes_requested) &&
		((state->tx_b - state->tx_a) <= state->mask)
		)
	{
		// enqueue character
		state->tx[state->tx_b & state->mask] = job->msg[job->bytes_transferred];
		state->tx_b++;
		job->bytes_transferred++;
	}

	if (job->bytes_transferred < job->bytes_requested)
	{
		int task = task_id_get();
		uart->task_tx = task;

		int_enable_nested(int_state);

		// while queue full
		task_wait();

		return OK;
	}

	// job complete
	uart->job_tx.msg = NULL;
	int_enable_nested(int_state);

	return OK;
}
#endif

int nrf52840_uart_recv_job(struct uart_api * uart)
{
	// receive characters to job until the job is complete or the the queue is empty
	// if the job is not complete, the interrupt handler will complete it and wake us
	// if idle_ends_recv is set and one or more bytes were transferred, we can end the job
	uart_reg state = uart->reg;
	struct uart_job * job = &uart->job_rx;

	int_state_t int_state = int_disable_nested();

	if (!job->msg)
	{
		// the interrupt handler has already completed the job
		int_enable_nested(int_state);
		return OK;
	}

	while (
		(job->bytes_transferred < job->bytes_requested) &&
		(state->rx_a != state->rx_b)
		)
	{
		// receive character
		job->msg[job->bytes_transferred] = state->rx[state->rx_a & state->mask];
		state->rx_a++;
		job->bytes_transferred++;
	}

	if (
		(job->bytes_transferred < job->bytes_requested) &&
		!(job->bytes_transferred && uart->config.idle_ends_recv)
		)
	{
		int task = task_id_get();
		uart->task_rx = task;

		int_enable_nested(int_state);

		// while queue full
		task_wait();

		return OK;
	}

	// job complete
	uart->job_rx.msg = NULL;
	int_enable_nested(int_state);

	return OK;
}

#if !NRF52840_USE_DMA
void __attribute__((interrupt)) nrf52840_uart_handler_raw_0(void)
{
	if (nrf52840_links[0])
	{
		nrf52840_uart_handler_int(nrf52840_links[0], 0);
	}
}

int nrf52840_uart_handler_int(struct uart_api * uart, int int_state)
{
	uart_reg state = uart->reg;
	NRF_UART_Type * reg = state->reg;

	nrf52840_uart_error_handler(uart);

	// clear RX timeout
	if (reg->EVENTS_RXTO)
	{
		reg->EVENTS_RXTO = UART_EVENTS_RXTO_EVENTS_RXTO_NotGenerated;
	}

	// received one or more characters
	while (reg->EVENTS_RXDRDY)
	{
		reg->EVENTS_RXDRDY = UART_EVENTS_RXDRDY_EVENTS_RXDRDY_NotGenerated;

		// receive character
		uint32_t c = reg->RXD;
		state->rx[state->rx_b & state->mask] = (char)c;
		state->rx_b++;

		if (uart->job_rx.msg)
		{
			uint32_t rx_complete = 0;

			do
			{
				uart->job_rx.msg[uart->job_rx.bytes_transferred] = state->rx[state->rx_a & state->mask];
				state->rx_a++;
				uart->job_rx.bytes_transferred++;

				if (uart->job_rx.bytes_transferred >= uart->job_rx.bytes_requested)
				{
					rx_complete = 1;
					break;
				}
			} while (state->rx_a != state->rx_b);

			if (rx_complete || uart->config.idle_ends_recv)
			{
				uart->job_rx.msg = NULL;

				// wake the driver task to resume processing
				uart->uart_wake_rx(uart);
			}
		}
		else if ((uart->task_rx != FAIL) && (state->rx_a != state->rx_b))
		{
			// wake the driver task to resume processing
			uart->uart_wake_rx(uart);
		}

		if ((state->rx_b - state->rx_a) > state->mask)
		{
			// RX queue full, drop the oldest character
			state->rx_a = state->rx_b - state->mask;
		}

		if (state->loopback)
		{
			if ((state->tx_b - state->tx_a) > state->mask)
			{
				// wait until space is available in the FIFO
				if (!state->tx_ready)
				{
					while (!reg->EVENTS_TXDRDY);
				}

				// send enqueued character
				nrf52840_uart_send_hw(uart, (uint32_t)state->tx[state->tx_a & state->mask]);
				state->tx_a++;
			}

			// enqueue character
			state->tx[state->tx_b & state->mask] = (char)c;
			state->tx_b++;
		}
	}

	// ready to send
	if (reg->EVENTS_TXDRDY)
	{
		reg->EVENTS_TXDRDY = UART_EVENTS_TXDRDY_EVENTS_TXDRDY_NotGenerated;
		reg->TASKS_STARTTX = UART_TASKS_STARTTX_TASKS_STARTTX_Trigger;

		if (state->tx_a != state->tx_b)
		{
			// the queue is not empty
			// send enqueued character
			state->tx_ready = 0;
			reg->TXD = (uint32_t)state->tx[state->tx_a & state->mask];
			state->tx_a++;

			if (uart->job_tx.msg)
			{
				// while the the job has characters and the TX queue is not full
				do
				{
					// enqueue character
					state->tx[state->tx_b & state->mask] = uart->job_tx.msg[uart->job_tx.bytes_transferred];
					state->tx_b++;
					uart->job_tx.bytes_transferred++;

					if (uart->job_tx.bytes_transferred >= uart->job_tx.bytes_requested)
					{
						uart->job_tx.msg = NULL;

						// wake the driver task to resume processing
						uart->uart_wake_tx(uart);
						break;
					}
				} while ((state->tx_b - state->tx_a) <= state->mask);
			}
			else if ((uart->task_tx != FAIL) && ((state->tx_b - state->tx_a) <= state->mask))
			{
				// wake the driver task to resume processing
				uart->uart_wake_tx(uart);
			}
		}
		else
		{
			// the queue is empty
			state->tx_ready = 1;
		}
	}

	return 0;
}
#endif
