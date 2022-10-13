/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SOC_INTEL_ADSP_COMMON_SOC_H_
#define ZEPHYR_SOC_INTEL_ADSP_COMMON_SOC_H_

#include <string.h>
#include <errno.h>
#include <zephyr/arch/xtensa/cache.h>
#include <zephyr/linker/sections.h>

#include <adsp_interrupt.h>

/* DSP Wall Clock Timers (0 and 1) */
#define DSP_WCT_IRQ(x) \
	SOC_AGGREGATE_IRQ((22 + x), CAVS_L2_AGG_INT_LEVEL2)

#define DSP_WCT_CS_TA(x)			BIT(x)
#define DSP_WCT_CS_TT(x)			BIT(4 + x)


extern void z_soc_mp_asm_entry(void);
extern void soc_mp_startup(uint32_t cpu);
extern void soc_start_core(int cpu_num);

extern bool soc_cpus_active[CONFIG_MP_NUM_CPUS];

/* Legacy cache APIs still used in a few places */
#define SOC_DCACHE_FLUSH(addr, size)		\
	z_xtensa_cache_flush((addr), (size))
#define SOC_DCACHE_INVALIDATE(addr, size)	\
	z_xtensa_cache_inv((addr), (size))
#define z_soc_cached_ptr(p) arch_xtensa_cached_ptr(p)
#define z_soc_uncached_ptr(p) arch_xtensa_uncached_ptr(p)

/**
 * @brief Halts and offlines a running CPU
 *
 * Enables power gating on the specified CPU, which cannot be the
 * current CPU or CPU 0.  The CPU must be idle; no application threads
 * may be runnable on it when this function is called (or at least the
 * CPU must be guaranteed to reach idle in finite time without
 * deadlock).  Actual CPU shutdown can only happen in the context of
 * the idle thread, and synchronization is an application
 * responsibility.  This function will hang if the other CPU fails to
 * reach idle.
 *
 * @note On older cAVS hardware, core power is controlled by the host.
 * This function must still be called for OS bookkeeping, but it is
 * insufficient without application coordination (and careful
 * synchronization!) with the host x86 environment.
 *
 * @param id CPU to halt, not current cpu or cpu 0
 * @return 0 on success, -EINVAL on error
 */
int soc_adsp_halt_cpu(int id);



static ALWAYS_INLINE void z_idelay(int n)
{
	while (n--) {
		__asm__ volatile("nop");
	}
}

/* memcopy used by boot loader */
static ALWAYS_INLINE void bmemcpy(void *dest, void *src, size_t bytes)
{
	uint32_t *d = (uint32_t *)dest;
	uint32_t *s = (uint32_t *)src;

	z_xtensa_cache_inv(src, bytes);
	for (size_t i = 0; i < (bytes >> 2); i++)
		d[i] = s[i];

	z_xtensa_cache_flush(dest, bytes);
}

/* bzero used by bootloader */
static ALWAYS_INLINE void bbzero(void *dest, size_t bytes)
{
	uint32_t *d = (uint32_t *)dest;

	for (size_t i = 0; i < (bytes >> 2); i++)
		d[i] = 0;

	z_xtensa_cache_flush(dest, bytes);
}


#endif /* ZEPHYR_SOC_INTEL_ADSP_COMMON_SOC_H_ */
