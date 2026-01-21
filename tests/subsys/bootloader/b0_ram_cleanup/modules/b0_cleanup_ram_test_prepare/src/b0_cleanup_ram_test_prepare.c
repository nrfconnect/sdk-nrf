/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <hal/nrf_timer.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/drivers/retained_mem.h>

#define CLEANUP_RAM_GAP_START ((uint32_t)__ramfunc_start)
#define CLEANUP_RAM_GAP_END ((uint32_t) __ramfunc_end)

/* Only the RAM outside of the defined linker sections will be populated,
 * as modifying the defined linker sections could lead to NSIB malfunction.
 */
#define RAM_TO_POPULATE_START ((uint32_t) _image_ram_end)
#define RAM_TO_POPULATE_END (CONFIG_SRAM_SIZE * 1024)
#define VALUE_TO_POPULATE 0xDEADBEAF

/* As peripheral registers are not cleared during RAM cleanup, we can
 * use a register of a peripheral unused by NSIB to save data which can then
 * be read by the application.
 * We use this trick to save the start and end addresses of the RAMFUNC region.
 * This region is not erased during the RAM cleanup, as it contains the code responsible
 * for performing the RAM cleanup, so it must be skipped when checking if the RAM cleanup
 * has been successfully performed.
 * The TIMER00 is not used by NSIB, and is therefore its CC[0] and CC[1] registers
 * are used for this purpose.
 */
#define RAMFUNC_START_SAVE_REGISTER NRF_TIMER00->CC[0]
#define RAMFUNC_END_SAVE_REGISTER NRF_TIMER00->CC[1]

#define RETAINED_DATA "RETAINED"

static int populate_ram(void)
{
	RAMFUNC_START_SAVE_REGISTER = CLEANUP_RAM_GAP_START;
	RAMFUNC_END_SAVE_REGISTER = CLEANUP_RAM_GAP_END;

	for (uint32_t addr = RAM_TO_POPULATE_START; addr < RAM_TO_POPULATE_END;
		 addr += sizeof(uint32_t)) {
		*(uint32_t *) addr = VALUE_TO_POPULATE;
	}

#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_retained_ram)
	const struct device *retained_mem_dev =
		DEVICE_DT_GET(DT_INST(0, zephyr_retained_ram));
	size_t retained_size;

	if (!device_is_ready(retained_mem_dev)) {
		printk("Retained memory device is not ready");
		return -ENODEV;
	}

	retained_size = retained_mem_size(retained_mem_dev);

	if (retained_size < strlen(RETAINED_DATA)) {
		printk("Retained memory size is too small");
		return -EINVAL;
	}

	retained_mem_write(retained_mem_dev, 0, RETAINED_DATA, strlen(RETAINED_DATA));
#endif
	return 0;
}

SYS_INIT(populate_ram, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
