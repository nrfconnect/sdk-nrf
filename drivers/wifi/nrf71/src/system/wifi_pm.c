/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief File containing power management definitions to control the power
 * state of the Wi-Fi subsystem.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>

#include <hal/nrf_lrcconf.h>
#include <hal/nrf_vpr.h>
#include <nrfx.h>

#include <system/wifi_pm.h>

LOG_MODULE_REGISTER(wifi_pm, CONFIG_WIFI_NRF71_LOG_LEVEL);

/* MEMCONF_WIFI non-secure instance */
#define MEMCONF_WIFI_NS_BASE            ((NRF_MEMCONF_Type *) 0x400F0000UL)

/* WIFISYS RPUSYS instance */
#define WIFICORE_RPUSYS_BASE            0x48080000UL
#define WIFI_CLOCK_ENABLE_SET_ALL       (WIFICORE_RPUSYS_BASE + 0x1B004UL)
#define WIFI_CLOCK_ENABLE_CLR_ALL       (WIFICORE_RPUSYS_BASE + 0x1B008UL)
#define WIFI_AUTOCG_SET_ALL             (WIFICORE_RPUSYS_BASE + 0x1B014UL)
#define WIFI_AUTOCG_CLR_ALL             (WIFICORE_RPUSYS_BASE + 0x1B018UL)
#define WIFI_RF_CLOCK_CTRL_SET          (WIFICORE_RPUSYS_BASE + 0x1B024UL)
#define WIFI_RF_CLOCK_CTRL_CLR          (WIFICORE_RPUSYS_BASE + 0x1B028UL)
#define WIFI_UPSCALE_REQ                (WIFICORE_RPUSYS_BASE + 0x1B038UL)
#define WIFI_RAM_ENABLE_SET_ALL         (WIFICORE_RPUSYS_BASE + 0x1B044UL)
#define WIFI_RAM_ENABLE_CLR_ALL         (WIFICORE_RPUSYS_BASE + 0x1B048UL)
#define WIFI_RESET_STATUS1              (WIFICORE_RPUSYS_BASE + 0x1B1A0UL)
#define WIFI_RESET_ASSERT1              (WIFICORE_RPUSYS_BASE + 0x1B1C0UL)
#define WIFI_RESET_DEASSERT1            (WIFICORE_RPUSYS_BASE + 0x1B1E0UL)

/* Enable-all trigger and RF internal+external clock mask */
#define WIFI_ENABLE_ALL_TRIGGER	        1UL
#define WIFI_RF_CLOCK_INT_EXT           0x3UL

/* System off token for Wi-Fi subsystem */
#define WIFI_SYSTEM_OFF_TOKEN           0x280002FCUL

/* RPU reset poll timeout */
#define RPU_RESET_TIMEOUT_US            3000

int nrf_wifi_power_on(void)
{
	LOG_DBG("Powering on Wi-Fi subsystem");

	/* Assert the LRC power-on request for the WIFICORE domain */
	nrf_lrcconf_poweron_force_set(NRF_WIFICORE_LRCCONF_LRC0,
				      NRF_LRCCONF_POWER_MAIN, true);

	/* Clear the system-off marker and restore Wi-Fi clocks, RAMs and
	 * automatic clock gating.
	 */
	sys_write32(0, WIFI_SYSTEM_OFF_TOKEN);
	sys_write32(WIFI_ENABLE_ALL_TRIGGER, WIFI_AUTOCG_SET_ALL);
	sys_write32(WIFI_ENABLE_ALL_TRIGGER, WIFI_RAM_ENABLE_SET_ALL);
	sys_write32(WIFI_ENABLE_ALL_TRIGGER, WIFI_CLOCK_ENABLE_SET_ALL);
	sys_write32(WIFI_RF_CLOCK_INT_EXT, WIFI_RF_CLOCK_CTRL_SET);
	__DSB();

	nrf_vpr_initpc_set(NRF_WIFICORE_LMAC_VPR,
			   (uint32_t)(uintptr_t)NRF_WICR->FIRMWARE.LMACINITPC);
	nrf_vpr_cpurun_set(NRF_WIFICORE_LMAC_VPR, true);

	return 0;
}

static void vpr_reset(NRF_VPR_Type *vpr)
{
	nrf_vpr_debugif_dmcontrol_mask_set(vpr, VPR_DEBUGIF_DMCONTROL_DMACTIVE_Msk |
						VPR_DEBUGIF_DMCONTROL_NDMRESET_Msk |
						VPR_DEBUGIF_DMCONTROL_ACKHAVERESET_Msk);

	nrf_vpr_debugif_dmcontrol_mask_set(vpr, VPR_DEBUGIF_DMCONTROL_ResetValue);
}

int nrf_wifi_power_off(void)
{
	uint32_t timeout_us = RPU_RESET_TIMEOUT_US;

	LOG_DBG("Powering off Wi-Fi subsystem");

	/* Halt the LMAC and UMAC processors and reset their VPR debug state */
	nrf_vpr_cpurun_set(NRF_WIFICORE_LMAC_VPR, false);
	nrf_vpr_cpurun_set(NRF_WIFICORE_UMAC_VPR, false);
	vpr_reset(NRF_WIFICORE_LMAC_VPR);
	vpr_reset(NRF_WIFICORE_UMAC_VPR);

	/* Force the next LMAC start to be classified as a cold boot. The RPU ROM
	 * checks these values before entering its startup sequence.
	 */
	MEMCONF_WIFI_NS_BASE->POWER[0].RET = 0;
	MEMCONF_WIFI_NS_BASE->POWER[0].RET2 = 0;
	MEMCONF_WIFI_NS_BASE->POWER[1].RET = 0;
	MEMCONF_WIFI_NS_BASE->POWER[1].RET2 = 0;

	/* Release the RF clock and voltage-upscale requests */
	sys_write32(WIFI_RF_CLOCK_INT_EXT, WIFI_RF_CLOCK_CTRL_CLR);
	sys_write32(0, WIFI_UPSCALE_REQ);

	/* Reset the RPU: assert, wait for the reset to take, then deassert */
	sys_write32(0xFFFFFFFF, WIFI_RESET_ASSERT1);
	__DSB();

	while ((sys_read32(WIFI_RESET_STATUS1) == 0) && timeout_us > 0) {
		k_busy_wait(1);
		timeout_us--;
	}

	if (timeout_us == 0) {
		LOG_WRN("Timed out waiting for Wi-Fi RESETSTATUS1");
		return -EIO;
	}

	sys_write32(0xFFFFFFFF, WIFI_RESET_DEASSERT1);

	/* Gate automatic clock-gating, all clocks and all RAMs */
	sys_write32(1, WIFI_AUTOCG_CLR_ALL);
	sys_write32(1, WIFI_CLOCK_ENABLE_CLR_ALL);
	sys_write32(1, WIFI_RAM_ENABLE_CLR_ALL);

	/* Disable retention for the WIFICORE active power domain so it starts from
	 * reset on the next power-on. The main domain retention is not configurable
	 * in hardware and is left as-is.
	 */
	nrf_lrcconf_retain_set(NRF_WIFICORE_LRCCONF_LRC0,
			       NRF_LRCCONF_POWER_DOMAIN_0, false);

	/* Drop the LRC power request */
	nrf_lrcconf_poweron_force_set(NRF_WIFICORE_LRCCONF_LRC0,
				      NRF_LRCCONF_POWER_MAIN |
				      NRF_LRCCONF_POWER_DOMAIN_0,
				      false);
	__DSB();

	return 0;
}
