/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2021 Intellinium <giuliano.franchetto@intellinium.com>
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <init.h>
#include <zephyr.h>
#include <nrf_modem.h>
#include <nrf_modem_os.h>
#include <nrf_modem_platform.h>
#include <nrf.h>
#include <nrfx_ipc.h>
#include <nrf_errno.h>
#include <errno.h>
#include <pm_config.h>
#include <logging/log.h>

#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_UART
#include <nrfx_uarte.h>
#endif

#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_RTT
#include <SEGGER_RTT.h>
#endif

#define UNUSED_FLAGS 0

/* Handle modem traces from IRQ context with lower priority. */
#define TRACE_IRQ EGU2_IRQn
#define TRACE_IRQ_PRIORITY 6

#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_UART
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

void nrf_modem_os_busywait(int32_t usec)
{
	k_busy_wait(usec);
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

/* Set OS errno from modem library.
 *
 * Note: The nrf_errnos are aligned with the libc minimal errnos used in Zephyr. Hence,
 *       we set the errno directly to the error code. A translation table is required if
 *       the os errnos does not align with the nrf_errnos. See @ref nrf_errno.h for a list
 *       of all errnos required. When adding a translation table, the errno_sanity.c file
 *       could be removed.
 */
void nrf_modem_os_errno_set(int err_code)
{
	errno = err_code;
}

bool nrf_modem_os_is_in_isr(void)
{
	return k_is_in_isr();
}

#define NRF_MODEM_OS_SEM_MAX 3
static struct k_sem nrf_modem_os_sems[NRF_MODEM_OS_SEM_MAX];

int nrf_modem_os_sem_init(void **sem,
	unsigned int initial_count, unsigned int limit)
{
	static uint8_t used;

	if (PART_OF_ARRAY(nrf_modem_os_sems, (struct k_sem *)*sem)) {
		goto recycle;
	}

	__ASSERT(used < NRF_MODEM_OS_SEM_MAX,
		 "Not enough semaphores in glue layer");

	*sem = &nrf_modem_os_sems[used++];

recycle:
	return k_sem_init((struct k_sem *)*sem, initial_count, limit);
}

void nrf_modem_os_sem_give(void *sem)
{
	__ASSERT(PART_OF_ARRAY(nrf_modem_os_sems, (struct k_sem *)sem),
		 "Uninitialised semaphore");

	k_sem_give((struct k_sem *)sem);
}

int nrf_modem_os_sem_take(void *sem, int timeout)
{
	int err;

	__ASSERT(PART_OF_ARRAY(nrf_modem_os_sems, (struct k_sem *)sem),
		 "Uninitialised semaphore");

	err = k_sem_take((struct k_sem *)sem, timeout == -1 ? K_FOREVER : K_MSEC(timeout));
	if (err == -EAGAIN) {
		return NRF_ETIMEDOUT;
	}

	return 0;
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

	nrf_modem_application_irq_handler();

	struct sleeping_thread *thread;

	/* Wake up all sleeping threads. */
	SYS_SLIST_FOR_EACH_CONTAINER(&sleeping_threads, thread, node) {
		k_sem_give(&thread->sem);
	}

	ISR_DIRECT_PM(); /* PM done after servicing interrupt for best latency
			  */
	return 1; /* We should check if scheduling decision should be made */
}

#ifdef CONFIG_NRF_MODEM_LIB_TRACE_ENABLED
ISR_DIRECT_DECLARE(trace_proxy_irq_handler)
{
	/*
	 * Process traces.
	 * The function has to be called even if UART traces are disabled.
	 */
	nrf_modem_trace_irq_handler();
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
#endif

void read_task_create(void)
{
	IRQ_DIRECT_CONNECT(NRF_MODEM_APPLICATION_IRQ,
			   NRF_MODEM_APPLICATION_IRQ_PRIORITY,
			   rpc_proxy_irq_handler, UNUSED_FLAGS);
	irq_enable(NRF_MODEM_APPLICATION_IRQ);
}

#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_UART
void trace_uart_init(void)
{
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
}
#endif

#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_RTT
#define RTT_BUF_SZ		(CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_RTT_BUF_SIZE)
static int trace_rtt_channel;
static char rtt_buffer[RTT_BUF_SZ];

static void trace_rtt_init(void)
{
	trace_rtt_channel = SEGGER_RTT_AllocUpBuffer("modem_trace", rtt_buffer,
		sizeof(rtt_buffer), SEGGER_RTT_MODE_NO_BLOCK_SKIP);

	if (trace_rtt_channel < 0) {
		LOG_ERR("Could not allocated RTT channel for modem trace (%d)",
			trace_rtt_channel);
	} else {
		LOG_INF("RTT channel for modem trace is now %d",
			trace_rtt_channel);
	}
}
#endif

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
	struct k_work_delayable work;
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
		k_work_reschedule(&shmem_task.work,
			K_MSEC(CONFIG_NRF_MODEM_LIB_SHMEM_TX_DUMP_PERIOD_MS));
#endif
		break;
	case LIBRARY:
#ifdef CONFIG_NRF_MODEM_LIB_HEAP_DUMP_PERIODIC
		nrf_modem_lib_heap_diagnose();
		k_work_reschedule(&heap_task.work,
			K_MSEC(CONFIG_NRF_MODEM_LIB_HEAP_DUMP_PERIOD_MS));
#endif
		break;
	}
}
#endif

static uint8_t log_level_lu(uint8_t level)
{
	switch (level) {
	case NRF_MODEM_LOG_LEVEL_NONE:
		return LOG_LEVEL_NONE;
	case NRF_MODEM_LOG_LEVEL_ERR:
		return LOG_LEVEL_ERR;
	case NRF_MODEM_LOG_LEVEL_WRN:
		return LOG_LEVEL_WRN;
	case NRF_MODEM_LOG_LEVEL_INF:
		return LOG_LEVEL_INF;
	case NRF_MODEM_LOG_LEVEL_DBG:
		return LOG_LEVEL_DBG;
	default:
		return LOG_LEVEL_NONE;
	}
}

const char *nrf_modem_os_log_strdup(const char *str)
{
	if (IS_ENABLED(CONFIG_LOG)) {
		return log_strdup(str);
	}

	return str;
}

void nrf_modem_os_log(int level, const char *fmt, ...)
{
	if (IS_ENABLED(CONFIG_LOG)) {
		uint16_t lev = log_level_lu(level);

		va_list ap;

		va_start(ap, fmt);

		if (IS_ENABLED(CONFIG_LOG_MODE_MINIMAL)) {
			/* Fallback to minimal implementation. */
			/* Based on Zephyr's. */
			printk("%c: ", z_log_minimal_level_to_char(lev));
			vprintk(fmt, ap);
			printk("\n");
		} else {
			struct log_msg_ids src_level = {
				.level = lev,
				.domain_id = CONFIG_LOG_DOMAIN_ID,
				.source_id = LOG_CURRENT_MODULE_ID()
			};

			log_generic(src_level, fmt, ap, LOG_STRDUP_SKIP);
		}
		va_end(ap);
	}
}

void nrf_modem_os_logdump(int level, const char *str, const void *data, size_t len)
{
	if (IS_ENABLED(CONFIG_LOG)) {
		uint16_t lev = log_level_lu(level);

		if (IS_ENABLED(CONFIG_LOG_MODE_MINIMAL)) {
			/* Fallback to minimal implementation. */
			/* Based on Zephyr's. */
			printk("%c: %s\n",
			       z_log_minimal_level_to_char(lev),
			       str);
			z_log_minimal_hexdump_print(lev, data, len);
		} else {
			struct log_msg_ids src_level = {
				.level = lev,
				.domain_id = CONFIG_LOG_DOMAIN_ID,
				.source_id = LOG_CURRENT_MODULE_ID()
			};
			log_hexdump(str, data, len, src_level);
		}
	}
}

/* This function is called by nrf_modem_init() */
void nrf_modem_os_init(void)
{
	sys_slist_init(&sleeping_threads);
	atomic_clear(&rpc_event_cnt);

	read_task_create();

	/* Configure and enable modem tracing over UART and RTT. */
#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_UART
	trace_uart_init();
#endif

#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_RTT
	trace_rtt_init();
#endif

#ifdef CONFIG_NRF_MODEM_LIB_TRACE_ENABLED
	trace_task_create();
#endif

	memset(&heap_diag, 0x00, sizeof(heap_diag));
	memset(&shmem_diag, 0x00, sizeof(shmem_diag));

	/* Initialize TX heap */
	k_heap_init(&shmem_heap,
		    (void *)PM_NRF_MODEM_LIB_TX_ADDRESS,
		    CONFIG_NRF_MODEM_LIB_SHMEM_TX_SIZE);

#if defined(CONFIG_NRF_MODEM_LIB_SHM_TX_DUMP_PERIODIC) || \
	defined(CONFIG_NRF_MODEM_LIB_HEAP_DUMP_PERIODIC)
	k_work_queue_start(&modem_diag_worqk, work_q_stack_area,
			   K_THREAD_STACK_SIZEOF(work_q_stack_area),
			   K_LOWEST_APPLICATION_THREAD_PRIO, NULL);
#endif

#ifdef CONFIG_NRF_MODEM_LIB_SHM_TX_DUMP_PERIODIC
	k_work_init_delayable(&shmem_task.work, diag_task);
	k_work_reschedule(&shmem_task.work,
		K_MSEC(CONFIG_NRF_MODEM_LIB_SHMEM_TX_DUMP_PERIOD_MS));
#endif

#ifdef CONFIG_NRF_MODEM_LIB_HEAP_DUMP_PERIODIC
	k_work_init_delayable(&heap_task.work, diag_task);
	k_work_reschedule(&heap_task.work,
		K_MSEC(CONFIG_NRF_MODEM_LIB_HEAP_DUMP_PERIOD_MS));
#endif
}

int32_t nrf_modem_os_trace_put(const uint8_t * const data, uint32_t len)
{
#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_UART
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

#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_RTT
	/* First, let's check if the buffer has been correctly
	 * allocated for the modem trace
	 */
	if (trace_rtt_channel < 0) {
		return 0;
	}

	uint32_t remaining_bytes = len;

	while (remaining_bytes) {
		uint8_t transfer_len = MIN(remaining_bytes, RTT_BUF_SZ);
		uint32_t idx = len - remaining_bytes;

		SEGGER_RTT_WriteSkipNoLock(trace_rtt_channel, &data[idx],
			transfer_len);
		remaining_bytes -= transfer_len;
	}
#endif
	return 0;
}
