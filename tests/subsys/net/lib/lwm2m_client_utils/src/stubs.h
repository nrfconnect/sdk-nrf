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
#include <zephyr/sys/reboot.h>

DECLARE_FAKE_VALUE_FUNC(int, lwm2m_engine_set_u16, const char *, uint16_t);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_engine_set_u8, const char *, uint8_t);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_engine_set_res_data_len, const char *, uint16_t);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_engine_get_res_buf, const char *, void **, uint16_t *,
			uint16_t *, uint8_t *);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_engine_get_u8, const char *, uint8_t *);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_engine_get_bool, const char *, bool *);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_engine_set_opaque, const char *, char *, uint16_t);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_engine_delete_obj_inst, const char *);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_engine_register_delete_callback, uint16_t,
			lwm2m_engine_user_cb_t);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_engine_register_create_callback, uint16_t,
			lwm2m_engine_user_cb_t);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_engine_register_post_write_callback, const char *,
			lwm2m_engine_set_data_cb_t);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_engine_create_obj_inst, const char *);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_path_to_string, char *, size_t, struct lwm2m_obj_path *, int);
DECLARE_FAKE_VALUE_FUNC(struct lwm2m_engine_obj_inst *, lwm2m_engine_get_obj_inst,
			const struct lwm2m_obj_path *);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_string_to_path, const char *, struct lwm2m_obj_path *, char);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_notify_observer, uint16_t, uint16_t, uint16_t);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_update_signal_meas_objects,
			const struct lte_lc_cells_info *const);
DECLARE_FAKE_VALUE_FUNC(struct lwm2m_ctx *, lwm2m_rd_client_ctx);
DECLARE_FAKE_VOID_FUNC(lwm2m_rd_client_update);
DECLARE_FAKE_VOID_FUNC(lwm2m_register_obj, struct lwm2m_engine_obj *);
DECLARE_FAKE_VALUE_FUNC(int, modem_key_mgmt_exists, nrf_sec_tag_t, enum modem_key_mgmt_cred_type,
			bool *);
DECLARE_FAKE_VALUE_FUNC(int, modem_key_mgmt_write, nrf_sec_tag_t, enum modem_key_mgmt_cred_type,
			const void *, size_t);
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
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_engine_create_res_inst, const char *);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_engine_set_res_buf, const char *, void *, uint16_t,
			     uint16_t, uint8_t);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_engine_set_u32, const char *, uint32_t);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_engine_set_s8, const char *, int8_t);
DECLARE_FAKE_VALUE_FUNC(int, modem_info_rsrp_register, rsrp_cb_t);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_engine_register_exec_callback, const char *,
		       lwm2m_engine_execute_cb_t);

/* List of fakes used by this unit tester */
#define DO_FOREACH_FAKE(FUNC) do { \
	FUNC(lwm2m_engine_set_u32)                      \
	FUNC(lwm2m_engine_set_u16)                      \
	FUNC(lwm2m_engine_set_u8)                       \
	FUNC(lwm2m_engine_set_res_data_len)             \
	FUNC(lwm2m_engine_get_res_buf)                  \
	FUNC(lwm2m_engine_get_u8)                       \
	FUNC(lwm2m_engine_get_bool)                     \
	FUNC(lwm2m_engine_set_s8)                       \
	FUNC(lwm2m_engine_set_opaque)                   \
	FUNC(lwm2m_engine_delete_obj_inst)              \
	FUNC(lwm2m_engine_register_delete_callback)     \
	FUNC(lwm2m_engine_register_create_callback)     \
	FUNC(lwm2m_engine_register_post_write_callback) \
	FUNC(lwm2m_engine_register_exec_callback)       \
	FUNC(lwm2m_engine_create_obj_inst)              \
	FUNC(lwm2m_engine_create_res_inst)              \
	FUNC(lwm2m_engine_set_res_buf)                  \
	FUNC(lwm2m_path_to_string)                      \
	FUNC(lwm2m_engine_get_obj_inst)                 \
	FUNC(lwm2m_string_to_path)                      \
	FUNC(lwm2m_notify_observer)                     \
	FUNC(lwm2m_register_obj)                        \
	FUNC(lwm2m_update_signal_meas_objects)          \
	FUNC(lwm2m_rd_client_ctx)                       \
	FUNC(lwm2m_rd_client_update)                    \
	FUNC(modem_key_mgmt_exists)                     \
	FUNC(modem_key_mgmt_write)                      \
	FUNC(modem_info_init)                           \
	FUNC(modem_info_params_init)                    \
	FUNC(modem_info_params_get)                     \
	FUNC(modem_info_rsrp_register)                  \
	FUNC(lte_lc_func_mode_set)                      \
	FUNC(lte_lc_connect)                            \
	FUNC(lte_lc_offline)                            \
	FUNC(lte_lc_func_mode_get)                      \
	FUNC(lte_lc_lte_mode_get)                       \
	FUNC(lte_lc_ptw_set)                            \
	FUNC(lte_lc_psm_param_set)                      \
	FUNC(lte_lc_psm_req)                            \
	FUNC(lte_lc_rai_param_set)                      \
	FUNC(lte_lc_rai_req)                            \
	FUNC(lte_lc_edrx_param_set)                     \
	FUNC(lte_lc_edrx_req)                           \
	FUNC(lte_lc_neighbor_cell_measurement)          \
	FUNC(lte_lc_register_handler)                   \
	FUNC(settings_load_subtree)                     \
	FUNC(settings_register)                         \
	FUNC(settings_subsys_init)                      \
	FUNC(settings_save_one)                         \
	FUNC(settings_delete)                           \
	FUNC(settings_name_next)                        \
	FUNC(engine_trigger_update)                     \
	} while (0)

#endif
