/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fp_fmdn_dult_integration, CONFIG_BT_FAST_PAIR_LOG_LEVEL);

#include <dult.h>

#include "fp_fmdn_dult_integration.h"
#include "fp_activation.h"
#include "fp_registration_data.h"

BUILD_ASSERT(CONFIG_BT_FAST_PAIR_FMDN_DULT_INTEGRATION_INIT_PRIORITY <
	     FP_ACTIVATION_INIT_PRIORITY_DEFAULT);
BUILD_ASSERT(CONFIG_BT_FAST_PAIR_FMDN_DULT_INTEGRATION_INIT_PRIORITY_LATE >
	     FP_ACTIVATION_INIT_PRIORITY_DEFAULT);
BUILD_ASSERT(CONFIG_BT_FAST_PAIR_FMDN_DULT_INTEGRATION_INIT_PRIORITY >
	     CONFIG_BT_FAST_PAIR_REGISTRATION_DATA_INIT_PRIORITY);

/* Assert that names are not empty. */
BUILD_ASSERT(sizeof(CONFIG_BT_FAST_PAIR_FMDN_DULT_MANUFACTURER_NAME) > 1);
BUILD_ASSERT(sizeof(CONFIG_BT_FAST_PAIR_FMDN_DULT_MODEL_NAME) > 1);

/* Firmware version of 0.0.0 causes Fast Pair Validator Android app test failure. */
BUILD_ASSERT((CONFIG_BT_FAST_PAIR_FMDN_DULT_FIRMWARE_VERSION_MAJOR != 0) ||
	     (CONFIG_BT_FAST_PAIR_FMDN_DULT_FIRMWARE_VERSION_MINOR != 0) ||
	     (CONFIG_BT_FAST_PAIR_FMDN_DULT_FIRMWARE_VERSION_REVISION != 0));

static uint8_t product_data[DULT_PRODUCT_DATA_LEN];

static const struct dult_user dult_user = {
	.product_data = product_data,
	.manufacturer_name = CONFIG_BT_FAST_PAIR_FMDN_DULT_MANUFACTURER_NAME,
	.model_name = CONFIG_BT_FAST_PAIR_FMDN_DULT_MODEL_NAME,
	.accessory_category = CONFIG_BT_FAST_PAIR_FMDN_DULT_ACCESSORY_CATEGORY,
	.accessory_capabilities = (
		(IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN_DULT_CAPABILITY_PLAY_SOUND) ?
			BIT(DULT_ACCESSORY_CAPABILITY_PLAY_SOUND_BIT_POS) : 0) |
		(IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN_DULT_CAPABILITY_ID_LOOKUP_BLE) ?
			BIT(DULT_ACCESSORY_CAPABILITY_ID_LOOKUP_BLE_BIT_POS) : 0)),

	.network_id = DULT_NETWORK_ID_GOOGLE,
	.firmware_version = {
		.major = CONFIG_BT_FAST_PAIR_FMDN_DULT_FIRMWARE_VERSION_MAJOR,
		.minor = CONFIG_BT_FAST_PAIR_FMDN_DULT_FIRMWARE_VERSION_MINOR,
		.revision = CONFIG_BT_FAST_PAIR_FMDN_DULT_FIRMWARE_VERSION_REVISION,
	},
};

const struct dult_user *fp_fmdn_dult_integration_user_get(void)
{
	return &dult_user;
}

static int dult_init(void)
{
	static const size_t model_id_offset = sizeof(product_data) - FP_REG_DATA_MODEL_ID_LEN;
	int err;

	err = fp_reg_data_get_model_id(&product_data[model_id_offset],
				       sizeof(product_data) - model_id_offset);
	if (err) {
		LOG_ERR("FMDN: fp_reg_data_get_model_id returned error: %d", err);
		return err;
	}

	err = dult_user_register(&dult_user);
	if (err) {
		LOG_ERR("FMDN: dult_user_register returned error: %d", err);
		return err;
	}

	return 0;
}

static int dult_uninit(void)
{
	int err;

	err = dult_reset(&dult_user);
	if (err) {
		LOG_ERR("FMDN: dult_reset returned error: %d", err);
		return err;
	}

	return 0;
}

static int dult_init_late(void)
{
	int err;

	err = dult_enable(&dult_user);
	if (err) {
		LOG_ERR("FMDN: dult_enable returned error: %d", err);
		return err;
	}

	return 0;
}

static int dult_uninit_early(void)
{
	/* Intentionally left empty. */
	return 0;
}

FP_ACTIVATION_MODULE_REGISTER(fp_fmdn_dult_integration,
			      CONFIG_BT_FAST_PAIR_FMDN_DULT_INTEGRATION_INIT_PRIORITY,
			      dult_init,
			      dult_uninit);

FP_ACTIVATION_MODULE_REGISTER(fp_fmdn_dult_integration_late,
			      CONFIG_BT_FAST_PAIR_FMDN_DULT_INTEGRATION_INIT_PRIORITY_LATE,
			      dult_init_late,
			      dult_uninit_early);
