/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief ESL client settings module
 */

#include <stdio.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>

#include "esl_client.h"
#include "esl_client_internal.h"

LOG_MODULE_DECLARE(esl_c);
extern uint16_t esl_c_tags_per_group;
extern uint8_t groups_per_button;
int esl_c_settings_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	struct bt_esl_client *esl_c_obj = esl_c_get_esl_c_obj();

	LOG_DBG("len %d %s", len, name);
	if (!strcmp(name, "esl_ap_key")) {
		if (len != EAD_KEY_MATERIAL_LEN) {
			return -EINVAL;
		}
		if (read_cb(cb_arg, esl_c_obj->esl_ap_key.key_v, len) > 0) {
			LOG_HEXDUMP_DBG(esl_c_obj->esl_ap_key.key_v, len, "read_cb esl_ap_key");
			return 0;
		}
	}

	if (!strcmp(name, "tags_per_group")) {
		if (len != sizeof(uint16_t)) {
			return -EINVAL;
		}
		if (read_cb(cb_arg, &esl_c_tags_per_group, len) > 0) {
			LOG_HEXDUMP_DBG(&esl_c_tags_per_group, len, "read_cb tags_per_group");
			return 0;
		}
	}

	if (!strcmp(name, "groups_per_button")) {
		if (len != sizeof(uint8_t)) {
			return -EINVAL;
		}
		if (read_cb(cb_arg, &groups_per_button, len) > 0) {
			LOG_HEXDUMP_DBG(&groups_per_button, len, "read_cb groups_per_button");
			return 0;
		}
	}

	return -ENOENT;
}

struct settings_handler esl_c_settings_conf = {.name = "esl_c", .h_set = esl_c_settings_set};

int esl_c_settings_init(void)
{
	int ret;

	ret = settings_subsys_init();
	if (ret) {
		LOG_ERR("Init setting failed: %d", ret);
		return ret;
	}

	ret = settings_register(&esl_c_settings_conf);
	if (ret) {
		LOG_ERR("Register setting failed: %d", ret);
		return ret;
	}

	ret = settings_load_subtree("esl_c");
	if (ret) {
		LOG_ERR("Load setting esl_c failed: %d", ret);
	}

	return ret;
}

int esl_c_setting_ap_key_save(void)
{
	int ret;
	struct bt_esl_client *esl_c_obj = esl_c_get_esl_c_obj();

	/* Write a single serialized value to persisted storage (if it has changed value). */
	ret = settings_save_one("esl_c/esl_ap_key", esl_c_obj->esl_ap_key.key_v,
				EAD_KEY_MATERIAL_LEN);
	if (ret) {
		LOG_ERR("save esl_c/esl_ap_key failed: %d", ret);
		return ret;
	}
	return 0;
}

int esl_c_setting_tags_per_group_save(void)
{
	int ret;

	/* Write a single serialized value to persisted storage (if it has changed value). */
	ret = settings_save_one("esl_c/tags_per_group", &esl_c_tags_per_group, sizeof(uint16_t));
	if (ret) {
		LOG_ERR("save esl_c/tags_per_group failed: %d", ret);
		return ret;
	}
	return 0;
}

int esl_c_setting_group_per_button_save(void)
{
	int ret;

	/* Write a single serialized value to persisted storage (if it has changed value). */
	ret = settings_save_one("esl_c/groups_per_button", &groups_per_button, sizeof(uint8_t));
	if (ret) {
		LOG_ERR("save esl_c/tags_per_group failed: %d", ret);
		return ret;
	}

	return 0;
}

int esl_c_setting_clear_all(void)
{
	return 0;
}
