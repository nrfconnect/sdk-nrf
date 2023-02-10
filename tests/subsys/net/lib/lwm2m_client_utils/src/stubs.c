/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "stubs.h"

/*
 * Stubs are defined here, so that multiple .C files can share them
 * without having linker issues.
 */

DEFINE_FFF_GLOBALS;

#define MAX_CB_CNT 5

uint8_t pre_write_cb_next;
uint8_t post_write_cb_next;

struct pre_write_cb {
	struct lwm2m_obj_path path;
	lwm2m_engine_get_data_cb_t cb;
};

struct post_write_cb {
	struct lwm2m_obj_path path;
	lwm2m_engine_set_data_cb_t cb;
};

struct pre_write_cb test_pre_write_cbs[MAX_CB_CNT];
struct post_write_cb test_post_write_cbs[MAX_CB_CNT];

int lwm2m_register_pre_write_callback_fake1(const struct lwm2m_obj_path *path,
					    lwm2m_engine_get_data_cb_t cb)
{
	if ((pre_write_cb_next + 1) == MAX_CB_CNT) {
		return -1;
	}

	memcpy(&test_pre_write_cbs[pre_write_cb_next].path, path, sizeof(struct lwm2m_obj_path));
	test_pre_write_cbs[pre_write_cb_next].cb = cb;
	pre_write_cb_next++;

	return 0;
}

int lwm2m_register_post_write_callback_fake1(const struct lwm2m_obj_path *path,
					     lwm2m_engine_set_data_cb_t cb)
{
	if ((post_write_cb_next + 1) == MAX_CB_CNT) {
		return -1;
	}

	memcpy(&test_post_write_cbs[post_write_cb_next].path, path, sizeof(struct lwm2m_obj_path));
	test_post_write_cbs[post_write_cb_next].cb = cb;
	post_write_cb_next++;

	return 0;
}

void test_lwm2m_engine_call_post_write_cb(const struct lwm2m_obj_path *path, uint16_t obj_inst_id,
					  uint16_t res_id, uint16_t res_inst_id, uint8_t *data,
					  uint16_t data_len, bool last_block, size_t total_size)
{
	size_t i = 0;

	for (i = 0; i < post_write_cb_next; i++) {
		if (lwm2m_obj_path_equal(path, &test_post_write_cbs[i].path)) {
			test_post_write_cbs[i].cb(obj_inst_id, res_id, res_inst_id, data, data_len,
						  last_block, total_size);
		}
	}
}

fota_download_callback_t test_fota_download_cb;

int fota_download_init_fake1(fota_download_callback_t client_callback)
{
	test_fota_download_cb = client_callback;
	return 0;
}

void test_fota_download_throw_evt(enum fota_download_evt_id id)
{
	const struct fota_download_evt evt = { .id = id };

	test_fota_download_cb(&evt);
}

void test_fota_download_throw_evt_error(enum fota_download_error_cause cause)
{
	const struct fota_download_evt evt = { .id = FOTA_DOWNLOAD_EVT_ERROR, .cause = cause };

	test_fota_download_cb(&evt);
}

enum dfu_target_image_type dfu_target_img_type_fake_return_mcuboot(const void *const buf,
								   size_t len)
{
	return DFU_TARGET_IMAGE_TYPE_MCUBOOT;
}

lwm2m_engine_set_data_cb_t test_lwm2m_firmware_write_cb;

void lwm2m_firmware_set_write_cb_fake1(lwm2m_engine_set_data_cb_t cb)
{
	test_lwm2m_firmware_write_cb = cb;
}

void test_call_lwm2m_firmware_set_write_cb(uint16_t obj_inst_id, uint16_t res_id,
					   uint16_t res_inst_id, uint8_t *data, uint16_t data_len,
					   bool last_block, size_t total_size)
{
	if (test_lwm2m_firmware_write_cb == NULL) {
		return;
	}

	test_lwm2m_firmware_write_cb(obj_inst_id, res_id, res_inst_id, data, data_len, last_block,
				     total_size);
}

void test_stubs_teardown(void)
{
	pre_write_cb_next = 0;
	memset(test_pre_write_cbs, 0, sizeof(test_pre_write_cbs));
	post_write_cb_next = 0;
	memset(test_post_write_cbs, 0, sizeof(test_post_write_cbs));

	test_fota_download_cb = NULL;
	test_lwm2m_firmware_write_cb = NULL;
}

DEFINE_FAKE_VALUE_FUNC(int, lwm2m_set_u16, const struct lwm2m_obj_path *, uint16_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_set_u8, const struct lwm2m_obj_path *, uint8_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_set_res_data_len, const struct lwm2m_obj_path *, uint16_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_get_res_buf, const struct lwm2m_obj_path *, void **, uint16_t *,
		       uint16_t *, uint8_t *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_get_u8, const struct lwm2m_obj_path *, uint8_t *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_get_bool, const struct lwm2m_obj_path *, bool *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_set_opaque, const struct lwm2m_obj_path *, const char *,
		       uint16_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_set_string, const struct lwm2m_obj_path *, const char *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_send, struct lwm2m_ctx *, const struct lwm2m_obj_path *, uint8_t,
		       bool);
DEFINE_FAKE_VALUE_FUNC(struct lwm2m_engine_res *, lwm2m_engine_get_res,
		       const struct lwm2m_obj_path *);
DEFINE_FAKE_VALUE_FUNC(struct lwm2m_engine_res_int *, lwm2m_engine_get_res_inst,
		       const struct lwm2m_obj_path *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_delete_object_inst, const struct lwm2m_obj_path *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_delete_res_inst, const struct lwm2m_obj_path *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_register_delete_callback, uint16_t, lwm2m_engine_user_cb_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_register_create_callback, uint16_t, lwm2m_engine_user_cb_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_register_pre_write_callback, const struct lwm2m_obj_path *,
		       lwm2m_engine_get_data_cb_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_register_post_write_callback, const struct lwm2m_obj_path *,
		       lwm2m_engine_set_data_cb_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_create_object_inst, const struct lwm2m_obj_path *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_create_obj_inst, uint16_t, uint16_t,
		       struct lwm2m_engine_obj_inst **);
DEFINE_FAKE_VALUE_FUNC(struct lwm2m_engine_obj_inst *, lwm2m_engine_get_obj_inst,
		       const struct lwm2m_obj_path *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_notify_observer, uint16_t, uint16_t, uint16_t);
DEFINE_FAKE_VALUE_FUNC(struct lwm2m_ctx *, lwm2m_rd_client_ctx);
DEFINE_FAKE_VOID_FUNC(lwm2m_rd_client_update);
DEFINE_FAKE_VOID_FUNC(lwm2m_register_obj, struct lwm2m_engine_obj *);
DEFINE_FAKE_VALUE_FUNC(int, modem_key_mgmt_exists, nrf_sec_tag_t, enum modem_key_mgmt_cred_type,
		       bool *);
DEFINE_FAKE_VALUE_FUNC(int, modem_key_mgmt_write, nrf_sec_tag_t, enum modem_key_mgmt_cred_type,
		       const void *, size_t);
DEFINE_FAKE_VALUE_FUNC(int, lte_lc_deinit);
DEFINE_FAKE_VALUE_FUNC(int, lte_lc_func_mode_set, enum lte_lc_func_mode);
DEFINE_FAKE_VALUE_FUNC(int, lte_lc_connect);
DEFINE_FAKE_VALUE_FUNC(int, lte_lc_offline);
DEFINE_FAKE_VALUE_FUNC(int, lte_lc_func_mode_get, enum lte_lc_func_mode *);
DEFINE_FAKE_VALUE_FUNC(int, lte_lc_lte_mode_get, enum lte_lc_lte_mode *);
DEFINE_FAKE_VALUE_FUNC(int, lte_lc_ptw_set, enum lte_lc_lte_mode, const char *);
DEFINE_FAKE_VALUE_FUNC(int, lte_lc_psm_param_set, const char *, const char *);
DEFINE_FAKE_VALUE_FUNC(int, lte_lc_psm_req, bool);
DEFINE_FAKE_VALUE_FUNC(int, lte_lc_rai_param_set, const char *);
DEFINE_FAKE_VALUE_FUNC(int, lte_lc_rai_req, bool);
DEFINE_FAKE_VALUE_FUNC(int, lte_lc_edrx_param_set, enum lte_lc_lte_mode, const char *);
DEFINE_FAKE_VALUE_FUNC(int, lte_lc_edrx_req, bool);
DEFINE_FAKE_VALUE_FUNC(int, lte_lc_neighbor_cell_measurement, struct lte_lc_ncellmeas_params *);
DEFINE_FAKE_VOID_FUNC(lte_lc_register_handler, lte_lc_evt_handler_t);
DEFINE_FAKE_VALUE_FUNC(int, nrf_modem_lib_init);
DEFINE_FAKE_VALUE_FUNC(int, nrf_modem_lib_shutdown);
DEFINE_FAKE_VALUE_FUNC(int, nrf_cloud_agps_process, const char *, size_t);
DEFINE_FAKE_VALUE_FUNC(int, nrf_cloud_pgps_begin_update);
DEFINE_FAKE_VALUE_FUNC(int, nrf_cloud_pgps_process_update, uint8_t *, size_t);
DEFINE_FAKE_VALUE_FUNC(int, nrf_cloud_pgps_finish_update);
DEFINE_FAKE_VALUE_FUNC(int, settings_load_subtree, const char *);
DEFINE_FAKE_VALUE_FUNC(int, settings_register, struct settings_handler *);
DEFINE_FAKE_VALUE_FUNC(int, settings_subsys_init);
DEFINE_FAKE_VALUE_FUNC(int, settings_save_one, const char *, const void *, size_t);
DEFINE_FAKE_VALUE_FUNC(int, settings_delete, const char *);
DEFINE_FAKE_VALUE_FUNC(int, settings_name_next, const char *, const char **);
DEFINE_FAKE_VALUE_FUNC(int, modem_info_init);
DEFINE_FAKE_VALUE_FUNC(int, modem_info_params_init, struct modem_param_info *);
DEFINE_FAKE_VALUE_FUNC(int, modem_info_params_get, struct modem_param_info *);
DEFINE_FAKE_VALUE_FUNC(int, modem_info_rsrp_register, rsrp_cb_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_create_res_inst, const struct lwm2m_obj_path *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_set_res_buf, const struct lwm2m_obj_path *, void *, uint16_t,
		       uint16_t, uint8_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_set_u32, const struct lwm2m_obj_path *, uint32_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_set_s8, const struct lwm2m_obj_path *, int8_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_set_s32, const struct lwm2m_obj_path *, int32_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_register_exec_callback, const struct lwm2m_obj_path *,
		       lwm2m_engine_execute_cb_t);
DEFINE_FAKE_VOID_FUNC(engine_trigger_update, bool);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_rai_req, enum lwm2m_rai_mode);
DEFINE_FAKE_VALUE_FUNC(struct net_if *, net_if_lookup_by_dev, const struct device *);
DEFINE_FAKE_VOID_FUNC(net_mgmt_add_event_callback, struct net_mgmt_event_callback *);
DEFINE_FAKE_VALUE_FUNC(int, net_mgmt_NET_REQUEST_WIFI_SCAN, uint32_t, struct net_if *, void *,
		       size_t);
DEFINE_FAKE_VALUE_FUNC(bool, boot_is_img_confirmed);
DEFINE_FAKE_VALUE_FUNC(int, boot_write_img_confirmed);
DEFINE_FAKE_VALUE_FUNC(int, dfu_target_mcuboot_set_buf, uint8_t *, size_t);
DEFINE_FAKE_VALUE_FUNC(int, dfu_target_init, int, int, size_t, dfu_target_callback_t);
DEFINE_FAKE_VALUE_FUNC(enum dfu_target_image_type, dfu_target_img_type, const void *const, size_t);
DEFINE_FAKE_VALUE_FUNC(int, dfu_target_offset_get, size_t *);
DEFINE_FAKE_VALUE_FUNC(int, dfu_target_write, const void *const, size_t);
DEFINE_FAKE_VALUE_FUNC(int, dfu_target_done, bool);
DEFINE_FAKE_VALUE_FUNC(int, dfu_target_reset);
DEFINE_FAKE_VALUE_FUNC(int, dfu_target_schedule_update, int);
DEFINE_FAKE_VALUE_FUNC(int, fota_download_init, fota_download_callback_t);
DEFINE_FAKE_VALUE_FUNC(int, fota_download_start_with_image_type, const char *, const char *, int,
		       uint8_t, size_t, const enum dfu_target_image_type);
DEFINE_FAKE_VALUE_FUNC(int, fota_download_target);
DEFINE_FAKE_VALUE_FUNC(int, fota_download_cancel);
DEFINE_FAKE_VOID_FUNC(lwm2m_firmware_set_write_cb, lwm2m_engine_set_data_cb_t);
DEFINE_FAKE_VALUE_FUNC(uint8_t, lwm2m_firmware_get_update_state_inst, uint16_t);
DEFINE_FAKE_VOID_FUNC(lwm2m_firmware_set_update_cb, lwm2m_engine_execute_cb_t);
DEFINE_FAKE_VOID_FUNC(lwm2m_firmware_set_update_state_inst, uint16_t, uint8_t);
DEFINE_FAKE_VOID_FUNC(lwm2m_firmware_set_update_result_inst, uint16_t, uint8_t);
