/****************************************************************************
 *
 * File:          nrf52840_uart_dma.c
 * Author(s):     Georgi Valkov (GV)
 * Project:       UART miniport driver for nrf52840
 * Description:   Hardware layer for DMA
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


/* Types ********************************************************************/

/* Global variables *********************************************************/
#if NRF52840_USE_DMA
extern struct uart_api * nrf52840_links[2];

fn_int_handler nrf52840_uart_handlers_dma[] =
{
	nrf52840_uart_handler_raw_dma_0,
	nrf52840_uart_handler_raw_dma_1,
};
#endif


/* Functions ****************************************************************/

#if NRF52840_USE_DMA
// deinit hardware interface with DMA
void nrf52840_uart_deinit_dma(struct uart_api * uart)
{
	int_state_t int_state = int_disable_nested();
	uart_reg state = uart->reg;
	NRF_UARTE_Type * reg = state->reg_dma;

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

	if (reg->ENABLE == UARTE_ENABLE_ENABLE_Enabled)
	{
		reg->EVENTS_TXSTOPPED = UARTE_EVENTS_TXSTOPPED_EVENTS_TXSTOPPED_NotGenerated;
		reg->TASKS_STOPRX = UARTE_TASKS_STOPRX_TASKS_STOPRX_Trigger;
		reg->TASKS_STOPTX = UARTE_TASKS_STOPTX_TASKS_STOPTX_Trigger;

		// wait for TX DMA to stop
		while (!reg->EVENTS_TXSTOPPED);

		reg->EVENTS_TXSTOPPED = UARTE_EVENTS_TXSTOPPED_EVENTS_TXSTOPPED_NotGenerated;
		reg->EVENTS_ENDRX = UARTE_EVENTS_ENDRX_EVENTS_ENDRX_NotGenerated;
		reg->EVENTS_ENDTX = UARTE_EVENTS_ENDTX_EVENTS_ENDTX_NotGenerated;

		char tmp = '\0';
		reg->RXD.PTR = (uint32_t)&tmp;
		reg->TXD.PTR = (uint32_t)&tmp;
		reg->RXD.MAXCNT = 0;
		reg->TXD.MAXCNT = 0;

		// perform an empty DMA to clear RXD.AMOUNT and TXD.AMOUNT
		reg->TASKS_FLUSHRX = UARTE_TASKS_FLUSHRX_TASKS_FLUSHRX_Trigger;
		reg->TASKS_STARTTX = UARTE_TASKS_STARTTX_TASKS_STARTTX_Trigger;

		while (!reg->EVENTS_ENDRX && !reg->EVENTS_ENDTX);

		reg->TASKS_STOPRX = UARTE_TASKS_STOPRX_TASKS_STOPRX_Trigger;
		reg->TASKS_STOPTX = UARTE_TASKS_STOPTX_TASKS_STOPTX_Trigger;

		while (!reg->EVENTS_TXSTOPPED);
	}

	reg->ENABLE = UART_ENABLE_ENABLE_Disabled;

	UARTE_PSEL_Type pin_select =
	{
		.RTS = -1,
		.TXD = -1,
		.CTS = -1,
		.RXD = -1,
	};

	reg->PSEL = pin_select;

	reg->ERRORSRC = -1;

	reg->EVENTS_CTS = UARTE_EVENTS_CTS_EVENTS_CTS_NotGenerated;
	reg->EVENTS_NCTS = UARTE_EVENTS_NCTS_EVENTS_NCTS_NotGenerated;
	reg->EVENTS_RXDRDY = UARTE_EVENTS_RXDRDY_EVENTS_RXDRDY_NotGenerated;
	reg->EVENTS_ENDRX = UARTE_EVENTS_ENDRX_EVENTS_ENDRX_NotGenerated;
	reg->EVENTS_TXDRDY = UARTE_EVENTS_TXDRDY_EVENTS_TXDRDY_NotGenerated;
	reg->EVENTS_ENDTX = UARTE_EVENTS_ENDTX_EVENTS_ENDTX_NotGenerated;
	reg->EVENTS_ERROR = UARTE_EVENTS_ERROR_EVENTS_ERROR_NotGenerated;
	reg->EVENTS_RXTO = UARTE_EVENTS_RXTO_EVENTS_RXTO_NotGenerated;
	reg->EVENTS_RXSTARTED = UARTE_EVENTS_RXSTARTED_EVENTS_RXSTARTED_NotGenerated;
	reg->EVENTS_TXSTARTED = UARTE_EVENTS_TXSTARTED_EVENTS_TXSTARTED_NotGenerated;
	reg->EVENTS_TXSTOPPED = UARTE_EVENTS_TXSTOPPED_EVENTS_TXSTOPPED_NotGenerated;

	uart->abort(uart);

	int_enable_nested(int_state);
}

// init hardware interface with DMA
int nrf52840_uart_init_dma(struct uart_api * uart)
{
	if (uart->status.valid)
	{
		// already initialised
		// update the configuration
		return nrf52840_uart_configure_dma(uart);
	}

	uart->deinit(uart);

	uart_reg state = uart->reg;
	NRF_UARTE_Type * reg = state->reg_dma;
	uart_config * config = &uart->config;

	if (OK != nrf52840_uart_configure_dma(uart))
	{
		return FAIL;
	}

	int_state_t int_state = int_disable_nested();

	// link used by the interrupt handler
	nrf52840_links[uart->index] = uart;

	if (OK == int_set_vector(uart->int_number, nrf52840_uart_handlers_dma[uart->index]))
	{
		int_source_configure(uart->int_number, config->int_priority, 0);
		int_source_enable(uart->int_number);

		// 0x00000280  TXSTOPPED=40 0000  TXSTARTED=10 0000  RXSTARTED=8 0000  RXTO=2 0000
		// ERROR=0200  ENDTX=0100  TXRDY=0080  ENDRX=0010  RXRDY=0004  NCTS=0002  CTS=0001
		reg->INTENSET =
			// UARTE_INTENSET_TXSTOPPED_Msk |
			// UARTE_INTENSET_TXSTARTED_Msk |
			// UARTE_INTENSET_RXSTARTED_Msk |
			// UARTE_INTENSET_RXTO_Msk |
			UARTE_INTENSET_ERROR_Msk |
			UARTE_INTENSET_ENDTX_Msk |
			UARTE_INTENSET_ENDRX_Msk |
			UARTE_INTENSET_TXDRDY_Msk |
			UARTE_INTENSET_RXDRDY_Msk |
			// UARTE_INTENSET_NCTS_Msk |
			// UARTE_INTENSET_CTS_Msk |
			0;
	}
	else
	{
		int_enable_nested(int_state);
		return FAIL;
	}

	config->use_interrupts = 1;
	state->tx_ready = 1;
	uart->status.valid = 1;

	reg->SHORTS =
		(UARTE_SHORTS_ENDRX_STOPRX_Disabled << UARTE_SHORTS_ENDRX_STOPRX_Pos) |
		(UARTE_SHORTS_ENDRX_STARTRX_Disabled << UARTE_SHORTS_ENDRX_STARTRX_Pos) |
		0;

	// assign DMA buffers
	reg->RXD.PTR = (uint32_t)state->rx;
	reg->TXD.PTR = (uint32_t)state->tx;
	reg->RXD.MAXCNT = state->size;
	reg->TXD.MAXCNT = 0;

	reg->ENABLE = UARTE_ENABLE_ENABLE_Enabled;
	reg->TASKS_STARTRX = UARTE_TASKS_STARTRX_TASKS_STARTRX_Trigger;

	reg->EVENTS_CTS = UARTE_EVENTS_CTS_EVENTS_CTS_NotGenerated;
	reg->EVENTS_NCTS = UARTE_EVENTS_NCTS_EVENTS_NCTS_NotGenerated;
	reg->EVENTS_RXDRDY = UARTE_EVENTS_RXDRDY_EVENTS_RXDRDY_NotGenerated;
	reg->EVENTS_ENDRX = UARTE_EVENTS_ENDRX_EVENTS_ENDRX_NotGenerated;
	reg->EVENTS_TXDRDY = UARTE_EVENTS_TXDRDY_EVENTS_TXDRDY_NotGenerated;
	reg->EVENTS_ENDTX = UARTE_EVENTS_ENDTX_EVENTS_ENDTX_NotGenerated;
	reg->EVENTS_ERROR = UARTE_EVENTS_ERROR_EVENTS_ERROR_NotGenerated;
	reg->EVENTS_RXTO = UARTE_EVENTS_RXTO_EVENTS_RXTO_NotGenerated;
	reg->EVENTS_RXSTARTED = UARTE_EVENTS_RXSTARTED_EVENTS_RXSTARTED_NotGenerated;
	reg->EVENTS_TXSTARTED = UARTE_EVENTS_TXSTARTED_EVENTS_TXSTARTED_NotGenerated;
	reg->EVENTS_TXSTOPPED = UARTE_EVENTS_TXSTOPPED_EVENTS_TXSTOPPED_NotGenerated;

	int_enable_nested(int_state);

	return OK;
}

int nrf52840_uart_configure_dma(struct uart_api * uart)
{
	if (OK != uart->set_baud_rate(uart, 0, 0))
	{
		return FAIL;
	}

	uart_reg state = uart->reg;
	NRF_UARTE_Type * reg = state->reg_dma;
	uart_config * config = &uart->config;

	if (state->pin_select_dma.CTS & state->pin_select_dma.RTS & NRF52840_UART_PSEL_DISCONNECT)
	{
		// CTS and RTS not connected, disable flow control
		config->flow_control = 0;
	}

	reg->CONFIG =
		((config->stop_bits_2 ? UARTE_CONFIG_STOP_Two : UARTE_CONFIG_STOP_One) << UARTE_CONFIG_STOP_Pos) |
		((config->parity ? UARTE_CONFIG_PARITY_Included : UARTE_CONFIG_PARITY_Excluded) << UARTE_CONFIG_PARITY_Pos) |
		((config->flow_control ? UARTE_CONFIG_HWFC_Enabled : UARTE_CONFIG_HWFC_Disabled) << UARTE_CONFIG_HWFC_Pos) |
		0;

	reg->PSEL = state->pin_select_dma;

	return OK;
}


// error handler
static void nrf52840_uart_error_handler_dma(struct uart_api * uart)
{
	NRF_UARTE_Type * reg = uart->reg->reg_dma;

	// clear error sources
	if (reg->ERRORSRC)
	{
		reg->ERRORSRC = -1;
	}

	// clear errors
	if (reg->EVENTS_ERROR)
	{
		reg->EVENTS_ERROR = UARTE_EVENTS_ERROR_EVENTS_ERROR_NotGenerated;
	}

	// clear CTS event
	if (reg->EVENTS_CTS)
	{
		reg->EVENTS_CTS = UARTE_EVENTS_CTS_EVENTS_CTS_NotGenerated;
	}

	// clear NCTS event
	if (reg->EVENTS_NCTS)
	{
		reg->EVENTS_NCTS = UARTE_EVENTS_NCTS_EVENTS_NCTS_NotGenerated;
	}
}

static void nrf52840_uart_start_rx_dma(struct uart_api * uart)
{
	uart_reg state = uart->reg;
	NRF_UARTE_Type * reg = state->reg_dma;

	// clear events
	reg->EVENTS_RXDRDY = UARTE_EVENTS_RXDRDY_EVENTS_RXDRDY_NotGenerated;
	reg->EVENTS_ENDRX = UARTE_EVENTS_ENDRX_EVENTS_ENDRX_NotGenerated;

	// update the start index
	state->rx_B += reg->RXD.AMOUNT;
	state->rx_b = state->rx_B;
	uint32_t index = state->rx_b & state->mask;

	if ((state->rx_b - state->rx_a) > state->mask)
	{
		// RX queue full, drop the oldest character
		// count will be set to 1
		state->rx_a = state->rx_b - state->mask;
	}

	uint32_t count = state->size + state->rx_a - state->rx_b;

	if ((index + count) > state->size)
	{
		// count ends past the end of the RX buffer, truncate it
		// the remaining data requires a new DMA request
		count = state->size - index;
	}

	// assign DMA buffers
	reg->RXD.PTR = (uint32_t)state->rx + index;
	reg->RXD.MAXCNT = count;

	// start TX DMA
	reg->TASKS_STARTRX = UARTE_TASKS_STARTRX_TASKS_STARTRX_Trigger;
}

static void nrf52840_uart_start_tx_dma(struct uart_api * uart)
{
	uart_reg state = uart->reg;
	NRF_UARTE_Type * reg = state->reg_dma;

	// clear events
	reg->EVENTS_TXDRDY = UARTE_EVENTS_TXDRDY_EVENTS_TXDRDY_NotGenerated;
	reg->EVENTS_ENDTX = UARTE_EVENTS_ENDTX_EVENTS_ENDTX_NotGenerated;
	state->tx_ready = 0;

	// update the start index
	state->tx_A += reg->TXD.AMOUNT;
	state->tx_a = state->tx_A;
	uint32_t index = state->tx_a & state->mask;
	uint32_t count = state->tx_b - state->tx_a;


	if ((index + count) > state->size)
	{
		// count ends past the end of the TX buffer, truncate it
		// the remaining data requires a new DMA request
		count = state->size - index;
	}

	// assign DMA buffers
	reg->TXD.PTR = (uint32_t)state->tx + index;
	reg->TXD.MAXCNT = count;

	// start TX DMA
	reg->TASKS_STARTTX = UARTE_TASKS_STARTTX_TASKS_STARTTX_Trigger;
}


// send byte with DMA
int nrf52840_uart_send_byte_dma(struct uart_api * uart, char c)
{
	uart_reg state = uart->reg;
	NRF_UARTE_Type * reg = state->reg_dma;

	int_state_t int_state = int_disable_nested();
	int_state_t faultmask = int_get_faultmask();

	if (int_state || faultmask)
	{
		// interrupts were previously disabled
		// async send unavailable
		nrf52840_uart_error_handler_dma(uart);

		if ((state->tx_b - state->tx_a) > state->mask)
		{
			// TX queue full
			// wait intil a character has been sent
			while (!reg->EVENTS_TXDRDY);

			reg->EVENTS_TXDRDY = UARTE_EVENTS_TXDRDY_EVENTS_TXDRDY_NotGenerated;

			// update the start index
			state->tx_a++;
		}
	}
	else if ((state->tx_b - state->tx_a) > state->mask)
	{
		// interrupts were enabled and TX queue full
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

	// enqueue character
	state->tx[state->tx_b & state->mask] = (char)c;
	state->tx_b++;

	if (reg->EVENTS_ENDTX || state->tx_ready)
	{
		// TX ready
		// start TX DMA
		nrf52840_uart_start_tx_dma(uart);
	}

	int_enable_nested(int_state);

	return OK;
}

// receive byte with DMA
int nrf52840_uart_recv_byte_dma(struct uart_api * uart, char * c)
{
	int b = OK;
	uart_reg state = uart->reg;
	NRF_UARTE_Type * reg = state->reg_dma;

	int_state_t int_state = int_disable_nested();
	int_state_t faultmask = int_get_faultmask();

	if (int_state || faultmask)
	{
		// interrupts are disabled
		if (state->rx_a == state->rx_b)
		{
			// wait until a character is available
			while (!reg->EVENTS_RXDRDY);

			reg->EVENTS_RXDRDY = UARTE_EVENTS_RXDRDY_EVENTS_RXDRDY_NotGenerated;

			// a character has been received, update the end index
			state->rx_b++;
		}

		// one ore more characters are available
		*c = state->rx[state->rx_a & state->mask];
		state->rx_a++;

		// RX buffer full
		if (reg->EVENTS_ENDRX)
		{
			nrf52840_uart_start_rx_dma(uart);
		}
	}
	else
	{
		// interrupts are enabled
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
	}

	int_enable_nested(int_state);

	return b;
}

int nrf52840_uart_send_job_dma(struct uart_api * uart)
{
	// queue characters from job until the job is complete or the the queue is full
	// if the job is not complete, the interrupt handler will complete it and wake us
	uart_reg state = uart->reg;
	NRF_UARTE_Type * reg = state->reg_dma;
	struct uart_job * job = &uart->job_tx;

	int_state_t int_state = int_disable_nested();

	if (!job->msg)
	{
		// the interrupt handler has already completed the job
		int_enable_nested(int_state);
		return OK;
	}

	uint32_t tx_queued = 0;

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
		tx_queued = 1;
	}

	if (!reg->EVENTS_ENDTX && state->tx_ready && tx_queued)
	{
		// TX ready
		// start TX DMA
		nrf52840_uart_start_tx_dma(uart);
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

static void nrf52840_uart_send_loopback(struct uart_api * uart)
{
	uart_reg state = uart->reg;
	NRF_UARTE_Type * reg = state->reg_dma;

	uint32_t tx_queued = 0;

	// while the RX queue has characters and the TX queue is not full
	while ((state->rx_a != state->rx_b) && ((state->tx_b - state->tx_a) <= state->mask))
	{
		// enqueue character
		state->tx[state->tx_b & state->mask] = state->rx[state->rx_a & state->mask];
		state->tx_b++;
		state->rx_a++;
		tx_queued = 1;
	}

	if (!reg->EVENTS_ENDTX && state->tx_ready && tx_queued)
	{
		// TX ready
		// start TX DMA
		nrf52840_uart_start_tx_dma(uart);
	}
}

void __attribute__((interrupt)) nrf52840_uart_handler_raw_dma_0(void)
{
	if (nrf52840_links[0])
	{
		nrf52840_uart_handler_int_dma(nrf52840_links[0], 0);
	}
}

void __attribute__((interrupt)) nrf52840_uart_handler_raw_dma_1(void)
{
	if (nrf52840_links[1])
	{
		nrf52840_uart_handler_int_dma(nrf52840_links[1], 0);
	}
}

int nrf52840_uart_handler_int_dma(struct uart_api * uart, int int_state)
{
	uart_reg state = uart->reg;
	NRF_UARTE_Type * reg = state->reg_dma;

	nrf52840_uart_error_handler_dma(uart);

	// clear RX timeout
	if (reg->EVENTS_RXTO)
	{
		reg->EVENTS_RXTO = UARTE_EVENTS_RXTO_EVENTS_RXTO_NotGenerated;
	}

	// RX started
	if (reg->EVENTS_RXSTARTED)
	{
		reg->EVENTS_RXSTARTED = UARTE_EVENTS_RXSTARTED_EVENTS_RXSTARTED_NotGenerated;
	}

	// received a character, may not be transferred to RX RAM yet
	if (reg->EVENTS_RXDRDY)
	{
		reg->EVENTS_RXDRDY = UARTE_EVENTS_RXDRDY_EVENTS_RXDRDY_NotGenerated;

		// a character has been received, update the end index
		state->rx_b++;

		if (uart->job_rx.msg)
		{
			uint32_t rx_complete = 0;
			uint32_t tx_queued = 0;

			do
			{
				uart->job_rx.msg[uart->job_rx.bytes_transferred] = state->rx[state->rx_a & state->mask];

				if (state->loopback && ((state->tx_b - state->tx_a) <= state->mask))
				{
					// enqueue character
					state->tx[state->tx_b & state->mask] = state->rx[state->rx_a & state->mask];
					state->tx_b++;
					tx_queued = 1;
				}

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

			if (!reg->EVENTS_ENDTX && state->tx_ready && tx_queued)
			{
				// TX ready
				// start TX DMA
				nrf52840_uart_start_tx_dma(uart);
			}
		}
		else
		{
			if ((uart->task_rx != FAIL) && (state->rx_a != state->rx_b))
			{
				// wake the driver task to resume processing
				uart->uart_wake_rx(uart);
			}

			if (state->loopback)
			{
				nrf52840_uart_send_loopback(uart);
			}
		}
	}

	// RX buffer full
	if (reg->EVENTS_ENDRX)
	{
		nrf52840_uart_start_rx_dma(uart);
	}

	// TX stopped
	if (reg->EVENTS_TXSTOPPED)
	{
		reg->EVENTS_TXSTOPPED = UARTE_EVENTS_TXSTOPPED_EVENTS_TXSTOPPED_NotGenerated;
	}

	// TX started
	if (reg->EVENTS_TXSTARTED)
	{
		reg->EVENTS_TXSTARTED = UARTE_EVENTS_TXSTARTED_EVENTS_TXSTARTED_NotGenerated;
	}

	// sent a character from TX RAM
	if (reg->EVENTS_TXDRDY)
	{
		reg->EVENTS_TXDRDY = UARTE_EVENTS_TXDRDY_EVENTS_TXDRDY_NotGenerated;

		// a character has been sent, update the start index
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

		if (state->loopback)
		{
			nrf52840_uart_send_loopback(uart);
		}
	}

	// TX buffer sent
	if (reg->EVENTS_ENDTX)
	{
		if (state->tx_b - state->tx_A - reg->TXD.AMOUNT)
		{
			// the queue is not empty
			// start TX DMA
			nrf52840_uart_start_tx_dma(uart);
		}
		else
		{
			// the queue is empty
			reg->EVENTS_ENDTX = UARTE_EVENTS_ENDTX_EVENTS_ENDTX_NotGenerated;
			state->tx_ready = 1;
		}
	}

	return 0;
}
#endif
