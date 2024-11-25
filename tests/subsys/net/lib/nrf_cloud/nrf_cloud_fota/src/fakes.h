/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/fff.h>
#include <zephyr/ztest.h>

/* define <zephyr/sys/reboot.h> without noreturn flag */
#define SYS_REBOOT_WARM 0
#define SYS_REBOOT_COLD 1
void sys_reboot(int type);

DEFINE_FFF_GLOBALS;

FAKE_VOID_FUNC(nrf_cloud_fota_job_free, struct nrf_cloud_fota_job_info *const);
FAKE_VALUE_FUNC(int, mqtt_publish, struct mqtt_client *, const struct mqtt_publish_param *);
FAKE_VALUE_FUNC(int, nrf_cloud_obj_cloud_encode, struct nrf_cloud_obj *const);
FAKE_VALUE_FUNC(int, nrf_cloud_obj_free, struct nrf_cloud_obj *const);
FAKE_VALUE_FUNC(int, nrf_cloud_obj_cloud_encoded_free, struct nrf_cloud_obj *const);
FAKE_VALUE_FUNC(int, nrf_cloud_obj_fota_job_update_create, struct nrf_cloud_obj *const , const struct nrf_cloud_fota_job *const);
FAKE_VALUE_FUNC(int, nrf_cloud_bootloader_fota_slot_set, struct nrf_cloud_settings_fota_job * const);
FAKE_VALUE_FUNC(int, nrf_cloud_fota_settings_save, struct nrf_cloud_settings_fota_job *);
FAKE_VALUE_FUNC(int, nrf_cloud_pending_fota_job_process, struct nrf_cloud_settings_fota_job * const , bool * const);
FAKE_VALUE_FUNC(bool, nrf_cloud_fota_is_type_modem, const enum nrf_cloud_fota_type);
FAKE_VOID_FUNC(sys_reboot, int);
FAKE_VOID_FUNC(nrf_cloud_download_end);
FAKE_VALUE_FUNC(int, nrf_cloud_fota_settings_load, struct nrf_cloud_settings_fota_job *);
FAKE_VALUE_FUNC(int, nrf_cloud_codec_init, struct nrf_cloud_os_mem_hooks *);
FAKE_VALUE_FUNC(int, fota_download_init, fota_download_callback_t);
FAKE_VALUE_FUNC(int, nrf_cloud_fota_smp_client_init, const void *);
FAKE_VOID_FUNC(nrf_cloud_free, void *);

FAKE_VOID_FUNC(nrf_cloud_fota_cb_handler, const struct nrf_cloud_fota_evt * const);

int fake_nrf_cloud_pending_fota_job_process__set_reboot(
	struct nrf_cloud_settings_fota_job * const job,
	bool * const reboot_required)
{
	ARG_UNUSED(job);
	*reboot_required = true;
	return 0;
};
