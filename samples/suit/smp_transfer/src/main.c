/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <dk_buttons_and_leds.h>
#ifdef CONFIG_MGMT_SUITFU
#include <suitfu_mgmt.h>
#endif /* CONFIG_MGMT_SUITFU */
#include <zephyr/storage/flash_map.h>
#include "common.h"
#include <zephyr/drivers/uart.h>
#include <zephyr/mgmt/mcumgr/transport/serial.h>

#ifdef CONFIG_SSF_SUIT_SERVICE_ENABLED
#include <sdfw/sdfw_services/suit_service.h>
#endif /* CONFIG_SSF_SUIT_SERVICE_ENABLED */

#define DFU_PARTITION_OFFSET  FIXED_PARTITION_OFFSET(dfu_partition)
#define DFU_PARTITION_SIZE    FIXED_PARTITION_SIZE(dfu_partition)
#define DFU_PARTITION_ADDRESS suit_plat_mem_nvm_ptr_get(DFU_PARTITION_OFFSET)

int main(void)
{
	int ret = dk_leds_init();

	if (ret) {
		printk("Cannot init LEDs (err: %d)\r\n", ret);
	}

	printk("Hello world from %s version: %d\r\n", CONFIG_BOARD,
	       CONFIG_SUIT_ENVELOPE_SEQUENCE_NUM);

#if (CONFIG_SSF_SUIT_SERVICE_ENABLED) && (CONFIG_SUIT_ENVELOPE_SEQUENCE_NUM == 1) &&               \
	(!CONFIG_MGMT_SUITFU)
	suit_plat_mreg_t update_candidate[] = {
		{
			.mem = DFU_PARTITION_ADDRESS,
			.size = DFU_PARTITION_SIZE,
		},
	};

	printk("Validate candidate: %p (0x%x)\r\n", update_candidate[0].mem,
	       update_candidate[0].size);
	bool dfu_partition_valid =
		suit_validate_candidate(update_candidate[0].mem, update_candidate[0].size);

	printk("DFU update candidate found in DFU partition: %d\r\n", dfu_partition_valid);
	if (dfu_partition_valid) {
		int ret = suit_trigger_update(update_candidate, ARRAY_SIZE(update_candidate));

		printk("DFU triggered with status: %d\r\n", ret);
	}
#endif

	if (IS_ENABLED(CONFIG_MCUMGR_TRANSPORT_BT)) {
		start_smp_bluetooth_adverts();
	}
	k_msleep(2000);

        // SUIT_MANIFEST_SEC_TOP = 0x10,
        // SUIT_MANIFEST_SEC_SDFW = 0x11,
        // SUIT_MANIFEST_SEC_SYSCTRL = 0x12,
        // SUIT_MANIFEST_APP_ROOT = 0x20,

	suit_manifest_role_t role = SUIT_MANIFEST_SEC_SDFW;
	suit_ssf_manifest_class_info_t class_info = {0};
	unsigned int seq_num = 0;
	suit_semver_raw_t semver_raw = {0};
	suit_digest_status_t digest_status = SUIT_DIGEST_UNKNOWN;
	int digest_alg_id = 0;
	uint8_t digest_buf[32] = {0};
	suit_plat_mreg_t digest;
	digest.mem = digest_buf;
	digest.size = sizeof(digest_buf);

	suit_get_supported_manifest_info(role, &class_info);

	suit_manifest_class_id_t class_id = {
		.raw = {0xd9, 0x6b, 0x40, 0xb7, 0x09, 0x2b, 0x5c, 0xd1, 0xa5, 0x9f, 0x9a, 0xf8,
			     0x0c, 0x33, 0x7e, 0xba}
	};

	suit_get_installed_manifest_info(&class_id, &seq_num, &semver_raw,
					      &digest_status, &digest_alg_id, &digest);

	while (1) {
		for (int i = 0; i < CONFIG_SUIT_ENVELOPE_SEQUENCE_NUM; i++) {
			dk_set_led_on(DK_LED1);
			k_msleep(250);
			dk_set_led_off(DK_LED1);
			k_msleep(250);
		}

		k_msleep(5000);
	}

	return 0;
}
