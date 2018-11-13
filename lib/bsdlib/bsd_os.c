/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <bsd_os.h>
#include <bsd.h>
#include <bsd_platform.h>
#include <nrf.h>

#include <nrf_errno.h>

#include <init.h>
#include <zephyr.h>
#include <zephyr/types.h>
#include <errno.h>

#ifdef CONFIG_BSD_LIBRARY_TRACE_ENABLED
#include <nrfx_uarte.h>
#endif

#ifndef ENOKEY
#define ENOKEY 2001
#endif

#ifndef EKEYEXPIRED
#define EKEYEXPIRED 2002
#endif

#ifndef EKEYREVOKED
#define EKEYREVOKED 2003
#endif

#ifndef EKEYREJECTED
#define EKEYREJECTED 2004
#endif

#define UNUSED_FLAGS 0

/* Handle modem traces from IRQ context with lower priority. */
#define TRACE_IRQ EGU2_IRQn
#define TRACE_IRQ_PRIORITY 6

#ifdef CONFIG_BSD_LIBRARY_TRACE_ENABLED
/* Use UARTE1 as a dedicated peripheral to print traces. */
static const nrfx_uarte_t uarte_inst = NRFX_UARTE_INSTANCE(1);
#endif

void IPC_IRQHandler(void);

int32_t bsd_os_timedwait(uint32_t context, uint32_t timeout)
{
	/* TODO: to be implemented */
	return 0;
}


void bsd_os_errno_set(int err_code)
{
	switch (err_code) {
	case NRF_EPERM:
		errno = EPERM;
		break;
	case NRF_ENOENT:
		errno = ENOENT;
		break;
	case NRF_EIO:
		errno = EIO;
		break;
	case NRF_EBADF:
		errno = EBADF;
		break;
	case NRF_ENOMEM:
		errno = ENOMEM;
		break;
	case NRF_EACCES:
		errno = EACCES;
		break;
	case NRF_EFAULT:
		errno = EFAULT;
		break;
	case NRF_EINVAL:
		errno = EINVAL;
		break;
	case NRF_EMFILE:
		errno = EMFILE;
		break;
	case NRF_EAGAIN:
		errno = EAGAIN;
		break;
	case NRF_EPROTOTYPE:
		errno = EPROTOTYPE;
		break;
	case NRF_ENOPROTOOPT:
		errno = ENOPROTOOPT;
		break;
	case NRF_EPROTONOSUPPORT:
		errno = EPROTONOSUPPORT;
		break;
	case NRF_ESOCKTNOSUPPORT:
		errno = ESOCKTNOSUPPORT;
		break;
	case NRF_EOPNOTSUPP:
		errno = EOPNOTSUPP;
		break;
	case NRF_EAFNOSUPPORT:
		errno = EAFNOSUPPORT;
		break;
	case NRF_EADDRINUSE:
		errno = EADDRINUSE;
		break;
	case NRF_ENETDOWN:
		errno = ENETDOWN;
		break;
	case NRF_ENETUNREACH:
		errno = ENETUNREACH;
		break;
	case NRF_ECONNRESET:
		errno = ECONNRESET;
		break;
	case NRF_EISCONN:
		errno = EISCONN;
		break;
	case NRF_ENOTCONN:
		errno = ENOTCONN;
		break;
	case NRF_ETIMEDOUT:
		errno = ETIMEDOUT;
		break;
	case NRF_ENOBUFS:
		errno = ENOBUFS;
		break;
	case NRF_EHOSTDOWN:
		errno = EHOSTDOWN;
		break;
	case NRF_EINPROGRESS:
		errno = EINPROGRESS;
		break;
	case NRF_ECANCELED:
		errno = ECANCELED;
		break;
	case NRF_ENOKEY:
		errno = ENOKEY;
		break;
	case NRF_EKEYEXPIRED:
		errno = EKEYEXPIRED;
		break;
	case NRF_EKEYREVOKED:
		errno = EKEYREVOKED;
		break;
	case NRF_EKEYREJECTED:
		errno = EKEYREJECTED;
		break;
	default:
		errno = EINVAL;
		break;
	}
}

void bsd_os_application_irq_set(void)
{
	NVIC_SetPendingIRQ(BSD_APPLICATION_IRQ);
}

void bsd_os_application_irq_clear(void)
{
	NVIC_ClearPendingIRQ(BSD_APPLICATION_IRQ);
}

void bsd_os_trace_irq_set(void)
{
	NVIC_SetPendingIRQ(TRACE_IRQ);
}

void bsd_os_trace_irq_clear(void)
{
	NVIC_ClearPendingIRQ(TRACE_IRQ);
}

ISR_DIRECT_DECLARE(ipc_proxy_irq_handler)
{
	IPC_IRQHandler();
	ISR_DIRECT_PM(); /* PM done after servicing interrupt for best latency
			  */
	return 1; /* We should check if scheduling decision should be made */
}

ISR_DIRECT_DECLARE(rpc_proxy_irq_handler)
{
	bsd_os_application_irq_handler();
	ISR_DIRECT_PM(); /* PM done after servicing interrupt for best latency
			  */
	return 1; /* We should check if scheduling decision should be made */
}

ISR_DIRECT_DECLARE(trace_proxy_irq_handler)
{
	/*
	 * Process traces.
	 * The function has to be called even if UART traces are disabled.
	 */
	bsd_os_trace_irq_handler();
	ISR_DIRECT_PM(); /* PM done after servicing interrupt for best latency
			  */
	return 1; /* We should check if scheduling decision should be made */
}

void trace_task_create(void)
{
	IRQ_DIRECT_CONNECT(TRACE_IRQ, TRACE_IRQ_PRIORITY,
			   trace_proxy_irq_handler, UNUSED_FLAGS);
	irq_enable(TRACE_IRQ);
}

void read_task_create(void)
{
	IRQ_DIRECT_CONNECT(BSD_APPLICATION_IRQ, BSD_APPLICATION_IRQ_PRIORITY,
			   rpc_proxy_irq_handler, UNUSED_FLAGS);
	irq_enable(BSD_APPLICATION_IRQ);
}

void trace_uart_init(void)
{
#ifdef CONFIG_BSD_LIBRARY_TRACE_ENABLED
	/* UART pins are defined in "nrf9160_pca10090.dts". */
	const nrfx_uarte_config_t config = {
		/* Use UARTE1 pins routed on VCOM2. */
		.pseltxd = CONFIG_UART_1_TX_PIN,
		.pselrxd = CONFIG_UART_1_RX_PIN,
		.pselcts = CONFIG_UART_1_CTS_PIN,
		.pselrts = CONFIG_UART_1_RTS_PIN,

		.hwfc = NRF_UARTE_HWFC_DISABLED,
		.parity = NRF_UARTE_PARITY_EXCLUDED,
		.baudrate = NRF_UARTE_BAUDRATE_1000000,

		/* IRQ handler not used. Blocking mode.*/
		.interrupt_priority = NRFX_UARTE_DEFAULT_CONFIG_IRQ_PRIORITY,
		.p_context = NULL,
	};

	/* Initialize nrfx UARTE driver in blocking mode. */
	/* TODO: use UARTE in non-blocking mode with IRQ handler. */
	nrfx_uarte_init(&uarte_inst, &config, NULL);
#endif
}

/* This function is called by bsd_init and must not be called explicitly. */
void bsd_os_init(void)
{
	read_task_create();

	/* Configure and enable modem tracing over UART. */
	trace_uart_init();
	trace_task_create();
}

int32_t bsd_os_trace_put(const uint8_t * const data, uint32_t len)
{
#ifdef CONFIG_BSD_LIBRARY_TRACE_ENABLED
	/* FIXME: Due to a bug in nrfx, max DMA transfers are 255 bytes. */

	/* Split RAM buffer into smaller chunks to be transferred using DMA. */
	u32_t remaining_bytes = len;

	while (remaining_bytes) {
		u8_t transfer_len = min(remaining_bytes, UINT8_MAX);
		u32_t idx = len - remaining_bytes;

		nrfx_uarte_tx(&uarte_inst, &data[idx], transfer_len);
		remaining_bytes -= transfer_len;
	}
#endif

	return 0;
}

static int _bsd_driver_init(struct device *unused)
{
	/* Setup the two IRQs used by the BSD library.
	 * Note: No enable irq_enable here. This is done through bsd_init.
	 */
	IRQ_DIRECT_CONNECT(BSD_NETWORK_IRQ, BSD_NETWORK_IRQ_PRIORITY,
			   ipc_proxy_irq_handler, UNUSED_FLAGS);
	bsd_init();

	return 0;
}

SYS_INIT(_bsd_driver_init, POST_KERNEL, 0);
