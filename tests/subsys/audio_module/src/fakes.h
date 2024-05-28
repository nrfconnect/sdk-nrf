/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FAKES_H_
#define _FAKES_H_

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <data_fifo.h>

#include "audio_module_test_common.h"

/**
 * @brief Deinitialize data FIFO structure.
 *
 * @param data_fifo  [in/out]  The data FIFO instance.
 */
void data_fifo_deinit(struct data_fifo *data_fifo);

/* Fake functions declaration. */
DECLARE_FAKE_VALUE_FUNC(int, data_fifo_pointer_first_vacant_get, struct data_fifo *, void **,
			k_timeout_t);
DECLARE_FAKE_VALUE_FUNC(int, data_fifo_block_lock, struct data_fifo *, void **, size_t);
DECLARE_FAKE_VALUE_FUNC(int, data_fifo_pointer_last_filled_get, struct data_fifo *, void **,
			size_t *, k_timeout_t);
DECLARE_FAKE_VOID_FUNC2(data_fifo_block_free, struct data_fifo *, void *);
DECLARE_FAKE_VALUE_FUNC(int, data_fifo_num_used_get, struct data_fifo *, uint32_t *, uint32_t *);
DECLARE_FAKE_VALUE_FUNC(int, data_fifo_empty, struct data_fifo *);
DECLARE_FAKE_VALUE_FUNC(int, data_fifo_init, struct data_fifo *);

/* List of fakes used by this unit tester */
#define DO_FOREACH_FAKE(FUNC)                                                                      \
	do {                                                                                       \
		FUNC(data_fifo_pointer_first_vacant_get)                                           \
		FUNC(data_fifo_block_lock)                                                         \
		FUNC(data_fifo_pointer_last_filled_get)                                            \
		FUNC(data_fifo_block_free)                                                         \
		FUNC(data_fifo_num_used_get)                                                       \
		FUNC(data_fifo_empty)                                                              \
		FUNC(data_fifo_init)                                                               \
	} while (0)

int fake_data_fifo_pointer_first_vacant_get__succeeds(struct data_fifo *data_fifo, void **data,
						      k_timeout_t timeout);
int fake_data_fifo_pointer_first_vacant_get__timeout_fails(struct data_fifo *data_fifo, void **data,
							   k_timeout_t timeout);
int fake_data_fifo_pointer_first_vacant_get__no_wait_fails(struct data_fifo *data_fifo, void **data,
							   k_timeout_t timeout);
int fake_data_fifo_pointer_first_vacant_get__invalid_fails(struct data_fifo *data_fifo, void **data,
							   k_timeout_t timeout);
int fake_data_fifo_block_lock__succeeds(struct data_fifo *data_fifo, void **data, size_t size);
int fake_data_fifo_block_lock__size_fails(struct data_fifo *data_fifo, void **data, size_t size);
int fake_data_fifo_block_lock__size_0_fails(struct data_fifo *data_fifo, void **data, size_t size);
int fake_data_fifo_block_lock__put_fails(struct data_fifo *data_fifo, void **data, size_t size);
int fake_data_fifo_pointer_last_filled_get__succeeds(struct data_fifo *data_fifo, void **data,
						     size_t *size, k_timeout_t timeout);
int fake_data_fifo_pointer_last_filled_get__no_wait_fails(struct data_fifo *data_fifo, void **data,
							  size_t *size, k_timeout_t timeout);
int fake_data_fifo_pointer_last_filled_get__timeout_fails(struct data_fifo *data_fifo, void **data,
							  size_t *size, k_timeout_t timeout);
void fake_data_fifo_block_free__succeeds(struct data_fifo *data_fifo, void *data);
int fake_data_fifo_num_used_get__succeeds(struct data_fifo *data_fifo, uint32_t *alloced_num,
					  uint32_t *locked_num);
int fake_data_fifo_num_used_get__fails(struct data_fifo *data_fifo, uint32_t *alloced_num,
				       uint32_t *locked_num);
int fake_data_fifo_empty__succeeds(struct data_fifo *data_fifo);
int fake_data_fifo_empty__count_fails(struct data_fifo *data_fifo);
int fake_data_fifo_empty__no_wait_fails(struct data_fifo *data_fifo);
int fake_data_fifo_empty__slab_init_fails(struct data_fifo *data_fifo);
int fake_data_fifo_empty__timeout_fails(struct data_fifo *data_fifo);
int fake_data_fifo_init__succeeds(struct data_fifo *data_fifo);
int fake_data_fifo_init__fails(struct data_fifo *data_fifo);

#endif /* _FAKES_H_ */
