/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>

#include <net/lwm2m_client_utils.h>
#include <modem/modem_key_mgmt.h>
#include <modem/lte_lc.h>
#include <modem/modem_info.h>
#include <zephyr/settings/settings.h>

#include "stubs.h"

/* LTE-FDD bearer & NB-IoT bearer */
#define LTE_FDD_BEARER 6U
#define NB_IOT_BEARER 7U

static rsrp_cb_t modem_info_rsrp_cb;

static uint8_t connmon_mode;

static int16_t modem_rsrp_resource;

static enum lte_lc_lte_mode lte_mode;

static lte_lc_evt_handler_t handler;

static struct modem_param_info modem_param = {
		.network.ip_address.value_string = "192.168.0.2",
		.network.apn.value_string = "nat.iot.no",
		.device.modem_fw.value_string = "mfw_nrf9160_1.3.1",
		.network.cellid_dec = 1234567,
		.network.mnc.value = 10,
		.network.mcc.value = 200,
		.network.area_code.value = 12345,
};

ZTEST_SUITE(lwm2m_client_utils_connmon, NULL, NULL, NULL, NULL, NULL);

static int set_string_custom_fake(const struct lwm2m_obj_path *path, const char *data)
{
	if (path->obj_id == 3 && path->obj_inst_id == 0 && path->res_id == 3) {
		zassert_mem_equal(modem_param.device.modem_fw.value_string, data, strlen(data),
				  "Wrong FW version");
	} else if (path->obj_id == 4 && path->obj_inst_id == 0 && path->res_id == 4 &&
		   path->res_inst_id == 0) {
		zassert_mem_equal(modem_param.network.ip_address.value_string, data, strlen(data),
				  "Wrong IP Address");
	} else if (path->obj_id == 4 && path->obj_inst_id == 0 && path->res_id == 7 &&
		   path->res_inst_id == 0){
		zassert_mem_equal(modem_param.network.apn.value_string, data, strlen(data),
				  "Wrong APN");
	} else {
		zassert(0, "Invalid path");
		return -EINVAL;
	}
	return 0;
}

static int set_s16_custom_fake(const struct lwm2m_obj_path *path, int16_t value)
{
	if (path->obj_id == 4 && path->obj_inst_id == 0 && path->res_id == 2) {
		zassert_equal((int16_t)RSRP_IDX_TO_DBM(modem_rsrp_resource), value);
	} else {
		zassert(0, "Invalid path");
		return -EINVAL;
	}
	return 0;
}

static int set_u8_custom_fake(const struct lwm2m_obj_path *path, uint8_t value)
{
	if (path->obj_id == 4 && path->obj_inst_id == 0 && path->res_id == 0) {
		if (lte_mode == LTE_LC_LTE_MODE_LTEM) {
			zassert_equal(LTE_FDD_BEARER, value);
		} else if (lte_mode == LTE_LC_LTE_MODE_NBIOT) {
			zassert_equal(NB_IOT_BEARER, value);
		} else {
			zassert(0, "Invalid mode");
		}
	} else {
		zassert(0, "Invalid path");
		return -EINVAL;
	}
	return 0;
}

static void copy_event_handler(lte_lc_evt_handler_t hd)
{
	handler = hd;
}

static int copy_modem_info(struct modem_param_info *params)
{
	params = &modem_param;
	return 0;
}
static int copy_rsrp_handler(rsrp_cb_t cb)
{
	modem_info_rsrp_cb = cb;
	return 0;
}

static void setup(void)
{
	/* Register resets */
	DO_FOREACH_FAKE(RESET_FAKE);

	/* reset common FFF internal structures */
	FFF_RESET_HISTORY();
	modem_info_rsrp_cb = NULL;
	connmon_mode = 0;
	modem_rsrp_resource = 0;
	lte_mode = LTE_LC_LTE_MODE_NONE;
}

/*
 * Tests for initializing the module
 */
ZTEST(lwm2m_client_utils_connmon, test_modem_info_init_fail)
{
	int rc;

	setup();

	modem_info_init_fake.return_val = -1;
	rc = lwm2m_init_connmon();
	zassert_equal(modem_info_init_fake.call_count, 1, "Modem info init not called");
	zassert_equal(rc, modem_info_init_fake.return_val, "wrong return value");
}

ZTEST(lwm2m_client_utils_connmon, test_modem_info_params_init_fail)
{
	int rc;

	setup();

	modem_info_params_init_fake.return_val = -2;
	rc = lwm2m_init_connmon();
	zassert_equal(modem_info_params_init_fake.call_count, 1, "Info params init not called");
	zassert_equal(rc, modem_info_params_init_fake.return_val, "wrong return value");
}

ZTEST(lwm2m_client_utils_connmon, test_init)
{
	setup();

	lwm2m_init_connmon();
	zassert_equal(lte_lc_register_handler_fake.call_count, 1);
}

ZTEST(lwm2m_client_utils_connmon, test_connected)
{
	struct lte_lc_evt evt = {0};

	setup();

	modem_rsrp_resource = 50;
	lte_lc_register_handler_fake.custom_fake = copy_event_handler;
	lwm2m_set_string_fake.custom_fake = set_string_custom_fake;
	lwm2m_set_s16_fake.custom_fake = set_s16_custom_fake;
	modem_info_params_get_fake.custom_fake = copy_modem_info;
	modem_info_rsrp_register_fake.custom_fake = copy_rsrp_handler;
	lwm2m_init_connmon();
	evt.type = LTE_LC_EVT_NW_REG_STATUS;
	evt.nw_reg_status = LTE_LC_NW_REG_REGISTERED_HOME;
	handler(&evt);
	k_sleep(K_MSEC(100));
	zassert_equal(modem_info_params_get_fake.call_count, 1, "Info params get not called");
	zassert_equal(lwm2m_set_string_fake.call_count, 3, "Strings not set");
	modem_info_rsrp_cb(modem_rsrp_resource);
	k_sleep(K_MSEC(100));
	zassert_equal(lwm2m_set_s16_fake.call_count, 1, "RSRP not set");
	evt.nw_reg_status = LTE_LC_NW_REG_NOT_REGISTERED;
	handler(&evt);
	k_sleep(K_MSEC(100));
	zassert_equal(modem_info_params_get_fake.call_count, 1, "Info params called");
	zassert_equal(lwm2m_set_string_fake.call_count, 3, "Strings set");
}

ZTEST(lwm2m_client_utils_connmon, test_update_disconnected)
{
	struct lte_lc_evt evt = {0};

	setup();

	lte_lc_register_handler_fake.custom_fake = copy_event_handler;
	lwm2m_set_string_fake.custom_fake = set_string_custom_fake;
	modem_info_params_get_fake.custom_fake = copy_modem_info;
	lwm2m_init_connmon();
	evt.type = LTE_LC_EVT_CELL_UPDATE;
	handler(&evt);
	k_sleep(K_MSEC(100));
	zassert_equal(modem_info_params_get_fake.call_count, 0, "Info params called");
	zassert_equal(lwm2m_set_string_fake.call_count, 0, "Strings set");
}

ZTEST(lwm2m_client_utils_connmon, test_update)
{
	struct lte_lc_evt evt = {0};

	setup();

	lte_lc_register_handler_fake.custom_fake = copy_event_handler;
	lwm2m_set_string_fake.custom_fake = set_string_custom_fake;
	modem_info_params_get_fake.custom_fake = copy_modem_info;
	lwm2m_init_connmon();
	evt.type = LTE_LC_EVT_NW_REG_STATUS;
	evt.nw_reg_status = LTE_LC_NW_REG_REGISTERED_HOME;
	handler(&evt);
	k_sleep(K_MSEC(100));
	zassert_equal(modem_info_params_get_fake.call_count, 1, "Info params get not called");
	zassert_equal(lwm2m_set_string_fake.call_count, 3, "Strings not set");
	evt.type = LTE_LC_EVT_CELL_UPDATE;
	handler(&evt);
	k_sleep(K_MSEC(100));
	zassert_equal(modem_info_params_get_fake.call_count, 2, "Info params called");
	zassert_equal(lwm2m_set_string_fake.call_count, 6, "Strings set");
	evt.type = LTE_LC_EVT_NW_REG_STATUS;
	evt.nw_reg_status = LTE_LC_NW_REG_NOT_REGISTERED;
	handler(&evt);
	k_sleep(K_MSEC(100));
	zassert_equal(modem_info_params_get_fake.call_count, 2, "Info params called");
	zassert_equal(lwm2m_set_string_fake.call_count, 6, "Strings set");
}

ZTEST(lwm2m_client_utils_connmon, test_lte_mode_update)
{
	struct lte_lc_evt evt = {0};

	setup();
	lte_mode = LTE_LC_LTE_MODE_LTEM;

	lte_lc_register_handler_fake.custom_fake = copy_event_handler;
	lwm2m_set_u8_fake.custom_fake = set_u8_custom_fake;
	lwm2m_init_connmon();
	evt.type = LTE_LC_EVT_LTE_MODE_UPDATE;
	evt.lte_mode = lte_mode;
	handler(&evt);
	zassert_equal(lwm2m_set_u8_fake.call_count, 1, "LTE mode not set");
}
