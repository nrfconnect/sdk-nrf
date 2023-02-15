/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief This source file is an adapter layer from MCUBoot's boot_nv_security_counter
 *        API onto the NCS/NSIB library bl_storage.
 */

#include <stdint.h>
#include <bl_storage.h>
#include "bootutil/fault_injection_hardening.h"

fih_int boot_nv_security_counter_init(void)
{
	return FIH_SUCCESS;
}

fih_int boot_nv_security_counter_get(uint32_t image_id, fih_int *security_cnt)
{
	int err;
	uint16_t cur_sec_cnt;

	if (security_cnt == NULL) {
		return FIH_FAILURE;
	}

	/* We support only one counter in MCUBOOT */
	if (image_id > 0) {
		return FIH_FAILURE;
	}

	err = get_monotonic_counter(BL_MONOTONIC_COUNTERS_DESC_MCUBOOT_ID0, &cur_sec_cnt);
	if (err != 0) {
		return FIH_FAILURE;
	}

	*security_cnt = cur_sec_cnt;

	return FIH_SUCCESS;
}

int32_t boot_nv_security_counter_update(uint32_t image_id, uint32_t img_security_cnt)
{
	/* We only support 16 bits image counters */
	if ((img_security_cnt & 0xFFFF0000) > 0) {
		return -1;
	}

	/* MCUBoot permits calling this function with img_security_cnt
	 * equal to the currently stored value (security_cnt). But
	 * set_monotonic_counter does not permit this, so we check the old
	 * value before updating.
	 */
	fih_int security_cnt;

	fih_int fih_err = boot_nv_security_counter_get(image_id, &security_cnt);

	if (fih_err != FIH_SUCCESS) {
		return fih_err;
	}

	if (security_cnt == (uint16_t)img_security_cnt) {
		return FIH_SUCCESS;
	}

	return set_monotonic_counter(BL_MONOTONIC_COUNTERS_DESC_MCUBOOT_ID0,
				     (uint16_t)img_security_cnt);
}
