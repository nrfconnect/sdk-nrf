/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/sys_heap.h>
#include <zephyr/sys/heap_listener.h>
#include <zephyr/logging/log.h>
#include <modem/nrf_modem_lib.h>

LOG_MODULE_DECLARE(nrf_modem, CONFIG_NRF_MODEM_LIB_LOG_LEVEL);

/* extern in nrf_modem_os.c */
uint32_t nrf_modem_lib_shmem_failed_allocs;	/* failed allocations on shared memory heap */
uint32_t nrf_modem_lib_failed_allocs;		/* failed allocations on library heap */

/* from nrf_modem_os.c */
extern struct k_heap nrf_modem_lib_shmem_heap;
extern struct k_heap nrf_modem_lib_heap;

#if CONFIG_NRF_MODEM_LIB_MEM_DIAG_DUMP
static struct k_work_delayable diag_work;
#endif

int nrf_modem_lib_diag_stats_get(struct nrf_modem_lib_diag_stats *stats)
{
	/* Prevent runtime stats get of uninitialized heap which causes unresponsiveness. */
	if (!nrf_modem_is_initialized()) {
		return -EPERM;
	}

	if (!stats) {
		return -EFAULT;
	}

	sys_heap_runtime_stats_get(&nrf_modem_lib_shmem_heap.heap, &stats->shmem.heap);
	sys_heap_runtime_stats_get(&nrf_modem_lib_heap.heap, &stats->library.heap);

	stats->shmem.failed_allocs = nrf_modem_lib_shmem_failed_allocs;
	stats->library.failed_allocs = nrf_modem_lib_failed_allocs;

	return 0;
}

#if CONFIG_NRF_MODEM_LIB_MEM_DIAG_ALLOC
static void on_heap_alloc(uintptr_t heap_id, void *mem, size_t bytes)
{
	LOG_INF("lib alloc %p, size %u", mem, bytes);
}

static void on_heap_free(uintptr_t heap_id, void *mem, size_t bytes)
{
	LOG_INF("lib free  %p, size %u", mem, bytes);
}

static void on_shmem_alloc(uintptr_t heap_id, void *mem, size_t bytes)
{
	LOG_INF("shm alloc %p, size %u", mem, bytes);
}

static void on_shmem_free(uintptr_t heap_id, void *mem, size_t bytes)
{
	LOG_INF("shm free  %p, size %u", mem, bytes);
}
#endif

#if CONFIG_NRF_MODEM_LIB_MEM_DIAG_DUMP
static void diag_task(struct k_work *item)
{
	struct nrf_modem_lib_diag_stats stats = { 0 };

	(void)nrf_modem_lib_diag_stats_get(&stats);

	LOG_INF("shm: free %.4u, allocated %.4u, max allocated %.4u, failed %u",
		stats.shmem.heap.free_bytes, stats.shmem.heap.allocated_bytes,
		stats.shmem.heap.max_allocated_bytes, nrf_modem_lib_shmem_failed_allocs);
	LOG_INF("lib: free %.4u, allocated %.4u, max allocated %.4u, failed %u",
		stats.library.heap.free_bytes, stats.library.heap.allocated_bytes,
		stats.library.heap.max_allocated_bytes, nrf_modem_lib_failed_allocs);

	k_work_reschedule(&diag_work, K_MSEC(CONFIG_NRF_MODEM_LIB_MEM_DIAG_DUMP_PERIOD_MS));
}
#endif

static int nrf_modem_lib_diag_sys_init(const struct device *unused)
{
#if CONFIG_NRF_MODEM_LIB_MEM_DIAG_ALLOC
	static HEAP_LISTENER_ALLOC_DEFINE(
		shmem_alloc_listener,
		HEAP_ID_FROM_POINTER(&nrf_modem_lib_shmem_heap.heap),
		on_shmem_alloc);

	static HEAP_LISTENER_FREE_DEFINE(
		shmem_free_listener,
		HEAP_ID_FROM_POINTER(&nrf_modem_lib_shmem_heap.heap),
		on_shmem_free);

	heap_listener_register(&shmem_alloc_listener);
	heap_listener_register(&shmem_free_listener);

	static HEAP_LISTENER_ALLOC_DEFINE(
		heap_alloc_listener,
		HEAP_ID_FROM_POINTER(&nrf_modem_lib_heap.heap),
		on_heap_alloc);

	static HEAP_LISTENER_FREE_DEFINE(
		heap_free_listener,
		HEAP_ID_FROM_POINTER(&nrf_modem_lib_heap.heap),
		on_heap_free);

	heap_listener_register(&heap_alloc_listener);
	heap_listener_register(&heap_free_listener);
#endif

#if CONFIG_NRF_MODEM_LIB_MEM_DIAG_DUMP
	k_work_init_delayable(&diag_work, diag_task);
	k_work_reschedule(&diag_work, K_MSEC(CONFIG_NRF_MODEM_LIB_MEM_DIAG_DUMP_PERIOD_MS));
#endif

	LOG_INF("Diagnostic initialized");

	return 0;
}

SYS_INIT(nrf_modem_lib_diag_sys_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
