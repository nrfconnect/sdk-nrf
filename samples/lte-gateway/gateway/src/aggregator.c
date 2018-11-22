/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include "aggregator.h"
#include <kernel.h>
#include <zephyr.h>
#include <stdio.h>
#include <zephyr/types.h>
#include <string.h>
#include <board.h>

K_FIFO_DEFINE(aggregator_fifo);

static u32_t entry_count;

struct fifo_entry {
	void *fifo_reserved;
	u8_t  length;
	u32_t data[ENTRY_MAX_SIZE];
};

int aggregator_put(struct sensor_data in_data)
{
	struct fifo_entry *fifo_data;

	if (entry_count >= FIFO_MAX_ELEMENT_COUNT) {
		aggregator_get(NULL);
	}

	fifo_data = k_malloc(sizeof(struct fifo_entry));
	if (fifo_data == NULL) {
		return -1;
	}
	memcpy(fifo_data->data, &in_data, sizeof(struct sensor_data));
	k_fifo_put(&aggregator_fifo, fifo_data);
	entry_count++;
	printk("Queue size %d\n", entry_count);
	return 0;
}

int aggregator_get(struct sensor_data *out_data)
{
	void *fifo_data;

	if (out_data == NULL) {
		/* If the get function is called with NULL the oldest element
		 * in the FIFO is removed. Useful for making space in the FIFO.
		 */
		fifo_data = k_fifo_get(&aggregator_fifo, K_NO_WAIT);
		entry_count--;
		k_free(fifo_data);
		return 0;
	}

	fifo_data = k_fifo_get(&aggregator_fifo, K_NO_WAIT);
	if (fifo_data == NULL) {
		return -1;
	}

	memcpy(out_data, ((struct fifo_entry *)fifo_data)->data,
	       sizeof(struct sensor_data));
	k_free(fifo_data);
	entry_count--;

	return 0;
}

void aggregator_purge(void)
{
	void *fifo_data = NULL;

	while (1) {
		fifo_data = k_fifo_get(&aggregator_fifo, K_NO_WAIT);
		if (fifo_data == NULL) {
			break;
		}
		k_free(fifo_data);
	};
	k_fifo_init(&aggregator_fifo);
}

int aggregator_element_count_get(void)
{
	return entry_count;
}

void aggregator_process(void)
{
}
