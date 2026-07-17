/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fp_fhn_dult_integration, CONFIG_BT_FAST_PAIR_LOG_LEVEL);

#include <dult/dult.h>
#include <dult/multi_user.h>

#include "fp_fhn_dult_integration.h"
#include "fp_fhn_callbacks.h"
#include "fp_activation.h"
#include "fp_registration_data.h"

BUILD_ASSERT(CONFIG_BT_FAST_PAIR_FHN_DULT_INTEGRATION_INIT_PRIORITY <
	     FP_ACTIVATION_INIT_PRIORITY_DEFAULT);
BUILD_ASSERT(CONFIG_BT_FAST_PAIR_FHN_DULT_INTEGRATION_INIT_PRIORITY >
	     CONFIG_BT_FAST_PAIR_REGISTRATION_DATA_INIT_PRIORITY);

/* Assert that names are not empty. */
BUILD_ASSERT(sizeof(CONFIG_BT_FAST_PAIR_FHN_DULT_MANUFACTURER_NAME) > 1);
BUILD_ASSERT(sizeof(CONFIG_BT_FAST_PAIR_FHN_DULT_MODEL_NAME) > 1);

/* Firmware version of 0.0.0 causes Fast Pair Validator Android app test failure. */
BUILD_ASSERT((CONFIG_BT_FAST_PAIR_FHN_DULT_FIRMWARE_VERSION_MAJOR != 0) ||
	     (CONFIG_BT_FAST_PAIR_FHN_DULT_FIRMWARE_VERSION_MINOR != 0) ||
	     (CONFIG_BT_FAST_PAIR_FHN_DULT_FIRMWARE_VERSION_REVISION != 0));

static uint8_t product_data[DULT_PRODUCT_DATA_LEN];

static const struct dult_user dult_user = {
	.product_data = product_data,
	.manufacturer_name = CONFIG_BT_FAST_PAIR_FHN_DULT_MANUFACTURER_NAME,
	.model_name = CONFIG_BT_FAST_PAIR_FHN_DULT_MODEL_NAME,
	.accessory_category = CONFIG_BT_FAST_PAIR_FHN_DULT_ACCESSORY_CATEGORY,
	.accessory_capabilities = (
		(IS_ENABLED(CONFIG_BT_FAST_PAIR_FHN_DULT_CAPABILITY_PLAY_SOUND) ?
			BIT(DULT_ACCESSORY_CAPABILITY_PLAY_SOUND_BIT_POS) : 0) |
		(IS_ENABLED(CONFIG_BT_FAST_PAIR_FHN_DULT_CAPABILITY_MOTION_DETECTOR_UT) ?
			BIT(DULT_ACCESSORY_CAPABILITY_MOTION_DETECTOR_UT_BIT_POS) : 0) |
		(IS_ENABLED(CONFIG_BT_FAST_PAIR_FHN_DULT_CAPABILITY_ID_LOOKUP_BLE) ?
			BIT(DULT_ACCESSORY_CAPABILITY_ID_LOOKUP_BLE_BIT_POS) : 0)),

	.network_id = DULT_NETWORK_ID_GOOGLE,
	.firmware_version = {
		.major = CONFIG_BT_FAST_PAIR_FHN_DULT_FIRMWARE_VERSION_MAJOR,
		.minor = CONFIG_BT_FAST_PAIR_FHN_DULT_FIRMWARE_VERSION_MINOR,
		.revision = CONFIG_BT_FAST_PAIR_FHN_DULT_FIRMWARE_VERSION_REVISION,
	},
};

const struct dult_user *fp_fhn_dult_integration_user_get(void)
{
	return &dult_user;
}

/* DULT delivers these from the system workqueue, so forwarding straight to the application
 * callback is safe: the application may drive the FHN/DULT lifecycle from within it.
 */
static void fhn_ownership_claimed(const struct dult_user *user, bool is_owner)
{
	ARG_UNUSED(user);

	fp_fhn_callbacks_dult_ownership_state_changed_notify(true, is_owner);
}

static void fhn_ownership_released(const struct dult_user *user, bool was_owner)
{
	ARG_UNUSED(user);

	fp_fhn_callbacks_dult_ownership_state_changed_notify(false, was_owner);
}

static const struct dult_multi_user_cb fhn_multi_user_cb = {
	.ownership_claimed = fhn_ownership_claimed,
	.ownership_released = fhn_ownership_released,
};

/* The runtime DULT association is done next to EIK commit operations. */

static int dult_init(void)
{
	static const size_t model_id_offset = sizeof(product_data) - FP_REG_DATA_MODEL_ID_LEN;
	int err;

	err = fp_reg_data_get_model_id(&product_data[model_id_offset],
				       sizeof(product_data) - model_id_offset);
	if (err) {
		LOG_ERR("FHN: fp_reg_data_get_model_id returned error: %d", err);
		return err;
	}

	err = dult_user_register(&dult_user);
	if (err) {
		LOG_ERR("FHN: dult_user_register returned error: %d", err);
		return err;
	}

	if (IS_ENABLED(CONFIG_DULT_MULTI_USER)) {
		err = dult_multi_user_cb_register(&dult_user, &fhn_multi_user_cb);
		if (err) {
			LOG_ERR("FHN: dult_multi_user_cb_register returned error: %d", err);
			return err;
		}
	}

	/* FHN does not install a dult_bt_anos_cb. The DULT built-in fallback
	 * applies: every ANOS write is rejected while FHN is not the associated
	 * user, and SEPARATED-only access is granted once FHN is associated.
	 */

	return 0;
}

static int dult_uninit(void)
{
	int err;

	if (IS_ENABLED(CONFIG_DULT_MULTI_USER)) {
		/* Full teardown so the next bring-up re-establishes state. */
		err = dult_user_unregister(&dult_user);
		if (err) {
			LOG_ERR("FHN: dult_user_unregister returned error: %d", err);
			return err;
		}
	}

	return 0;
}

FP_ACTIVATION_MODULE_REGISTER(fp_fhn_dult_integration,
			      CONFIG_BT_FAST_PAIR_FHN_DULT_INTEGRATION_INIT_PRIORITY,
			      dult_init,
			      dult_uninit);
