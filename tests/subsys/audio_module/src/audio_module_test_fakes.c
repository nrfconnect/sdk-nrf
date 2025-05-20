/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <data_fifo.h>

#include "audio_module_test_common.h"
#include "audio_module_test_fakes.h"

/* Overload the message buffer pointer with a point to one of the an arrays below */
int fifo_num;

struct test_slab_queue {
	size_t head;
	size_t tail;
	size_t size;
	struct k_sem sem;

	void **data[FAKE_FIFO_MSG_QUEUE_SIZE];
	struct audio_module_message msg[FAKE_FIFO_MSG_QUEUE_SIZE];
};

struct test_msg_fifo_queue {
	size_t head;
	size_t tail;
	size_t size;
	struct k_sem sem;

	void **data[FAKE_FIFO_MSG_QUEUE_SIZE];
};

/* FIFO "slab" */
static struct test_slab_queue test_fifo_slab[FAKE_FIFO_NUM];

/* FIFO "message" queue */
static struct test_msg_fifo_queue test_fifo_msg_queue[FAKE_FIFO_NUM];

/*
 * Stubs are defined here, so that multiple *.c files can share them
 * without having linker issues.
 */
DEFINE_FFF_GLOBALS;

DEFINE_FAKE_VALUE_FUNC(int, data_fifo_pointer_first_vacant_get, struct data_fifo *, void **,
		       k_timeout_t);
DEFINE_FAKE_VALUE_FUNC(int, data_fifo_block_lock, struct data_fifo *, void **, size_t);
DEFINE_FAKE_VALUE_FUNC(int, data_fifo_pointer_last_filled_get, struct data_fifo *, void **,
		       size_t *, k_timeout_t);
DEFINE_FAKE_VOID_FUNC2(data_fifo_block_free, struct data_fifo *, void *);
DEFINE_FAKE_VALUE_FUNC(int, data_fifo_num_used_get, struct data_fifo *, uint32_t *, uint32_t *);
DEFINE_FAKE_VALUE_FUNC(int, data_fifo_empty, struct data_fifo *);
DEFINE_FAKE_VALUE_FUNC(int, data_fifo_uninit, struct data_fifo *);
DEFINE_FAKE_VALUE_FUNC(int, data_fifo_init, struct data_fifo *);
DEFINE_FAKE_VALUE_FUNC(bool, data_fifo_state, struct data_fifo *);

void fake_fifo_counter_reset(void)
{
	fifo_num = 0;
}

/* Custom fakes implementation */
int fake_data_fifo_pointer_first_vacant_get__succeeds(struct data_fifo *data_fifo, void **data,
						      k_timeout_t timeout)
{
	int ret;
	struct test_slab_queue *test_fifo_slab_data =
		(struct test_slab_queue *)data_fifo->slab_buffer;

	zassert_not_equal(data_fifo, NULL, "Data FIFO pointer is NULL");

	ret = k_sem_take(&test_fifo_slab_data->sem, timeout);
	zassert_equal(ret, 0, "Failed to take the slab semaphore: ret %d", ret);

	*data = &test_fifo_slab_data->msg[test_fifo_slab_data->head];
	test_fifo_slab_data->head = (test_fifo_slab_data->head + 1) % test_fifo_slab_data->size;

	return 0;
}

int fake_data_fifo_pointer_first_vacant_get__timeout_fails(struct data_fifo *data_fifo, void **data,
							   k_timeout_t timeout)
{
	ARG_UNUSED(data_fifo);
	ARG_UNUSED(data);
	ARG_UNUSED(timeout);

	return -ENOMSG;
}

int fake_data_fifo_pointer_first_vacant_get__no_wait_fails(struct data_fifo *data_fifo, void **data,
							   k_timeout_t timeout)
{
	ARG_UNUSED(data_fifo);
	ARG_UNUSED(data);
	ARG_UNUSED(timeout);

	return -EAGAIN;
}

int fake_data_fifo_pointer_first_vacant_get__invalid_fails(struct data_fifo *data_fifo, void **data,
							   k_timeout_t timeout)
{
	ARG_UNUSED(data_fifo);
	ARG_UNUSED(data);
	ARG_UNUSED(timeout);

	return -EINVAL;
}

int fake_data_fifo_block_lock__succeeds(struct data_fifo *data_fifo, void **data, size_t size)
{
	ARG_UNUSED(size);
	struct test_msg_fifo_queue *test_fifo_msg =
		(struct test_msg_fifo_queue *)data_fifo->msgq_buffer;

	zassert_not_equal(data_fifo, NULL, "Data FIFO pointer is NULL");

	k_sem_give(&test_fifo_msg->sem);

	test_fifo_msg->data[test_fifo_msg->head] = *data;
	test_fifo_msg->head = (test_fifo_msg->head + 1) % test_fifo_msg->size;

	return 0;
}

int fake_data_fifo_block_lock__size_fails(struct data_fifo *data_fifo, void **data, size_t size)
{
	ARG_UNUSED(data_fifo);
	ARG_UNUSED(data);
	ARG_UNUSED(size);

	return -ENOMEM;
}

int fake_data_fifo_block_lock__size_0_fails(struct data_fifo *data_fifo, void **data, size_t size)
{
	ARG_UNUSED(data_fifo);
	ARG_UNUSED(data);
	ARG_UNUSED(size);

	return -EINVAL;
}

int fake_data_fifo_block_lock__put_fails(struct data_fifo *data_fifo, void **data, size_t size)
{
	ARG_UNUSED(data_fifo);
	ARG_UNUSED(data);
	ARG_UNUSED(size);

	return -ESPIPE;
}

int fake_data_fifo_pointer_last_filled_get__succeeds(struct data_fifo *data_fifo, void **data,
						     size_t *size, k_timeout_t timeout)
{
	int ret;
	struct audio_module_message *msg;
	struct test_msg_fifo_queue *test_fifo_msg =
		(struct test_msg_fifo_queue *)data_fifo->msgq_buffer;

	zassert_not_equal(data_fifo, NULL, "Data FIFO pointer is NULL");

	ret = k_sem_take(&test_fifo_msg->sem, timeout);
	zassert_equal(ret, 0, "Failed to take the message semaphore: ret %d", ret);

	msg = (struct audio_module_message *)test_fifo_msg->data[test_fifo_msg->tail];

	test_fifo_msg->data[test_fifo_msg->tail] = NULL;
	test_fifo_msg->tail = (test_fifo_msg->tail + 1) % test_fifo_msg->size;

	*data = msg;
	*size = sizeof(struct audio_module_message);

	return 0;
}

int fake_data_fifo_pointer_last_filled_get__no_wait_fails(struct data_fifo *data_fifo, void **data,
							  size_t *size, k_timeout_t timeout)
{
	ARG_UNUSED(data_fifo);
	ARG_UNUSED(data);
	ARG_UNUSED(size);
	ARG_UNUSED(timeout);

	return -ENOMSG;
}

int fake_data_fifo_pointer_last_filled_get__timeout_fails(struct data_fifo *data_fifo, void **data,
							  size_t *size, k_timeout_t timeout)
{
	ARG_UNUSED(data_fifo);
	ARG_UNUSED(data);
	ARG_UNUSED(size);
	ARG_UNUSED(timeout);

	return -EAGAIN;
}

void fake_data_fifo_block_free__succeeds(struct data_fifo *data_fifo, void *data)
{
	ARG_UNUSED(data);

	zassert_not_equal(data_fifo, NULL, "Data FIFO pointer is NULL");

	struct test_slab_queue *test_fifo_slab_data =
		(struct test_slab_queue *)data_fifo->slab_buffer;

	k_sem_give(&test_fifo_slab_data->sem);
	test_fifo_slab_data->tail = (test_fifo_slab_data->tail + 1) % test_fifo_slab_data->size;
}

int fake_data_fifo_num_used_get__succeeds(struct data_fifo *data_fifo, uint32_t *alloced_num,
					  uint32_t *locked_num)
{
	struct test_msg_fifo_queue *test_fifo_msg =
		(struct test_msg_fifo_queue *)data_fifo->msgq_buffer;

	*alloced_num = test_fifo_msg->head - test_fifo_msg->tail;
	*locked_num = k_sem_count_get(&test_fifo_msg->sem);

	return 0;
}

int fake_data_fifo_num_used_get__fails(struct data_fifo *data_fifo, uint32_t *alloced_num,
				       uint32_t *locked_num)
{
	ARG_UNUSED(data_fifo);
	ARG_UNUSED(alloced_num);
	ARG_UNUSED(locked_num);

	return -EACCES;
}

int fake_data_fifo_empty__succeeds(struct data_fifo *data_fifo)
{
	int ret;
	struct test_msg_fifo_queue *test_fifo_msg =
		(struct test_msg_fifo_queue *)data_fifo->msgq_buffer;
	struct test_slab_queue *test_fifo_slab_data =
		(struct test_slab_queue *)data_fifo->slab_buffer;

	zassert_not_equal(data_fifo, NULL, "Data FIFO pointer is NULL");

	test_fifo_msg->head = 0;
	test_fifo_msg->tail = 0;
	test_fifo_msg->size = FAKE_FIFO_MSG_QUEUE_SIZE;
	k_sem_reset(&test_fifo_msg->sem);

	test_fifo_slab_data->head = 0;
	test_fifo_slab_data->tail = 0;
	test_fifo_slab_data->size = FAKE_FIFO_MSG_QUEUE_SIZE;
	ret = k_sem_init(&test_fifo_slab_data->sem, FAKE_FIFO_MSG_QUEUE_SIZE,
			 FAKE_FIFO_MSG_QUEUE_SIZE);
	zassert_equal(ret, 0, "Failed to initialize the slab semaphore: ret %d", ret);

	return 0;
}

int fake_data_fifo_empty__count_fails(struct data_fifo *data_fifo)
{
	ARG_UNUSED(data_fifo);

	return -EACCES;
}

int fake_data_fifo_empty__no_wait_fails(struct data_fifo *data_fifo)
{
	ARG_UNUSED(data_fifo);

	return -ENOMSG;
}

int fake_data_fifo_empty__slab_init_fails(struct data_fifo *data_fifo)
{
	ARG_UNUSED(data_fifo);

	return -EINVAL;
}

int fake_data_fifo_empty__timeout_fails(struct data_fifo *data_fifo)
{
	ARG_UNUSED(data_fifo);

	return -EAGAIN;
}

int fake_data_fifo_uninit__succeeds(struct data_fifo *data_fifo)
{
	int ret;

	zassert_not_equal(data_fifo, NULL, "Data FIFO pointer is NULL");

	if (data_fifo->initialized) {
		ret = data_fifo_empty(data_fifo);
		if (ret) {
			return ret;
		}

		data_fifo->initialized = false;
	}

	return 0;
}

int fake_data_fifo_uninit__fails(struct data_fifo *data_fifo)
{
	ARG_UNUSED(data_fifo);

	return -EINVAL;
}

int fake_data_fifo_init__succeeds(struct data_fifo *data_fifo)
{
	int ret;
	struct test_msg_fifo_queue *test_fifo_msg = &test_fifo_msg_queue[fifo_num];
	struct test_slab_queue *test_fifo_slab_data = &test_fifo_slab[fifo_num];

	zassert_not_equal(data_fifo, NULL, "Data FIFO pointer is NULL");

	data_fifo->msgq_buffer = (char *)test_fifo_msg;
	data_fifo->slab_buffer = (char *)test_fifo_slab_data;

	data_fifo->elements_max = FAKE_FIFO_MSG_QUEUE_SIZE;
	data_fifo->block_size_max = TEST_MOD_DATA_SIZE;
	data_fifo->initialized = true;

	fifo_num += 1;

	test_fifo_msg->head = 0;
	test_fifo_msg->tail = 0;
	test_fifo_msg->size = FAKE_FIFO_MSG_QUEUE_SIZE;
	ret = k_sem_init(&test_fifo_msg->sem, 0, FAKE_FIFO_MSG_QUEUE_SIZE);
	zassert_equal(ret, 0, "Failed to initialize the message semaphore: ret %d", ret);

	test_fifo_slab_data->head = 0;
	test_fifo_slab_data->tail = 0;
	test_fifo_slab_data->size = FAKE_FIFO_MSG_QUEUE_SIZE;
	ret = k_sem_init(&test_fifo_slab_data->sem, FAKE_FIFO_MSG_QUEUE_SIZE,
			 FAKE_FIFO_MSG_QUEUE_SIZE);
	zassert_equal(ret, 0, "Failed to initialize the slab semaphore: ret %d", ret);

	for (int i = 0; i < FAKE_FIFO_MSG_QUEUE_SIZE; i++) {
		test_fifo_slab_data->data[i] = (void **)&test_fifo_slab_data->msg[i];
	}

	data_fifo->initialized = true;

	return 0;
}

int fake_data_fifo_init__fails(struct data_fifo *data_fifo)
{
	ARG_UNUSED(data_fifo);

	return -EINVAL;
}

bool fake_data_fifo_state__succeeds(struct data_fifo *data_fifo)
{
	zassert_not_equal(data_fifo, NULL, "Data FIFO pointer is NULL");

	return data_fifo->initialized;
}
