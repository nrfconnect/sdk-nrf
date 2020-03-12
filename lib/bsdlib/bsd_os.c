/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <string.h>
#include <bsd_os.h>
#include <bsd_platform.h>

#include <nrf.h>
#include <nrf_errno.h>

#include <init.h>
#include <zephyr.h>
#include <zephyr/types.h>
#include <errno.h>
#include <logging/log.h>

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

#define THREAD_MONITOR_ENTRIES 10

LOG_MODULE_REGISTER(bsdlib);

struct sleeping_thread {
	sys_snode_t node;
	struct k_sem sem;
};

/* An array of thread ID and RPC counter pairs, used to avoid race conditions.
 * It allows to identify whether it is safe to put the thread to sleep or not.
 */
static struct thread_monitor_entry {
	k_tid_t id; /* Thread ID. */
	int cnt; /* Last RPC event count. */
} thread_event_monitor[THREAD_MONITOR_ENTRIES];

/* A list of threads that are sleeping and should be woken up on next event. */
static sys_slist_t sleeping_threads;

/* RPC event counter, incremented on each RPC event. */
static atomic_t rpc_event_cnt;

/* Get thread monitor structure assigned to a specific thread id, with a RPC
 * counter value at which bsdlib last checked the 'readiness' of a thread
 */
static struct thread_monitor_entry *thread_monitor_entry_get(k_tid_t id)
{
	struct thread_monitor_entry *entry = thread_event_monitor;
	struct thread_monitor_entry *new_entry = thread_event_monitor;
	int entry_age, oldest_entry_age = 0;

	for ( ; PART_OF_ARRAY(thread_event_monitor, entry); entry++) {
		if (entry->id == id) {
			return entry;
		} else if (entry->id == 0) {
			/* Uninitialized field. */
			new_entry = entry;
			break;
		}

		/* Identify oldest entry. */
		entry_age = rpc_event_cnt - entry->cnt;
		if (entry_age > oldest_entry_age) {
			oldest_entry_age = entry_age;
			new_entry = entry;
		}
	}

	new_entry->id = id;
	new_entry->cnt = rpc_event_cnt - 1;

	return new_entry;
}

/* Update thread monitor entry RPC counter. */
static void thread_monitor_entry_update(struct thread_monitor_entry *entry)
{
	entry->cnt = rpc_event_cnt;
}

/* Verify that thread can be put into sleep (no RPC event occured in a
 * meantime), or whether we should return to bsdlib to re-verify if a sleep is
 * needed.
 */
static bool can_thread_sleep(struct thread_monitor_entry *entry)
{
	bool allow_to_sleep = true;

	if (rpc_event_cnt != entry->cnt) {
		thread_monitor_entry_update(entry);
		allow_to_sleep = false;
	}

	return allow_to_sleep;
}

/* Initialize sleeping thread structure. */
static void sleeping_thread_init(struct sleeping_thread *thread)
{
	k_sem_init(&thread->sem, 0, 1);
}

/* Add thread to the sleeping threads list. Will return information whether
 * the thread was allowed to sleep or not.
 */
static bool sleeping_thread_add(struct sleeping_thread *thread)
{
	bool allow_to_sleep = false;
	struct thread_monitor_entry *entry;

	u32_t key = irq_lock();

	entry = thread_monitor_entry_get(k_current_get());

	if (can_thread_sleep(entry)) {
		allow_to_sleep = true;
		sys_slist_append(&sleeping_threads, &thread->node);
	}

	irq_unlock(key);

	return allow_to_sleep;
}

/* Remove a thread form the sleeping threads list. */
static void sleeping_thread_remove(struct sleeping_thread *thread)
{
	struct thread_monitor_entry *entry;

	u32_t key = irq_lock();

	sys_slist_find_and_remove(&sleeping_threads, &thread->node);

	entry = thread_monitor_entry_get(k_current_get());
	thread_monitor_entry_update(entry);

	irq_unlock(key);
}

int32_t bsd_os_timedwait(uint32_t context, int32_t *timeout)
{
	struct sleeping_thread thread;
	s64_t start, remaining;

	start = k_uptime_get();

	if (*timeout == 0) {
		k_yield();
		return NRF_ETIMEDOUT;
	}

	if (*timeout < 0) {
		*timeout = K_FOREVER;
	}

	sleeping_thread_init(&thread);

	if (!sleeping_thread_add(&thread)) {
		return 0;
	}

	(void)k_sem_take(&thread.sem, *timeout);

	sleeping_thread_remove(&thread);

	if (*timeout == K_FOREVER) {
		return 0;
	}

	/* Calculate how much time is left until timeout. */
	remaining = *timeout - (k_uptime_get() - start);
	*timeout = remaining > 0 ? remaining : 0;

	if (*timeout == 0) {
		return NRF_ETIMEDOUT;
	}

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
	case NRF_ENOEXEC:
		errno = ENOEXEC;
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
	case NRF_EDOM:
		errno = EDOM;
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
	case NRF_ENETRESET:
		errno = ENETRESET;
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
	case NRF_EALREADY:
		errno = EALREADY;
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
	case NRF_EMSGSIZE:
		errno = EMSGSIZE;
		break;
	default:
		/* Catch untranslated errnos.
		 * Log the untranslated errno and return a magic value
		 * to make sure this situation is clearly distinguishable.
		 */
		__ASSERT(false, "Untranslated errno %d set by bsdlib!", err_code);
		LOG_ERR("Untranslated errno %d set by bsdlib!", err_code);
		errno = 0xBAADBAAD;
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
	atomic_inc(&rpc_event_cnt);

	bsd_os_application_irq_handler();

	struct sleeping_thread *thread;

	/* Wake up all sleeping threads. */
	SYS_SLIST_FOR_EACH_CONTAINER(&sleeping_threads, thread, node) {
		k_sem_give(&thread->sem);
	}

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
		.pseltxd = DT_NORDIC_NRF_UARTE_UART_1_TX_PIN,
		.pselrxd = DT_NORDIC_NRF_UARTE_UART_1_RX_PIN,
		.pselcts = NRF_UARTE_PSEL_DISCONNECTED,
		.pselrts = NRF_UARTE_PSEL_DISCONNECTED,

		.hal_cfg.hwfc = NRF_UARTE_HWFC_DISABLED,
		.hal_cfg.parity = NRF_UARTE_PARITY_EXCLUDED,
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
	sys_slist_init(&sleeping_threads);
	atomic_clear(&rpc_event_cnt);

	read_task_create();

	/* Configure and enable modem tracing over UART. */
	trace_uart_init();
	trace_task_create();
}

int32_t bsd_os_trace_put(const uint8_t * const data, uint32_t len)
{
#ifdef CONFIG_BSD_LIBRARY_TRACE_ENABLED
	/* Max DMA transfers are 255 bytes.
	 * Split RAM buffer into smaller chunks
	 * to be transferred using DMA.
	 */
	u32_t remaining_bytes = len;

	while (remaining_bytes) {
		u8_t transfer_len = MIN(remaining_bytes, UINT8_MAX);
		u32_t idx = len - remaining_bytes;

		nrfx_uarte_tx(&uarte_inst, &data[idx], transfer_len);
		remaining_bytes -= transfer_len;
	}
#endif

	return 0;
}
