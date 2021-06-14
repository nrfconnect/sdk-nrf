/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include <zephyr.h>
#include <settings/settings.h>
#include <stdlib.h>
#include <stdio.h>

#include <net/lwm2m_client_utils_fota.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(lwm2m_fota_settings, CONFIG_LWM2M_CLIENT_UTILS_LOG_LEVEL);

#define MAX_INSTANCE_COUNT CONFIG_LWM2M_FIRMWARE_INSTANCE_COUNT

static struct update_counter uc[MAX_INSTANCE_COUNT];

static const char fctr_path[] = "fota/counter";
static const char ctr_str[] = "counter";

int fota_update_counter_read(uint16_t obj_inst_id, struct update_counter *update_counter)
{
	memcpy(update_counter, &(uc[obj_inst_id]), sizeof(uc[obj_inst_id]));
	return 0;
}

int fota_update_counter_update(uint16_t obj_inst_id, enum counter_type type, uint32_t new_value)
{
	char id[6];
	char path[sizeof(fctr_path) + 1 + 5]; /* prefix + '/' + obj insta id digits(uint16_t)*/

	snprintf(id, sizeof(id), "%hu", obj_inst_id);
	snprintf(path, sizeof(path), "%s/", fctr_path);
	strncat(&path[strlen(fctr_path)], id, strlen(id));

	if (type == COUNTER_UPDATE) {
		uc[obj_inst_id].update = new_value;
	} else {
		uc[obj_inst_id].current = new_value;
	}

	return settings_save_one(path, &uc[obj_inst_id], sizeof(uc[obj_inst_id]));
}

static int set(const char *key, size_t len_rd, settings_read_cb read_cb,
	       void *cb_arg)
{
	const char *next;

	if (!key) {
		goto inval;
	}

	settings_name_next(key, &next);

	if (strncmp(key, ctr_str, strlen(ctr_str)) == 0) {
		uint16_t id = atoi(next);

		if (id >= MAX_INSTANCE_COUNT) {
			LOG_ERR("Invalid update counter ID %hu", id);
			__ASSERT(false, "Invalid update counter ID %hu", id);
			goto inval;
		}

		int len = read_cb(cb_arg, &uc[id], sizeof(uc[id]));

		if (len < sizeof(uc[id])) {
			LOG_ERR("Unable to read update counter; resetting");
			memset(&uc[id], 0, sizeof(uc[id]));
		}

		return 0;
	}

inval:
	return -ENOENT;
}

static struct settings_handler fota_settings = {
	.name = "fota",
	.h_set = set,
};

int fota_settings_init(void)
{
	int err;

	err = settings_subsys_init();
	if (err) {
		LOG_ERR("settings_subsys_init failed (err %d)", err);
		return err;
	}

	err = settings_register(&fota_settings);
	if (err) {
		LOG_ERR("settings_register failed (err %d)", err);
		return err;
	}

	return 0;
}
