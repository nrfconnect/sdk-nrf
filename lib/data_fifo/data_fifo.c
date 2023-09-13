/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "data_fifo.h"

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(data_fifo, CONFIG_DATA_FIFO_LOG_LEVEL);

static struct k_spinlock lock;

/** @brief Checks that the elements in the msgq and slab are legal.
 * I.e. the number of msgq elements cannot be more than mem blocks used.
 */
static int msgq_slab_legal_used_elements(struct data_fifo *data_fifo, uint32_t *msgq_num_used_in,
					 uint32_t *slab_blocks_num_used_in)
{
	/* Lock so msgq and slab reads are in sync */
	k_spinlock_key_t key = k_spin_lock(&lock);

	uint32_t msgq_num_used = k_msgq_num_used_get(&data_fifo->msgq);
	uint32_t slab_blocks_num_used = k_mem_slab_num_used_get(&data_fifo->mem_slab);

	k_spin_unlock(&lock, key);

	if (slab_blocks_num_used < msgq_num_used) {
		LOG_ERR("Num used mgsq %d cannot be larger than used blocks %d", msgq_num_used,
			slab_blocks_num_used);
		return -EACCES;
	}

	if (msgq_num_used_in != NULL && slab_blocks_num_used_in != NULL) {
		*msgq_num_used_in = msgq_num_used;
		*slab_blocks_num_used_in = slab_blocks_num_used;
	}

	return 0;
}

int data_fifo_pointer_first_vacant_get(struct data_fifo *data_fifo, void **data,
				       k_timeout_t timeout)
{
	__ASSERT_NO_MSG(data_fifo != NULL);
	__ASSERT_NO_MSG(data_fifo->initialized);
	int ret;

	ret = k_mem_slab_alloc(&data_fifo->mem_slab, data, timeout);
	return ret;
}

int data_fifo_block_lock(struct data_fifo *data_fifo, void **data, size_t size)
{
	__ASSERT_NO_MSG(data_fifo != NULL);
	__ASSERT_NO_MSG(data_fifo->initialized);
	int ret;

	if (size > data_fifo->block_size_max) {
		LOG_ERR("Size %zu too big", size);
		return -ENOMEM;
	} else if (size == 0) {
		LOG_ERR("Size is zero");
		return -EINVAL;
	}

	struct data_fifo_msgq msgq_tmp;

	msgq_tmp.block_ptr = *data;
	msgq_tmp.size = size;

	/* Since num elements in the slab and msgq are equal, there
	 * must be space in the queue. if k_msg_put fails, it
	 * is fatal.
	 */
	ret = k_msgq_put(&data_fifo->msgq, &msgq_tmp, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Fatal error %d from k_msgq_put", ret);
		return -ESPIPE;
	}

	return 0;
}

int data_fifo_pointer_last_filled_get(struct data_fifo *data_fifo, void **data, size_t *size,
				      k_timeout_t timeout)
{
	__ASSERT_NO_MSG(data_fifo != NULL);
	__ASSERT_NO_MSG(data_fifo->initialized);
	int ret;

	struct data_fifo_msgq msgq_tmp;

	ret = k_msgq_get(&data_fifo->msgq, &msgq_tmp, timeout);
	if (ret) {
		return ret;
	}

	*data = msgq_tmp.block_ptr;
	*size = msgq_tmp.size;
	return 0;
}

void data_fifo_block_free(struct data_fifo *data_fifo, void *data)
{
	__ASSERT_NO_MSG(data_fifo != NULL);
	__ASSERT_NO_MSG(data_fifo->initialized);

	k_mem_slab_free(&data_fifo->mem_slab, data);
}

int data_fifo_num_used_get(struct data_fifo *data_fifo, uint32_t *alloced_num, uint32_t *locked_num)
{
	__ASSERT_NO_MSG(data_fifo != NULL);
	__ASSERT_NO_MSG(data_fifo->initialized);
	int ret;

	uint32_t msgq_num_used = UINT32_MAX;
	uint32_t slab_blocks_num_used = UINT32_MAX;

	ret = msgq_slab_legal_used_elements(data_fifo, &msgq_num_used, &slab_blocks_num_used);
	if (ret) {
		return ret;
	}

	*locked_num = msgq_num_used;
	*alloced_num = slab_blocks_num_used;

	return ret;
}

int data_fifo_empty(struct data_fifo *data_fifo)
{
	uint32_t fifo_alloced_num, fifo_locked_num;
	int ret;
	void *old_data;
	size_t size;

	ret = data_fifo_num_used_get(data_fifo, &fifo_alloced_num, &fifo_locked_num);
	if (ret) {
		LOG_ERR("Failed to get num used in FIFO");
		return ret;
	}

	for (int i = 0; i < fifo_locked_num; i++) {
		ret = data_fifo_pointer_last_filled_get(data_fifo, &old_data, &size, K_NO_WAIT);
		if (ret == -ENOMSG) {
			/* If there are no more blocks in FIFO, break */
			break;
		} else if (ret) {
			return ret;
		}

		data_fifo_block_free(data_fifo, old_data);
	}

	/* Re-init k_mem_slab to reset the number of alloced slabs */
	ret = k_mem_slab_init(&data_fifo->mem_slab, data_fifo->slab_buffer,
			      data_fifo->block_size_max, data_fifo->elements_max);
	if (ret) {
		LOG_ERR("Slab init failed with %d", ret);
		return ret;
	}

	return 0;
}

int data_fifo_init(struct data_fifo *data_fifo)
{
	__ASSERT_NO_MSG(data_fifo != NULL);
	__ASSERT_NO_MSG(!data_fifo->initialized);
	__ASSERT_NO_MSG(data_fifo->elements_max != 0);
	__ASSERT_NO_MSG(data_fifo->block_size_max != 0);
	__ASSERT_NO_MSG((data_fifo->block_size_max % WB_UP(1)) == 0);
	int ret;

	k_msgq_init(&data_fifo->msgq, data_fifo->msgq_buffer, sizeof(struct data_fifo_msgq),
		    data_fifo->elements_max);

	ret = k_mem_slab_init(&data_fifo->mem_slab, data_fifo->slab_buffer,
			      data_fifo->block_size_max, data_fifo->elements_max);
	if (ret) {
		LOG_ERR("Slab init failed with %d", ret);
		return ret;
	}

	data_fifo->initialized = true;

	return ret;
}
