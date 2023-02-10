/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef STUBS_H_
#define STUBS_H_

#include <zephyr/ztest.h>
#include <zephyr/fff.h>

#include <net/lwm2m_client_utils.h>
#include <modem/modem_key_mgmt.h>
#include <modem/modem_info.h>
#include <modem/lte_lc.h>
#include <zephyr/settings/settings.h>
#include <dfu/dfu_target.h>
#include <net/fota_download.h>

int lwm2m_engine_register_pre_write_callback_fake1(const struct lwm2m_obj_path *path,
						   lwm2m_engine_get_data_cb_t cb);
int lwm2m_register_post_write_callback_fake1(const struct lwm2m_obj_path *path,
					     lwm2m_engine_set_data_cb_t cb);
void test_lwm2m_engine_call_post_write_cb(const struct lwm2m_obj_path *path, uint16_t obj_inst_id,
					  uint16_t res_id, uint16_t res_inst_id, uint8_t *data,
					  uint16_t data_len, bool last_block, size_t total_size);
int fota_download_init_fake1(fota_download_callback_t client_callback);
void test_fota_download_throw_evt(enum fota_download_evt_id id);
void test_fota_download_throw_evt_error(enum fota_download_error_cause cause);
enum dfu_target_image_type dfu_target_img_type_fake_return_mcuboot(const void *const buf,
								   size_t len);
void lwm2m_firmware_set_write_cb_fake1(lwm2m_engine_set_data_cb_t cb);
void test_call_lwm2m_firmware_set_write_cb(uint16_t obj_inst_id, uint16_t res_id,
					   uint16_t res_inst_id, uint8_t *data, uint16_t data_len,
					   bool last_block, size_t total_size);
void test_stubs_teardown(void);

DECLARE_FAKE_VALUE_FUNC(int, lwm2m_set_u16, const struct lwm2m_obj_path *, uint16_t);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_set_u8, const struct lwm2m_obj_path *, uint8_t);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_set_res_data_len, const struct lwm2m_obj_path *, uint16_t);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_get_res_buf, const struct lwm2m_obj_path *, void **, uint16_t *,
			uint16_t *, uint8_t *);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_get_u8, const struct lwm2m_obj_path *, uint8_t *);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_get_bool, const struct lwm2m_obj_path *, bool *);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_set_opaque, const struct lwm2m_obj_path *, const char *,
			uint16_t);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_set_string, const struct lwm2m_obj_path *, const char *)
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_send, struct lwm2m_ctx *, const struct lwm2m_obj_path *, uint8_t,
			bool);
DECLARE_FAKE_VALUE_FUNC(struct lwm2m_engine_res *, lwm2m_engine_get_res,
			const struct lwm2m_obj_path *);
DECLARE_FAKE_VALUE_FUNC(struct lwm2m_engine_res_int *, lwm2m_engine_get_res_inst,
			const struct lwm2m_obj_path *);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_delete_object_inst, const struct lwm2m_obj_path *);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_delete_res_inst, const struct lwm2m_obj_path *);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_register_delete_callback, uint16_t, lwm2m_engine_user_cb_t);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_register_create_callback, uint16_t, lwm2m_engine_user_cb_t);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_register_pre_write_callback, const struct lwm2m_obj_path *,
			lwm2m_engine_get_data_cb_t);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_register_post_write_callback, const struct lwm2m_obj_path *,
			lwm2m_engine_set_data_cb_t);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_create_object_inst, const struct lwm2m_obj_path *);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_create_obj_inst, uint16_t, uint16_t,
			struct lwm2m_engine_obj_inst **);
DECLARE_FAKE_VALUE_FUNC(struct lwm2m_engine_obj_inst *, lwm2m_engine_get_obj_inst,
			const struct lwm2m_obj_path *);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_notify_observer, uint16_t, uint16_t, uint16_t);
DECLARE_FAKE_VALUE_FUNC(struct lwm2m_ctx *, lwm2m_rd_client_ctx);
DECLARE_FAKE_VOID_FUNC(lwm2m_rd_client_update);
DECLARE_FAKE_VOID_FUNC(lwm2m_register_obj, struct lwm2m_engine_obj *);
DECLARE_FAKE_VALUE_FUNC(int, modem_key_mgmt_exists, nrf_sec_tag_t, enum modem_key_mgmt_cred_type,
			bool *);
DECLARE_FAKE_VALUE_FUNC(int, modem_key_mgmt_write, nrf_sec_tag_t, enum modem_key_mgmt_cred_type,
			const void *, size_t);
DECLARE_FAKE_VALUE_FUNC(int, lte_lc_deinit);
DECLARE_FAKE_VALUE_FUNC(int, lte_lc_func_mode_set, enum lte_lc_func_mode);
DECLARE_FAKE_VALUE_FUNC(int, lte_lc_connect);
DECLARE_FAKE_VALUE_FUNC(int, lte_lc_offline);
DECLARE_FAKE_VALUE_FUNC(int, lte_lc_func_mode_get, enum lte_lc_func_mode *);
DECLARE_FAKE_VALUE_FUNC(int, lte_lc_ptw_set, enum lte_lc_lte_mode, const char *);
DECLARE_FAKE_VALUE_FUNC(int, lte_lc_psm_param_set, const char *, const char *);
DECLARE_FAKE_VALUE_FUNC(int, lte_lc_psm_req, bool);
DECLARE_FAKE_VALUE_FUNC(int, lte_lc_rai_param_set, const char *);
DECLARE_FAKE_VALUE_FUNC(int, lte_lc_rai_req, bool);
DECLARE_FAKE_VALUE_FUNC(int, lte_lc_edrx_param_set, enum lte_lc_lte_mode, const char *);
DECLARE_FAKE_VALUE_FUNC(int, lte_lc_edrx_req, bool);
DECLARE_FAKE_VALUE_FUNC(int, lte_lc_neighbor_cell_measurement, struct lte_lc_ncellmeas_params *);
DECLARE_FAKE_VOID_FUNC(lte_lc_register_handler, lte_lc_evt_handler_t);
DECLARE_FAKE_VALUE_FUNC(int, nrf_modem_lib_init);
DECLARE_FAKE_VALUE_FUNC(int, nrf_modem_lib_shutdown);
DECLARE_FAKE_VALUE_FUNC(int, nrf_cloud_agps_process, const char *, size_t);
DECLARE_FAKE_VALUE_FUNC(int, nrf_cloud_pgps_begin_update);
DECLARE_FAKE_VALUE_FUNC(int, nrf_cloud_pgps_process_update, uint8_t *, size_t);
DECLARE_FAKE_VALUE_FUNC(int, nrf_cloud_pgps_finish_update);
DECLARE_FAKE_VALUE_FUNC(int, settings_load_subtree, const char *);
DECLARE_FAKE_VALUE_FUNC(int, settings_register, struct settings_handler *);
DECLARE_FAKE_VALUE_FUNC(int, settings_subsys_init);
DECLARE_FAKE_VALUE_FUNC(int, settings_save_one, const char *, const void *, size_t);
DECLARE_FAKE_VALUE_FUNC(int, settings_delete, const char *);
DECLARE_FAKE_VALUE_FUNC(int, settings_name_next, const char *, const char **);
DECLARE_FAKE_VOID_FUNC(engine_trigger_update, bool);
DECLARE_FAKE_VALUE_FUNC(int, modem_info_init);
DECLARE_FAKE_VALUE_FUNC(int, modem_info_params_init, struct modem_param_info *);
DECLARE_FAKE_VALUE_FUNC(int, modem_info_params_get, struct modem_param_info *);
DECLARE_FAKE_VALUE_FUNC(int, lte_lc_lte_mode_get, enum lte_lc_lte_mode *);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_create_res_inst, const struct lwm2m_obj_path *);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_set_res_buf, const struct lwm2m_obj_path *, void *, uint16_t,
			uint16_t, uint8_t);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_set_u32, const struct lwm2m_obj_path *, uint32_t);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_set_s8, const struct lwm2m_obj_path *, int8_t);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_set_s32, const struct lwm2m_obj_path *, int32_t);
DECLARE_FAKE_VALUE_FUNC(int, modem_info_rsrp_register, rsrp_cb_t);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_register_exec_callback, const struct lwm2m_obj_path *,
			lwm2m_engine_execute_cb_t);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_rai_req, enum lwm2m_rai_mode);
DECLARE_FAKE_VALUE_FUNC(struct net_if *, net_if_lookup_by_dev, const struct device *);
DECLARE_FAKE_VOID_FUNC(net_mgmt_add_event_callback, struct net_mgmt_event_callback *);
DECLARE_FAKE_VALUE_FUNC(int, net_mgmt_NET_REQUEST_WIFI_SCAN, uint32_t, struct net_if *, void *,
			size_t);
DECLARE_FAKE_VALUE_FUNC(bool, boot_is_img_confirmed);
DECLARE_FAKE_VALUE_FUNC(int, boot_write_img_confirmed);
DECLARE_FAKE_VALUE_FUNC(int, dfu_target_mcuboot_set_buf, uint8_t *, size_t);
DECLARE_FAKE_VALUE_FUNC(int, dfu_target_init, int, int, size_t, dfu_target_callback_t);
DECLARE_FAKE_VALUE_FUNC(enum dfu_target_image_type, dfu_target_img_type, const void *const, size_t);
DECLARE_FAKE_VALUE_FUNC(int, dfu_target_offset_get, size_t *);
DECLARE_FAKE_VALUE_FUNC(int, dfu_target_write, const void *const, size_t);
DECLARE_FAKE_VALUE_FUNC(int, dfu_target_done, bool);
DECLARE_FAKE_VALUE_FUNC(int, dfu_target_reset);
DECLARE_FAKE_VALUE_FUNC(int, dfu_target_schedule_update, int);
DECLARE_FAKE_VALUE_FUNC(int, fota_download_init, fota_download_callback_t);
DECLARE_FAKE_VALUE_FUNC(int, fota_download_start_with_image_type, const char *, const char *, int,
			uint8_t, size_t, const enum dfu_target_image_type);
DECLARE_FAKE_VALUE_FUNC(int, fota_download_target);
DECLARE_FAKE_VALUE_FUNC(int, fota_download_cancel);
DECLARE_FAKE_VOID_FUNC(lwm2m_firmware_set_write_cb, lwm2m_engine_set_data_cb_t);
DECLARE_FAKE_VALUE_FUNC(uint8_t, lwm2m_firmware_get_update_state_inst, uint16_t);
DECLARE_FAKE_VOID_FUNC(lwm2m_firmware_set_update_cb, lwm2m_engine_execute_cb_t);
DECLARE_FAKE_VOID_FUNC(lwm2m_firmware_set_update_state_inst, uint16_t, uint8_t);
DECLARE_FAKE_VOID_FUNC(lwm2m_firmware_set_update_result_inst, uint16_t, uint8_t);

/* List of fakes used by this unit tester */
#define DO_FOREACH_FAKE(FUNC)                                                                      \
	do {                                                                                       \
		FUNC(lwm2m_set_u32)                                                                \
		FUNC(lwm2m_set_u16)                                                                \
		FUNC(lwm2m_set_u8)                                                                 \
		FUNC(lwm2m_set_res_data_len)                                                       \
		FUNC(lwm2m_get_res_buf)                                                            \
		FUNC(lwm2m_get_u8)                                                                 \
		FUNC(lwm2m_get_bool)                                                               \
		FUNC(lwm2m_set_s8)                                                                 \
		FUNC(lwm2m_set_s32)                                                                \
		FUNC(lwm2m_set_opaque)                                                             \
		FUNC(lwm2m_set_string)                                                             \
		FUNC(lwm2m_send)                                                                   \
		FUNC(lwm2m_engine_get_res)                                                         \
		FUNC(lwm2m_engine_get_res_inst)                                                    \
		FUNC(lwm2m_delete_object_inst)                                                     \
		FUNC(lwm2m_delete_object_inst)                                                     \
		FUNC(lwm2m_delete_res_inst)                                                        \
		FUNC(lwm2m_register_create_callback)                                               \
		FUNC(lwm2m_register_pre_write_callback)                                            \
		FUNC(lwm2m_register_post_write_callback)                                           \
		FUNC(lwm2m_register_exec_callback)                                                 \
		FUNC(lwm2m_create_object_inst)                                                     \
		FUNC(lwm2m_create_res_inst)                                                        \
		FUNC(lwm2m_set_res_buf)                                                            \
		FUNC(lwm2m_create_obj_inst)                                                        \
		FUNC(lwm2m_engine_get_obj_inst)                                                    \
		FUNC(lwm2m_notify_observer)                                                        \
		FUNC(lwm2m_register_obj)                                                           \
		FUNC(lwm2m_rd_client_ctx)                                                          \
		FUNC(lwm2m_rd_client_update)                                                       \
		FUNC(modem_key_mgmt_exists)                                                        \
		FUNC(modem_key_mgmt_write)                                                         \
		FUNC(modem_info_init)                                                              \
		FUNC(modem_info_params_init)                                                       \
		FUNC(modem_info_params_get)                                                        \
		FUNC(modem_info_rsrp_register)                                                     \
		FUNC(lte_lc_deinit)                                                                \
		FUNC(lte_lc_func_mode_set)                                                         \
		FUNC(lte_lc_connect)                                                               \
		FUNC(lte_lc_offline)                                                               \
		FUNC(lte_lc_func_mode_get)                                                         \
		FUNC(lte_lc_lte_mode_get)                                                          \
		FUNC(lte_lc_ptw_set)                                                               \
		FUNC(lte_lc_psm_param_set)                                                         \
		FUNC(lte_lc_psm_req)                                                               \
		FUNC(lte_lc_rai_param_set)                                                         \
		FUNC(lte_lc_rai_req)                                                               \
		FUNC(lte_lc_edrx_param_set)                                                        \
		FUNC(lte_lc_edrx_req)                                                              \
		FUNC(lte_lc_neighbor_cell_measurement)                                             \
		FUNC(nrf_modem_lib_init)                                                           \
		FUNC(nrf_modem_lib_shutdown)                                                       \
		FUNC(lte_lc_register_handler)                                                      \
		FUNC(nrf_cloud_agps_process)                                                       \
		FUNC(nrf_cloud_pgps_begin_update)                                                  \
		FUNC(nrf_cloud_pgps_process_update)                                                \
		FUNC(nrf_cloud_pgps_finish_update)                                                 \
		FUNC(settings_load_subtree)                                                        \
		FUNC(settings_register)                                                            \
		FUNC(settings_subsys_init)                                                         \
		FUNC(settings_save_one)                                                            \
		FUNC(settings_delete)                                                              \
		FUNC(settings_name_next)                                                           \
		FUNC(engine_trigger_update)                                                        \
		FUNC(lwm2m_rai_req)                                                                \
		FUNC(net_if_lookup_by_dev)                                                         \
		FUNC(net_mgmt_add_event_callback)                                                  \
		FUNC(net_mgmt_NET_REQUEST_WIFI_SCAN)                                               \
		FUNC(boot_write_img_confirmed)                                                     \
		FUNC(boot_is_img_confirmed)                                                        \
		FUNC(dfu_target_mcuboot_set_buf)                                                   \
		FUNC(dfu_target_init)                                                              \
		FUNC(dfu_target_img_type)                                                          \
		FUNC(dfu_target_offset_get)                                                        \
		FUNC(dfu_target_write)                                                             \
		FUNC(dfu_target_done)                                                              \
		FUNC(dfu_target_reset)                                                             \
		FUNC(dfu_target_schedule_update)                                                   \
		FUNC(fota_download_init)                                                           \
		FUNC(fota_download_start_with_image_type)                                          \
		FUNC(fota_download_target)                                                         \
		FUNC(fota_download_cancel)                                                         \
		FUNC(lwm2m_firmware_set_write_cb)                                                  \
		FUNC(lwm2m_firmware_get_update_state_inst)                                         \
		FUNC(lwm2m_firmware_set_update_cb)                                                 \
		FUNC(lwm2m_firmware_set_update_state_inst)                                         \
		FUNC(lwm2m_firmware_set_update_result_inst)                                        \
                                                                                                   \
	} while (0)

#endif
