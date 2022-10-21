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
#include <zephyr/settings/settings.h>
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
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_engine_set_opaque, const char *, char *, uint16_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_engine_delete_obj_inst, const char *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_engine_register_delete_callback, uint16_t,
		       lwm2m_engine_user_cb_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_engine_register_create_callback, uint16_t,
		       lwm2m_engine_user_cb_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_engine_register_post_write_callback, const char *,
		       lwm2m_engine_set_data_cb_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_engine_create_obj_inst, const char *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_path_to_string, char *, size_t, struct lwm2m_obj_path *, int);
DEFINE_FAKE_VALUE_FUNC(struct lwm2m_engine_obj_inst *, lwm2m_engine_get_obj_inst,
		       const struct lwm2m_obj_path *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_string_to_path, const char *, struct lwm2m_obj_path *, char);
DEFINE_FAKE_VALUE_FUNC(int, modem_key_mgmt_exists, nrf_sec_tag_t, enum modem_key_mgmt_cred_type,
		       bool *);
DEFINE_FAKE_VALUE_FUNC(int, modem_key_mgmt_write, nrf_sec_tag_t, enum modem_key_mgmt_cred_type,
		       const void *, size_t);
DEFINE_FAKE_VALUE_FUNC(int, lte_lc_func_mode_set, enum lte_lc_func_mode);
DEFINE_FAKE_VALUE_FUNC(int, lte_lc_connect);
DEFINE_FAKE_VALUE_FUNC(int, lte_lc_func_mode_get, enum lte_lc_func_mode *);
DEFINE_FAKE_VALUE_FUNC(int, settings_load_subtree, const char *);
DEFINE_FAKE_VALUE_FUNC(int, settings_register, struct settings_handler *);
DEFINE_FAKE_VALUE_FUNC(int, settings_subsys_init);
DEFINE_FAKE_VALUE_FUNC(int, settings_save_one, const char *, const void *, size_t);
DEFINE_FAKE_VALUE_FUNC(int, settings_delete, const char *);
DEFINE_FAKE_VOID_FUNC(engine_trigger_update, bool);
