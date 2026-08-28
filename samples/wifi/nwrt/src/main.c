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

/* Temporary bring-up debug exported by drivers/timer/nrf_grtc_timer.c. */
extern volatile uint32_t z_grtc_dbg_fires;
extern volatile uint64_t z_grtc_dbg_cc_val[4];
extern volatile uint64_t z_grtc_dbg_last_count[4];
extern volatile uint64_t z_grtc_dbg_now[4];
extern volatile uint32_t z_grtc_dbg_dticks[4];
extern volatile uint32_t z_grtc_dbg_sets;
extern volatile uint32_t z_grtc_dbg_set_ticks[4];
extern volatile uint64_t z_grtc_dbg_set_cc[4];
extern volatile uint64_t z_grtc_dbg_set_now[4];
extern volatile uint32_t z_grtc_dbg_set_rawl[4];
extern volatile uint32_t z_grtc_dbg_set_rawh[4];

/*
 * Temporary GRTC timebase sanity check. k_busy_wait() is a pure CPU-cycle delay
 * (nrfx_coredep), independent of the GRTC, so it gives a trustworthy ~1s wall
 * reference. k_sleep() is GRTC driven. On a healthy timer both windows report
 * ~1000 ms of uptime and ~1e6 cycles. If k_sleep() returns with far fewer
 * cycles than a real second, the GRTC compare is firing early (spurious tick
 * jump), which is what collapses the driver's 30s IPC-bind timeout.
 */
static void nwrt_clock_sanity(void)
{
    uint32_t c0, c1;
    int64_t u0, u1;

    /* Identify the running image and the timebase constants. If the compile-
     * time macro and the runtime function disagree, the frequency is read at
     * runtime; if either is below CONFIG_SYS_CLOCK_TICKS_PER_SEC, CYC_PER_TICK
     * truncates to 0 and every timeout degenerates.
     */
    printk("nwrt: build %s %s\n", __DATE__, __TIME__);
    printk("nwrt: cfg_hw=%d fn_hw=%d ticks=%d cyc_per_tick=%d\n",
           CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
           sys_clock_hw_cycles_per_sec(),
           CONFIG_SYS_CLOCK_TICKS_PER_SEC,
           CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC);

    /* Is the syscounter even advancing while the CPU is active? Read it twice
     * around a pure CPU-cycle delay (nrfx_coredep, independent of the GRTC).
     */
    c0 = k_cycle_get_32();
    k_busy_wait(1000000);
    c1 = k_cycle_get_32();
    printk("nwrt: busy_wait(1s): raw cycles %u -> %u (+%u)\n", c0, c1, c1 - c0);

    u0 = k_uptime_get();
    c0 = k_cycle_get_32();
    k_sleep(K_SECONDS(1));
    u1 = k_uptime_get();
    c1 = k_cycle_get_32();
    printk("nwrt: k_sleep(1s):  uptime +%lld ms, cycles +%u\n",
           u1 - u0, c1 - c0);

    /* The driver reads the indexed GRTC.SYSCOUNTER[GRTC_IRQ_GROUP], not the
     * non-indexed alias at 0x514/0x518. Probe the alias and both candidate
     * indices (1 = non-secure app, 2 = secure app) to see which one the driver
     * hits and whether the indexed L/H words are valid. GRTC base 0x500E2000.
     */
    {
#define GRTC_BASE 0x500E2000UL
#define RD(off)   (*(volatile uint32_t *)(GRTC_BASE + (off)))
        uint32_t al = RD(0x514), ah = RD(0x518);
        uint32_t l1 = RD(0x730), h1 = RD(0x734);
        uint32_t l2 = RD(0x740), h2 = RD(0x744);

        printk("nwrt: alias    L=%u H=0x%08x\n", al, ah);
        printk("nwrt: SYSC[1]  L=0x%08x H=0x%08x (V=%u LOADED=%u BUSY=%u OVF=%u)\n",
               l1, h1, h1 & 0xFFFFFU, (h1 >> 29) & 1U, (h1 >> 30) & 1U,
               (h1 >> 31) & 1U);
        printk("nwrt: SYSC[2]  L=0x%08x H=0x%08x (V=%u LOADED=%u BUSY=%u OVF=%u)\n",
               l2, h2, h2 & 0xFFFFFU, (h2 >> 29) & 1U, (h2 >> 30) & 1U,
               (h2 >> 31) & 1U);
#undef RD
#undef GRTC_BASE
    }

    printk("nwrt: grtc sets=%u fires=%u\n", z_grtc_dbg_sets, z_grtc_dbg_fires);
    for (uint32_t i = 0; i < 4U && i < z_grtc_dbg_sets; i++) {
        printk("nwrt:  set[%u] ticks=%u now=%llu (counter) rawL=0x%08x rawH=0x%08x\n",
               i, z_grtc_dbg_set_ticks[i],
               (unsigned long long)z_grtc_dbg_set_now[i],
               z_grtc_dbg_set_rawl[i], z_grtc_dbg_set_rawh[i]);
    }
    for (uint32_t i = 0; i < 4U && i < z_grtc_dbg_fires; i++) {
        printk("nwrt:  fire[%u] cc_val=%llu last_count=%llu now=%llu dticks=%u\n",
               i, (unsigned long long)z_grtc_dbg_cc_val[i],
               (unsigned long long)z_grtc_dbg_last_count[i],
               (unsigned long long)z_grtc_dbg_now[i],
               z_grtc_dbg_dticks[i]);
    }
}

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
    nwrt_clock_sanity();

#if !defined(CONFIG_NWRT_SWD_ONLY)
    nwrt_wait_rpu_ready();
#endif

    nwrt_shadow_init();

    for (;;)
    {
        nwrt_shadow_poll();
    }
}
