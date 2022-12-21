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

DEFINE_FAKE_VALUE_FUNC(int, lwm2m_engine_set_u16, const char *, uint16_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_engine_set_u8, const char *, uint8_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_engine_set_res_data_len, const char *, uint16_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_engine_get_res_buf, const char *, void **, uint16_t *, uint16_t *,
		       uint8_t *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_engine_get_u8, const char *, uint8_t *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_engine_get_bool, const char *, bool *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_engine_set_opaque, const char *, const char *, uint16_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_engine_set_string, const char*, const char*);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_engine_send, struct lwm2m_ctx *,
		       char const **, uint8_t, bool);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_engine_delete_obj_inst, const char *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_engine_register_delete_callback, uint16_t,
		       lwm2m_engine_user_cb_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_engine_register_create_callback, uint16_t,
		       lwm2m_engine_user_cb_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_engine_register_post_write_callback, const char *,
		       lwm2m_engine_set_data_cb_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_engine_create_obj_inst, const char *);
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
DEFINE_FAKE_VALUE_FUNC(int, lte_lc_rai_param_set, const char *);
DEFINE_FAKE_VALUE_FUNC(int, lte_lc_rai_req, bool);
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
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_engine_create_res_inst, const char *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_engine_set_res_buf, const char *, void *, uint16_t,
			     uint16_t, uint8_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_engine_set_u32, const char *, uint32_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_engine_set_s8, const char *, int8_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_engine_set_s32, const char *, int32_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_engine_register_exec_callback, const char *,
		       lwm2m_engine_execute_cb_t);
DEFINE_FAKE_VOID_FUNC(engine_trigger_update, bool);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_rai_req, enum lwm2m_rai_mode);
