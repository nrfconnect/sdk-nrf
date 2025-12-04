/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/storage/flash_map.h>
#include <zephyr/init.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/sys/reboot.h>
#include <uicr/uicr.h>

static const uint8_t local_periphconf_inc[] = {
#include <periphconf.bin.inc>
};

#define MRAM_BASE 0x0e000000

static struct uicr_periphconf_entry *global_periphconf =
	(struct uicr_periphconf_entry *)(MRAM_BASE +
					 DT_REG_ADDR(DT_NODELABEL(periphconf_partition)));

int check_and_copy_local_periphconf_to_global(void)
{
	const struct uicr_periphconf_entry *local_periphconf =
		(struct uicr_periphconf_entry *)local_periphconf_inc;
	const size_t local_periphconf_len =
		sizeof(local_periphconf_inc) / sizeof(struct uicr_periphconf_entry);

	for (volatile int i = 0; i < local_periphconf_len; i++) {
		if (local_periphconf[i].regptr == 0xFFFFFFFF &&
		    local_periphconf[i].value == 0xFFFFFFFF) {
			return 0;
		} else if (local_periphconf[i].regptr == global_periphconf[i].regptr &&
			   local_periphconf[i].value == global_periphconf[i].value) {
			continue;
		} else {
			for (volatile int j = i; j < local_periphconf_len; j++) {
				sys_write32(local_periphconf[j].regptr,
					    (uintptr_t)&global_periphconf[j].regptr);
				sys_write32(local_periphconf[j].value,
					    (uintptr_t)&global_periphconf[j].value);
			}

			sys_reboot(SYS_REBOOT_WARM);

			return -1;
		}
	}
	return 0;
}

/* Devices are initialized with PRE_KERNEL_1, and this needs to be fixed by then, so use EARLY */
SYS_INIT(check_and_copy_local_periphconf_to_global, EARLY, 0);
