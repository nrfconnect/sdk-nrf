/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing work specific definitions for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>

#include "zephyr_work.h"

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_LOG_LEVEL);

K_THREAD_STACK_DEFINE(bh_wq_stack_area, CONFIG_NRF700X_BH_WQ_STACK_SIZE);
struct k_work_q zep_wifi_bh_q;

K_THREAD_STACK_DEFINE(irq_wq_stack_area, CONFIG_NRF700X_IRQ_WQ_STACK_SIZE);
struct k_work_q zep_wifi_intr_q;

struct zep_work_item zep_work_item[CONFIG_NRF700X_WORKQ_MAX_ITEMS];

int get_free_work_item_index(void)
{
	int i;

	for (i = 0; i < CONFIG_NRF700X_WORKQ_MAX_ITEMS; i++) {
		if (zep_work_item[i].in_use)
			continue;
		return i;
	}

	return -1;
}

void workqueue_callback(struct k_work *work)
{
	struct zep_work_item *item = CONTAINER_OF(work, struct zep_work_item, work);

	item->callback(item->data);
}

struct zep_work_item *work_alloc(void)
{
	int free_work_index = get_free_work_item_index();

	if (free_work_index < 0) {
		LOG_ERR("%s: Reached maximum work items", __func__);
		return NULL;
	}

	zep_work_item[free_work_index].in_use = true;

	return &zep_work_item[free_work_index];
}

static int workqueue_init(void)
{
	k_work_queue_init(&zep_wifi_bh_q);

	k_work_queue_start(&zep_wifi_bh_q,
						bh_wq_stack_area,
						K_THREAD_STACK_SIZEOF(bh_wq_stack_area),
						CONFIG_NRF700X_BH_WQ_PRIORITY,
						NULL);

	k_work_queue_init(&zep_wifi_intr_q);

	k_work_queue_start(&zep_wifi_intr_q,
						irq_wq_stack_area,
						K_THREAD_STACK_SIZEOF(irq_wq_stack_area),
						CONFIG_NRF700X_IRQ_WQ_PRIORITY,
						NULL);

	return 0;
}

void work_init(struct zep_work_item *item, void (*callback)(unsigned long),
		  unsigned long data)
{
	item->callback = callback;
	item->data = data;

	k_work_init(&item->work, workqueue_callback);
}

void work_schedule(struct zep_work_item *item)
{
	k_work_submit_to_queue(&zep_wifi_bh_q, &item->work);
}

void work_kill(struct zep_work_item *item)
{
	/* TODO: Based on context, use _sync version */
	k_work_cancel(&item->work);
}

void work_free(struct zep_work_item *item)
{
	item->in_use = 0;
}

SYS_INIT(workqueue_init, POST_KERNEL, 0);
