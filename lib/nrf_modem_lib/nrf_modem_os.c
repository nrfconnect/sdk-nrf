/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <init.h>
#include <zephyr.h>
#include <nrf_modem_os.h>
#include <nrf_modem_platform.h>
#include <nrf.h>
#include <nrfx_ipc.h>
#include <nrf_errno.h>
#include <errno.h>
#include <pm_config.h>
#include <logging/log.h>

#ifdef CONFIG_NRF_MODEM_LIB_TRACE_ENABLED
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

#ifdef CONFIG_NRF_MODEM_LIB_TRACE_ENABLED
/* Use UARTE1 as a dedicated peripheral to print traces. */
static const nrfx_uarte_t uarte_inst = NRFX_UARTE_INSTANCE(1);
#endif

#define THREAD_MONITOR_ENTRIES 10

LOG_MODULE_REGISTER(nrf_modem_lib, CONFIG_NRF_MODEM_LIB_LOG_LEVEL);

struct mem_diagnostic_info {
	uint32_t failed_allocs;
};

struct sleeping_thread {
	sys_snode_t node;
	struct k_sem sem;
};

/* Shared memory heap
 * This heap is not initialized with the K_HEAP macro because
 * it should be initialized in the shared memory area reserved by
 * the Partition Manager. This happens in `nrf_modem_os_init()`.
 */
static struct k_heap shmem_heap;

/* Library heap, defined here in application RAM */
static K_HEAP_DEFINE(library_heap, CONFIG_NRF_MODEM_LIB_HEAP_SIZE);

/* Store information about failed allocations */
static struct mem_diagnostic_info shmem_diag;
static struct mem_diagnostic_info heap_diag;

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
 * counter value at which nrf_modem_lib last checked the 'readiness' of a thread
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
 * meantime), or whether we should return to nrf_modem_lib to re-verify if a sleep is
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

	uint32_t key = irq_lock();

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

	uint32_t key = irq_lock();

	sys_slist_find_and_remove(&sleeping_threads, &thread->node);

	entry = thread_monitor_entry_get(k_current_get());
	thread_monitor_entry_update(entry);

	irq_unlock(key);
}

int32_t nrf_modem_os_timedwait(uint32_t context, int32_t *timeout)
{
	struct sleeping_thread thread;
	int64_t start, remaining;

	start = k_uptime_get();

	if (*timeout == 0) {
		k_yield();
		return NRF_ETIMEDOUT;
	}

	if (*timeout < 0) {
		*timeout = SYS_FOREVER_MS;
	}

	sleeping_thread_init(&thread);

	if (!sleeping_thread_add(&thread)) {
		return 0;
	}

	(void)k_sem_take(&thread.sem, SYS_TIMEOUT_MS(*timeout));

	sleeping_thread_remove(&thread);

	if (*timeout == SYS_FOREVER_MS) {
		return 0;
	}

	/* Calculate how much time is left until timeout. */
	remaining = *timeout - k_uptime_delta(&start);
	*timeout = remaining > 0 ? remaining : 0;

	if (*timeout == 0) {
		return NRF_ETIMEDOUT;
	}

	return 0;
}

void nrf_modem_os_errno_set(int err_code)
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
	case NRF_ECONNABORTED:
		errno = ECONNABORTED;
		break;
	default:
		/* Catch untranslated errnos.
		 * Log the untranslated errno and return a magic value
		 * to make sure this situation is clearly distinguishable.
		 */
		__ASSERT(false, "Untranslated errno %d set by nrf_modem_lib!", err_code);
		LOG_ERR("Untranslated errno %d set by nrf_modem_lib!", err_code);
		errno = 0xBAADBAAD;
		break;
	}
}

void nrf_modem_os_application_irq_set(void)
{
	NVIC_SetPendingIRQ(NRF_MODEM_APPLICATION_IRQ);
}

void nrf_modem_os_application_irq_clear(void)
{
	NVIC_ClearPendingIRQ(NRF_MODEM_APPLICATION_IRQ);
}

void nrf_modem_os_trace_irq_set(void)
{
	NVIC_SetPendingIRQ(TRACE_IRQ);
}

void nrf_modem_os_trace_irq_clear(void)
{
	NVIC_ClearPendingIRQ(TRACE_IRQ);
}

ISR_DIRECT_DECLARE(rpc_proxy_irq_handler)
{
	atomic_inc(&rpc_event_cnt);

	nrf_modem_os_application_irq_handler();

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
	nrf_modem_os_trace_irq_handler();
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
	IRQ_DIRECT_CONNECT(NRF_MODEM_APPLICATION_IRQ,
			   NRF_MODEM_APPLICATION_IRQ_PRIORITY,
			   rpc_proxy_irq_handler, UNUSED_FLAGS);
	irq_enable(NRF_MODEM_APPLICATION_IRQ);
}

void trace_uart_init(void)
{
#ifdef CONFIG_NRF_MODEM_LIB_TRACE_ENABLED
	/* UART pins are defined in "nrf9160dk_nrf9160.dts". */
	const nrfx_uarte_config_t config = {
		/* Use UARTE1 pins routed on VCOM2. */
		.pseltxd = DT_PROP(DT_NODELABEL(uart1), tx_pin),
		.pselrxd = DT_PROP(DT_NODELABEL(uart1), rx_pin),
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

void *nrf_modem_os_alloc(size_t bytes)
{
	void *addr = k_heap_alloc(&library_heap, bytes, K_NO_WAIT);
#ifdef CONFIG_NRF_MODEM_LIB_DEBUG_ALLOC
	if (addr) {
		LOG_INF("alloc(%d) -> %p", bytes, addr);
	} else {
		heap_diag.failed_allocs++;
		LOG_WRN("alloc(%d) -> %p", bytes, addr);
	}
#endif
	return addr;
}

void nrf_modem_os_free(void *mem)
{
	k_heap_free(&library_heap, mem);
#ifdef CONFIG_NRF_MODEM_LIB_DEBUG_ALLOC
	LOG_INF("free(%p)", mem);
#endif
}

void *nrf_modem_os_shm_tx_alloc(size_t bytes)
{
	void *addr = k_heap_alloc(&shmem_heap, bytes, K_NO_WAIT);
#ifdef CONFIG_NRF_MODEM_LIB_DEBUG_SHM_TX_ALLOC
	if (addr) {
		LOG_INF("shm_tx_alloc(%d) -> %p", bytes, addr);
	} else {
		shmem_diag.failed_allocs++;
		LOG_WRN("shm_tx_alloc(%d) -> %p", bytes, addr);
	}
#endif
	return addr;
}

void nrf_modem_os_shm_tx_free(void *mem)
{
	k_heap_free(&shmem_heap, mem);
#ifdef CONFIG_NRF_MODEM_LIB_DEBUG_SHM_TX_ALLOC
	LOG_INF("shm_tx_free(%p)", mem);
#endif
}

void nrf_modem_lib_heap_diagnose(void)
{
	printk("nrf_modem heap dump:\n");
	sys_heap_print_info(&library_heap.heap, false);
	printk("Failed allocations: %u\n", heap_diag.failed_allocs);
}

void nrf_modem_lib_shm_tx_diagnose(void)
{
	printk("nrf_modem tx dump:\n");
	sys_heap_print_info(&shmem_heap.heap, false);
	printk("Failed allocations: %u\n", shmem_diag.failed_allocs);
}

#if defined(CONFIG_NRF_MODEM_LIB_SHM_TX_DUMP_PERIODIC) || \
	defined(CONFIG_NRF_MODEM_LIB_HEAP_DUMP_PERIODIC)

static K_THREAD_STACK_DEFINE(work_q_stack_area, 512);
static struct k_work_q modem_diag_worqk;

enum heap_type {
	SHMEM, LIBRARY
};

struct task {
	struct k_delayed_work work;
	enum heap_type type;
};

#ifdef CONFIG_NRF_MODEM_LIB_SHM_TX_DUMP_PERIODIC
static struct task shmem_task = { .type = SHMEM };
#endif
#ifdef CONFIG_NRF_MODEM_LIB_HEAP_DUMP_PERIODIC
static struct task heap_task  = { .type = LIBRARY };
#endif

static void diag_task(struct k_work *item)
{
	struct task *t = CONTAINER_OF(item, struct task, work);

	switch (t->type) {
	case SHMEM:
#ifdef CONFIG_NRF_MODEM_LIB_SHM_TX_DUMP_PERIODIC
		nrf_modem_lib_shm_tx_diagnose();
		k_delayed_work_submit(&shmem_task.work,
			K_MSEC(CONFIG_NRF_MODEM_LIB_SHMEM_TX_DUMP_PERIOD_MS));
#endif
		break;
	case LIBRARY:
#ifdef CONFIG_NRF_MODEM_LIB_HEAP_DUMP_PERIODIC
		nrf_modem_lib_heap_diagnose();
		k_delayed_work_submit(&heap_task.work,
			K_MSEC(CONFIG_NRF_MODEM_LIB_HEAP_DUMP_PERIOD_MS));
#endif
		break;
	}
}
#endif

/* This function is called by nrf_modem_init() */
void nrf_modem_os_init(void)
{
	sys_slist_init(&sleeping_threads);
	atomic_clear(&rpc_event_cnt);

	read_task_create();

	/* Configure and enable modem tracing over UART. */
	trace_uart_init();
	trace_task_create();

	memset(&heap_diag, 0x00, sizeof(heap_diag));
	memset(&shmem_diag, 0x00, sizeof(shmem_diag));

	/* Initialize TX heap */
	k_heap_init(&shmem_heap,
		    (void *)PM_NRF_MODEM_LIB_TX_ADDRESS,
		    CONFIG_NRF_MODEM_LIB_SHMEM_TX_SIZE);

#if defined(CONFIG_NRF_MODEM_LIB_SHM_TX_DUMP_PERIODIC) || \
	defined(CONFIG_NRF_MODEM_LIB_HEAP_DUMP_PERIODIC)
	k_work_q_start(&modem_diag_worqk, work_q_stack_area,
		       K_THREAD_STACK_SIZEOF(work_q_stack_area),
		       K_LOWEST_APPLICATION_THREAD_PRIO);
#endif

#ifdef CONFIG_NRF_MODEM_LIB_SHM_TX_DUMP_PERIODIC
	k_delayed_work_init(&shmem_task.work, diag_task);
	k_delayed_work_submit(&shmem_task.work,
		K_MSEC(CONFIG_NRF_MODEM_LIB_SHMEM_TX_DUMP_PERIOD_MS));
#endif

#ifdef CONFIG_NRF_MODEM_LIB_HEAP_DUMP_PERIODIC
	k_delayed_work_init(&heap_task.work, diag_task);
	k_delayed_work_submit(&heap_task.work,
		K_MSEC(CONFIG_NRF_MODEM_LIB_HEAP_DUMP_PERIOD_MS));
#endif
}

int32_t nrf_modem_os_trace_put(const uint8_t * const data, uint32_t len)
{
#ifdef CONFIG_NRF_MODEM_LIB_TRACE_ENABLED
	/* Max DMA transfers are 255 bytes.
	 * Split RAM buffer into smaller chunks
	 * to be transferred using DMA.
	 */
	uint32_t remaining_bytes = len;

	while (remaining_bytes) {
		uint8_t transfer_len = MIN(remaining_bytes, UINT8_MAX);
		uint32_t idx = len - remaining_bytes;

		nrfx_uarte_tx(&uarte_inst, &data[idx], transfer_len);
		remaining_bytes -= transfer_len;
	}
#endif

	return 0;
}
