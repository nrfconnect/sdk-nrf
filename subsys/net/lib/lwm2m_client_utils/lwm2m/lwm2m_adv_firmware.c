/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define LOG_MODULE_NAME lwm2m_adv_firmware
#define LOG_LEVEL CONFIG_LWM2M_CLIENT_UTILS_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <zephyr/init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include <net/lwm2m_client_utils.h>

#define FIRMWARE_VERSION_MAJOR 1
#define FIRMWARE_VERSION_MINOR 0

#define MAX_INSTANCE_COUNT CONFIG_LWM2M_CLIENT_UTILS_ADV_FOTA_INSTANCE_COUNT

/* Firmware resource IDs */
#define FIRMWARE_PACKAGE_ID			0
#define FIRMWARE_PACKAGE_URI_ID			1
#define FIRMWARE_UPDATE_ID			2
#define FIRMWARE_STATE_ID			3
#define FIRMWARE_UPDATE_RESULT_ID		5
#define FIRMWARE_PACKAGE_NAME_ID		6	/* unused */
#define FIRMWARE_PACKAGE_VERSION_ID		7	/* unused */
#define FIRMWARE_UPDATE_PROTO_SUPPORT_ID	8	/* unused */
#define FIRMWARE_UPDATE_DELIV_METHOD_ID		9
#define FIRMWARE_CANCEL_ID                      10
#define FIRMWARE_SEVERITY_ID                    11	/* unused */
#define FIRMWARE_LAST_STATE_CHANGE_TIME_ID      12
#define FIRMWARE_MAXIMUM_DEFERRED_PERIOD_ID     13	/* unused */
#define FIRMWARE_COMPONENT_NAME_ID              14
#define FIRMWARE_CURRENT_VERSION_ID             15
#define FIRMWARE_LINKED_INSTANCES_ID            16
#define FIRMWARE_CONFLICTING_INSTANCES_ID       17	/* unused */

#define FIRMWARE_MAX_ID				17
#define LINKED_INSTANCES_LEN			(MAX_INSTANCE_COUNT - 1)
#define PACKAGE_URI_LEN				255

#define DELIVERY_METHOD_PULL_ONLY		0
#define DELIVERY_METHOD_PUSH_ONLY		1
#define DELIVERY_METHOD_BOTH			2


/*
 * Calculate resource instances as follows:
 * start with FIRMWARE_MAX_ID
 * subtract EXEC resource and multi-resources then add how many multi-resources we need
 */
#define RESOURCE_INSTANCE_COUNT	(FIRMWARE_MAX_ID - 3 + 2 * LINKED_INSTANCES_LEN)

/* resource state variables */
static uint8_t update_state[MAX_INSTANCE_COUNT];
static uint8_t update_result[MAX_INSTANCE_COUNT];
static uint8_t delivery_method[MAX_INSTANCE_COUNT];
static time_t last_change[MAX_INSTANCE_COUNT];
static char package_uri[MAX_INSTANCE_COUNT][PACKAGE_URI_LEN];
static char component_version[MAX_INSTANCE_COUNT][PACKAGE_URI_LEN];
static struct lwm2m_objlnk linked_instances[MAX_INSTANCE_COUNT * LINKED_INSTANCES_LEN];
static struct lwm2m_objlnk conflict_instances[MAX_INSTANCE_COUNT * LINKED_INSTANCES_LEN];

/* A varying number of firmware object instances exists */
static struct lwm2m_engine_obj firmware;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD(FIRMWARE_PACKAGE_ID, W, OPAQUE),
	OBJ_FIELD(FIRMWARE_PACKAGE_URI_ID, RW, STRING),
	OBJ_FIELD_EXECUTE(FIRMWARE_UPDATE_ID),
	OBJ_FIELD(FIRMWARE_STATE_ID, R, U8),
	OBJ_FIELD(FIRMWARE_UPDATE_RESULT_ID, R, U8),
	OBJ_FIELD(FIRMWARE_PACKAGE_NAME_ID, R_OPT, STRING),
	OBJ_FIELD(FIRMWARE_PACKAGE_VERSION_ID, R_OPT, STRING),
	OBJ_FIELD(FIRMWARE_UPDATE_PROTO_SUPPORT_ID, R_OPT, U8),
	OBJ_FIELD(FIRMWARE_UPDATE_DELIV_METHOD_ID, R, U8),
	OBJ_FIELD_EXECUTE(FIRMWARE_CANCEL_ID),
	OBJ_FIELD(FIRMWARE_LAST_STATE_CHANGE_TIME_ID, R, TIME),
	OBJ_FIELD(FIRMWARE_COMPONENT_NAME_ID, R, STRING),
	OBJ_FIELD(FIRMWARE_CURRENT_VERSION_ID, R, STRING),
	OBJ_FIELD(FIRMWARE_LINKED_INSTANCES_ID, R_OPT, OBJLNK),
	OBJ_FIELD(FIRMWARE_CONFLICTING_INSTANCES_ID, R_OPT, OBJLNK),
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][FIRMWARE_MAX_ID];
static struct lwm2m_engine_res_inst res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];
static lwm2m_engine_set_data_cb_t write_cb[MAX_INSTANCE_COUNT];
static lwm2m_engine_execute_cb_t update_cb[MAX_INSTANCE_COUNT];
static int next_instance;

uint8_t lwm2m_adv_firmware_get_update_state(uint16_t obj_inst_id)
{
	return update_state[obj_inst_id];
}

void lwm2m_adv_firmware_set_update_state(uint16_t obj_inst_id, uint8_t state)
{
	int ret;
	bool error = false;
	struct lwm2m_obj_path path;
	time_t now;

	if (update_state[obj_inst_id] == state) {
		LOG_DBG("Already at state %d", state);
		return;
	}
	lwm2m_registry_lock();

	path = LWM2M_OBJ(LWM2M_OBJECT_ADV_FIRMWARE_ID, obj_inst_id, FIRMWARE_UPDATE_RESULT_ID);

	/* Check LWM2M SPEC appendix E.6.1 */
	switch (state) {
	case STATE_DOWNLOADING:
		if (update_state[obj_inst_id] == STATE_IDLE) {
			lwm2m_set_u8(&path, RESULT_DEFAULT);
		} else {
			error = true;
		}
		break;
	case STATE_DOWNLOADED:
		if (update_state[obj_inst_id] == STATE_DOWNLOADING) {
			lwm2m_set_u8(&path, RESULT_DEFAULT);
		} else if (update_state[obj_inst_id] == STATE_UPDATING) {
			lwm2m_set_u8(&path, RESULT_UPDATE_FAILED);
		} else {
			error = true;
		}
		break;
	case STATE_UPDATING:
		if (update_state[obj_inst_id] != STATE_DOWNLOADED) {
			error = true;
		}
		break;
	case STATE_IDLE:
		if (update_state[obj_inst_id] == STATE_DOWNLOADING) {
			lwm2m_set_u8(&path, 10);
		}
		break;
	default:
		LOG_ERR("Unhandled state: %u", state);
		lwm2m_registry_unlock();
		return;
	}

	if (error) {
		LOG_ERR("Invalid state transition: %u -> %u", update_state[obj_inst_id], state);
	}

	path.res_id = FIRMWARE_STATE_ID;
	lwm2m_set_u8(&path, state);

	LOG_DBG("Update %d/%d/%d state = %d", path.obj_id, path.obj_inst_id, path.res_id, state);
	now = time(NULL);
	path.res_id = FIRMWARE_LAST_STATE_CHANGE_TIME_ID;
	ret = lwm2m_set_time(&path, now);
	if (ret) {
		LOG_ERR("Failed to set timestamp");
	}
	lwm2m_registry_unlock();
}

uint8_t lwm2m_adv_firmware_get_update_result(uint16_t obj_inst_id)
{
	return update_result[obj_inst_id];
}

void lwm2m_adv_firmware_set_update_result(uint16_t obj_inst_id, uint8_t result)
{
	uint8_t state;
	bool error = false;
	struct lwm2m_obj_path path;
	lwm2m_registry_lock();

	switch (result) {
	case RESULT_DEFAULT:
		/* Usually this is cancell operation */
		if (update_state[obj_inst_id] != STATE_IDLE) {
			result = RESULT_ADV_FOTA_CANCELLED;
		}

		lwm2m_adv_firmware_set_update_state(obj_inst_id, STATE_IDLE);
		break;
	case RESULT_SUCCESS:
		if (update_state[obj_inst_id] != STATE_UPDATING) {
			error = true;
			state = update_state[obj_inst_id];
		}

		lwm2m_adv_firmware_set_update_state(obj_inst_id, STATE_IDLE);
		break;
	case RESULT_NO_STORAGE:
	case RESULT_OUT_OF_MEM:
	case RESULT_CONNECTION_LOST:
	case RESULT_UNSUP_FW:
	case RESULT_INVALID_URI:
	case RESULT_UNSUP_PROTO:
		if (update_state[obj_inst_id] != STATE_DOWNLOADING) {
			error = true;
			state = update_state[obj_inst_id];
		}

		lwm2m_adv_firmware_set_update_state(obj_inst_id, STATE_IDLE);
		break;
	case RESULT_INTEGRITY_FAILED:
		if (update_state[obj_inst_id] != STATE_DOWNLOADING &&
		    update_state[obj_inst_id] != STATE_UPDATING) {
			error = true;
			state = update_state[obj_inst_id];
		}

		lwm2m_adv_firmware_set_update_state(obj_inst_id, STATE_IDLE);
		break;
	case RESULT_UPDATE_FAILED:
		if (update_state[obj_inst_id] != STATE_DOWNLOADING &&
		    update_state[obj_inst_id] != STATE_UPDATING) {
			error = true;
			state = update_state[obj_inst_id];
		}

		lwm2m_adv_firmware_set_update_state(obj_inst_id, STATE_IDLE);
		break;

	case RESULT_ADV_CONFLICT_STATE:
		lwm2m_adv_firmware_set_update_state(obj_inst_id, STATE_IDLE);
		break;
	default:
		LOG_ERR("Unhandled result: %u", result);
		lwm2m_registry_unlock();
		return;
	}

	if (error) {
		LOG_ERR("Unexpected result(%u) set while state is %u", result, state);
	}

	path = LWM2M_OBJ(LWM2M_OBJECT_ADV_FIRMWARE_ID, obj_inst_id, FIRMWARE_UPDATE_RESULT_ID);
	lwm2m_set_u8(&path, result);
	lwm2m_registry_unlock();

	LOG_DBG("Update result = %d", result);
}

static int package_write_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
			    uint8_t *data, uint16_t data_len, bool last_block, size_t total_size)
{
	uint8_t state;
	int ret = 0;
	lwm2m_engine_set_data_cb_t callback;

	state = lwm2m_adv_firmware_get_update_state(obj_inst_id);
	if (data_len == 0U || (data_len == 1U && data[0] == '\0')) {
		if (state == STATE_UPDATING) {
			/* Cancel at Updating state is not accepted */
			LOG_WRN("Download has already completed");
			return -EPERM;
		}

		lwm2m_adv_firmware_set_update_result(obj_inst_id, RESULT_DEFAULT);
		return 0;

	} else if (state == STATE_IDLE) {
		lwm2m_adv_firmware_set_update_state(obj_inst_id, STATE_DOWNLOADING);
	} else if (state != STATE_DOWNLOADING) {
		LOG_WRN("Cannot download: state = %d", state);
		return -EPERM;
	}

	callback = lwm2m_adv_firmware_get_write_cb(obj_inst_id);
	if (callback) {
		ret = callback(obj_inst_id, res_id, res_inst_id, data, data_len, last_block,
			       total_size);
	}

	if (ret >= 0) {
		if (last_block) {
			lwm2m_adv_firmware_set_update_state(obj_inst_id, STATE_DOWNLOADED);
		}

		return 0;
	} else if (ret == -ENOMEM) {
		lwm2m_adv_firmware_set_update_result(obj_inst_id, RESULT_OUT_OF_MEM);
	} else if (ret == -ENOSPC) {
		lwm2m_adv_firmware_set_update_result(obj_inst_id, RESULT_NO_STORAGE);
		/* Response 4.13 (RFC7959, section 2.9.3) */
		/* TODO: should include size1 option to indicate max size */
		ret = -EFBIG;
	} else if (ret == -EFAULT) {
		lwm2m_adv_firmware_set_update_result(obj_inst_id, RESULT_INTEGRITY_FAILED);
	} else if (ret == -ENOMSG) {
		lwm2m_adv_firmware_set_update_result(obj_inst_id, RESULT_UNSUP_FW);
	} else if (ret == -EAGAIN) {
		if (state == STATE_IDLE) {
			/* Mark conflict state  */
			lwm2m_adv_firmware_set_update_result(obj_inst_id,
							     RESULT_ADV_CONFLICT_STATE);
		}
	} else {
		lwm2m_adv_firmware_set_update_result(obj_inst_id, RESULT_UPDATE_FAILED);
	}

	return ret;
}

lwm2m_engine_set_data_cb_t lwm2m_adv_firmware_get_write_cb(uint16_t obj_inst_id)
{
	return write_cb[obj_inst_id];
}

lwm2m_engine_execute_cb_t lwm2m_adv_firmware_get_update_cb(uint16_t obj_inst_id)
{
	return update_cb[obj_inst_id];
}

static bool update_object_link_parse(char *data_ptr, uint16_t obj_inst_id,
				     struct lwm2m_objlnk *value)
{
	unsigned long id;
	char *end;
	char *idp = data_ptr;

	for (int idx = 0; idx < 2; idx++) {
		errno = 0;
		id = strtoul(idp, &end, 10);

		idp = end + 1;

		if ((id == 0 && errno == ERANGE) || id > 65535) {
			LOG_WRN("decoded id %lu out of range[0..65535]", id);
			return false;
		}

		switch (idx) {
		case 0:
			value->obj_id = id;
			LOG_DBG("Object id %d", value->obj_id);
			continue;
		case 1:
			value->obj_inst = id;
			LOG_DBG("Object instance %d", value->obj_inst);
			continue;
		}
	}
	if (value->obj_id != LWM2M_OBJECT_ADV_FIRMWARE_ID || value->obj_inst == obj_inst_id ||
	    value->obj_inst >= next_instance) {
		return false;
	}

	return true;
}

static int update_arguments_parse(uint16_t obj_inst_id, char *args, uint16_t args_len)
{
	struct lwm2m_objlnk object_link;
	char *temp = args;
	char *s_ptr;
	char *e_ptr;
	uint8_t update_parameter;
	int n_linked = 0;
	int n_conflicts = 0;

	/* Remove existing object links as this is a new update */
	for (int i = 0; i < LINKED_INSTANCES_LEN; ++i) {
		lwm2m_delete_res_inst(&LWM2M_OBJ(LWM2M_OBJECT_ADV_FIRMWARE_ID, obj_inst_id,
						  FIRMWARE_CONFLICTING_INSTANCES_ID, i));
		lwm2m_delete_res_inst(&LWM2M_OBJ(LWM2M_OBJECT_ADV_FIRMWARE_ID, obj_inst_id,
						  FIRMWARE_LINKED_INSTANCES_ID, i));
	}

	if (!args || !args_len) {
		return 0;
	}

	/* Parse possible parameters */
	if (args && args_len) {
		update_parameter = *args++;
		args_len--;
		if (update_parameter != '0') {
			LOG_ERR("Update parameter not supported %d", update_parameter);
			return -1;
		}

		while (args_len) {
			/* Discover Object link start and end */
			s_ptr = strchr(temp, '<');
			e_ptr = strchr(temp, '>');
			if (s_ptr && e_ptr) {
				args_len -= e_ptr - args;
				temp = e_ptr + 1;
				s_ptr++;
				if (*s_ptr == '/') {
					s_ptr++;
				}

				if (!update_object_link_parse(s_ptr, obj_inst_id, &object_link)) {
					return -1;
				}

				if (lwm2m_adv_firmware_get_update_state(object_link.obj_inst) !=
				    STATE_DOWNLOADED) {
					struct lwm2m_obj_path path =
						LWM2M_OBJ(LWM2M_OBJECT_ADV_FIRMWARE_ID, obj_inst_id,
							  FIRMWARE_CONFLICTING_INSTANCES_ID,
							  n_conflicts++);
					lwm2m_create_res_inst(&path);
					lwm2m_set_objlnk(&path, &object_link);
					return -1;
				}
				struct lwm2m_obj_path path =
					LWM2M_OBJ(LWM2M_OBJECT_ADV_FIRMWARE_ID, obj_inst_id,
						  FIRMWARE_LINKED_INSTANCES_ID, n_linked++);
				lwm2m_create_res_inst(&path);
				lwm2m_set_objlnk(&path, &object_link);
			} else {
				/* No data with Parameter */
				args_len = 0;
			}
		}
	}

	return 0;
}

static int firmware_update_cb(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len)
{
	lwm2m_engine_execute_cb_t callback;
	uint8_t state;
	int ret;

	state = lwm2m_adv_firmware_get_update_state(obj_inst_id);
	if (state != STATE_DOWNLOADED) {
		LOG_ERR("State other than downloaded: %d", state);
		return -EPERM;
	}

	if (update_arguments_parse(obj_inst_id, args, args_len)) {
		return -ECANCELED;
	}

	lwm2m_adv_firmware_set_update_state(obj_inst_id, STATE_UPDATING);

	callback = lwm2m_adv_firmware_get_update_cb(obj_inst_id);
	if (callback) {
		ret = callback(obj_inst_id, args, args_len);
		if (ret < 0) {
			LOG_ERR("Failed to update firmware: %d", ret);
			lwm2m_adv_firmware_set_update_result(
				obj_inst_id,
				ret == -EINVAL ? RESULT_INTEGRITY_FAILED : RESULT_UPDATE_FAILED);
			return 0;
		}
	}

		return 0;
}

static int cancel(uint16_t id, uint8_t *args, uint16_t args_len)
{
	uint8_t state;

	LOG_DBG("Cancelling Advanced FOTA id %d", id);
	state = lwm2m_adv_firmware_get_update_state(id);
	if (state != STATE_DOWNLOADED && state != STATE_DOWNLOADING) {
		LOG_ERR("State other than downloaded or downloading: %d", state);
		return -EPERM;
	}

	/* Writing empty URI will trigger cancelling */
	lwm2m_set_opaque(&LWM2M_OBJ(LWM2M_OBJECT_ADV_FIRMWARE_ID, id, FIRMWARE_PACKAGE_URI_ID),
			 NULL, 0);
	return 0;
}

static struct lwm2m_engine_obj_inst *firmware_create(uint16_t obj_inst_id)
{
	int i = 0, j = 0;

	/* Secure outside called lwm2m_create_obj_inst() create is still possible */
	if (obj_inst_id >= MAX_INSTANCE_COUNT) {
		return NULL;
	}

	init_res_instance(res_inst[obj_inst_id], ARRAY_SIZE(res_inst[obj_inst_id]));

	/* initialize instance resource data */
	INIT_OBJ_RES_OPT(FIRMWARE_PACKAGE_ID, res[obj_inst_id], i, res_inst[obj_inst_id], j, 1,
			 false, true, NULL, NULL, NULL, package_write_cb, NULL);
	INIT_OBJ_RES_LEN(FIRMWARE_PACKAGE_URI_ID, res[obj_inst_id], i, res_inst[obj_inst_id], j, 1,
			 false, true, package_uri[obj_inst_id], PACKAGE_URI_LEN, 0, NULL, NULL,
			 NULL, NULL, NULL);
	INIT_OBJ_RES_EXECUTE(FIRMWARE_UPDATE_ID, res[obj_inst_id], i, firmware_update_cb);
	INIT_OBJ_RES_DATA(FIRMWARE_STATE_ID, res[obj_inst_id], i, res_inst[obj_inst_id], j,
			  &(update_state[obj_inst_id]), sizeof(update_state[obj_inst_id]));
	INIT_OBJ_RES_DATA(FIRMWARE_UPDATE_RESULT_ID, res[obj_inst_id], i, res_inst[obj_inst_id], j,
			  &(update_result[obj_inst_id]), sizeof(update_result[obj_inst_id]));
	INIT_OBJ_RES_OPTDATA(FIRMWARE_PACKAGE_NAME_ID, res[obj_inst_id], i, res_inst[obj_inst_id],
			     j);
	INIT_OBJ_RES_OPTDATA(FIRMWARE_PACKAGE_VERSION_ID, res[obj_inst_id], i,
			     res_inst[obj_inst_id], j);
	INIT_OBJ_RES_MULTI_OPTDATA(FIRMWARE_UPDATE_PROTO_SUPPORT_ID, res[obj_inst_id], i,
				   res_inst[obj_inst_id], j, 4, false);
	INIT_OBJ_RES_DATA(FIRMWARE_UPDATE_DELIV_METHOD_ID, res[obj_inst_id], i,
			  res_inst[obj_inst_id], j, &(delivery_method[obj_inst_id]),
			  sizeof(delivery_method[obj_inst_id]));
	INIT_OBJ_RES_EXECUTE(FIRMWARE_CANCEL_ID, res[obj_inst_id], i, cancel);
	INIT_OBJ_RES_DATA(FIRMWARE_LAST_STATE_CHANGE_TIME_ID, res[obj_inst_id], i,
			  res_inst[obj_inst_id], j, &last_change[obj_inst_id], sizeof(time_t));
	INIT_OBJ_RES_OPTDATA(FIRMWARE_COMPONENT_NAME_ID, res[obj_inst_id], i, res_inst[obj_inst_id],
			     j);
	INIT_OBJ_RES_DATA_LEN(FIRMWARE_CURRENT_VERSION_ID, res[obj_inst_id], i,
			      res_inst[obj_inst_id], j, &component_version[obj_inst_id],
			      sizeof(component_version[obj_inst_id]), 0);
	INIT_OBJ_RES_MULTI_DATA_LEN(FIRMWARE_LINKED_INSTANCES_ID, res[obj_inst_id], i,
				    res_inst[obj_inst_id], j, LINKED_INSTANCES_LEN, false,
				    &(linked_instances[obj_inst_id * LINKED_INSTANCES_LEN]),
				    sizeof(struct lwm2m_objlnk), 0);
	INIT_OBJ_RES_MULTI_DATA_LEN(FIRMWARE_CONFLICTING_INSTANCES_ID, res[obj_inst_id], i,
				    res_inst[obj_inst_id], j, LINKED_INSTANCES_LEN, false,
				    &(conflict_instances[obj_inst_id * LINKED_INSTANCES_LEN]),
				    sizeof(struct lwm2m_objlnk), 0);

	inst[obj_inst_id].resources = res[obj_inst_id];
	inst[obj_inst_id].resource_count = i;

	LOG_DBG("Create LWM2M firmware instance: %d", obj_inst_id);
	return &inst[obj_inst_id];
}

int lwm2m_adv_firmware_create_inst(const char *component, lwm2m_engine_set_data_cb_t write_callback,
				   lwm2m_engine_execute_cb_t update_callback)
{
	int ret;
	int idx;
	uint16_t len;
	struct lwm2m_engine_obj_inst *obj_inst = NULL;

	if (next_instance == MAX_INSTANCE_COUNT) {
		return -ENOMEM;
	}

	idx = next_instance;

	package_uri[idx][0] = '\0';
	update_state[idx] = STATE_IDLE;
	update_result[idx] = RESULT_DEFAULT;
	delivery_method[idx] = DELIVERY_METHOD_BOTH;
	write_cb[idx] = write_callback;
	update_cb[idx] = update_callback;
	last_change[idx] = time(NULL);

	ret = lwm2m_create_obj_inst(LWM2M_OBJECT_ADV_FIRMWARE_ID, idx, &obj_inst);
	if (ret < 0) {
		LOG_ERR("Create LWM2M instance %d error: %d", idx, ret);
		return ret;
	}

	len = strlen(component);
	ret = lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_ADV_FIRMWARE_ID, idx,
				FIRMWARE_COMPONENT_NAME_ID), (void *)component, len, len,
				LWM2M_RES_DATA_FLAG_RO);
	if (ret < 0) {
		goto fail;
	}

	next_instance++;
	return idx;

fail:
	LOG_ERR("Failed to create Advanced firmware object id %d, ret = %d", idx, ret);
	lwm2m_delete_obj_inst(LWM2M_OBJECT_ADV_FIRMWARE_ID, idx);
	return ret;
}

static int lwm2m_adv_firmware_init(void)
{
	/* Set default values */
	firmware.obj_id = LWM2M_OBJECT_ADV_FIRMWARE_ID;
	firmware.version_major = 1;
	firmware.version_minor = 0;
	firmware.is_core = false;
	firmware.fields = fields;
	firmware.field_count = ARRAY_SIZE(fields);
	firmware.max_instance_count = MAX_INSTANCE_COUNT;
	firmware.create_cb = firmware_create;
	lwm2m_register_obj(&firmware);

	return 0;
}

SYS_INIT(lwm2m_adv_firmware_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
