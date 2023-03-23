/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef STUBS_H_
#define STUBS_H_

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <nrf_modem.h>
#include <modem/nrf_modem_lib.h>
#include <net/lwm2m_client_utils.h>
#include <modem/modem_key_mgmt.h>
#include <modem/modem_info.h>
#include <modem/lte_lc.h>
#include <dfu/dfu_target_mcuboot.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/reboot.h>
#include "lwm2m_engine.h"
#include <net/fota_download.h>

DECLARE_FAKE_VALUE_FUNC(int, lwm2m_notify_observer, uint16_t, uint16_t, uint16_t);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_update_signal_meas_objects,
			const struct lte_lc_cells_info *const);
DECLARE_FAKE_VALUE_FUNC(struct lwm2m_ctx *, lwm2m_rd_client_ctx);
DECLARE_FAKE_VOID_FUNC(lwm2m_rd_client_update);
DECLARE_FAKE_VALUE_FUNC(int, modem_key_mgmt_exists, nrf_sec_tag_t, enum modem_key_mgmt_cred_type,
			bool *);
DECLARE_FAKE_VALUE_FUNC(int, modem_key_mgmt_write, nrf_sec_tag_t, enum modem_key_mgmt_cred_type,
			const void *, size_t);
DECLARE_FAKE_VALUE_FUNC(int, lte_lc_func_mode_set, enum lte_lc_func_mode);
DECLARE_FAKE_VALUE_FUNC(int, lte_lc_connect);
DECLARE_FAKE_VALUE_FUNC(int, lte_lc_offline);
DECLARE_FAKE_VALUE_FUNC(int, lte_lc_deinit);
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
DECLARE_FAKE_VALUE_FUNC(int, modem_info_rsrp_register, rsrp_cb_t);
DECLARE_FAKE_VALUE_FUNC(int, dfu_target_mcuboot_set_buf, uint8_t *, size_t);
DECLARE_FAKE_VALUE_FUNC(int, nrf_modem_lib_shutdown);
DECLARE_FAKE_VALUE_FUNC(int, nrf_modem_lib_init);
DECLARE_FAKE_VALUE_FUNC(int, fota_download_init, fota_download_callback_t);
DECLARE_FAKE_VALUE_FUNC(int, fota_download_start_with_image_type, const char *, const char *, int,
			uint8_t, size_t, const enum dfu_target_image_type);
DECLARE_FAKE_VALUE_FUNC(int, fota_download_cancel);
DECLARE_FAKE_VALUE_FUNC(int, fota_download_target);
DECLARE_FAKE_VOID_FUNC(client_acknowledge);
DECLARE_FAKE_VALUE_FUNC(bool, boot_is_img_confirmed);
DECLARE_FAKE_VALUE_FUNC(int, boot_write_img_confirmed);
DECLARE_FAKE_VALUE_FUNC(int, dfu_target_mcuboot_init, size_t, int, dfu_target_callback_t);
DECLARE_FAKE_VALUE_FUNC(int, dfu_target_mcuboot_offset_get, size_t *);
DECLARE_FAKE_VALUE_FUNC(int, dfu_target_mcuboot_write, const void *const, size_t);
DECLARE_FAKE_VALUE_FUNC(int, dfu_target_mcuboot_done, bool);
DECLARE_FAKE_VALUE_FUNC(int, dfu_target_mcuboot_schedule_update, int);
DECLARE_FAKE_VALUE_FUNC(int, dfu_target_mcuboot_reset);
DECLARE_FAKE_VALUE_FUNC(bool, dfu_target_mcuboot_identify, const void *const);
DECLARE_FAKE_VALUE_FUNC(int, dfu_target_modem_delta_init, size_t, int, dfu_target_callback_t);
DECLARE_FAKE_VALUE_FUNC(int, dfu_target_modem_delta_offset_get, size_t *);
DECLARE_FAKE_VALUE_FUNC(int, dfu_target_modem_delta_write, const void *const, size_t);
DECLARE_FAKE_VALUE_FUNC(int, dfu_target_modem_delta_done, bool);
DECLARE_FAKE_VALUE_FUNC(int, dfu_target_modem_delta_schedule_update, int);
DECLARE_FAKE_VALUE_FUNC(int, dfu_target_modem_delta_reset);
DECLARE_FAKE_VALUE_FUNC(bool, dfu_target_modem_delta_identify, const void *const);
DECLARE_FAKE_VALUE_FUNC(int, modem_info_string_get, enum modem_info, char *, const size_t);
DECLARE_FAKE_VALUE_FUNC(int, boot_read_bank_header, uint8_t, struct mcuboot_img_header *, size_t);
DECLARE_FAKE_VOID_FUNC(clear_attrs, void *);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_notify_observer_path, const struct lwm2m_obj_path *);
DECLARE_FAKE_VOID_FUNC(engine_remove_observer_by_id, uint16_t, int32_t);
DECLARE_FAKE_VALUE_FUNC(int, lwm2m_firmware_start_transfer, uint16_t, char *);

/* List of fakes used by this unit tester */
#define DO_FOREACH_FAKE(FUNC)                                                                      \
	do {                                                                                       \
		FUNC(lwm2m_notify_observer)                                                        \
		FUNC(lwm2m_update_signal_meas_objects)                                             \
		FUNC(lwm2m_rd_client_ctx)                                                          \
		FUNC(lwm2m_rd_client_update)                                                       \
		FUNC(modem_key_mgmt_exists)                                                        \
		FUNC(modem_key_mgmt_write)                                                         \
		FUNC(modem_info_init)                                                              \
		FUNC(modem_info_params_init)                                                       \
		FUNC(modem_info_params_get)                                                        \
		FUNC(modem_info_rsrp_register)                                                     \
		FUNC(lte_lc_func_mode_set)                                                         \
		FUNC(lte_lc_connect)                                                               \
		FUNC(lte_lc_deinit)                                                                \
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
		FUNC(lte_lc_register_handler)                                                      \
		FUNC(settings_load_subtree)                                                        \
		FUNC(settings_register)                                                            \
		FUNC(settings_subsys_init)                                                         \
		FUNC(settings_save_one)                                                            \
		FUNC(settings_delete)                                                              \
		FUNC(settings_name_next)                                                           \
		FUNC(engine_trigger_update)                                                        \
		FUNC(dfu_target_mcuboot_set_buf)                                                   \
		FUNC(nrf_modem_lib_shutdown)                                                       \
		FUNC(nrf_modem_lib_init)                                                           \
		FUNC(fota_download_init)                                                           \
		FUNC(fota_download_start_with_image_type)                                          \
		FUNC(fota_download_cancel)                                                         \
		FUNC(fota_download_target)                                                         \
		FUNC(client_acknowledge)                                                           \
		FUNC(boot_is_img_confirmed)                                                        \
		FUNC(boot_write_img_confirmed)                                                     \
		FUNC(dfu_target_mcuboot_init)                                                      \
		FUNC(dfu_target_mcuboot_offset_get)                                                \
		FUNC(dfu_target_mcuboot_write)                                                     \
		FUNC(dfu_target_mcuboot_done)                                                      \
		FUNC(dfu_target_mcuboot_schedule_update)                                           \
		FUNC(dfu_target_mcuboot_reset)                                                     \
		FUNC(dfu_target_mcuboot_identify)                                                  \
		FUNC(dfu_target_modem_delta_init)                                                  \
		FUNC(dfu_target_modem_delta_offset_get)                                            \
		FUNC(dfu_target_modem_delta_write)                                                 \
		FUNC(dfu_target_modem_delta_done)                                                  \
		FUNC(dfu_target_modem_delta_schedule_update)                                       \
		FUNC(dfu_target_modem_delta_reset)                                                 \
		FUNC(dfu_target_modem_delta_identify)                                              \
		FUNC(modem_info_string_get)                                                        \
		FUNC(boot_read_bank_header)                                                        \
		FUNC(clear_attrs)                                                                  \
		FUNC(lwm2m_notify_observer_path)                                                   \
		FUNC(engine_remove_observer_by_id)                                                 \
		FUNC(lwm2m_firmware_start_transfer)                                                \
	} while (0)

#endif
