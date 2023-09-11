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
#include <zephyr/sys/reboot.h>
#include "stubs.h"

/*
 * Stubs are defined here, so that multiple .C files can share them
 * without having linker issues.
 */

DEFINE_FFF_GLOBALS;

DEFINE_FAKE_VALUE_FUNC(int, lwm2m_set_u16, const struct lwm2m_obj_path *, uint16_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_set_u8, const struct lwm2m_obj_path *, uint8_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_set_res_data_len, const struct lwm2m_obj_path *, uint16_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_get_res_buf, const struct lwm2m_obj_path *, void **, uint16_t *,
		       uint16_t *, uint8_t *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_get_u8, const struct lwm2m_obj_path *, uint8_t *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_get_bool, const struct lwm2m_obj_path *, bool *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_set_opaque, const struct lwm2m_obj_path *, const char *,
		       uint16_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_set_string, const struct lwm2m_obj_path *, const char*);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_send_cb, struct lwm2m_ctx *,
		       const struct lwm2m_obj_path *, uint8_t, lwm2m_send_cb_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_delete_object_inst, const struct lwm2m_obj_path *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_register_delete_callback, uint16_t,
		       lwm2m_engine_user_cb_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_register_create_callback, uint16_t,
		       lwm2m_engine_user_cb_t);
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
DEFINE_FAKE_VALUE_FUNC(int, lte_lc_func_mode_set, enum lte_lc_func_mode);
DEFINE_FAKE_VALUE_FUNC(int, lte_lc_connect);
DEFINE_FAKE_VALUE_FUNC(int, lte_lc_offline);
DEFINE_FAKE_VALUE_FUNC(int, lte_lc_func_mode_get, enum lte_lc_func_mode *);
DEFINE_FAKE_VALUE_FUNC(int, lte_lc_lte_mode_get, enum lte_lc_lte_mode *);
DEFINE_FAKE_VALUE_FUNC(int, lte_lc_ptw_set, enum lte_lc_lte_mode, const char *);
DEFINE_FAKE_VALUE_FUNC(int, lte_lc_psm_param_set, const char *, const char *);
DEFINE_FAKE_VALUE_FUNC(int, lte_lc_psm_req, bool);
DEFINE_FAKE_VALUE_FUNC(int, lte_lc_edrx_param_set, enum lte_lc_lte_mode, const char *);
DEFINE_FAKE_VALUE_FUNC(int, lte_lc_edrx_req, bool);
DEFINE_FAKE_VALUE_FUNC(int, lte_lc_neighbor_cell_measurement, struct lte_lc_ncellmeas_params *);
DEFINE_FAKE_VOID_FUNC(lte_lc_register_handler, lte_lc_evt_handler_t);
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
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_set_s16, const struct lwm2m_obj_path *, int16_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_set_s32, const struct lwm2m_obj_path *, int32_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_register_exec_callback, const struct lwm2m_obj_path *,
		       lwm2m_engine_execute_cb_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_set_default_sockopt, struct lwm2m_ctx *);
DEFINE_FAKE_VOID_FUNC(engine_trigger_update, bool);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_rai_req, enum lwm2m_rai_mode);
DEFINE_FAKE_VALUE_FUNC(struct net_if*, net_if_lookup_by_dev, const struct device *);
DEFINE_FAKE_VOID_FUNC(net_mgmt_add_event_callback, struct net_mgmt_event_callback *);
DEFINE_FAKE_VALUE_FUNC(int, net_mgmt_NET_REQUEST_WIFI_SCAN, uint32_t, struct net_if *,
		       void *, size_t);
DEFINE_FAKE_VALUE_FUNC(int, lte_lc_conn_eval_params_get, struct lte_lc_conn_eval_params *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_engine_pause);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_engine_resume);
DEFINE_FAKE_VALUE_FUNC(int, at_parser_max_params_from_str, const char *, char **,
		       struct at_param_list *, size_t);
DEFINE_FAKE_VALUE_FUNC(int, at_params_int_get, const struct at_param_list *, size_t, int32_t *);
DEFINE_FAKE_VALUE_FUNC(int, at_params_unsigned_short_get, const struct at_param_list *, size_t,
		       uint16_t *);
DEFINE_FAKE_VALUE_FUNC_VARARG(int, nrf_modem_at_cmd_async, nrf_modem_at_resp_handler_t,
			      const char *, ...);
DEFINE_FAKE_VALUE_FUNC(int, at_params_list_init, struct at_param_list *, size_t);
DEFINE_FAKE_VALUE_FUNC(int, z_impl_zsock_setsockopt, int, int, int, const void *, socklen_t);
