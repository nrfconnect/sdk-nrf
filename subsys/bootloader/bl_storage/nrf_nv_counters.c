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
#include "bootutil/bootutil_public.h"

fih_int boot_nv_security_counter_init(void)
{
	return FIH_SUCCESS;
}

fih_int boot_nv_security_counter_get(uint32_t image_id, fih_int *security_cnt)
{
	int err;
	uint16_t cur_sec_cnt;

	if (security_cnt == NULL) {
		FIH_RET(FIH_FAILURE);
	}

	/* We support only one counter in MCUBOOT */
	if (image_id > 0) {
		FIH_RET(FIH_FAILURE);
	}

	err = get_monotonic_counter(BL_MONOTONIC_COUNTERS_DESC_MCUBOOT_ID0, &cur_sec_cnt);
	if (err != 0) {
		FIH_RET(FIH_FAILURE);
	}

	*security_cnt = fih_int_encode(cur_sec_cnt);

	FIH_RET(FIH_SUCCESS);
}

int32_t boot_nv_security_counter_update(uint32_t image_id, uint32_t img_security_cnt)
{
	/* We only support 16 bits image counters */
	if ((img_security_cnt & 0xFFFF0000) > 0) {
		return -BOOT_EBADARGS;
	}

	/* MCUBoot permits calling this function with img_security_cnt
	 * equal to the currently stored value (security_cnt). But
	 * set_monotonic_counter does not permit this, so we check the old
	 * value before updating.
	 */
	fih_int security_cnt;

	fih_int fih_err;
	int err;

	FIH_CALL(boot_nv_security_counter_get, fih_err, image_id, &security_cnt);
	if (FIH_NOT_EQ(fih_err, FIH_SUCCESS)) {
		return -BOOT_EBADSTATUS;
	}

	if (fih_int_decode(security_cnt) == img_security_cnt) {
		return 0;
	}

	err = set_monotonic_counter(BL_MONOTONIC_COUNTERS_DESC_MCUBOOT_ID0,
				    (uint16_t)img_security_cnt);

	return err == 0 ? 0 : -BOOT_EBADSTATUS;
}
