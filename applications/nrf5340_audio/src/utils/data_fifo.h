/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _DATA_FIFO_H_
#define _DATA_FIFO_H_

#include <stddef.h>
#include <stdint.h>
#include <kernel.h>

/* The queue elements hold a pointer to a memory block in a slab and the
 * number of bytes written to that block.
 */
struct data_fifo_msgq {
	void *block_ptr;
	size_t size;
};

struct data_fifo {
	char *msgq_buffer;
	char *slab_buffer;
	struct k_mem_slab mem_slab;
	struct k_msgq msgq;
	uint32_t elements_max;
	size_t block_size_max;
	bool initialized;
};

#define DATA_FIFO_DEFINE(name, elements_max_in, block_size_max_in)                                 \
	char __aligned(WB_UP(1))                                                                   \
		_msgq_buffer_##name[(elements_max_in) * sizeof(struct data_fifo_msgq)] = { 0 };    \
	char __aligned(WB_UP(1))                                                                   \
		_slab_buffer_##name[(elements_max_in) * (block_size_max_in)] = { 0 };              \
	struct data_fifo name = { .msgq_buffer = _msgq_buffer_##name,                              \
				  .slab_buffer = _slab_buffer_##name,                              \
				  .block_size_max = block_size_max_in,                             \
				  .elements_max = elements_max_in,                                 \
				  .initialized = false }

/**
 * @brief Get pointer to first vacant block in slab.
 *
 * Gives pointer to the first vacant memory block in the
 * slab.
 *
 * @param data_fifo Pointer to the data_fifo structure
 * @param data Double pointer to the memory area. If this function returns with
 *	success, the caller is now able to write to this memory block. Note that
 *	the write operation must not exceeed the block size max given to
 *	DATA_FIFO_DEFINE.
 * @param timeout Non-negative waiting period to wait for operation to complete
 *	(in milliseconds). Use K_NO_WAIT to return without waiting,
 *	or K_FOREVER to wait as long as necessary.
 *
 * @retval 0 Memory allocated.
 * @retval Return values from k_mem_slab_alloc.
 */
int data_fifo_pointer_first_vacant_get(struct data_fifo *data_fifo, void **data,
				       k_timeout_t timeout);

/**
 * @brief Confirm that memory block use has finished
 * and the block put into the message queue.
 *
 * There is no mechanism blocking this region from being written to or read from.
 * Hence, this block should not be used before it is later fetched
 * by using data_fifo_pointer_last_filled_get.
 *
 * @param data_fifo Pointer to the data_fifo structure.
 * @param data Double pointer to the memory block which has been written to.
 * @param size Number of bytes written. Must be equal to or smaller
 *		than the block size max.
 *
 * @retval 0		Block has been sumbitted to the message queue.
 * @retval -ENOMEM	size is larger than block size max.
 * @retval -EINVAL	Supplied size is zero
 * @retval -ESPIPE	Generic return if an error occurs in k_msg_put.
 *			Since data has already been added to the slab, there
 *			must be space in the message queue.
 */
int data_fifo_block_lock(struct data_fifo *data_fifo, void **data, size_t size);

/**
 * @brief Get pointer to first (oldest) filled block in slab.
 *
 * This returns a pointer to the first filled block in the
 * slab (FIFO).
 *
 * @param data_fifo Pointer to the data_fifo structure.
 * @param data Double pointer to the block. If this functions returns with
 *	success, the caller is now able to read from this memory block.
 * @param size Actual size in bytes of the stored data. Note that this may
 *	be equal to or less than the block size.
 * @param timeout Non-negative waiting period to wait for operation to complete
 *	(in milliseconds). Use K_NO_WAIT to return without waiting,
 *	or K_FOREVER to wait as long as necessary.
 *
 * @retval 0 Memory pointer retrieved.
 * @retval Return values from k_msgq_get.
 */
int data_fifo_pointer_last_filled_get(struct data_fifo *data_fifo, void **data, size_t *size,
				      k_timeout_t timeout);

/**
 * @brief Free the data block after reading.
 *
 * Read has finished in the given data block.
 *
 * @param data_fifo Pointer to the data_fifo structure.
 * @param data Double pointer to the memory area which is to be freed.
 *
 * @retval 0	Memory block is freed.
 * @retval Return values from k_mem_slab_free.
 */
int data_fifo_block_free(struct data_fifo *data_fifo, void **data);

/**
 * @brief See how many alloced and locked blocks are in the system.
 *
 * @param data_fifo Pointer to the data_fifo structure.
 * @param alloced_num Number of used blocks in the slab.
 * @param locked_num Number of used items in the message queue.
 *
 * @retval 0		Success
 * @retval -EACCES	Illegal combination of used message queue items
 *			and slabs. If an error occurs, parameters
 *			will be set to UINT32_MAX.
 */
int data_fifo_num_used_get(struct data_fifo *data_fifo, uint32_t *alloced_num,
			   uint32_t *locked_num);

/**
 * @brief Initialise the data_fifo.
 *
 * @param data_fifo Pointer to the data_fifo structure.
 *
 * @retval 0		Success
 * @retval Return values from k_mem_slab_init.
 */
int data_fifo_init(struct data_fifo *data_fifo);

#endif /* _DATA_FIFO_H_ */
