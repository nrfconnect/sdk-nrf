/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */


/** @file
 * @brief This source file is an adapter layer from MCUBoot's boot_nv_security_counter
 *        API onto the NCS/NSIB library bl_monotonic_counters.
 */

#include <stdint.h>
#include <bl_monotonic_counters.h>
#include "bootutil/fault_injection_hardening.h"

static const uint32_t mcuboot_counter_id[3] = {
    BL_MONOTONIC_COUNTERS_DESC_MCUBOOT_ID0,
    BL_MONOTONIC_COUNTERS_DESC_MCUBOOT_ID1,
    BL_MONOTONIC_COUNTERS_DESC_MCUBOOT_ID2
};

fih_int boot_nv_security_counter_init(void) {
    return FIH_SUCCESS;
}

fih_int boot_nv_security_counter_get(uint32_t image_id, fih_int *security_cnt)
{
    int err;
    uint16_t cur_sec_cnt;

    if (security_cnt == NULL) {
        return FIH_FAILURE;
    }

    /* We support up to 3 counters in MCUBOOT */
    if (image_id > 2) {
        return FIH_FAILURE;
    }

    err = get_monotonic_counter(mcuboot_counter_id[image_id], &cur_sec_cnt);
    if (err != 0) {
        return FIH_FAILURE;
    }

    *security_cnt = cur_sec_cnt;

    return FIH_SUCCESS;
}

int32_t boot_nv_security_counter_update(uint32_t image_id,
                                        uint32_t img_security_cnt)
{
    int err;
    uint16_t counter_value;

    /* We only support 16 bits image counters */
    if( (img_security_cnt & 0xFFFF0000) > 0){
        return -1;
    }

    /* We support up to 3 counters in MCUBOOT */
    if (image_id > 2) {
        return FIH_FAILURE;
    }

    err = get_monotonic_counter(mcuboot_counter_id[image_id], &counter_value);
    if (err != 0) {
        return FIH_FAILURE;
    }

   /* MCUBoot requires that counter updates to the same value be permitted.
    * But set_monotonic_counter does not permit this, so we check the old
    * value before updating.
    */
    if (counter_value == (uint16_t) img_security_cnt){
        return FIH_SUCCESS;
    }

    return set_monotonic_counter(mcuboot_counter_id[image_id], (uint16_t) img_security_cnt);
}
