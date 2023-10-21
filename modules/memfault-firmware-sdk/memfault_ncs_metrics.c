/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>

#include <modem/lte_lc.h>
#include <modem/lte_lc_trace.h>
#include <memfault/metrics/metrics.h>
#include <memfault/core/platform/overrides.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(memfault_ncs_metrics, CONFIG_MEMFAULT_NCS_LOG_LEVEL);

#include "memfault_ncs_metrics.h"
#include "memfault_lte_metrics.h"
#include "memfault_bt_metrics.h"

#define HEX_NAME_LENGTH 11

static sys_slist_t thread_list = SYS_SLIST_STATIC_INIT(&thread_list);

static struct memfault_ncs_metrics_thread *thread_search_by_name(const char *name)
{
	struct memfault_ncs_metrics_thread *thread;

	SYS_SLIST_FOR_EACH_CONTAINER(&thread_list, thread, node) {
		if (strncmp(thread->thread_name, name, CONFIG_THREAD_MAX_NAME_LEN) == 0) {
			return thread;
		}
	}

	return NULL;
}

static void stack_check(const struct k_thread *cthread, void *user_data)
{
	struct k_thread *thread = (struct k_thread *)cthread;
	struct memfault_ncs_metrics_thread *thread_metrics;
	char hexname[HEX_NAME_LENGTH];
	const char *name;
	size_t unused;
	int err;

	ARG_UNUSED(user_data);

	name = k_thread_name_get((k_tid_t)thread);
	if (!name || name[0] == '\0') {
		name = hexname;

		snprintk(hexname, sizeof(hexname), "%p", (void *)thread);
		LOG_DBG("No thread name registered for %s", name);
		return;
	}

	thread_metrics = thread_search_by_name(name);
	if (!thread_metrics) {
		LOG_DBG("Not relevant stack: %s", name);
		return;
	}

	err = k_thread_stack_space_get(thread, &unused);
	if (err) {
		LOG_WRN(" %-20s: unable to get stack space (%d)", name, err);
		return;
	}

	LOG_DBG("Unused %s stack size: %d", thread_metrics->thread_name, unused);

	err = memfault_metrics_heartbeat_set_unsigned(thread_metrics->key, unused);
	if (err) {
		LOG_ERR("Failed to set unused stack metrics");
	}
}

int memfault_ncs_metrics_thread_add(struct memfault_ncs_metrics_thread *thread)
{
	if (!thread) {
		return -EINVAL;
	}

	if (!thread->thread_name) {
		return -EINVAL;
	}

	sys_slist_append(&thread_list, &thread->node);

	return 0;
}

void memfault_ncs_metrics_collect_data(void)
{
	if (IS_ENABLED(CONFIG_MEMFAULT_NCS_STACK_METRICS)) {
		k_thread_foreach_unlocked(stack_check, NULL);
	}

	if (IS_ENABLED(CONFIG_MEMFAULT_NCS_BT_METRICS)) {
		memfault_bt_metrics_update();
	}

	if (IS_ENABLED(CONFIG_MEMFAULT_NCS_LTE_METRICS)) {
		memfault_lte_metrics_update();
	}
}

#if defined(CONFIG_MEMFAULT_NCS_IMPLEMENT_METRICS_COLLECTION)
/* Overriding weak implementation in Memfault SDK.
 * The function is run on every heartbeat and can be used to get fresh updates
 * of metrics.
 */
void memfault_metrics_heartbeat_collect_data(void)
{
	memfault_ncs_metrics_collect_data();
}
#endif

void memfault_ncs_metrics_init(void)
{
	if (IS_ENABLED(CONFIG_MEMFAULT_NCS_LTE_METRICS)) {
		memfault_lte_metrics_init();
	}

	if (IS_ENABLED(CONFIG_MEMFAULT_NCS_BT_METRICS)) {
		memfault_bt_metrics_init();
	}
}
