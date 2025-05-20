/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>

#include <net/lwm2m_client_utils.h>
#include <modem/nrf_modem_lib.h>
#include <modem/modem_key_mgmt.h>
#include <modem/lte_lc.h>
#include <modem/modem_info.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/reboot.h>
#include <net/fota_download.h>
#include "stubs.h"

/*
 * Stubs are defined here, so that multiple .C files can share them
 * without having linker issues.
 */

DEFINE_FFF_GLOBALS;

DEFINE_FAKE_VALUE_FUNC(int, lwm2m_notify_observer, uint16_t, uint16_t, uint16_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_update_signal_meas_objects,
		       const struct lte_lc_cells_info *const);
DEFINE_FAKE_VALUE_FUNC(struct lwm2m_ctx *, lwm2m_rd_client_ctx);
DEFINE_FAKE_VOID_FUNC(lwm2m_rd_client_update);
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
DEFINE_FAKE_VALUE_FUNC(int, modem_info_get_hw_version, char *, uint8_t);
DEFINE_FAKE_VOID_FUNC(engine_trigger_update, bool);
DEFINE_FAKE_VALUE_FUNC(int, dfu_target_mcuboot_set_buf, uint8_t *, size_t);
DEFINE_FAKE_VALUE_FUNC(int, nrf_modem_lib_shutdown);
DEFINE_FAKE_VALUE_FUNC(int, nrf_modem_lib_init);
DEFINE_FAKE_VALUE_FUNC(int, fota_download_init, fota_download_callback_t);
DEFINE_FAKE_VALUE_FUNC(int, fota_download_start_with_image_type, const char *, const char *, int,
		       uint8_t, size_t, const enum dfu_target_image_type);
DEFINE_FAKE_VALUE_FUNC(int, fota_download_cancel);
DEFINE_FAKE_VALUE_FUNC(int, fota_download_target);
DEFINE_FAKE_VOID_FUNC(client_acknowledge);
DEFINE_FAKE_VALUE_FUNC(bool, boot_is_img_confirmed);
DEFINE_FAKE_VALUE_FUNC(int, boot_write_img_confirmed);
DEFINE_FAKE_VALUE_FUNC(int, dfu_target_mcuboot_init, size_t, int, dfu_target_callback_t);
DEFINE_FAKE_VALUE_FUNC(int, dfu_target_mcuboot_offset_get, size_t *);
DEFINE_FAKE_VALUE_FUNC(int, dfu_target_mcuboot_write, const void *const, size_t);
DEFINE_FAKE_VALUE_FUNC(int, dfu_target_mcuboot_done, bool);
DEFINE_FAKE_VALUE_FUNC(int, dfu_target_mcuboot_schedule_update, int);
DEFINE_FAKE_VALUE_FUNC(int, dfu_target_mcuboot_reset);
DEFINE_FAKE_VALUE_FUNC(bool, dfu_target_mcuboot_identify, const void *const);
DEFINE_FAKE_VALUE_FUNC(int, dfu_target_modem_delta_init, size_t, int, dfu_target_callback_t);
DEFINE_FAKE_VALUE_FUNC(int, dfu_target_modem_delta_offset_get, size_t *);
DEFINE_FAKE_VALUE_FUNC(int, dfu_target_modem_delta_write, const void *const, size_t);
DEFINE_FAKE_VALUE_FUNC(int, dfu_target_modem_delta_done, bool);
DEFINE_FAKE_VALUE_FUNC(int, dfu_target_modem_delta_schedule_update, int);
DEFINE_FAKE_VALUE_FUNC(int, dfu_target_modem_delta_reset);
DEFINE_FAKE_VALUE_FUNC(bool, dfu_target_modem_delta_identify, const void *const);
DEFINE_FAKE_VALUE_FUNC(int, modem_info_string_get, enum modem_info, char *, const size_t);
DEFINE_FAKE_VALUE_FUNC(int, boot_read_bank_header, uint8_t, struct mcuboot_img_header *, size_t);
DEFINE_FAKE_VOID_FUNC(clear_attrs, void *);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_notify_observer_path, const struct lwm2m_obj_path *);
DEFINE_FAKE_VOID_FUNC(engine_remove_observer_by_id, uint16_t, int32_t);
DEFINE_FAKE_VALUE_FUNC(int, lwm2m_firmware_start_transfer, uint16_t, char *);
DEFINE_FAKE_VALUE_FUNC(int, fota_download_util_stream_init);
DEFINE_FAKE_VALUE_FUNC(int, fota_download_util_dfu_target_init, enum dfu_target_image_type);
DEFINE_FAKE_VALUE_FUNC(int, fota_download_util_download_start, const char *,
		       enum dfu_target_image_type, int, fota_download_callback_t);
DEFINE_FAKE_VALUE_FUNC(int, fota_download_util_download_cancel);
DEFINE_FAKE_VALUE_FUNC(int, fota_download_util_image_schedule, enum dfu_target_image_type);
DEFINE_FAKE_VALUE_FUNC(int, fota_download_util_image_reset, enum dfu_target_image_type);
DEFINE_FAKE_VALUE_FUNC(int, fota_download_util_apply_update, enum dfu_target_image_type);

static int lwm2m_engine_init(void)
{
	STRUCT_SECTION_FOREACH(lwm2m_init_func, init) {
		int ret = init->f();

		if (ret) {
			printf("Init function %p returned %d\n", init, ret);
		}
	}
	return 0;
}

SYS_INIT(lwm2m_engine_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

FUNC_NORETURN void sys_reboot(int types)
{
	while (1) {
		k_sleep(K_SECONDS(1));
	}
}
