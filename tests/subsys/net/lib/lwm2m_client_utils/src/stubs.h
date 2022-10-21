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
#include <modem/lte_lc.h>
#include <zephyr/settings/settings.h>

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
DECLARE_FAKE_VALUE_FUNC(int, modem_key_mgmt_exists, nrf_sec_tag_t, enum modem_key_mgmt_cred_type,
			bool *);
DECLARE_FAKE_VALUE_FUNC(int, modem_key_mgmt_write, nrf_sec_tag_t, enum modem_key_mgmt_cred_type,
			const void *, size_t);
DECLARE_FAKE_VALUE_FUNC(int, lte_lc_func_mode_set, enum lte_lc_func_mode);
DECLARE_FAKE_VALUE_FUNC(int, lte_lc_connect);
DECLARE_FAKE_VALUE_FUNC(int, lte_lc_func_mode_get, enum lte_lc_func_mode *);
DECLARE_FAKE_VALUE_FUNC(int, settings_load_subtree, const char *);
DECLARE_FAKE_VALUE_FUNC(int, settings_register, struct settings_handler *);
DECLARE_FAKE_VALUE_FUNC(int, settings_subsys_init);
DECLARE_FAKE_VALUE_FUNC(int, settings_save_one, const char *, const void *, size_t);
DECLARE_FAKE_VALUE_FUNC(int, settings_delete, const char *);
DECLARE_FAKE_VOID_FUNC(engine_trigger_update, bool);

/* List of fakes used by this unit tester */
#define DO_FOREACH_FAKE(FUNC) do { \
	FUNC(lwm2m_engine_set_u16)                      \
	FUNC(lwm2m_engine_set_u8)                       \
	FUNC(lwm2m_engine_set_res_data_len)             \
	FUNC(lwm2m_engine_get_res_buf)                  \
	FUNC(lwm2m_engine_get_u8)                       \
	FUNC(lwm2m_engine_get_bool)                     \
	FUNC(lwm2m_engine_set_opaque)                   \
	FUNC(lwm2m_engine_delete_obj_inst)              \
	FUNC(lwm2m_engine_register_delete_callback)     \
	FUNC(lwm2m_engine_register_create_callback)     \
	FUNC(lwm2m_engine_register_post_write_callback) \
	FUNC(lwm2m_engine_create_obj_inst)              \
	FUNC(lwm2m_path_to_string)                      \
	FUNC(lwm2m_engine_get_obj_inst)                 \
	FUNC(lwm2m_string_to_path)                      \
	FUNC(modem_key_mgmt_exists)                     \
	FUNC(modem_key_mgmt_write)                      \
	FUNC(lte_lc_func_mode_set)                      \
	FUNC(lte_lc_connect)                            \
	FUNC(lte_lc_func_mode_get)                      \
	FUNC(settings_load_subtree)                     \
	FUNC(settings_register)                         \
	FUNC(settings_subsys_init)                      \
	FUNC(settings_save_one)                         \
	FUNC(settings_delete)                           \
	FUNC(engine_trigger_update)                     \
	} while (0)

#endif
