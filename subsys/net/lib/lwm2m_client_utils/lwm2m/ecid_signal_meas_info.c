/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define LOG_MODULE_NAME net_lwm2m_obj_signal_meas_info
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <string.h>
#include <zephyr/init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include <net/lwm2m_client_utils.h>

#define SIGNAL_MEAS_INFO_VERSION_MAJOR 1
#define SIGNAL_MEAS_INFO_VERSION_MINOR 0

/* Signal Measurement Info resource IDs */
#define SIGNAL_MEAS_INFO_PHYS_CELL_ID		0
#define SIGNAL_MEAS_INFO_ECGI			1
#define SIGNAL_MEAS_INFO_ARFCN_EUTRA		2
#define SIGNAL_MEAS_INFO_RSRP_RESULT		3
#define SIGNAL_MEAS_INFO_RSRQ_RESULT		4
#define SIGNAL_MEAS_INFO_UE_RXTX_TIMEDIFF	5

#define SIGNAL_MEAS_INFO_MAX_ID			6

#define MAX_INSTANCE_COUNT CONFIG_LWM2M_CLIENT_UTILS_SIGNAL_MEAS_INFO_INSTANCE_COUNT

static int32_t cell_id[MAX_INSTANCE_COUNT];
static int32_t ecgi[MAX_INSTANCE_COUNT];
static int32_t arfcn_eutra[MAX_INSTANCE_COUNT];
static int32_t rsrp_result[MAX_INSTANCE_COUNT];
static int32_t rsrq_result[MAX_INSTANCE_COUNT];
static int32_t ue_rxtx_timediff[MAX_INSTANCE_COUNT];

/*
 * Calculate resource instances as follows:
 * start with SIGNAL_MEAS_INFO_MAX_ID
 */
#define RESOURCE_INSTANCE_COUNT	(SIGNAL_MEAS_INFO_MAX_ID)

static struct lwm2m_engine_obj signal_meas_info;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(SIGNAL_MEAS_INFO_PHYS_CELL_ID, R, S32),
	OBJ_FIELD_DATA(SIGNAL_MEAS_INFO_ECGI, R_OPT, S32),
	OBJ_FIELD_DATA(SIGNAL_MEAS_INFO_ARFCN_EUTRA, R, S32),
	OBJ_FIELD_DATA(SIGNAL_MEAS_INFO_RSRP_RESULT, R, S32),
	OBJ_FIELD_DATA(SIGNAL_MEAS_INFO_RSRQ_RESULT, R_OPT, S32),
	OBJ_FIELD_DATA(SIGNAL_MEAS_INFO_UE_RXTX_TIMEDIFF, R_OPT, S32),
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][SIGNAL_MEAS_INFO_MAX_ID];
static struct lwm2m_engine_res_inst res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

int lwm2m_signal_meas_info_inst_id_to_index(uint16_t obj_inst_id)
{
	int i;

	for (i = 0; i < MAX_INSTANCE_COUNT; i++) {
		if (inst[i].obj && inst[i].obj_inst_id == obj_inst_id) {
			return i;
		}
	}

	return -ENOENT;
}

int lwm2m_signal_meas_info_index_to_inst_id(int index)
{
	if (index >= MAX_INSTANCE_COUNT) {
		return -EINVAL;
	}

	/* not instantiated */
	if (!inst[index].obj) {
		return -ENOENT;
	}

	return inst[index].obj_inst_id;
}

static struct lwm2m_engine_obj_inst *signal_meas_info_create(uint16_t obj_inst_id)
{
	int index, i = 0, j = 0;

	/* Check that there is no other instance with this ID */
	for (index = 0; index < MAX_INSTANCE_COUNT; index++) {
		if (inst[index].obj && inst[index].obj_inst_id == obj_inst_id) {
			LOG_ERR("Instance already exists, %d", obj_inst_id);
			return NULL;
		}
	}

	for (index = 0; index < MAX_INSTANCE_COUNT; index++) {
		if (!inst[index].obj) {
			break;
		}
	}

	if (index >= MAX_INSTANCE_COUNT) {
		LOG_ERR("Not enough memory to create object instance.");
		return NULL;
	}

	(void)memset(res[index], 0,
		     sizeof(res[index][0]) * ARRAY_SIZE(res[index]));
	init_res_instance(res_inst[index], ARRAY_SIZE(res_inst[index]));

	/* initialize instance resource data */
	INIT_OBJ_RES_DATA(SIGNAL_MEAS_INFO_PHYS_CELL_ID, res[index], i, res_inst[index], j,
			  &cell_id[index], sizeof(*cell_id));
	INIT_OBJ_RES_DATA(SIGNAL_MEAS_INFO_ECGI, res[index], i, res_inst[index], j,
			  &ecgi[index], sizeof(*ecgi));
	INIT_OBJ_RES_DATA(SIGNAL_MEAS_INFO_ARFCN_EUTRA, res[index], i, res_inst[index], j,
			  &arfcn_eutra[index], sizeof(*arfcn_eutra));
	INIT_OBJ_RES_DATA(SIGNAL_MEAS_INFO_RSRP_RESULT, res[index], i, res_inst[index], j,
			  &rsrp_result[index], sizeof(*rsrp_result));
	INIT_OBJ_RES_DATA(SIGNAL_MEAS_INFO_RSRQ_RESULT, res[index], i, res_inst[index], j,
			  &rsrq_result[index], sizeof(rsrq_result));
	INIT_OBJ_RES_DATA(SIGNAL_MEAS_INFO_UE_RXTX_TIMEDIFF, res[index], i, res_inst[index], j,
			  &ue_rxtx_timediff[index], sizeof(*ue_rxtx_timediff));

	inst[index].resources = res[index];
	inst[index].resource_count = i;
	LOG_DBG("Created a signal measurement info object: %d", obj_inst_id);
	return &inst[index];
}

static int lwm2m_signal_meas_info_init(const struct device *dev)
{
	int ret = 0;

	signal_meas_info.obj_id = ECID_SIGNAL_MEASUREMENT_INFO_OBJECT_ID;
	signal_meas_info.version_major = SIGNAL_MEAS_INFO_VERSION_MAJOR;
	signal_meas_info.version_minor = SIGNAL_MEAS_INFO_VERSION_MINOR;
	signal_meas_info.is_core = false;
	signal_meas_info.fields = fields;
	signal_meas_info.field_count = ARRAY_SIZE(fields);
	signal_meas_info.max_instance_count = MAX_INSTANCE_COUNT;
	signal_meas_info.create_cb = signal_meas_info_create;
	lwm2m_register_obj(&signal_meas_info);

	/* auto create all instances */
	for (int i = 0; i < MAX_INSTANCE_COUNT; i++) {
		struct lwm2m_engine_obj_inst *obj_inst = NULL;

		ret = lwm2m_create_obj_inst(ECID_SIGNAL_MEASUREMENT_INFO_OBJECT_ID, i, &obj_inst);
		if (ret < 0) {
			LOG_ERR("Create LWM2M server instance %d error: %d", i, ret);
		}
	}

	return 0;
}

SYS_INIT(lwm2m_signal_meas_info_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
