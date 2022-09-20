/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <string.h>
#include <zephyr/zephyr.h>
#include <zephyr/settings/settings.h>

#include <zephyr/net/wifi.h>
#include <zephyr/net/wifi_mgmt.h>

#include "wifi_provisioning.h"
#include "wifi_prov_internal.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(wifi_prov, CONFIG_WIFI_PROVISIONING_LOG_LEVEL);

K_MUTEX_DEFINE(prov_lock);
bool prov_state;
struct wifi_config_t config;
bool save_in_ram_only;

struct direct_immediate_value {
	size_t len;
	void *dest;
	uint8_t fetched;
};

static int direct_loader_immediate_value(const char *name, size_t len,
					 settings_read_cb read_cb, void *cb_arg,
					 void *param)
{
	const char *next;
	size_t name_len;
	int rc;
	struct direct_immediate_value *one_value =
					(struct direct_immediate_value *)param;

	name_len = settings_name_next(name, &next);

	if (name_len == 0) {
		if (len == one_value->len) {
			rc = read_cb(cb_arg, one_value->dest, len);
			if (rc >= 0) {
				one_value->fetched = 1;
				LOG_DBG("immediate load: OK.");
				return 0;
			}

			LOG_DBG("fail (err %d).", rc);
			return rc;
		}
		return -EINVAL;
	}

	/* other keys aren't served by the callback
	 * Return success in order to skip them
	 * and keep storage processing.
	 */
	return 0;
}

int load_immediate_value(const char *name, void *dest, size_t len)
{
	int rc;
	struct direct_immediate_value dov;

	dov.fetched = 0;
	dov.len = len;
	dov.dest = dest;

	rc = settings_load_subtree_direct(name, direct_loader_immediate_value,
					  (void *)&dov);
	if (rc == 0) {
		if (!dov.fetched) {
			rc = -ENOENT;
		}
	}

	return rc;
}

int wifi_config_save(void *buf, uint16_t size)
{
	int rc;

	rc = settings_save_one("wifi_prov/wifi_config", buf, size);
	if (rc != 0) {
		LOG_WRN("Error! Cannot write condig.");
		return -EIO;
	}
	return 0;
}

int wifi_config_read(void *buf, uint16_t size)
{
	int rc;

	rc = load_immediate_value("wifi_prov/wifi_config", buf, size);
	if (rc != 0) {
		LOG_WRN("Error! Cannot read config.");
		return -EIO;
	}
	return 0;
}

int wifi_config_delete(void)
{
	int rc;

	rc = settings_delete("wifi_prov/wifi_config");
	if (rc != 0) {
		LOG_WRN("Error! Cannot delete config.");
		return -EIO;
	}
	return 0;
}

int wifi_settings_init(void)
{
	int err;

	err = settings_subsys_init();
	if (err) {
		LOG_WRN("settings_subsys_init failed (err %d).", err);
		return err;
	}
	settings_load();
	if (err) {
		LOG_WRN("settings_load failed (err %d).", err);
		return err;
	}

	return 0;
}

int wifi_has_config(void)
{
	bool has_config = false;

	if (k_mutex_lock(&prov_lock, K_NO_WAIT) == 0) {
		has_config = prov_state;
		k_mutex_unlock(&prov_lock);
	} else {
		LOG_WRN("Cannot get prov state.");
		return -EBUSY;
	}
	return has_config;
}

int wifi_get_config(struct wifi_config_t *config_to_copy)
{
	if (k_mutex_lock(&prov_lock, K_NO_WAIT) == 0) {
		memcpy(config_to_copy, &config, sizeof(struct wifi_config_t));
		k_mutex_unlock(&prov_lock);
	} else {
		LOG_WRN("Cannot get prov config.");
		return -EBUSY;
	}
	return 0;
}

int wifi_set_config(struct wifi_config_t *config_to_copy, bool ram_only_flag)
{
	if (k_mutex_lock(&prov_lock, K_NO_WAIT) == 0) {
		save_in_ram_only = ram_only_flag;
		memcpy(&config, config_to_copy, sizeof(struct wifi_config_t));
		k_mutex_unlock(&prov_lock);
	} else {
		LOG_WRN("Cannot set prov config.");
		return -EBUSY;
	}
	return 0;
}

int wifi_commit_config(void)
{
	if (k_mutex_lock(&prov_lock, K_NO_WAIT) == 0) {
		prov_state = true;

#ifdef CONFIG_WIFI_PROVISIONING_NVM_SETTINGS
		if (save_in_ram_only == false) {
			wifi_config_save(&config, sizeof(struct wifi_config_t));
		}
#endif

		k_mutex_unlock(&prov_lock);
	} else {
		LOG_WRN("Cannot commit prov config.");
		return -EBUSY;
	}
	return 0;
}
int wifi_remove_config(void)
{
	if (k_mutex_lock(&prov_lock, K_NO_WAIT) == 0) {
		prov_state = false;
#ifdef CONFIG_WIFI_PROVISIONING_NVM_SETTINGS
		wifi_config_delete();
#endif
		k_mutex_unlock(&prov_lock);
	} else {
		LOG_WRN("Cannot remove prov config.");
		return -EBUSY;
	}
	return 0;
}

int wifi_config_init(void)
{
	if (k_mutex_lock(&prov_lock, K_NO_WAIT) == 0) {
		prov_state = false;
		save_in_ram_only = false;
#ifdef CONFIG_WIFI_PROVISIONING_NVM_SETTINGS
		wifi_settings_init();
		if (wifi_config_read(&config, sizeof(struct wifi_config_t)) == 0) {
			prov_state = true;
		}
#endif
		k_mutex_unlock(&prov_lock);
	} else {
		LOG_WRN("Cannot initialize prov config.");
		return -EBUSY;
	}
	return 0;
}
