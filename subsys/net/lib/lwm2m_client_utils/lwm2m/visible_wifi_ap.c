/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define LOG_MODULE_NAME net_lwm2m_obj_visible_wifi_ap
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <string.h>
#include <zephyr/init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include <net/lwm2m_client_utils_location.h>

#define VISIBLE_WIFI_AP_VERSION_MAJOR 1
#define VISIBLE_WIFI_AP_VERSION_MINOR 0

/* Signal Measurement Info resource IDs */
#define VISIBLE_WIFI_AP_MAC_ADDRESS_ID		0
#define VISIBLE_WIFI_AP_CHANNEL_ID		1
#define VISIBLE_WIFI_AP_FREQUENCY_ID		2
#define VISIBLE_WIFI_AP_SIGNAL_STRENGTH_ID	3
#define VISIBLE_WIFI_AP_SSID_ID			4

#define VISIBLE_WIFI_AP_MAX_ID			5

#define MAX_INSTANCE_COUNT CONFIG_LWM2M_CLIENT_UTILS_VISIBLE_WIFI_AP_INSTANCE_COUNT

#define MAC_ADDRESS_BYTES	6
/* Maximum SSID length 32 + \0 */
#define SSID_LENGTH		33

static uint8_t mac_address[MAX_INSTANCE_COUNT][MAC_ADDRESS_BYTES];
static int32_t channel[MAX_INSTANCE_COUNT];
static int32_t frequency[MAX_INSTANCE_COUNT];
static int32_t signal_strength[MAX_INSTANCE_COUNT];
static char ssid[MAX_INSTANCE_COUNT][SSID_LENGTH];


/*
 * Calculate resource instances as follows:
 * start with VISIBLE_WIFI_AP_MAX_ID
 */
#define RESOURCE_INSTANCE_COUNT	(VISIBLE_WIFI_AP_MAX_ID)

static struct lwm2m_engine_obj visible_wifi_ap;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(VISIBLE_WIFI_AP_MAC_ADDRESS_ID, R, OPAQUE),
	OBJ_FIELD_DATA(VISIBLE_WIFI_AP_CHANNEL_ID, R_OPT, S32),
	OBJ_FIELD_DATA(VISIBLE_WIFI_AP_FREQUENCY_ID, R_OPT, S32),
	OBJ_FIELD_DATA(VISIBLE_WIFI_AP_SIGNAL_STRENGTH_ID, R_OPT, S32),
	OBJ_FIELD_DATA(VISIBLE_WIFI_AP_SSID_ID, R_OPT, STRING),
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][VISIBLE_WIFI_AP_MAX_ID];
static struct lwm2m_engine_res_inst res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

static struct lwm2m_engine_obj_inst *visible_wifi_ap_create(uint16_t obj_inst_id)
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
	INIT_OBJ_RES_DATA(VISIBLE_WIFI_AP_MAC_ADDRESS_ID, res[index], i, res_inst[index], j,
			  &mac_address[index], sizeof(*mac_address));
	INIT_OBJ_RES_DATA(VISIBLE_WIFI_AP_CHANNEL_ID, res[index], i, res_inst[index], j,
			  &channel[index], sizeof(*channel));
	INIT_OBJ_RES_DATA(VISIBLE_WIFI_AP_FREQUENCY_ID, res[index], i, res_inst[index], j,
			  &frequency[index], sizeof(*frequency));
	INIT_OBJ_RES_DATA(VISIBLE_WIFI_AP_SIGNAL_STRENGTH_ID, res[index], i, res_inst[index], j,
			  &signal_strength[index], sizeof(*signal_strength));
	INIT_OBJ_RES_DATA(VISIBLE_WIFI_AP_SSID_ID, res[index], i, res_inst[index], j,
			  &ssid[index], sizeof(ssid));

	inst[index].resources = res[index];
	inst[index].resource_count = i;
	LOG_DBG("Created a visible Wi-Fi access point object: %d", obj_inst_id);
	return &inst[index];
}

static int lwm2m_visible_wifi_ap_init(void)
{
	int ret = 0;

	visible_wifi_ap.obj_id = VISIBLE_WIFI_AP_OBJECT_ID;
	visible_wifi_ap.version_major = VISIBLE_WIFI_AP_VERSION_MAJOR;
	visible_wifi_ap.version_minor = VISIBLE_WIFI_AP_VERSION_MINOR;
	visible_wifi_ap.is_core = false;
	visible_wifi_ap.fields = fields;
	visible_wifi_ap.field_count = ARRAY_SIZE(fields);
	visible_wifi_ap.max_instance_count = MAX_INSTANCE_COUNT;
	visible_wifi_ap.create_cb = visible_wifi_ap_create;
	lwm2m_register_obj(&visible_wifi_ap);

	/* auto create all instances */
	for (int i = 0; i < MAX_INSTANCE_COUNT; i++) {
		struct lwm2m_engine_obj_inst *obj_inst = NULL;

		ret = lwm2m_create_obj_inst(VISIBLE_WIFI_AP_OBJECT_ID, i, &obj_inst);
		if (ret < 0) {
			LOG_ERR("Create visible Wi-Fi AP instance %d error: %d", i, ret);
		}
	}

	return 0;
}

LWM2M_OBJ_INIT(lwm2m_visible_wifi_ap_init);
