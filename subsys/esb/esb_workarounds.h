/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ESB_WORKAROUNDS_H__
#define ESB_WORKAROUNDS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <zephyr/kernel.h>

#include <hal/nrf_radio.h>

#if NRF54H_ERRATA_216_ENABLE_WORKAROUND
#include <zephyr/drivers/mbox.h>
#endif /* NRF54H_ERRATA_216_ENABLE_WORKAROUND */

/* nRF52 Workaround 182 */
static void esb_apply_nrf52_182(void);

/* nRF52 Workaround 143 */
static void esb_apply_nrf52_143(uint32_t base_address_mask);

/* nRF54H Workaround 84 */
/* HMPAN-84 is implemented in the ESB samples in the clocks_start function */

/* nRF54H Workaround 103 */
static void esb_apply_nrf54h_103(void);

/* nRF54H Workaround 216 */
static void esb_apply_nrf54h_216(bool on);

/* nRF54H Workaround 229 */
static void esb_apply_nrf54h_229(void);

/* nRF54L Workaround 20 */
/* MLTPAN-20 is implemented in the ESB samples in the clocks_start function */

/* nRF54L Workaround 39 */
/* MLTPAN-39 is implemented in the ESB samples in the clocks_start function */

#if NRF52_ERRATA_143_ENABLE_WORKAROUND
static void esb_apply_nrf52_143(uint32_t base_address_mask)
{
	/* Load the two addresses before comparing them to ensure
	 * defined ordering of volatile accesses.
	 */
	uint32_t addr0 = nrf_radio_base0_get(NRF_RADIO) & base_address_mask;
	uint32_t addr1 = nrf_radio_base1_get(NRF_RADIO) & base_address_mask;

	if (addr0 == addr1) {
		uint32_t radio_prefix0 = nrf_radio_prefix0_get(NRF_RADIO);
		uint32_t radio_prefix1 = nrf_radio_prefix1_get(NRF_RADIO);

		uint8_t prefix0 = radio_prefix0 & RADIO_PREFIX0_AP0_Msk;
		uint8_t prefix1 = (radio_prefix0 & RADIO_PREFIX0_AP1_Msk) >> RADIO_PREFIX0_AP1_Pos;
		uint8_t prefix2 = (radio_prefix0 & RADIO_PREFIX0_AP2_Msk) >> RADIO_PREFIX0_AP2_Pos;
		uint8_t prefix3 = (radio_prefix0 & RADIO_PREFIX0_AP3_Msk) >> RADIO_PREFIX0_AP3_Pos;
		uint8_t prefix4 = radio_prefix1 & RADIO_PREFIX1_AP4_Msk;
		uint8_t prefix5 = (radio_prefix1 & RADIO_PREFIX1_AP5_Msk) >> RADIO_PREFIX1_AP5_Pos;
		uint8_t prefix6 = (radio_prefix1 & RADIO_PREFIX1_AP6_Msk) >> RADIO_PREFIX1_AP6_Pos;
		uint8_t prefix7 = (radio_prefix1 & RADIO_PREFIX1_AP7_Msk) >> RADIO_PREFIX1_AP7_Pos;

		if ((prefix0 == prefix1) || (prefix0 == prefix2) ||
		    (prefix0 == prefix3) || (prefix0 == prefix4) ||
		    (prefix0 == prefix5) || (prefix0 == prefix6) ||
		    (prefix0 == prefix7)) {
			/* This will cause a 3dBm sensitivity loss,
			 * avoid using such address combinations if possible.
			 */
			*(volatile uint32_t *)0x40001774 =
				((*(volatile uint32_t *)0x40001774) & 0xfffffffe) | 0x01000000;
		}
	}
}
#else
static void esb_apply_nrf52_143(uint32_t base_address_mask)
{
	ARG_UNUSED(base_address_mask);
}
#endif /* NRF52_ERRATA_143_ENABLE_WORKAROUND */

#if NRF52_ERRATA_182_ENABLE_WORKAROUND
static void esb_apply_nrf52_182(void)
{
	*(volatile uint32_t *)0x4000173C |= (1 << 10);
}
#else
static void esb_apply_nrf52_182(void)
{
	/* No implementation needed */
}
#endif /* NRF52_ERRATA_182_ENABLE_WORKAROUND */

#if NRF54H_ERRATA_103_ENABLE_WORKAROUND
static void esb_apply_nrf54h_103(void)
{
	if ((*(volatile uint32_t *)0x5302C8A0 == 0x80000000) ||
						(*(volatile uint32_t *)0x5302C8A0 == 0x0058120E)) {
		*(volatile uint32_t *)0x5302C8A0 = 0x0058090E;
	}

	*(volatile uint32_t *)0x5302C8A4 = 0x00F8AA5F;
	*(volatile uint32_t *)0x5302C8A8 = 0x00C00030;
	*(volatile uint32_t *)0x5302C8AC = 0x00A80030;
	*(volatile uint32_t *)0x5302C7AC = 0x8672827A;
	*(volatile uint32_t *)0x5302C7B0 = 0x7E768672;
	*(volatile uint32_t *)0x5302C7B4 = 0x0406007E;
	*(volatile uint32_t *)0x5302C7E4 = 0x0412C384;
}
#else
static void esb_apply_nrf54h_103(void)
{
	/* No implementation needed */
}
#endif /* NRF54H_ERRATA_103_ENABLE_WORKAROUND */

#if NRF54H_ERRATA_216_ENABLE_WORKAROUND
static inline void esb_apply_nrf54h_216(bool on)
{
	static const struct mbox_dt_spec on_channel =
			MBOX_DT_SPEC_GET(DT_NODELABEL(cpurad_cpusys_errata216_mboxes), on_req);
	static const struct mbox_dt_spec off_channel =
			MBOX_DT_SPEC_GET(DT_NODELABEL(cpurad_cpusys_errata216_mboxes), off_req);
	const struct mbox_dt_spec *spec = on ? &on_channel : &off_channel;

	if (!IS_ENABLED(NRF54H_ERRATA_216_ENABLE_WORKAROUND)) {
		return;
	}

	if (mbox_send_dt(spec, NULL)) {
		/* Should not happen. */
		__ASSERT_NO_MSG(false);
	}
}
#else
static void esb_apply_nrf54h_216(bool on)
{
	ARG_UNUSED(on);
}
#endif /* NRF54H_ERRATA_216_ENABLE_WORKAROUND*/

#if defined(CONFIG_SOC_SERIES_NRF54HX)
static void esb_apply_nrf54h_229(void)
{
	if (*(volatile uint32_t *)0x0FFFE46C == 0x0) {
		*(volatile uint32_t *)0x5302C7D8 = 0x00000004;
	}
}
#else
static void esb_apply_nrf54h_229(void)
{
	/* No implementation needed */
}
#endif /* (CONFIG_SOC_SERIES_NRF54HX) */

#ifdef __cplusplus
}
#endif

#endif /* ESB_WORKAROUNDS_H__ */
