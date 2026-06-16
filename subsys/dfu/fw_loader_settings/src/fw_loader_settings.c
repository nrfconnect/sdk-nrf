/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <dfu/fw_loader_settings.h>
#include <zephyr/kernel.h>

#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/settings/settings.h>

#if defined(CONFIG_FW_LOADER_SETTINGS_ADV_NAME_ACCESS_PROTECT)
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#include <zephyr/mgmt/mcumgr/grp/settings_mgmt/settings_mgmt_callbacks.h>
#endif

LOG_MODULE_REGISTER(fw_loader_settings, CONFIG_FW_LOADER_SETTINGS_LOG_LEVEL);

#define FW_LOADER_SETTINGS_SUBTREE "fw_loader"
#define FW_LOADER_SETTINGS_ADV_NAME_KEY "adv_name"
#define FW_LOADER_SETTINGS_ADV_NAME_FULL \
	FW_LOADER_SETTINGS_SUBTREE "/" FW_LOADER_SETTINGS_ADV_NAME_KEY

static char adv_name[CONFIG_FW_LOADER_SETTINGS_ADV_NAME_LEN];
static bool adv_name_set;

static int fw_loader_settings_set(const char *key, size_t len, settings_read_cb read_cb,
				  void *cb_arg)
{
	ssize_t rc;

	if (strcmp(key, FW_LOADER_SETTINGS_ADV_NAME_KEY) != 0) {
		return -ENOENT;
	}

	/* Length excludes the terminating NUL, which is added below. The longest
	 * accepted name is therefore CONFIG_FW_LOADER_SETTINGS_ADV_NAME_LEN - 1.
	 */
	if (len == 0 || len >= CONFIG_FW_LOADER_SETTINGS_ADV_NAME_LEN) {
		LOG_ERR("Adv name rejected: invalid length %u (max %u)", (unsigned int)len,
			CONFIG_FW_LOADER_SETTINGS_ADV_NAME_LEN - 1);
		return -EINVAL;
	}

	rc = read_cb(cb_arg, adv_name, sizeof(adv_name) - 1);
	if (rc < 0) {
		LOG_ERR("Adv name read failed: %d", (int)rc);
		return rc;
	}

	adv_name[rc] = '\0';
	adv_name_set = true;

	return 0;
}

static int fw_loader_settings_get(const char *key, char *val, int val_len_max)
{
	if (strcmp(key, FW_LOADER_SETTINGS_ADV_NAME_KEY) != 0) {
		return -ENOENT;
	}

	if (!adv_name_set) {
		return -ENOENT;
	}

	val_len_max = MIN(val_len_max, (int)strlen(adv_name));
	memcpy(val, adv_name, val_len_max);

	return val_len_max;
}

static int fw_loader_settings_export(int (*export_func)(const char *name, const void *value,
							size_t val_len))
{
	if (!adv_name_set) {
		return 0;
	}

	return export_func(FW_LOADER_SETTINGS_ADV_NAME_FULL, adv_name, strlen(adv_name));
}

SETTINGS_STATIC_HANDLER_DEFINE(fw_loader, FW_LOADER_SETTINGS_SUBTREE, fw_loader_settings_get,
			       fw_loader_settings_set, NULL, fw_loader_settings_export);

#if defined(CONFIG_FW_LOADER_SETTINGS_ADV_NAME_ACCESS_PROTECT)
static bool fw_loader_settings_access_allowed(const struct settings_mgmt_access *access)
{
	if (!access->name) {
		return true;
	}

	switch (access->access) {
	case SETTINGS_ACCESS_READ:
	case SETTINGS_ACCESS_WRITE:
	case SETTINGS_ACCESS_DELETE:
	case SETTINGS_ACCESS_SAVE:
		return strcmp(access->name, FW_LOADER_SETTINGS_ADV_NAME_FULL) == 0;
	default:
		return false;
	}
}

static enum mgmt_cb_return fw_loader_settings_access_callback(uint32_t event,
							      enum mgmt_cb_return prev_status,
							      int32_t *rc, uint16_t *group,
							      bool *abort_more, void *data,
							      size_t data_size)
{
	struct settings_mgmt_access *access = data;

	ARG_UNUSED(event);
	ARG_UNUSED(prev_status);
	ARG_UNUSED(group);
	ARG_UNUSED(abort_more);

	if (!data || data_size != sizeof(*access) ||
	    fw_loader_settings_access_allowed(access)) {
		return MGMT_CB_OK;
	}

	*rc = MGMT_ERR_EPERUSER;

	return MGMT_CB_ERROR_RC;
}

static struct mgmt_callback settings_access_cb = {
	.callback = fw_loader_settings_access_callback,
	.event_id = MGMT_EVT_OP_SETTINGS_MGMT_ACCESS,
};
#endif

static int fw_loader_settings_backend_init(void)
{
	int err;

	err = settings_subsys_init();
	if (err != 0) {
		LOG_ERR("settings_subsys_init failed: %d", err);
		return err;
	}

#if defined(CONFIG_FW_LOADER_SETTINGS_ADV_NAME_ACCESS_PROTECT)
	mgmt_callback_register(&settings_access_cb);
#endif

	return 0;
}

SYS_INIT(fw_loader_settings_backend_init, APPLICATION, 0);

int fw_loader_settings_adv_name_load(char *buf, size_t buf_len)
{
	ssize_t len;

	if (buf_len == 0U) {
		return -EINVAL;
	}

	len = settings_load_one(FW_LOADER_SETTINGS_ADV_NAME_FULL, buf, buf_len - 1);
	if (len <= 0) {
		return -ENOENT;
	}

	buf[len] = '\0';

	return 0;
}
