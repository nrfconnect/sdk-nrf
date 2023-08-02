/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief ESL settings module
 */

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/init.h>
#include <zephyr/settings/settings.h>

#include "esl.h"
#include "esl_settings.h"
#include "esl_internal.h"

LOG_MODULE_REGISTER(ESL_settings, CONFIG_BT_ESL_LOG_LEVEL);

static int esl_settings_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	struct bt_esls *esl_obj = esl_get_esl_obj();

	LOG_DBG("len %d %s", len, name);
	if (!strcmp(name, "esl_addr")) {
		if (len != sizeof(uint16_t)) {
			return -EINVAL;
		}

		if (read_cb(cb_arg, &esl_obj->esl_chrc.esl_addr, len) > 0) {
			atomic_set_bit(&esl_obj->configuring_state, ESL_ADDR_SET_BIT);
			LOG_HEXDUMP_DBG(&esl_obj->esl_chrc.esl_addr, len, "read_cb esl_addr");
			return 0;
		}
	} else if (!strcmp(name, "esl_ap_key")) {
		if (len != EAD_KEY_MATERIAL_LEN) {
			return -EINVAL;
		}

		if (read_cb(cb_arg, esl_obj->esl_chrc.esl_ap_key.key_v, len) > 0) {
			atomic_set_bit(&esl_obj->configuring_state, ESL_AP_SYNC_KEY_SET_BIT);
			LOG_HEXDUMP_DBG(esl_obj->esl_chrc.esl_ap_key.key_v, len,
					"read_cb esl_ap_key");
			return 0;
		}
	} else if (!strcmp(name, "esl_rsp_key")) {
		if (len != EAD_KEY_MATERIAL_LEN) {
			return -EINVAL;
		}

		if (read_cb(cb_arg, esl_obj->esl_chrc.esl_rsp_key.key_v, len) > 0) {
			atomic_set_bit(&esl_obj->configuring_state, ESL_RESPONSE_KEY_SET_BIT);
			LOG_HEXDUMP_DBG(esl_obj->esl_chrc.esl_rsp_key.key_v, len,
					"read_cb esl_rsp_key");
			return 0;
		}
	} else if (!strcmp(name, "esl_abs_time")) {
		if (len != sizeof(uint8_t)) {
			return -EINVAL;
		}

		atomic_set_bit(&esl_obj->configuring_state, ESL_ABS_TIME_SET_BIT);
		LOG_DBG("ABS been set in prevoius configuring");
		return 0;
	}

	return -ENOENT;
}

struct settings_handler esl_settings_conf = {.name = "esl", .h_set = esl_settings_set};
int esl_settings_init(void)
{
	int ret;

	ret = settings_subsys_init();
	if (ret) {
		LOG_ERR("Init setting failed: %d", ret);
		return ret;
	}

	ret = settings_register(&esl_settings_conf);
	if (ret) {
		LOG_ERR("Register setting failed: %d", ret);
		return ret;
	}

	ret = settings_load_subtree("esl");
	if (ret) {
		LOG_ERR("Load setting failed: %d", ret);
	}

	return ret;
}

int esl_settings_addr_save(void)
{
	struct bt_esls *esl_obj = esl_get_esl_obj();
	int ret;

	/* Write a single serialized value to persisted storage (if it has changed value). */
	LOG_DBG("esl/esl_addr, %x", esl_obj->esl_chrc.esl_addr);
	ret = settings_save_one("esl/esl_addr", &esl_obj->esl_chrc.esl_addr, sizeof(uint16_t));
	if (ret) {
		LOG_ERR("save esl/esl_addr failed: %d", ret);
		return ret;
	}

	return 0;
}

int esl_settings_ap_key_save(void)
{
	struct bt_esls *esl_obj = esl_get_esl_obj();
	int ret;

	/* Write a single serialized value to persisted storage (if it has changed value). */
	LOG_HEXDUMP_DBG(esl_obj->esl_chrc.esl_ap_key.key_v, EAD_KEY_MATERIAL_LEN, "esl/esl_ap_key");
	ret = settings_save_one("esl/esl_ap_key", esl_obj->esl_chrc.esl_ap_key.key_v,
				EAD_KEY_MATERIAL_LEN);
	if (ret) {
		LOG_ERR("save esl/esl_ap_key failed: %d", ret);
		return ret;
	}

	return 0;
}

int esl_settings_rsp_key_save(void)
{
	struct bt_esls *esl_obj = esl_get_esl_obj();
	int ret;

	/* Write a single serialized value to persisted storage (if it has changed value). */
	LOG_HEXDUMP_DBG(esl_obj->esl_chrc.esl_rsp_key.key_v, EAD_KEY_MATERIAL_LEN,
			"esl/esl_rsp_key");
	ret = settings_save_one("esl/esl_rsp_key", esl_obj->esl_chrc.esl_rsp_key.key_v,
				EAD_KEY_MATERIAL_LEN);
	if (ret) {
		LOG_ERR("save esl/esl_rsp_key failed: %d", ret);
		return ret;
	}

	return 0;
}

int esl_settings_abs_save(void)
{
	int ret;
	const uint8_t value = 1;

	/* Write a single serialized value to persisted storage (if it has changed value). */
	ret = settings_save_one("esl/esl_abs_time", &value, sizeof(uint8_t));
	if (ret) {
		LOG_ERR("save esl/esl_abs_time failed: %d", ret);
		return ret;
	}

	return 0;
}

int esl_settings_delete_provisioned_data(void)
{
	struct bt_esls *esl_obj = esl_get_esl_obj();
	int ret;

	/* Delete stored esl_addr. */
	ret = settings_delete("esl/esl_addr");
	if (ret) {
		LOG_ERR("delete esl/esl_addr failed: %d", ret);
	}

	atomic_clear_bit(&esl_obj->configuring_state, ESL_ADDR_SET_BIT);

	/* Delete stored esl_ap_key. */
	ret = settings_delete("esl/esl_ap_key");
	if (ret) {
		LOG_ERR("delete esl/esl_ap_key failed: %d", ret);
	}

	atomic_clear_bit(&esl_obj->configuring_state, ESL_AP_SYNC_KEY_SET_BIT);

	/* Delete stored esl_rsp_key. */
	ret = settings_delete("esl/esl_rsp_key");
	if (ret) {
		LOG_ERR("delete esl/esl_rsp_key failed: %d", ret);
	}

	atomic_clear_bit(&esl_obj->configuring_state, ESL_RESPONSE_KEY_SET_BIT);

	/* Delete stored abs set flag. */
	ret = settings_delete("esl/esl_abs_time");
	if (ret) {
		LOG_ERR("delete esl/esl_abs_time failed: %d", ret);
	}

	atomic_clear_bit(&esl_obj->configuring_state, ESL_ABS_TIME_SET_BIT);

	/* Reset Update Complete flag*/
	atomic_clear_bit(&esl_obj->configuring_state, ESL_UPDATE_COMPLETE_SET_BIT);
	ret = settings_commit();

	return ret;
}
