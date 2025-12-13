/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/sys/reboot.h>
#include <uicr/uicr.h>

#define PERIPHCONF_NODE	       DT_NODELABEL(periphconf_partition)
#define MRAM_NODE	       DT_GPARENT(PERIPHCONF_NODE)
#define MRAM_WORD_SIZE	       DT_PROP(MRAM_NODE, write_block_size)
#define GLOBAL_PERIPHCONF_ADDR DT_REG_ADDR(MRAM_NODE) + DT_REG_ADDR(PERIPHCONF_NODE)

/* The global PERIPHCONF blob is the one pointed to by UICR.PERIPHCONF and hence the one which
 * will be applied by IronSide SE.
 */
static struct uicr_periphconf_entry *global_periphconf =
	(struct uicr_periphconf_entry *)(GLOBAL_PERIPHCONF_ADDR);

/* The local PERIPHCONF contained within the image firmware blob. This could be out of sync with
 * the global PERIPHCONF if a DFU has been updated with a change in PERIPHCONF.
 */
static const uint8_t local_periphconf_inc[] = {
#include <periphconf.bin.inc>
};

/* Since we are executing pre-kernel, we cannot use the proper MRAM driver. Hence we rely on
 * the blob being MRAM word size aligned since write are committed when the last 4-byte word
 * in the 16-byte MRAM word is written.
 */
BUILD_ASSERT(sizeof(local_periphconf_inc) % MRAM_WORD_SIZE == 0,
	     "PERIPHCONF blob is not MRAM word size aligned.");

/* Compare the local PERIPHCONF blob with the global one. If an entry is found that differs, copy
 * the remaining entries and trigger a restart. No action is taken if the blobs are equal.
 */
int check_and_copy_local_periphconf_to_global(void)
{
	const struct uicr_periphconf_entry *local_periphconf =
		(struct uicr_periphconf_entry *)local_periphconf_inc;
	const size_t local_periphconf_len =
		sizeof(local_periphconf_inc) / sizeof(struct uicr_periphconf_entry);

	for (int i = 0; i < local_periphconf_len; i++) {
		if (local_periphconf[i].regptr == 0xFFFFFFFF &&
		    local_periphconf[i].value == 0xFFFFFFFF) {
			return 0;
		} else if (local_periphconf[i].regptr == global_periphconf[i].regptr &&
			   local_periphconf[i].value == global_periphconf[i].value) {
			continue;
		} else {
			for (int j = i; j < local_periphconf_len; j++) {
				sys_write32(local_periphconf[j].regptr,
					    (uintptr_t)&global_periphconf[j].regptr);
				sys_write32(local_periphconf[j].value,
					    (uintptr_t)&global_periphconf[j].value);
			}

			/* IronSide applies the configuration in PERIPHCONF before booting the
			 * local domains, so a reboot is needed for the changes to be applied.
			 */
			sys_reboot(SYS_REBOOT_WARM);

			CODE_UNREACHABLE;

			return -1;
		}
	}

	return 0;
}

/* Devices are initialized with PRE_KERNEL_1, and this needs to be fixed by then, so use EARLY */
SYS_INIT(check_and_copy_local_periphconf_to_global, EARLY, 0);
