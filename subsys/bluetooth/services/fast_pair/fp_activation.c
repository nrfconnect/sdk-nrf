/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fast_pair, CONFIG_BT_FAST_PAIR_LOG_LEVEL);

#include <bluetooth/services/fast_pair.h>
#include "fp_activation.h"
#include "fp_storage.h"

/* Enum used to describe Fast Pair state. */
enum state {
	STATE_DISABLED,
	STATE_ENABLED,
	STATE_ENABLING,
	STATE_DISABLING,
	STATE_ERROR,

	STATE_COUNT,
};

static atomic_t state = ATOMIC_INIT(STATE_DISABLED);

static int modules_init(void)
{
	int err = 0;
	size_t module_idx = 0;

	STRUCT_SECTION_FOREACH(fp_activation_module, module) {
		err = module->module_init();
		if (err) {
			break;
		}

		module_idx++;
	}

	if (err) {
		for (int i = module_idx; i >= 0; i--) {
			struct fp_activation_module *module;

			STRUCT_SECTION_GET(fp_activation_module, i, &module);
			(void)module->module_uninit();
		}
	}

	return err;
}

static int modules_uninit(void)
{
	int first_err = 0;
	int module_count;

	STRUCT_SECTION_COUNT(fp_activation_module, &module_count);

	for (int i = module_count - 1; i >= 0; i--) {
		int err;
		struct fp_activation_module *module;

		STRUCT_SECTION_GET(fp_activation_module, i, &module);
		err = module->module_uninit();
		if (err && !first_err) {
			first_err = err;
		}
	}

	return first_err;
}

int bt_fast_pair_enable(void)
{
	int err;

	/* It is assumed that this function executes in the cooperative thread context. */
	__ASSERT_NO_MSG(!k_is_preempt_thread());
	__ASSERT_NO_MSG(!k_is_in_isr());

	if (!bt_is_ready()) {
		LOG_ERR("BLE stack shall be enabled before enabling Fast Pair");
		return -ENOPROTOOPT;
	}

	/* Verify the enabling conditions. */
	if (atomic_get(&state) == STATE_ENABLED) {
		LOG_ERR("Fast Pair already enabled");
		return -EALREADY;
	}

	atomic_set(&state, STATE_ENABLING);

	err = modules_init();
	if (err) {
		LOG_ERR("modules_init returned error %d", err);
		atomic_set(&state, STATE_ERROR);
	} else {
		atomic_set(&state, STATE_ENABLED);
		LOG_DBG("Fast Pair is enabled");
	}

	return err;
}

int bt_fast_pair_disable(void)
{
	int err;

	/* It is assumed that this function executes in the cooperative thread context. */
	__ASSERT_NO_MSG(!k_is_preempt_thread());
	__ASSERT_NO_MSG(!k_is_in_isr());

	/* Verify the disabling conditions. */
	if (atomic_get(&state) == STATE_DISABLED) {
		LOG_ERR("Fast Pair already disabled");
		return -EALREADY;
	}

	atomic_set(&state, STATE_DISABLING);

	err = modules_uninit();
	if (err) {
		LOG_ERR("modules_uninit returned error %d", err);
		atomic_set(&state, STATE_ERROR);
	} else {
		atomic_set(&state, STATE_DISABLED);
		LOG_DBG("Fast Pair is disabled");
	}

	return err;
}

bool bt_fast_pair_is_ready(void)
{
	return (atomic_get(&state) == STATE_ENABLED);
}
