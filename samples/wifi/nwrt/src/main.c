/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* @file
 * @brief nWRT - Wi-Fi radio test with SWD shadow control.
 *
 * The nRF71 Wi-Fi driver brings up FMAC/RPU at init. This app polls nwrt_shadow
 * from main (including while waiting for rpu_ctx). Host writes params + submit
 * (pending low 16 bits; optional seq high 16 bits for lab debug); firmware
 * applies and runs FMAC calls when RPU is ready.
 */

#include <nwrt_conf.h>
#include <nwrt_shadow.h>
#include <zephyr/kernel.h>

#define NWRT_RPU_READY_TIMEOUT_MS 30000

#if !defined(CONFIG_NWRT_SWD_ONLY)
static void nwrt_wait_rpu_ready(void)
{
    int64_t deadline = k_uptime_get() + NWRT_RPU_READY_TIMEOUT_MS;

    while (k_uptime_get() < deadline)
    {
        nwrt_shadow_poll();

        if (nwrt_fmac_dev_ctx() != NULL)
        {
            return;
        }

        k_msleep(10);
    }
}
#endif /* !CONFIG_NWRT_SWD_ONLY */

int main(void)
{
    /* Shadow is initialized at PRE_KERNEL_2; re-init here would race the poll
     * in nwrt_wait_rpu_ready() and drop an early host command.
     */
#if !defined(CONFIG_NWRT_SWD_ONLY)
    nwrt_wait_rpu_ready();
#endif

    for (;;)
    {
        nwrt_shadow_poll();
    }
}
