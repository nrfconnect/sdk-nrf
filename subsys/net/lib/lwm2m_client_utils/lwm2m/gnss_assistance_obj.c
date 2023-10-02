/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define LOG_MODULE_NAME net_lwm2m_obj_gnss_assistance
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <string.h>
#include <zephyr/init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include <net/lwm2m_client_utils_location.h>
#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGNSS)
#include <net/nrf_cloud_agps.h>
#endif
#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS)
#include <net/nrf_cloud_pgps.h>
#endif
#include "gnss_assistance_obj.h"

#define GNSS_ASSIST_VERSION_MAJOR 1
#define GNSS_ASSIST_VERSION_MINOR 0

/* Location Assistance resource IDs */
#define GNSS_ASSIST_ASSIST_TYPE				0
#define GNSS_ASSIST_AGNSS_MASK				1
#define GNSS_ASSIST_PGPS_PRED_COUNT			2
#define GNSS_ASSIST_PGPS_PRED_INTERVAL			3
#define GNSS_ASSIST_PGPS_START_GPS_DAY			4
#define GNSS_ASSIST_PGPS_START_GPS_TIME_OF_DAY		5
#define GNSS_ASSIST_ASSIST_DATA				6
#define GNSS_ASSIST_RESULT_CODE				7
#define GNSS_ASSIST_ELEVATION_MASK			8

#define GNSS_ASSIST_MAX_ID				9

#define PREDICTION_COUNT_MIN				1
#define PREDICTION_COUNT_MAX				168

#define PREDICTION_INTERVAL_MIN				120
#define PREDICTION_INTERVAL_MAX				480

#define START_TIME_MAX					86399

static int32_t assist_type;
static uint32_t agnss_mask;
static int32_t pgps_pred_count;
static int32_t pgps_pred_interval;
static int32_t pgps_start_gps_day;
static int32_t pgps_start_gps_time_of_day;
static int32_t result;
static int32_t satellite_elevation_mask;

static uint32_t bytes_downloaded;

bool request_ongoing;

static gnss_assistance_get_result_code_cb_t result_code_cb;

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGNSS)
char *assist_buf;
#endif
static char assist_data[CONFIG_LWM2M_COAP_BLOCK_SIZE];


/*
 * Calculate resource instances as follows:
 * start with GNSS_ASSIST_MAX_ID
 */
#define RESOURCE_INSTANCE_COUNT	(GNSS_ASSIST_MAX_ID)

static struct lwm2m_engine_obj gnss_assistance;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(GNSS_ASSIST_ASSIST_TYPE, R, S32),
	OBJ_FIELD_DATA(GNSS_ASSIST_AGNSS_MASK, R_OPT, U32),
	OBJ_FIELD_DATA(GNSS_ASSIST_PGPS_PRED_COUNT, R_OPT, S32),
	OBJ_FIELD_DATA(GNSS_ASSIST_PGPS_PRED_INTERVAL, R_OPT, S32),
	OBJ_FIELD_DATA(GNSS_ASSIST_PGPS_START_GPS_DAY, R_OPT, S32),
	OBJ_FIELD_DATA(GNSS_ASSIST_PGPS_START_GPS_TIME_OF_DAY, R_OPT, S32),
	OBJ_FIELD_DATA(GNSS_ASSIST_ASSIST_DATA, W, OPAQUE),
	OBJ_FIELD_DATA(GNSS_ASSIST_RESULT_CODE, W, S32),
	OBJ_FIELD_DATA(GNSS_ASSIST_ELEVATION_MASK, R_OPT, S32)
};

static struct lwm2m_engine_obj_inst inst;
static struct lwm2m_engine_res res[GNSS_ASSIST_MAX_ID];
static struct lwm2m_engine_res_inst res_inst[RESOURCE_INSTANCE_COUNT];

static void *get_assist_buf(uint16_t obj_inst_id, uint16_t res_id,
			      uint16_t res_inst_id, size_t *data_len)
{
	*data_len = sizeof(assist_data);
	return assist_data;
}

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGNSS)
static int gnss_assist_write_agnss(uint8_t *data, uint16_t data_len, bool last_block,
				  uint32_t bytes_downloaded)
{
	int err;

	if (assist_buf == NULL) {
		return -ENOMEM;
	}

	memcpy(&assist_buf[bytes_downloaded], &data[0], data_len);

	if (last_block) {
		err = nrf_cloud_agps_process(assist_buf, bytes_downloaded + data_len);
		if (err) {
			LOG_WRN("Unable to process A-GNSS data, error: %d", err);
		} else {
			LOG_INF("A-GNSS data processed");
		}
		k_free(assist_buf);
		assist_buf = NULL;
		return err;
	}

	return 0;
}
#endif

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS)
static int gnss_assist_write_pgps(uint8_t *data, uint16_t data_len, bool last_block,
				  uint32_t bytes_downloaded)
{
	int err;

	if (bytes_downloaded == 0) {
		err = nrf_cloud_pgps_begin_update();

		if (err) {
			LOG_ERR("Unable to begin P-GPS update, error: %d", err);
			return err;
		}
	}

	err = nrf_cloud_pgps_process_update(data, data_len);

	if (err) {
		LOG_ERR("Unable to process P-GPS data, error: %d", err);
		nrf_cloud_pgps_finish_update();
		return err;
	}

	if (last_block) {
		LOG_INF("P-GPS data processed");
		err = nrf_cloud_pgps_finish_update();
	}

	return err;
}
#endif

static int gnss_assist_write_cb(uint16_t obj_inst_id, uint16_t res_id,
			    uint16_t res_inst_id, uint8_t *data, uint16_t data_len,
			    bool last_block, size_t total_size)
{
	int err = 0;

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGNSS)
	if (assist_type == ASSISTANCE_REQUEST_TYPE_AGNSS) {
		err = gnss_assist_write_agnss(data, data_len, last_block, bytes_downloaded);
	}
#endif

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS)
	if (assist_type == ASSISTANCE_REQUEST_TYPE_PGPS) {
		err = gnss_assist_write_pgps(data, data_len, last_block, bytes_downloaded);
	}
#endif

	if (last_block) {
		bytes_downloaded = 0;
		request_ongoing = false;
	} else {
		bytes_downloaded += data_len;
	}

	if (err) {
		LOG_ERR("Error writing %s assistance data (%d)",
			(assist_type == ASSISTANCE_REQUEST_TYPE_AGNSS) ? "A-GNSS" : "P-GPS", err);
	}

	return err;
}

void gnss_assistance_prepare_download(void)
{
	bytes_downloaded = 0;
	request_ongoing = true;
}

bool location_assist_gnss_is_busy(void)
{
	return request_ongoing;
}

void gnss_assistance_set_result_code_cb(gnss_assistance_get_result_code_cb_t cb)
{
	result_code_cb = cb;
}

static int gnss_assistance_result_code_cb(uint16_t obj_inst_id, uint16_t res_id,
					  uint16_t res_inst_id, uint8_t *data,
					  uint16_t data_len, bool last_block,
					  size_t total_size)
{
	int32_t resdata;

	if (data_len != sizeof(int32_t)) {
		return -EINVAL;
	}

	resdata =  *(int32_t *)data;

	if (result_code_cb) {
		result_code_cb(resdata);
	}

	if (resdata != LOCATION_ASSIST_RESULT_CODE_OK) {
		LOG_ERR("Result code %d", resdata);
	}
	return 0;
}

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGNSS)
int location_assist_agnss_alloc_buf(size_t buf_size)
{
	/* Free the memory if it wasn't unallocated in the last final package */
	if (assist_buf != NULL) {
		k_free(assist_buf);
		assist_buf = NULL;
	}

	assist_buf = k_malloc(buf_size);

	if (assist_buf == NULL) {
		return -ENOMEM;
	}

	return 0;
}

void location_assist_agnss_request_set(uint32_t request_mask)
{
	LOG_INF("Requesting A-GNSS data, mask 0x%08x", request_mask);
	/* Store mask to object resource */
	agnss_mask = request_mask;
}

void location_assist_agnss_set_elevation_mask(int32_t elevation_mask)
{
	satellite_elevation_mask = elevation_mask;
}

int32_t location_assist_agnss_get_elevation_mask(void)
{
	return satellite_elevation_mask;
}
#endif /* CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGNSS*/

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS)
int location_assist_pgps_set_prediction_count(int32_t predictions)
{
	if (predictions < PREDICTION_COUNT_MIN || predictions > PREDICTION_COUNT_MAX) {
		return -EINVAL;
	}

	pgps_pred_count = predictions;
	return 0;
}

int location_assist_pgps_set_prediction_interval(int32_t interval)
{
	if (interval < PREDICTION_INTERVAL_MIN || interval > PREDICTION_INTERVAL_MAX ||
	    interval % PREDICTION_INTERVAL_MIN != 0) {
		return -EINVAL;
	}

	pgps_pred_interval = interval;
	return 0;
}

void location_assist_pgps_set_start_gps_day(int32_t gps_day)
{
	pgps_start_gps_day = gps_day;
}

int location_assist_pgps_set_start_time(int32_t start_time)
{
	if (start_time < 0 || start_time > START_TIME_MAX) {
		return -EINVAL;
	}

	pgps_start_gps_time_of_day = start_time;
	return 0;
}

int location_assist_pgps_get_start_gps_day(void)
{
	return pgps_start_gps_day;
}
#endif

int location_assist_gnss_type_set(int assistance_type)
{
	if (assistance_type != ASSISTANCE_REQUEST_TYPE_AGNSS &&
	    assistance_type != ASSISTANCE_REQUEST_TYPE_PGPS) {
		return -EINVAL;
	}

	assist_type = assistance_type;
	return 0;
}

int location_assist_gnss_type_get(void)
{
	return assist_type;
}

int32_t location_assist_gnss_get_result_code(void)
{
	return result;
}

static struct lwm2m_engine_obj_inst *gnss_assist_create(uint16_t obj_inst_id)
{
	int i = 0, j = 0;

	init_res_instance(res_inst, ARRAY_SIZE(res_inst));
#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGNSS)
	satellite_elevation_mask = CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGNSS_ELEVATION_MASK;
#endif
#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS)
	pgps_pred_count = CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS_PREDICTION_COUNT;
	pgps_pred_interval = CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS_PREDICTION_INTERVAL;
	pgps_start_gps_day = CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS_STARTING_DAY;
	pgps_start_gps_time_of_day = CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS_STARTING_TIME;
#endif

	/* initialize instance resource data */
	INIT_OBJ_RES_DATA(GNSS_ASSIST_ASSIST_TYPE, res, i, res_inst, j,
			  &assist_type, sizeof(assist_type));
	INIT_OBJ_RES_DATA(GNSS_ASSIST_AGNSS_MASK, res, i, res_inst, j,
			  &agnss_mask, sizeof(agnss_mask));
	INIT_OBJ_RES_DATA(GNSS_ASSIST_PGPS_PRED_COUNT, res, i, res_inst, j,
			  &pgps_pred_count, sizeof(pgps_pred_count));
	INIT_OBJ_RES_DATA(GNSS_ASSIST_PGPS_PRED_INTERVAL, res, i, res_inst, j,
			  &pgps_pred_interval, sizeof(pgps_pred_interval));
	INIT_OBJ_RES_DATA(GNSS_ASSIST_PGPS_START_GPS_DAY, res, i, res_inst, j,
			  &pgps_start_gps_day, sizeof(pgps_start_gps_day));
	INIT_OBJ_RES_DATA(GNSS_ASSIST_PGPS_START_GPS_TIME_OF_DAY, res, i, res_inst, j,
			  &pgps_start_gps_time_of_day, sizeof(pgps_start_gps_time_of_day));
	INIT_OBJ_RES_OPT(GNSS_ASSIST_ASSIST_DATA, res, i, res_inst, j, 1, false, true, NULL,
			 get_assist_buf, NULL, gnss_assist_write_cb, NULL);
	INIT_OBJ_RES(GNSS_ASSIST_RESULT_CODE, res, i, res_inst, j, 1, false, true, &result,
		     sizeof(result), NULL, NULL, NULL, gnss_assistance_result_code_cb, NULL);
	INIT_OBJ_RES_DATA(GNSS_ASSIST_ELEVATION_MASK, res, i, res_inst, j,
			  &satellite_elevation_mask, sizeof(satellite_elevation_mask));

	inst.resources = res;
	inst.resource_count = i;
	LOG_DBG("Created a GNSS assistance object: %d", obj_inst_id);
	return &inst;
}

static int lwm2m_gnss_assist_init(void)
{
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	int ret = 0;

	gnss_assistance.obj_id = GNSS_ASSIST_OBJECT_ID;
	gnss_assistance.version_major = GNSS_ASSIST_VERSION_MAJOR;
	gnss_assistance.version_minor = GNSS_ASSIST_VERSION_MINOR;
	gnss_assistance.is_core = false;
	gnss_assistance.fields = fields;
	gnss_assistance.field_count = ARRAY_SIZE(fields);
	gnss_assistance.max_instance_count = 1U;
	gnss_assistance.create_cb = gnss_assist_create;
	lwm2m_register_obj(&gnss_assistance);

	ret = lwm2m_create_obj_inst(GNSS_ASSIST_OBJECT_ID, 0, &obj_inst);
	if (ret < 0) {
		LOG_ERR("Create a GNSS assistance object error: %d", ret);
	}

	return 0;
}

SYS_INIT(lwm2m_gnss_assist_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
