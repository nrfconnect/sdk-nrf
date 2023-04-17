/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/net/lwm2m.h>
#include <modem/lte_lc.h>
#include <net/lwm2m_client_utils.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lwm2m_cellconn, CONFIG_LWM2M_CLIENT_UTILS_LOG_LEVEL);

#define LWM2M_RES_DATA_FLAG_RW 0
#define CELLCONN_VERSION_MAJOR 1
#if defined(CONFIG_LWM2M_CELL_CONN_OBJ_VERSION_1_1)
#define CELLCONN_VERSION_MINOR 1
#define CELLCONN_MAX_ID 15
#else
#define CELLCONN_VERSION_MINOR 0
#define CELLCONN_MAX_ID 12
#endif /* defined(CONFIG_LWM2M_CELL_CONN_OBJ_VERSION_1_1) */

#define CELLCONN_SMSC_ADDRESS_ID 0
#define CELLCONN_DISABLE_RADIO_PERIOD_ID 1
#define CELLCONN_MODULE_ACTIVATION_CODE_ID 2
#define CELLCONN_VENDOR_EXTENSIONS_ID 3
#define CELLCONN_PSM_TIMER_ID 4
#define CELLCONN_ACTIVE_TIMER_ID 5
#define CELLCONN_SERVING_PLMN_RATE_CTRL_ID 6
#define CELLCONN_EDRX_PARAM_LU_MODE_ID 7
#define CELLCONN_EDRX_PARAM_WBS1_MODE_ID 8
#define CELLCONN_EDRX_PARAM_NBS1_MODE_ID 9
#define CELLCONN_EDRX_PARAM_AGB_MODE_ID 10
#define CELLCONN_ACTIVATED_PROFILE_NAMES_ID 11
#if defined(CONFIG_LWM2M_CELL_CONN_OBJ_VERSION_1_1)
#define CELLCONN_SUPPORTED_POWER_SAVING_MODES_ID 12
#define CELLCONN_ACTIVE_POWER_SAVING_MODES_ID 13
#define CELLCONN_RELEASE_ASSISTANCE_INDICATION_USAGE_ID 14
#endif

#if defined(CONFIG_LWM2M_CELL_CONN_APN_PROFILE_COUNT)
#define PROFILE_COUNT_MAX CONFIG_LWM2M_CELL_CONN_APN_PROFILE_COUNT
#else
#define PROFILE_COUNT_MAX 3
#endif

/*
 * Calculate resource instances as follows:
 * start with CELLCONN_MAX_ID
 * subtract MULTI resources because their counts include 0 resource (1)
 * if object version is 1.1 subtract resource for CELLCONN_SERVING_PLMN_RATE_CTRL_ID
 * add CELLCONN_ACTIVATED_PROFILE_NAMES_ID resource instances
 */
#if defined(CONFIG_LWM2M_CELL_CONN_OBJ_VERSION_1_1)
#define RESOURCE_INSTANCE_COUNT (CELLCONN_MAX_ID - 1 - 1 + PROFILE_COUNT_MAX)
#else
#define RESOURCE_INSTANCE_COUNT (CELLCONN_MAX_ID - 1 + PROFILE_COUNT_MAX)
#endif

static struct lwm2m_engine_obj cellconn;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(CELLCONN_SMSC_ADDRESS_ID, RW_OPT, STRING), /* Optional, single-instance */
	OBJ_FIELD_DATA(CELLCONN_DISABLE_RADIO_PERIOD_ID, RW_OPT,
		       U16), /* Optional, single-instance */
	OBJ_FIELD_DATA(CELLCONN_MODULE_ACTIVATION_CODE_ID, RW_OPT,
		       STRING), /* Optional, single-instance */
	OBJ_FIELD_DATA(CELLCONN_VENDOR_EXTENSIONS_ID, R_OPT, OBJLNK), /* Optional, single-instance*/
	OBJ_FIELD_DATA(CELLCONN_PSM_TIMER_ID, RW_OPT, S32), /* Optional, single-instance*/
	OBJ_FIELD_DATA(CELLCONN_ACTIVE_TIMER_ID, RW_OPT, S32), /* Optional, single-instance*/
#if !defined(CONFIG_LWM2M_CELL_CONN_OBJ_VERSION_1_1)
	OBJ_FIELD_DATA(CELLCONN_SERVING_PLMN_RATE_CTRL_ID, R_OPT,
		       S32), /* Optional, single-instance*/
#endif
	OBJ_FIELD_DATA(CELLCONN_EDRX_PARAM_LU_MODE_ID, RW_OPT,
		       OPAQUE), /* Optional, single-instance*/
	OBJ_FIELD_DATA(CELLCONN_EDRX_PARAM_WBS1_MODE_ID, RW_OPT,
		       OPAQUE), /* Optional, single-instance*/
	OBJ_FIELD_DATA(CELLCONN_EDRX_PARAM_NBS1_MODE_ID, RW_OPT,
		       OPAQUE), /* Optional, single-instance*/
	OBJ_FIELD_DATA(CELLCONN_EDRX_PARAM_AGB_MODE_ID, RW_OPT,
		       OPAQUE), /* Optional, single-instance*/
	OBJ_FIELD_DATA(CELLCONN_ACTIVATED_PROFILE_NAMES_ID, R,
		       OBJLNK), /* Mandatory, multi-instance */
#if defined(CONFIG_LWM2M_CELL_CONN_OBJ_VERSION_1_1)
	OBJ_FIELD_DATA(CELLCONN_SUPPORTED_POWER_SAVING_MODES_ID, R_OPT,
		       U8), /* Optional, single-instance*/
	OBJ_FIELD_DATA(CELLCONN_ACTIVE_POWER_SAVING_MODES_ID, RW_OPT,
		       U8), /* Optional, single-instance */
	OBJ_FIELD_DATA(CELLCONN_RELEASE_ASSISTANCE_INDICATION_USAGE_ID, RW_OPT,
		       U8) /* Optional, single-instance */
#endif
};

static struct lwm2m_engine_obj_inst inst;
static struct lwm2m_engine_res res[CELLCONN_MAX_ID];
static struct lwm2m_engine_res_inst res_inst[RESOURCE_INSTANCE_COUNT];
static struct lwm2m_objlnk profile_name[PROFILE_COUNT_MAX];

static struct lwm2m_engine_obj_inst *cellconn_create(uint16_t obj_inst_id)
{
	int i = 0, j = 0;

	if (inst.resource_count) {
		LOG_ERR("Only 1 instance of cellular connectivity object can exist.");
		return NULL;
	}

	/* initialize Object Links to MAX_ID values (null link) */
	for (int x = 0; x < PROFILE_COUNT_MAX; x++) {
		profile_name[x].obj_id = profile_name[x].obj_inst = LWM2M_OBJLNK_MAX_ID;
	}

	init_res_instance(res_inst, ARRAY_SIZE(res_inst));

	/* initialize instance resource data */
	INIT_OBJ_RES_OPTDATA(CELLCONN_SMSC_ADDRESS_ID, res, i, res_inst, j);
	INIT_OBJ_RES_OPTDATA(CELLCONN_DISABLE_RADIO_PERIOD_ID, res, i, res_inst, j);
	INIT_OBJ_RES_OPTDATA(CELLCONN_MODULE_ACTIVATION_CODE_ID, res, i, res_inst, j);
	INIT_OBJ_RES_OPTDATA(CELLCONN_VENDOR_EXTENSIONS_ID, res, i, res_inst, j);
	INIT_OBJ_RES_OPTDATA(CELLCONN_PSM_TIMER_ID, res, i, res_inst, j);
	INIT_OBJ_RES_OPTDATA(CELLCONN_ACTIVE_TIMER_ID, res, i, res_inst, j);
#if !defined(CONFIG_LWM2M_CELL_CONN_OBJ_VERSION_1_1)
	INIT_OBJ_RES_OPTDATA(CELLCONN_SERVING_PLMN_RATE_CTRL_ID, res, i, res_inst, j);
#endif
	INIT_OBJ_RES_OPTDATA(CELLCONN_EDRX_PARAM_LU_MODE_ID, res, i, res_inst, j);
	INIT_OBJ_RES_OPTDATA(CELLCONN_EDRX_PARAM_WBS1_MODE_ID, res, i, res_inst, j);
	INIT_OBJ_RES_OPTDATA(CELLCONN_EDRX_PARAM_NBS1_MODE_ID, res, i, res_inst, j);
	INIT_OBJ_RES_OPTDATA(CELLCONN_EDRX_PARAM_AGB_MODE_ID, res, i, res_inst, j);
	INIT_OBJ_RES_MULTI_DATA(CELLCONN_ACTIVATED_PROFILE_NAMES_ID, res, i, res_inst, j,
				PROFILE_COUNT_MAX, false, profile_name, sizeof(*profile_name));
#if defined(CONFIG_LWM2M_CELL_CONN_OBJ_VERSION_1_1)
	INIT_OBJ_RES_OPTDATA(CELLCONN_SUPPORTED_POWER_SAVING_MODES_ID, res, i, res_inst, j);
	INIT_OBJ_RES_OPTDATA(CELLCONN_ACTIVE_POWER_SAVING_MODES_ID, res, i, res_inst, j);
	INIT_OBJ_RES_OPTDATA(CELLCONN_RELEASE_ASSISTANCE_INDICATION_USAGE_ID, res, i, res_inst, j);
#endif
	inst.resources = res;
	inst.resource_count = i;

	LOG_DBG("Create LwM2M cellular connectivity instance: %d", obj_inst_id);

	return &inst;
}

static int lwm2m_cellconn_init(void)
{
	cellconn.obj_id = LWM2M_OBJECT_CELLULAR_CONNECTIVITY_ID;
	cellconn.version_major = CELLCONN_VERSION_MAJOR;
	cellconn.version_minor = CELLCONN_VERSION_MINOR;
	cellconn.is_core = false;
	cellconn.fields = fields;
	cellconn.field_count = ARRAY_SIZE(fields);
	cellconn.max_instance_count = 1U;
	cellconn.create_cb = cellconn_create;
	lwm2m_register_obj(&cellconn);

	LOG_DBG("Init LwM2M cellular connectivity instance");
	return 0;
}

#define PSM_ONLY_MODE 1
#define EDRX_ONLY_MODE 2
#define PSM_AND_EDRX_MODE 3

static char smsc_addr[20];
static uint16_t disable_radio_period;
static uint16_t disable_radio_period_tmp;
static struct lwm2m_objlnk *apn_profile0;
static struct lwm2m_objlnk *apn_profile1;
static int32_t tau;
static int32_t active_time;
static uint8_t edrx_wbs1;
static uint8_t edrx_nbs1;
static uint8_t active_psm_modes;
#if defined(CONFIG_LWM2M_CELL_CONN_OBJ_VERSION_1_1)
static uint8_t supported_psm_modes = PSM_AND_EDRX_MODE;
static uint8_t rai_usage;
#endif
static struct k_work radio_period_work;
static struct k_work offline_work;
static char rptau[9] = CONFIG_LTE_PSM_REQ_RPTAU;
static char rat[9] = CONFIG_LTE_PSM_REQ_RAT;
static enum lte_lc_lte_mode lte_mode = LTE_LC_LTE_MODE_NONE;

static void radio_period_update(struct k_work *work)
{
	int err;

	LOG_DBG("Set radio online");
	disable_radio_period_tmp = 0;
	lwm2m_set_u16(&LWM2M_OBJ(10, 0, 1), disable_radio_period_tmp);

	err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_NORMAL);
	if (err) {
		LOG_ERR("Unable to set modem online, error %d", err);
	}
}

static void set_radio_offline(struct k_work *work)
{
	int err;

	k_sleep(K_MSEC(200));
	LOG_DBG("Set radio offline");

	err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_OFFLINE);
	if (err) {
		LOG_ERR("Unable to set modem offline, error %d", err);
	}
}

static void radio_period_timer_fn(struct k_timer *dummy)
{
	k_work_submit(&radio_period_work);
}

K_TIMER_DEFINE(radio_period_timer, radio_period_timer_fn, NULL);

static int disable_radio_period_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
				   uint8_t *data, uint16_t data_len, bool last_block,
				   size_t total_size)
{
	uint16_t period = *(uint16_t *)data;

	if (disable_radio_period_tmp == period) {
		return 0;
	}

	disable_radio_period_tmp = period;
	LOG_INF("Disable radio period: %u minutes", period);
	k_work_submit(&offline_work);
	k_timer_start(&radio_period_timer, K_MINUTES(disable_radio_period_tmp), K_NO_WAIT);

	return 0;
}

static int edrx_update_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
			  uint8_t *data, uint16_t data_len, bool last_block, size_t total_size)
{
	int err;
	char edrx_param[5] = "";
	char pwt_param[5] = "";
	int j = 0;

	/* Paging Time Window (PTW), is encoded as octet 3 (bit 8 to 5) in
	 * [3GPP-TS_24.008, clause 10.5.5.32]. Convert bits to null-terminated string
	 */
	for (int i = 7; i >= 4; i--) {
		pwt_param[j] = *data & (1U << i) ? '1' : '0';
		j++;
	}

	/* EDRX value, is encoded as octet 3 (bit 4 to 1) in
	 * [3GPP-TS_24.008, clause 10.5.5.32]. Convert bits to null-terminated string
	 */
	j = 0;
	for (int i = 3; i >= 0; i--) {
		edrx_param[j] = *data & (1U << i) ? '1' : '0';
		j++;
	}

	LOG_INF("EDRX param value: 0x%x pwt:%s edrx:%s", *data, pwt_param, edrx_param);
	LOG_DBG("LTE mode: %s", lte_mode == LTE_LC_LTE_MODE_NONE ? "None" :
					lte_mode == LTE_LC_LTE_MODE_LTEM ? "LTE-M" :
					lte_mode == LTE_LC_LTE_MODE_NBIOT ? "NB-IoT" : "Unknown");

	err = lte_lc_ptw_set(lte_mode, pwt_param);
	if (err) {
		LOG_ERR("PWT set error %d", err);
		return err;
	}

	err = lte_lc_edrx_param_set(lte_mode, edrx_param);
	if (err) {
		LOG_ERR("EDRX set error %d", err);
		return err;
	}

	err = lte_lc_edrx_req(true);
	if (err) {
		LOG_ERR("EDRX enable error %d", err);
		return err;
	}

	return 0;
}

static void cache_active_time_str(int time)
{
	int j;
	char str[9] = "00000000";
	size_t str_len = sizeof(str) - 1;
	uint8_t val;

	/* Active Timer = T3324, [3GPP-TS_24.008, section 10.5.7.3 (GPRS Timer)]
	 * describes how seconds are encoded as octet 3 (bit 8 to 1).
	 * The value in seconds has to be compatible with T3324, otherwise
	 * conversion will not be exact
	 */
	if (time < 0) {
		str[0] = str[1] = str[2] = '1';
		val = 0xFF;
	} else if (time <= 60) {
		val = time / 2;
	} else if (time <= 1860) {
		str[2] = '1';
		val = time / 60;
	} else {
		str[1] = '1';
		val = time / 360;
	}

	j = 3;
	for (int i = 4; i >= 0; i--) {
		str[j] = val & (1U << i) ? '1' : '0';
		j++;
	}
	LOG_INF("Active timer %d s, bits: %s", time, str);
	memcpy(rat, str, str_len);
}

static void cache_psm_time_str(int time)
{
	int j;
	char str[9] = "00000000";
	size_t str_len = sizeof(str) - 1;
	uint8_t val;

	/* PSM Timer = Extended T3412, [3GPP-TS_24.008, section 10.5.7.4a (GPRS Timer)]
	 * describes how seconds are encoded as octet 3 (bit 8 to 1).
	 * The value in seconds has to be compatible with T3412, otherwise
	 * conversion will not be exact
	 */
	if (time < 0) {
		str[0] = str[1] = str[2] = '1';
		val = 0xFF;
	} else if (time <= 60) {
		val = time / 2;
		str[1] = '1';
		str[2] = '1';
	} else if (time <= 930) {
		val = time / 30;
		str[0] = '1';
	} else if (time <= 1860) {
		val = time / 60;
		str[0] = '1';
		str[2] = '1';
	} else if (time <= 18600) {
		val = time / 600;
	} else if (time <= 111600) {
		val = time / 3600;
		str[2] = '1';
	} else if (time <= 1116000) {
		val = time / 36000;
		str[1] = '1';
	} else {
		val = time / 1152000;
		str[0] = '1';
		str[1] = '1';
	}

	j = 3;
	for (int i = 4; i >= 0; i--) {
		str[j] = val & (1U << i) ? '1' : '0';
		j++;
	}
	LOG_INF("PSM timer %d s, bits: %s", time, str);
	memcpy(rptau, str, str_len);
}

static int active_time_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
			  uint8_t *data, uint16_t data_len, bool last_block, size_t total_size)
{
	int err;
	int32_t time = *(int32_t *)data;

	cache_active_time_str(time);

	err = lte_lc_psm_param_set(rptau, rat);
	if (err) {
		LOG_ERR("PSM set error %d", err);
		return err;
	}

	err = lte_lc_psm_req(true);
	if (err) {
		LOG_ERR("PSM req error %d", err);
		return err;
	}

	return 0;
}

static int psm_time_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id, uint8_t *data,
		       uint16_t data_len, bool last_block, size_t total_size)
{
	int err;
	int32_t time = *(int32_t *)data;

	cache_psm_time_str(time);

	err = lte_lc_psm_param_set(rptau, rat);
	if (err) {
		LOG_ERR("PSM set error %d", err);
		return err;
	}

	err = lte_lc_psm_req(true);
	if (err) {
		LOG_ERR("PSM req error %d", err);
		return err;
	}

	return 0;
}

static uint8_t get_edrx_kconfig(enum lte_lc_lte_mode lte_mode)
{
	char str[9] = "";

	if (lte_mode == LTE_LC_LTE_MODE_LTEM) {
		sprintf(str, CONFIG_LTE_PTW_VALUE_LTE_M);
		sprintf(str + strlen(str), CONFIG_LTE_EDRX_REQ_VALUE_LTE_M);
	} else {
		sprintf(str, CONFIG_LTE_PTW_VALUE_NBIOT);
		sprintf(str + strlen(str), CONFIG_LTE_EDRX_REQ_VALUE_NBIOT);
	}
	LOG_DBG("EDRX string: %s", str);

	return strtol(str, NULL, 2);
}

#if defined(CONFIG_LWM2M_CELL_CONN_OBJ_VERSION_1_1)
static int active_psm_update_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
				uint8_t *data, uint16_t data_len, bool last_block,
				size_t total_size)
{
	int err;

	LOG_DBG("PSM value: %d", *data);

	err = lte_lc_psm_req(*data & 1U);
	if (err) {
		LOG_ERR("PSM req error %d", err);
		return err;
	}

	err = lte_lc_edrx_req(*data & 2U);
	if (err) {
		LOG_ERR("EDRX req error %d", err);
		return err;
	}

	return 0;
}

static uint8_t get_rai_kconfig(void)
{
	uint8_t ret = 0;

	if (IS_ENABLED(CONFIG_LWM2M_CLIENT_UTILS_RAI)) {
		ret = 2; /* RAI used when transmitting last message. No response required */
	} else {
		ret = 1; /* RAI not used */
	}

	return ret;
}

static int rai_update_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id, uint8_t *data,
			 uint16_t data_len, bool last_block, size_t total_size)
{
	int err = 0;

	LOG_DBG("RAI value: %d", *data);

	switch (*data & 7U) {
#if defined(CONFIG_LWM2M_CLIENT_UTILS_RAI)
	case 1:
		err = lwm2m_rai_req(LWM2M_RAI_MODE_DISABLED);
		break;
	case 2:
		err = lwm2m_rai_req(LWM2M_RAI_MODE_ENABLED);
		break;
#endif
	case 6:
		LOG_WRN("Unsupported RAI mode");
		return -ENOTSUP;
		;
	default:
		LOG_ERR("Invalid RAI param");
		return -EINVAL;
		;
	}

	if (err) {
		LOG_ERR("RAI request error %d", err);
		return err;
	}

	return 0;
}
#endif

static void psm_update(struct lte_lc_psm_cfg psm_cfg)
{
	uint8_t psm_mode_tmp = active_psm_modes;

	if (psm_cfg.tau != tau) {
		tau = psm_cfg.tau;
		cache_psm_time_str(psm_cfg.tau);
		lwm2m_notify_observer(LWM2M_OBJECT_CELLULAR_CONNECTIVITY_ID, 0,
				      CELLCONN_PSM_TIMER_ID);
	}

	if (psm_cfg.active_time != active_time) {
		active_time = psm_cfg.active_time;
		cache_active_time_str(psm_cfg.active_time);
		lwm2m_notify_observer(LWM2M_OBJECT_CELLULAR_CONNECTIVITY_ID, 0,
				      CELLCONN_ACTIVE_TIMER_ID);
	}

	if (psm_cfg.active_time > 0) {
		psm_mode_tmp |= 1U;
	} else {
		psm_mode_tmp &= ~1U;
	}

	if (psm_mode_tmp != active_psm_modes) {
		active_psm_modes = psm_mode_tmp;
		lwm2m_notify_observer(LWM2M_OBJECT_CELLULAR_CONNECTIVITY_ID, 0,
				      CELLCONN_ACTIVE_POWER_SAVING_MODES_ID);
	}
}

static void edrx_update(struct lte_lc_edrx_cfg edrx_cfg)
{
	uint8_t edrx;
	uint8_t ptw;
	uint8_t val;
	uint8_t idx = 0;
	uint8_t psm_mode_tmp = active_psm_modes;

	/* Lookup table for eDRX WB-S1 mode */
	static const uint8_t edrx_lookup_ltem[16] = {
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 13, 13
	};
	/* Lookup table for eDRX NB-S1 mode */
	static const uint8_t edrx_lookup_nbiot[16] = {
		2, 2, 2, 3, 2, 5, 2, 2, 2, 9, 10, 11, 12, 13, 14, 15
	};
	static const uint16_t multipliers[16] = {
		0, 1, 2, 4, 6, 8, 10, 12, 14, 16, 32, 64, 128, 256, 512, 1024
	};

	/* The eDRX value is a multiple of 10.24 seconds, except for the
	 * special case of idx == 0 for LTE-M, where the value is 5.12 seconds.
	 */
	if (edrx_cfg.edrx > 10.0) {
		val = round(edrx_cfg.edrx / 10.24);
		for (int i = 1; i < 16; i++) {
			if (val == multipliers[i]) {
				idx = i;
				break;
			}
		}
	}

	/* Convert eDRX and PTW value in seconds back to bits as described in
	 * [3GPP-TS_24.008, clause 10.5.5.32]
	 */
	if (edrx_cfg.mode == LTE_LC_LTE_MODE_LTEM) {
		ptw = round(edrx_cfg.ptw / 1.28) - 1;
		edrx = edrx_lookup_ltem[idx];
	} else {
		ptw = round(edrx_cfg.ptw / 2.56) - 1;
		edrx = edrx_lookup_nbiot[idx];
	}

	edrx |= (ptw << 4U);

	LOG_DBG("EDRX: 0x%x", edrx);
	if (active_time > 0) {
		psm_mode_tmp = PSM_AND_EDRX_MODE;
	} else {
		psm_mode_tmp = EDRX_ONLY_MODE;
	}

	if (psm_mode_tmp != active_psm_modes) {
		active_psm_modes = psm_mode_tmp;
		lwm2m_notify_observer(LWM2M_OBJECT_CELLULAR_CONNECTIVITY_ID, 0,
				      CELLCONN_ACTIVE_POWER_SAVING_MODES_ID);
	}

	if (edrx_cfg.mode == LTE_LC_LTE_MODE_LTEM) {
		if (edrx_wbs1 != edrx) {
			edrx_wbs1 = edrx;
			lwm2m_notify_observer(LWM2M_OBJECT_CELLULAR_CONNECTIVITY_ID, 0,
					      CELLCONN_EDRX_PARAM_WBS1_MODE_ID);
		}
	} else {
		if (edrx_nbs1 != edrx) {
			edrx_nbs1 = edrx;
			lwm2m_notify_observer(LWM2M_OBJECT_CELLULAR_CONNECTIVITY_ID, 0,
					      CELLCONN_EDRX_PARAM_NBS1_MODE_ID);
		}
	}
}

static void lte_event_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_PSM_UPDATE:
		psm_update(evt->psm_cfg);
		break;
	case LTE_LC_EVT_EDRX_UPDATE:
		LOG_DBG("LTE EDRX update: mode %s, edrx %.2f s, Paging time %.2f s",
			(evt->edrx_cfg.mode == 7 ? "LTE" : "NBIOT"), evt->edrx_cfg.edrx,
			evt->edrx_cfg.ptw);
		edrx_update(evt->edrx_cfg);
		break;
	case LTE_LC_EVT_LTE_MODE_UPDATE:
		LOG_DBG("LTE mode update %d", evt->lte_mode);
		lte_mode = evt->lte_mode;
		break;
	default:
		break;
	}
}

static int lwm2m_lte_handler_register(void)
{
	lte_lc_register_handler(lte_event_handler);

	return 0;
}

int lwm2m_init_cellular_connectivity_object(void)
{
	uint16_t data_len;
	uint8_t data_flags;

	/* create object */
	lwm2m_create_object_inst(&LWM2M_OBJ(10, 0));
	lwm2m_set_res_buf(&LWM2M_OBJ(10, 0, 0), smsc_addr, sizeof(smsc_addr), sizeof(smsc_addr),
			  LWM2M_RES_DATA_FLAG_RW);

	lwm2m_set_res_buf(&LWM2M_OBJ(10, 0, 1), &disable_radio_period, sizeof(disable_radio_period),
			  sizeof(disable_radio_period), LWM2M_RES_DATA_FLAG_RW);
	lwm2m_register_post_write_callback(&LWM2M_OBJ(10, 0, 1), disable_radio_period_cb);

	lwm2m_set_res_buf(&LWM2M_OBJ(10, 0, 4), &tau, sizeof(tau), sizeof(tau),
			  LWM2M_RES_DATA_FLAG_RW);
	lwm2m_register_post_write_callback(&LWM2M_OBJ(10, 0, 4), psm_time_cb);
	lwm2m_set_res_buf(&LWM2M_OBJ(10, 0, 5), &active_time, sizeof(active_time),
			  sizeof(active_time), LWM2M_RES_DATA_FLAG_RW);
	lwm2m_register_post_write_callback(&LWM2M_OBJ(10, 0, 5), active_time_cb);

	edrx_wbs1 = get_edrx_kconfig(LTE_LC_LTE_MODE_LTEM);
	lwm2m_set_res_buf(&LWM2M_OBJ(10, 0, 8), &edrx_wbs1, sizeof(edrx_wbs1),
			  sizeof(edrx_wbs1), LWM2M_RES_DATA_FLAG_RW);
	lwm2m_register_post_write_callback(&LWM2M_OBJ(10, 0, 8), edrx_update_cb);
	edrx_nbs1 = get_edrx_kconfig(LTE_LC_LTE_MODE_NBIOT);
	lwm2m_set_res_buf(&LWM2M_OBJ(10, 0, 9), &edrx_nbs1, sizeof(edrx_nbs1), sizeof(edrx_nbs1),
			  LWM2M_RES_DATA_FLAG_RW);
	lwm2m_register_post_write_callback(&LWM2M_OBJ(10, 0, 9), edrx_update_cb);
#if defined(CONFIG_LWM2M_CELL_CONN_OBJ_VERSION_1_1)
	lwm2m_set_res_buf(&LWM2M_OBJ(10, 0, 12), &supported_psm_modes, sizeof(supported_psm_modes),
			  sizeof(supported_psm_modes), LWM2M_RES_DATA_FLAG_RO);
	lwm2m_set_res_buf(&LWM2M_OBJ(10, 0, 13), &active_psm_modes, sizeof(active_psm_modes),
			  sizeof(active_psm_modes), LWM2M_RES_DATA_FLAG_RW);
	lwm2m_register_post_write_callback(&LWM2M_OBJ(10, 0, 13), active_psm_update_cb);
	rai_usage = get_rai_kconfig();
	lwm2m_set_res_buf(&LWM2M_OBJ(10, 0, 14), &rai_usage, sizeof(rai_usage), sizeof(rai_usage),
			  LWM2M_RES_DATA_FLAG_RW);
	lwm2m_register_post_write_callback(&LWM2M_OBJ(10, 0, 14), rai_update_cb);
#endif
	lwm2m_create_res_inst(&LWM2M_OBJ(10, 0, 11, 0));
	lwm2m_get_res_buf(&LWM2M_OBJ(10, 0, 11, 0), (void **)&apn_profile0, NULL, &data_len,
			  &data_flags);
	lwm2m_create_res_inst(&LWM2M_OBJ(10, 0, 11, 1));
	lwm2m_get_res_buf(&LWM2M_OBJ(10, 0, 11, 1), (void **)&apn_profile1, NULL, &data_len,
			  &data_flags);

	k_work_init(&radio_period_work, radio_period_update);
	k_work_init(&offline_work, set_radio_offline);
	lwm2m_lte_handler_register();

	return 0;
}

SYS_INIT(lwm2m_cellconn_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
