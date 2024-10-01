/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <nrf_modem.h>
#include <nrf_modem_os.h>
#include <nrf.h>
#include <nrf_errno.h>
#include <errno.h>
#include <pm_config.h>
#include <zephyr/logging/log.h>

#define UNUSED_FLAGS 0
#define THREAD_MONITOR_ENTRIES 10

LOG_MODULE_REGISTER(nrf_modem, CONFIG_NRF_MODEM_LIB_LOG_LEVEL);

struct sleeping_thread {
	sys_snode_t node;
	struct k_sem sem;
	uint32_t context;
};

/* Heaps, extern in diag.c */

/* Shared memory heap */
struct k_heap nrf_modem_lib_shmem_heap;
/* Library heap */
struct k_heap nrf_modem_lib_heap;
static uint8_t library_heap_buf[CONFIG_NRF_MODEM_LIB_HEAP_SIZE];

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
static void sleeping_thread_init(struct sleeping_thread *thread, uint32_t context)
{
	k_sem_init(&thread->sem, 0, 1);
	thread->context = context;
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

	sleeping_thread_init(&thread, context);

/* GCC 13 will emit a warning related to dangling pointer when
 * stacked variable "thread.node" is added to the slist.
 * This can be safely ignored, as "thread" is both added and
 * removed from the slist within the same scope.
 */
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdangling-pointer"
#endif /* __GNUC__ */
	if (!sleeping_thread_add(&thread)) {
		return 0;
	}
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif /* __GNUC__ */

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

int nrf_modem_os_sleep(uint32_t timeout)
{
	if (timeout == NRF_MODEM_OS_NO_WAIT || timeout == NRF_MODEM_OS_FOREVER) {
		return -NRF_EINVAL;
	}

	return k_sleep(K_MSEC(timeout));
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

	if (used >= NRF_MODEM_OS_NUM_SEM_REQUIRED) {
		return -NRF_ENOMEM;
	}

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

static struct k_mutex nrf_modem_os_mutexes[NRF_MODEM_OS_NUM_MUTEX_REQUIRED];

int nrf_modem_os_mutex_init(void **mutex)
{
	static uint8_t used;

	if (PART_OF_ARRAY(nrf_modem_os_mutexes, (struct k_mutex *)*mutex)) {
		goto recycle;
	}

	__ASSERT(used < NRF_MODEM_OS_NUM_MUTEX_REQUIRED,
		 "Not enough mutexes in glue layer");

	if (used >= NRF_MODEM_OS_NUM_MUTEX_REQUIRED) {
		return -NRF_ENOMEM;
	}

	*mutex = &nrf_modem_os_mutexes[used++];

recycle:
	return k_mutex_init((struct k_mutex *)*mutex);
}

int nrf_modem_os_mutex_unlock(void *mutex)
{
	__ASSERT(PART_OF_ARRAY(nrf_modem_os_mutexes, (struct k_mutex *)mutex),
		 "Uninitialised mutex");

	return k_mutex_unlock((struct k_mutex *)mutex);
}

int nrf_modem_os_mutex_lock(void *mutex, int timeout)
{
	int err;

	__ASSERT(PART_OF_ARRAY(nrf_modem_os_mutexes, (struct k_mutex *)mutex),
		 "Uninitialised mutex");

	err = k_mutex_lock((struct k_mutex *)mutex, timeout == -1 ? K_FOREVER : K_MSEC(timeout));
	if (err) {
		return -NRF_EAGAIN;
	}

	return 0;
}


void nrf_modem_os_event_notify(uint32_t context)
{
	atomic_inc(&rpc_event_cnt);

	struct sleeping_thread *thread;

	SYS_SLIST_FOR_EACH_CONTAINER(&sleeping_threads, thread, node) {
		/* Wake sleeping thread if context of the thread matches, is 0 or the notify
		 * context is 0.
		 */
		if ((thread->context == context) || (context == 0) || (thread->context == 0)) {
			k_sem_give(&thread->sem);
		}
	}
}

void *nrf_modem_os_alloc(size_t bytes)
{
	extern uint32_t nrf_modem_lib_failed_allocs;
	void * const addr = k_heap_alloc(&nrf_modem_lib_heap, bytes, K_NO_WAIT);

	if (IS_ENABLED(CONFIG_NRF_MODEM_LIB_MEM_DIAG_ALLOC) && !addr) {
		nrf_modem_lib_failed_allocs++;
	}

	return addr;
}

void nrf_modem_os_free(void *mem)
{
	k_heap_free(&nrf_modem_lib_heap, mem);
}

void *nrf_modem_os_shm_tx_alloc(size_t bytes)
{
	extern uint32_t nrf_modem_lib_shmem_failed_allocs;
	void * const addr = k_heap_alloc(&nrf_modem_lib_shmem_heap, bytes, K_NO_WAIT);

	if (IS_ENABLED(CONFIG_NRF_MODEM_LIB_MEM_DIAG_ALLOC) && !addr) {
		nrf_modem_lib_shmem_failed_allocs++;
	}

	return addr;
}

void nrf_modem_os_shm_tx_free(void *mem)
{
	k_heap_free(&nrf_modem_lib_shmem_heap, mem);
}

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

#if CONFIG_LOG_MODE_MINIMAL
		/* Fallback to minimal implementation. */
		printk("%c: ", z_log_minimal_level_to_char(level));
		z_log_minimal_vprintk(fmt, ap);
		printk("\n");
#else
		void *source;

		if (IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING)) {
			source = (void *)__log_current_dynamic_data;
		} else {
			source = (void *)__log_current_const_data;
		}

		z_log_msg_runtime_vcreate(Z_LOG_LOCAL_DOMAIN_ID, source, level,
					  NULL, 0, 0, fmt, ap);
#endif /* CONFIG_LOG_MODE_MINIMAL */

	va_end(ap);
#endif /* CONFIG_LOG */
}

void nrf_modem_os_logdump(int level, const char *str, const void *data, size_t len)
{
#if defined(CONFIG_LOG)
	level = log_level_translate(level);

#if CONFIG_LOG_MODE_MINIMAL
		/* Fallback to minimal implementation. */
		printk("%c: %s\n", z_log_minimal_level_to_char(level), str);
		z_log_minimal_hexdump_print(level, data, len);
#else
		void *source;

		if (IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING)) {
			source = (void *)__log_current_dynamic_data;
		} else {
			source = (void *)__log_current_const_data;
		}

		z_log_msg_runtime_vcreate(Z_LOG_LOCAL_DOMAIN_ID, source, level,
					  data, len, 0, str, (va_list) { 0 });
#endif /* CONFIG_LOG_MODE_MINIMAL */

#endif /* CONFIG_LOG */
}

/* On application initialization */
static int on_init(void)
{
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
	/* Initialize heaps */
	k_heap_init(&nrf_modem_lib_heap, library_heap_buf, sizeof(library_heap_buf));
	k_heap_init(&nrf_modem_lib_shmem_heap, (void *)PM_NRF_MODEM_LIB_TX_ADDRESS,
		    CONFIG_NRF_MODEM_LIB_SHMEM_TX_SIZE);
}

void nrf_modem_os_shutdown(void)
{
	struct sleeping_thread *thread;

	/* Wake up all sleeping threads. */
	SYS_SLIST_FOR_EACH_CONTAINER(&sleeping_threads, thread, node) {
		k_sem_give(&thread->sem);
	}
}

SYS_INIT(on_init, POST_KERNEL, 0);
