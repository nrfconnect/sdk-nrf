/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/net/lwm2m.h>
#include <zephyr/logging/log.h>
#include <lwm2m_engine.h>
#include <lwm2m_obj_binaryappdata.h>
#include <net/lwm2m_binaryappdata.h>

LOG_MODULE_REGISTER(lwm2m_binaryappdata, CONFIG_LWM2M_CLIENT_UTILS_LOG_LEVEL);

#define MAX_INSTANCE      CONFIG_LWM2M_BINARYAPPDATA_INSTANCE_COUNT
#define MAX_DATA_INSTANCE CONFIG_LWM2M_BINARYAPPDATA_DATA_INSTANCE_COUNT

#define BINARYAPPDATA_DATA_SIZE      CONFIG_LWM2M_BINARYAPPDATA_DATA_SIZE
#define BINARYAPPDATA_DESCRIPION_LEN 32
#define BINARYAPPDATA_FORMAT_LEN     32

static lwm2m_binaryappdata_recv_cb data_recv_cb;

static uint8_t data[MAX_INSTANCE][MAX_DATA_INSTANCE][BINARYAPPDATA_DATA_SIZE];
static uint8_t priority[MAX_INSTANCE];
static time_t creation_time[MAX_INSTANCE];
static uint8_t description[MAX_INSTANCE][BINARYAPPDATA_DESCRIPION_LEN + 1];
static uint8_t format[MAX_INSTANCE][BINARYAPPDATA_FORMAT_LEN + 1];
static uint16_t app_id[MAX_INSTANCE];

static int data_write_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
			 uint8_t *data, uint16_t data_len, bool last_block,
			 size_t total_size, size_t offset)
{
	int err = 0;

	if (data_recv_cb) {
		err = data_recv_cb(obj_inst_id, res_inst_id, data, data_len);
	}

	return err;
}

int lwm2m_binaryappdata_read(uint16_t obj_inst_id, uint16_t res_inst_id, void **data_ptr,
			     uint16_t *data_len)
{
	return lwm2m_get_res_buf(&LWM2M_OBJ(19, obj_inst_id, 0, res_inst_id),
				 data_ptr, NULL, data_len, NULL);
}

int lwm2m_binaryappdata_write(uint16_t obj_inst_id, uint16_t res_inst_id, const void *data_ptr,
			      uint16_t data_len)
{
	return lwm2m_set_opaque(&LWM2M_OBJ(19, obj_inst_id, 0, res_inst_id), data_ptr, data_len);
}

int lwm2m_binaryappdata_set_recv_cb(lwm2m_binaryappdata_recv_cb recv_cb)
{
	data_recv_cb = recv_cb;

	return 0;
}

static int lwm2m_init_binaryappdata_cb(void)
{
	for (int obj_inst_id = 0; obj_inst_id < MAX_INSTANCE; obj_inst_id++) {
		lwm2m_create_object_inst(&LWM2M_OBJ(19, obj_inst_id));

		for (int res_inst_id = 0; res_inst_id < MAX_DATA_INSTANCE; res_inst_id++) {
			struct lwm2m_obj_path path = LWM2M_OBJ(19, obj_inst_id, 0, res_inst_id);

			lwm2m_set_res_buf(&path, &data[obj_inst_id][res_inst_id],
					  sizeof(data[obj_inst_id][res_inst_id]), 0, 0);
			lwm2m_register_post_write_callback(&path, data_write_cb);
		}

		lwm2m_set_res_buf(&LWM2M_OBJ(19, obj_inst_id, 1), &priority[obj_inst_id],
				  sizeof(priority[0]), sizeof(priority[0]), 0);
		lwm2m_set_res_buf(&LWM2M_OBJ(19, obj_inst_id, 2), &creation_time[obj_inst_id],
				  sizeof(creation_time[0]), sizeof(creation_time[0]), 0);
		lwm2m_set_res_buf(&LWM2M_OBJ(19, obj_inst_id, 3), description[obj_inst_id],
				  sizeof(description[0]), 0, 0);
		lwm2m_set_res_buf(&LWM2M_OBJ(19, obj_inst_id, 4), format[obj_inst_id],
				  sizeof(format[0]), 0, 0);
		lwm2m_set_res_buf(&LWM2M_OBJ(19, obj_inst_id, 5), &app_id[obj_inst_id],
				  sizeof(app_id[0]), sizeof(app_id[0]), 0);
	}

	return 0;
}

LWM2M_APP_INIT(lwm2m_init_binaryappdata_cb);
