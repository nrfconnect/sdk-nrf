/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "aggregator.h"
#include <zephyr/kernel.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/types.h>
#include <string.h>
#include <zephyr/irq.h>

K_FIFO_DEFINE(aggregator_fifo);

static uint32_t entry_count;

struct fifo_entry {
	void *fifo_reserved;
	uint8_t data[sizeof(struct sensor_data)];
};

int aggregator_put(struct sensor_data in_data)
{
	struct fifo_entry *fifo_data = NULL;
	uint32_t  lock = irq_lock();
	int    err  = 0;

	if (entry_count == FIFO_MAX_ELEMENT_COUNT) {
		fifo_data = k_fifo_get(&aggregator_fifo, K_NO_WAIT);

		__ASSERT(fifo_data != NULL, "fifo_data should not be NULL");

		entry_count--;
	}

	if (fifo_data == NULL) {
		fifo_data = k_malloc(sizeof(struct fifo_entry));
	}

	if (fifo_data == NULL) {
		err = -ENOMEM;
		goto exit;
	}

	memcpy(fifo_data->data, &in_data, sizeof(struct sensor_data));

	k_fifo_put(&aggregator_fifo, fifo_data);
	entry_count++;

exit:
	irq_unlock(lock);
	return err;
}

int aggregator_get(struct sensor_data *out_data)
{
	void  *fifo_data;
	int   err = 0;

	if (out_data == NULL) {
		return -EINVAL;
	}

	uint32_t lock = irq_lock();

	fifo_data = k_fifo_get(&aggregator_fifo, K_NO_WAIT);
	if (fifo_data == NULL) {
		err = -ENODATA;
		goto exit;
	}

	memcpy(out_data, ((struct fifo_entry *)fifo_data)->data,
	       sizeof(struct sensor_data));

	k_free(fifo_data);
	entry_count--;

exit:
	irq_unlock(lock);
	return err;
}

void aggregator_clear(void)
{
	void *fifo_data;
	uint32_t lock = irq_lock();

	while (1) {
		fifo_data = k_fifo_get(&aggregator_fifo, K_NO_WAIT);

		if (fifo_data == NULL) {
			break;
		}

		k_free(fifo_data);
	};

	entry_count = 0;
	irq_unlock(lock);
}

int aggregator_element_count_get(void)
{
	return entry_count;
}
