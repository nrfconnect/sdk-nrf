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

#define UNUSED_FLAGS 0


void IPC_IRQHandler(void);


s32_t bsd_os_timedwait(u32_t context, u32_t timeout)
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
/* Keyrelated errnos are not included in zephyrs errno.h.
 *	case NRF_ENOKEY:
 *		errno = ENOKEY;
 *		break;
 *	case NRF_EKEYEXPIRED:
 *		errno = EKEYEXPIRED;
 *		break;
 *	case NRF_EKEYREVOKED:
 *		errno = EKEYREVOKED;
 *		break;
 *	case NRF_EKEYREJECTED:
 *		errno = EKEYREJECTED;
 *		break;
 */
	case NRF_ENOKEY:
	case NRF_EKEYEXPIRED:
	case NRF_EKEYREVOKED:
	case NRF_EKEYREJECTED:
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


void read_task_create(void)
{
	IRQ_DIRECT_CONNECT(BSD_APPLICATION_IRQ, BSD_APPLICATION_IRQ_PRIORITY,
			   rpc_proxy_irq_handler, UNUSED_FLAGS);
	irq_enable(BSD_APPLICATION_IRQ);
}

/* This function is called by bsd_init and must not be called explicitly. */
void bsd_os_init(void)
{
	read_task_create();
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
