/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <init.h>
#include <device.h>
#include <zephyr.h>
#include <zephyr/types.h>

#include <bsd.h>
#include <bsd_platform.h>

#ifdef CONFIG_LTE_LINK_CONTROL
#include <modem/lte_lc.h>
#endif

#ifndef CONFIG_TRUSTED_EXECUTION_NONSECURE
#error  bsdlib must be run as non-secure firmware.\
	Are you building for the correct board ?
#endif

struct shutdown_thread {
	sys_snode_t node;
	struct k_sem sem;
};

static sys_slist_t shutdown_threads;
static bool first_time_init;
static struct k_mutex slist_mutex;

extern void ipc_proxy_irq_handler(void);

static int init_ret;

static int _bsdlib_init(const struct device *unused)
{
	if (!first_time_init) {
		sys_slist_init(&shutdown_threads);
		k_mutex_init(&slist_mutex);
		first_time_init = true;
	}

	/* Setup the network IRQ used by the BSD library.
	 * Note: No call to irq_enable() here, that is done through bsd_init().
	 */
	IRQ_DIRECT_CONNECT(BSD_NETWORK_IRQ, BSD_NETWORK_IRQ_PRIORITY,
			   ipc_proxy_irq_handler, 0);

	const bsd_init_params_t init_params = {
		.trace_on = true,
		.bsd_memory_address = BSD_RESERVED_MEMORY_ADDRESS,
		.bsd_memory_size = BSD_RESERVED_MEMORY_SIZE
	};

	init_ret = bsd_init(&init_params);

	k_mutex_lock(&slist_mutex, K_FOREVER);
	if (sys_slist_peek_head(&shutdown_threads) != NULL) {
		struct shutdown_thread *thread, *next_thread;

		/* Wake up all sleeping threads. */
		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&shutdown_threads, thread,
					     next_thread, node) {
			k_sem_give(&thread->sem);
		}
	}
	k_mutex_unlock(&slist_mutex);

	if (IS_ENABLED(CONFIG_BSD_LIBRARY_SYS_INIT)) {
		/* bsd_init() returns values from a different namespace
		 * than Zephyr's. Make sure to return something in Zephyr's
		 * namespace, in this case 0, when called during SYS_INIT.
		 * Non-zero values in SYS_INIT are currently ignored.
		 */
		return 0;
	}

	return init_ret;
}

void bsdlib_shutdown_wait(void)
{
	struct shutdown_thread thread;

	k_sem_init(&thread.sem, 0, 1);

	k_mutex_lock(&slist_mutex, K_FOREVER);
	sys_slist_append(&shutdown_threads, &thread.node);
	k_mutex_unlock(&slist_mutex);

	(void)k_sem_take(&thread.sem, K_FOREVER);

	k_mutex_lock(&slist_mutex, K_FOREVER);
	sys_slist_find_and_remove(&shutdown_threads, &thread.node);
	k_mutex_unlock(&slist_mutex);
}

int bsdlib_init(void)
{
	return _bsdlib_init(NULL);
}

int bsdlib_get_init_ret(void)
{
	return init_ret;
}

int bsdlib_shutdown(void)
{
#ifdef CONFIG_LTE_LINK_CONTROL
	lte_lc_deinit();
#endif
	bsd_shutdown();

	return 0;
}

#if defined(CONFIG_BSD_LIBRARY_SYS_INIT)
/* Initialize during SYS_INIT */
SYS_INIT(_bsdlib_init, POST_KERNEL, 0);
#endif
