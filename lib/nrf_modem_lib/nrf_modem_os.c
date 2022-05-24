/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2021 Intellinium <giuliano.franchetto@intellinium.com>
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <nrf_modem.h>
#include <nrf_modem_os.h>
#include <nrf_modem_platform.h>
#include <nrf.h>
#include <nrfx_ipc.h>
#include <nrf_errno.h>
#include <errno.h>
#include <pm_config.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_NRF_MODEM_LIB_TRACE_ENABLED
#include <modem/nrf_modem_lib_trace.h>
#endif

#define UNUSED_FLAGS 0

/* Handle communication with application and modem traces from IRQ contexts
 * with the lowest available priority.
 */
#define APPLICATION_IRQ EGU1_IRQn
#define APPLICATION_IRQ_PRIORITY IRQ_PRIO_LOWEST
#define TRACE_IRQ EGU2_IRQn
#define TRACE_IRQ_PRIORITY IRQ_PRIO_LOWEST

#define THREAD_MONITOR_ENTRIES 10

LOG_MODULE_REGISTER(nrf_modem, CONFIG_NRF_MODEM_LIB_LOG_LEVEL);

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

/* Dedicated heap for modem traces  */
static K_HEAP_DEFINE(trace_heap, CONFIG_NRF_MODEM_LIB_TRACE_HEAP_SIZE);

/* Store information about failed allocations */
static struct mem_diagnostic_info shmem_diag;
static struct mem_diagnostic_info heap_diag;
static struct mem_diagnostic_info trace_heap_diag;

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

	if (!nrf_modem_is_initialized()) {
		return -NRF_ESHUTDOWN;
	}

	start = k_uptime_get();

	if (*timeout == 0) {
		k_yield();
		return -NRF_EAGAIN;
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

	if (!nrf_modem_is_initialized()) {
		return -NRF_ESHUTDOWN;
	}

	if (*timeout == SYS_FOREVER_MS) {
		return 0;
	}

	/* Calculate how much time is left until timeout. */
	remaining = *timeout - k_uptime_delta(&start);
	*timeout = remaining > 0 ? remaining : 0;

	if (*timeout == 0) {
		return -NRF_EAGAIN;
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
	__ASSERT(err_code > 0, "Tried to set negative error code, %d", err_code);
	errno = err_code;
}

bool nrf_modem_os_is_in_isr(void)
{
	return k_is_in_isr();
}

static struct k_sem nrf_modem_os_sems[NRF_MODEM_OS_NUM_SEM_REQUIRED];

int nrf_modem_os_sem_init(void **sem,
	unsigned int initial_count, unsigned int limit)
{
	static uint8_t used;

	if (PART_OF_ARRAY(nrf_modem_os_sems, (struct k_sem *)*sem)) {
		goto recycle;
	}

	__ASSERT(used < NRF_MODEM_OS_NUM_SEM_REQUIRED,
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
	if (err) {
		return -NRF_EAGAIN;
	}

	return 0;
}

unsigned int nrf_modem_os_sem_count_get(void *sem)
{
	return k_sem_count_get(sem);
}

void nrf_modem_os_application_irq_set(void)
{
	NVIC_SetPendingIRQ(APPLICATION_IRQ);
}

void nrf_modem_os_application_irq_clear(void)
{
	NVIC_ClearPendingIRQ(APPLICATION_IRQ);
}

void nrf_modem_os_trace_irq_set(void)
{
	NVIC_SetPendingIRQ(TRACE_IRQ);
}

void nrf_modem_os_trace_irq_clear(void)
{
	NVIC_ClearPendingIRQ(TRACE_IRQ);
}

void nrf_modem_os_trace_irq_disable(void)
{
	irq_disable(TRACE_IRQ);
}

void nrf_modem_os_trace_irq_enable(void)
{
	irq_enable(TRACE_IRQ);
}

void nrf_modem_os_event_notify(void)
{
	atomic_inc(&rpc_event_cnt);

	struct sleeping_thread *thread;

	/* Wake up all sleeping threads. */
	SYS_SLIST_FOR_EACH_CONTAINER(&sleeping_threads, thread, node) {
		k_sem_give(&thread->sem);
	}
}

ISR_DIRECT_DECLARE(rpc_proxy_irq_handler)
{
	nrf_modem_application_irq_handler();

	nrf_modem_os_event_notify();

	ISR_DIRECT_PM(); /* PM done after servicing interrupt for best latency
			  */
	return 1; /* We should check if scheduling decision should be made */
}

#ifdef CONFIG_NRF_MODEM_LIB_TRACE_ENABLED
ISR_DIRECT_DECLARE(trace_proxy_irq_handler)
{
	/* Process modem traces. */
	nrf_modem_trace_irq_handler();
	ISR_DIRECT_PM(); /* PM done after servicing interrupt for best latency
			  */
	return 1; /* We should check if scheduling decision should be made */
}

void trace_irq_init(void)
{
	IRQ_DIRECT_CONNECT(TRACE_IRQ, TRACE_IRQ_PRIORITY,
			   trace_proxy_irq_handler, UNUSED_FLAGS);
	irq_enable(TRACE_IRQ);
}
#endif

void read_task_create(void)
{
	IRQ_DIRECT_CONNECT(APPLICATION_IRQ, APPLICATION_IRQ_PRIORITY,
			   rpc_proxy_irq_handler, UNUSED_FLAGS);
	irq_enable(APPLICATION_IRQ);
}

void *nrf_modem_os_alloc(size_t bytes)
{
	void *addr = k_heap_alloc(&library_heap, bytes, K_NO_WAIT);

	if (IS_ENABLED(CONFIG_NRF_MODEM_LIB_DEBUG_ALLOC)) {
		if (addr) {
			LOG_DBG("alloc(%d) -> %p", bytes, addr);
		} else {
			LOG_WRN("alloc(%d) -> NULL", bytes);
			heap_diag.failed_allocs++;
		}
	}

	return addr;
}

void nrf_modem_os_free(void *mem)
{
	k_heap_free(&library_heap, mem);

	if (IS_ENABLED(CONFIG_NRF_MODEM_LIB_DEBUG_ALLOC)) {
		LOG_DBG("free(%p)", mem);
	}
}

void *nrf_modem_os_trace_alloc(size_t bytes)
{
	void *addr = k_heap_alloc(&trace_heap, bytes, K_NO_WAIT);

	if (IS_ENABLED(CONFIG_NRF_MODEM_LIB_DEBUG_ALLOC)) {
		if (addr) {
			LOG_DBG("trace_alloc(%d) -> %p", bytes, addr);
		} else {
			LOG_WRN("trace_alloc(%d) -> NULL", bytes);
			trace_heap_diag.failed_allocs++;
		}
	}

	return addr;
}

void nrf_modem_os_trace_free(void *mem)
{
	k_heap_free(&trace_heap, mem);

	if (IS_ENABLED(CONFIG_NRF_MODEM_LIB_DEBUG_ALLOC)) {
		LOG_DBG("trace_free(%p)", mem);
	}
}

void *nrf_modem_os_shm_tx_alloc(size_t bytes)
{
	void *addr = k_heap_alloc(&shmem_heap, bytes, K_NO_WAIT);

	if (IS_ENABLED(CONFIG_NRF_MODEM_LIB_DEBUG_SHM_TX_ALLOC)) {
		if (addr) {
			LOG_DBG("shm_tx_alloc(%d) -> %p", bytes, addr);
		} else {
			LOG_WRN("shm_tx_alloc(%d) -> NULL", bytes);
			shmem_diag.failed_allocs++;
		}
	}
	return addr;
}

void nrf_modem_os_shm_tx_free(void *mem)
{
	k_heap_free(&shmem_heap, mem);

	if (IS_ENABLED(CONFIG_NRF_MODEM_LIB_DEBUG_SHM_TX_ALLOC)) {
		LOG_DBG("shm_tx_free(%p)", mem);
	}
}

void nrf_modem_lib_heap_diagnose(void)
{
	printk("\nnrf_modem heap dump:\n");
	sys_heap_print_info(&library_heap.heap, false);
	printk("Failed allocations: %u\n", heap_diag.failed_allocs);
}

void nrf_modem_lib_trace_heap_diagnose(void)
{
	printk("\nnrf_modem trace heap dump:\n");
	sys_heap_print_info(&trace_heap.heap, false);
	printk("Failed allocations: %u\n", trace_heap_diag.failed_allocs);
}

void nrf_modem_lib_shm_tx_diagnose(void)
{
	printk("\nnrf_modem tx dump:\n");
	sys_heap_print_info(&shmem_heap.heap, false);
	printk("Failed allocations: %u\n", shmem_diag.failed_allocs);
}

#if defined(CONFIG_NRF_MODEM_LIB_SHM_TX_DUMP_PERIODIC) || \
	defined(CONFIG_NRF_MODEM_LIB_HEAP_DUMP_PERIODIC) || \
	defined(CONFIG_NRF_MODEM_LIB_TRACE_HEAP_DUMP_PERIODIC)

enum heap_type {
	SHMEM, LIBRARY, TRACE
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
#ifdef CONFIG_NRF_MODEM_LIB_TRACE_HEAP_DUMP_PERIODIC
static struct task trace_heap_task = { .type = TRACE };
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
	case TRACE:
#ifdef CONFIG_NRF_MODEM_LIB_TRACE_HEAP_DUMP_PERIODIC
		nrf_modem_lib_trace_heap_diagnose();
		k_work_reschedule(&trace_heap_task.work,
			K_MSEC(CONFIG_NRF_MODEM_LIB_TRACE_HEAP_DUMP_PERIOD_MS));
#endif
		break;
	}
}
#endif

#if defined(CONFIG_LOG)
static uint8_t log_level_translate(uint8_t level)
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
#endif

void nrf_modem_os_log(int level, const char *fmt, ...)
{
#if defined(CONFIG_LOG)
	level = log_level_translate(level);
	if (level > CONFIG_NRF_MODEM_LIB_LOG_LEVEL) {
		return;
	}

	va_list ap;
	va_start(ap, fmt);

	if (IS_ENABLED(CONFIG_LOG_MODE_MINIMAL)) {
		/* Fallback to minimal implementation. */
		printk("%c: ", z_log_minimal_level_to_char(level));
		z_log_minimal_vprintk(fmt, ap);
		printk("\n");
	} else {
		z_log_msg2_runtime_vcreate(
			CONFIG_LOG_DOMAIN_ID, __log_current_dynamic_data,
			level, NULL, 0, 0, fmt, ap);
	}

	va_end(ap);
#endif /* CONFIG_LOG */
}

void nrf_modem_os_logdump(int level, const char *str, const void *data, size_t len)
{
#if defined(CONFIG_LOG)
	level = log_level_translate(level);

	if (IS_ENABLED(CONFIG_LOG_MODE_MINIMAL)) {
		/* Fallback to minimal implementation. */
		printk("%c: %s\n", z_log_minimal_level_to_char(level), str);
		z_log_minimal_hexdump_print(level, data, len);
	} else {
		struct log_msg_ids src_level = {
			.level = level,
			.domain_id = CONFIG_LOG_DOMAIN_ID,
			.source_id = LOG_CURRENT_MODULE_ID()
		};
		log_hexdump(str, data, len, src_level);
	}
#endif /* CONFIG_LOG */
}

/* On application initialization */
static int on_init(const struct device *dev)
{
	(void) dev;

	/* The list of sleeping threads should only be initialized once at the application
	 * initialization. This is because we want to keep the list intact regardless of modem
	 * reinitialization to wake sleeping threads on modem initialization.
	 */
	sys_slist_init(&sleeping_threads);
	atomic_clear(&rpc_event_cnt);

	return 0;
}

/* On modem initialization.
 * This function is called by nrf_modem_init()
 */
void nrf_modem_os_init(void)
{
	read_task_create();

#ifdef CONFIG_NRF_MODEM_LIB_TRACE_ENABLED
	int err = nrf_modem_lib_trace_init(&trace_heap);

	if (!err) {
		trace_irq_init();
	} else {
		LOG_ERR("nrf_modem_lib_trace_init failed with error %d.", err);
	}

#endif

	memset(&heap_diag, 0x00, sizeof(heap_diag));
	memset(&shmem_diag, 0x00, sizeof(shmem_diag));

	/* Initialize TX heap */
	k_heap_init(&shmem_heap,
		    (void *)PM_NRF_MODEM_LIB_TX_ADDRESS,
		    CONFIG_NRF_MODEM_LIB_SHMEM_TX_SIZE);

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

#ifdef CONFIG_NRF_MODEM_LIB_TRACE_HEAP_DUMP_PERIODIC
	k_work_init_delayable(&trace_heap_task.work, diag_task);
	k_work_reschedule(&trace_heap_task.work,
		K_MSEC(CONFIG_NRF_MODEM_LIB_TRACE_HEAP_DUMP_PERIOD_MS));
#endif
}

void nrf_modem_os_shutdown(void)
{
	struct sleeping_thread *thread;

	/* Wake up all sleeping threads. */
	SYS_SLIST_FOR_EACH_CONTAINER(&sleeping_threads, thread, node) {
		k_sem_give(&thread->sem);
	}

#if CONFIG_NRF_MODEM_LIB_TRACE_ENABLED
	nrf_modem_lib_trace_deinit();
#endif
}

int32_t nrf_modem_os_trace_put(const uint8_t *const data, uint32_t len)
{
#ifdef CONFIG_NRF_MODEM_LIB_TRACE_ENABLED
	int err;

	err = nrf_modem_lib_trace_process(data, len);
	if (err) {
		LOG_ERR("nrf_modem_lib_trace_process failed, err %d", err);
	}

#ifndef CONFIG_NRF_MODEM_LIB_TRACE_THREAD_PROCESSING
	err = nrf_modem_trace_processed_callback(data, len);
	if (err) {
		LOG_ERR("nrf_modem_trace_processed_callback failed, err %d", err);
	}
#endif /* CONFIG_NRF_MODEM_LIB_TRACE_THREAD_PROCESSING */

#endif /* CONFIG_NRF_MODEM_LIB_TRACE_ENABLED */
	return 0;
}

SYS_INIT(on_init, POST_KERNEL, 0);
