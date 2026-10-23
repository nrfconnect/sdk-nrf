/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief COEXC hardware initialization (CCMALLOW, CCCONF, TURNAROUND).
 *
 * Programs three COEXC register blocks: CCMALLOW, CCCONF, and TURNAROUND.
 * Two antenna layouts are supported (different CCMALLOW tables):
 *   COEX_SHARED_ANT_CFG    Wi-Fi and SR share one physical antenna
 *   COEX_SEPARATE_ANT_CFG  radios use separate antennas
 */

#include <errno.h>
#include <string.h>

#include <zephyr/logging/log.h>

/* Wi-Fi FMAC coexistence transport (cmd/event/reg access). */
#include <drivers/wifi/nrf71/nrf71_wifi_coex.h>

/* COEXC register addresses and antenna configuration types. */
#include <nrf71_coex_hw_regs.h>
/* CD2CM / CM2CD message structures shared with RPU firmware. */
#include <nrf71_coex_if.h>

LOG_MODULE_DECLARE(nrf71_sr_coex, CONFIG_NRF71_SR_COEX_DRIVER_LOG_LEVEL);

/*
 * CCCONF priority field: which PTI slot each COEXC client occupies. Wi-Fi uses
 * four clients (high/low priority Rx and Tx), SR uses two, and two more are
 * reserved for an external radio.
 */
#define WLAN_HIGH_PTI_RX_CCCONF_PTI 1U
#define WLAN_HIGH_PTI_TX_CCCONF_PTI 2U
#define WLAN_LOW_PTI_RX_CCCONF_PTI  4U
#define WLAN_LOW_PTI_TX_CCCONF_PTI  5U
#define SR_RX_CCCONF_PTI            2U
#define SR_TX_CCCONF_PTI            3U
#define EXT_RX_CCCONF_PTI           1U
#define EXT_TX_CCCONF_PTI           1U

/* Client turnaround (revoke) time in microseconds, and converted to COEXC clock ticks. */
#define TURNAROUND_WIFI_RX_IN_US    1U
#define TURNAROUND_WIFI_TX_IN_US    1U
#define TURNAROUND_SR_RX_IN_US      1U
#define TURNAROUND_SR_TX_IN_US      1U
#define TURNAROUND_EXT_RX_IN_US     1U
#define TURNAROUND_EXT_TX_IN_US     1U

#define TURNAROUND_WIFI_RX          (TURNAROUND_WIFI_RX_IN_US * COEXC_CLK_FREQ_MHZ)
#define TURNAROUND_WIFI_TX          (TURNAROUND_WIFI_TX_IN_US * COEXC_CLK_FREQ_MHZ)
#define TURNAROUND_SR_RX            (TURNAROUND_SR_RX_IN_US * COEXC_CLK_FREQ_MHZ)
#define TURNAROUND_SR_TX            (TURNAROUND_SR_TX_IN_US * COEXC_CLK_FREQ_MHZ)
#define TURNAROUND_EXT_RX           (TURNAROUND_EXT_RX_IN_US * COEXC_CLK_FREQ_MHZ)
#define TURNAROUND_EXT_TX           (TURNAROUND_EXT_TX_IN_US * COEXC_CLK_FREQ_MHZ)

#define COEXC_OFFSET_BETWEEN_CLIENT_REGS COEXC_OFFSET_BETWEEN_MODES

/** Write one COEXC register via nrf71_wifi_coex_reg_write (coex_transport.c). */
static int cd_coexc_reg_write(uint32_t abs_addr, uint32_t value)
{
	int ret = nrf71_wifi_coex_reg_write(abs_addr, value);

	if (ret != 0) {
		LOG_ERR("COEXC write 0x%08x = 0x%08x failed: %s (%d)", abs_addr, value,
			strerror(-ret), ret);
	}

	return ret;
}

/**
 * Program COEXC CCMALLOW, CCCONF, and TURNAROUND values for the selected
 * shared or separate antenna configuration.
 *
 * Any failing register access aborts the sequence and is reported to the
 * caller, so a partially programmed COEXC block is never mistaken for a
 * configured one.
 */
int cd_coexc_configuration(enum coex_antenna_cfg_type antenna_cfg_type)
{
	uint32_t ccconf_config[COEXC_NUM_CLIENTS];
	/* Turnaround register values per COEXC client */
	static const uint32_t turnaround_config[COEXC_NUM_CLIENTS] = {
		TURNAROUND_WIFI_RX, TURNAROUND_WIFI_TX, TURNAROUND_WIFI_RX, TURNAROUND_WIFI_TX,
		TURNAROUND_SR_RX, TURNAROUND_SR_TX, TURNAROUND_EXT_RX, TURNAROUND_EXT_TX,
	};

	/* CCCONF mode field. Only Wi-Fi is band dependent; SR and external use 0. */
	enum coexc_mode_wifi_t coexc_mode_wifi = COEXC_MODE_WIFI_2PT4G;
	unsigned int coexc_mode_sr = 0U;
	unsigned int coexc_mode_ext = 0U;

	/*
	 * Precomputed CCMALLOW tables for the separate antenna configuration.
	 * One row per COEXC client, one column per operating mode.
	 */
	static const uint32_t ccmallow_sep_ant[COEXC_NUM_CLIENTS][COEXC_NUM_MODES] = {
		{0x00011111UL, 0x00222222UL, 0x00444444UL, 0x00000000UL},
		{0x00001111UL, 0x00202222UL, 0x00404444UL, 0x00000000UL},
		{0x00011111UL, 0x00222222UL, 0x00444444UL, 0x00000000UL},
		{0x00101111UL, 0x00202222UL, 0x00404444UL, 0x00000000UL},
		{0x00110101UL, 0x00220202UL, 0x00440404UL, 0x00000000UL},
		{0x00111000UL, 0x00222222UL, 0x44000000UL, 0x00000000UL},
		{0x11000000UL, 0x22000000UL, 0x44000000UL, 0x00000000UL},
		{0x11000000UL, 0x22000000UL, 0x44000000UL, 0x00000000UL},
	};

	/* Same layout for shared-antenna configuration. */
	static const uint32_t ccmallow_sha_ant[COEXC_NUM_CLIENTS][COEXC_NUM_MODES] = {
		{0x00010000UL, 0x00022222UL, 0x00044444UL, 0x00000000UL},
		{0x00000000UL, 0x00202222UL, 0x00404444UL, 0x00000000UL},
		{0x00010000UL, 0x00022222UL, 0x00044444UL, 0x00000000UL},
		{0x00000000UL, 0x00202222UL, 0x00404444UL, 0x00000000UL},
		{0x00110101UL, 0x00220202UL, 0x00440404UL, 0x00000000UL},
		{0x00110000UL, 0x00222020UL, 0x44000000UL, 0x00000000UL},
		{0x11000000UL, 0x22000000UL, 0x44000000UL, 0x00000000UL},
		{0x11000000UL, 0x22000000UL, 0x44000000UL, 0x00000000UL},
	};

	const uint32_t (*p_ccmallow)[COEXC_NUM_MODES];
	int ret;

	if ((antenna_cfg_type != COEX_SEPARATE_ANT_CFG) &&
	    (antenna_cfg_type != COEX_SHARED_ANT_CFG)) {
		return -EINVAL;
	}

	p_ccmallow = (antenna_cfg_type == COEX_SEPARATE_ANT_CFG) ? ccmallow_sep_ant
								  : ccmallow_sha_ant;

	LOG_DBG("Configuring COEXC (CCMALLOW, CCCONF, TURNAROUND)");

	/* Block 1: addressed as base + client stride + mode stride. */
	for (uint32_t client = 0U; client < COEXC_NUM_CLIENTS; client++) {
		const uint32_t base_client_offset = client * COEXC_OFFSET_BETWEEN_CLIENTS;

		for (uint32_t mode = 0U; mode < COEXC_NUM_MODES; mode++) {
			const uint32_t offset = (COEXC_OFFSET_BETWEEN_MODES * mode) +
				base_client_offset;

			ret = cd_coexc_reg_write(ABS_COEXC_CCMALLOW_0_MODE_0 + offset,
						 p_ccmallow[client][mode]);
			if (ret != 0) {
				return ret;
			}
		}
	}

	/* Block 2: CCCONF, pack priority and mode per client. */
	ccconf_config[0] = (WLAN_HIGH_PTI_RX_CCCONF_PTI << COEXC_CCCONF_0_PRIORITY_SHIFT) |
		(coexc_mode_wifi << COEXC_CCCONF_0_MODE_SHIFT);
	ccconf_config[1] = (WLAN_HIGH_PTI_TX_CCCONF_PTI << COEXC_CCCONF_0_PRIORITY_SHIFT) |
		(coexc_mode_wifi << COEXC_CCCONF_0_MODE_SHIFT);
	ccconf_config[2] = (WLAN_LOW_PTI_RX_CCCONF_PTI << COEXC_CCCONF_0_PRIORITY_SHIFT) |
		(coexc_mode_wifi << COEXC_CCCONF_0_MODE_SHIFT);
	ccconf_config[3] = (WLAN_LOW_PTI_TX_CCCONF_PTI << COEXC_CCCONF_0_PRIORITY_SHIFT) |
		(coexc_mode_wifi << COEXC_CCCONF_0_MODE_SHIFT);
	ccconf_config[4] = (SR_RX_CCCONF_PTI << COEXC_CCCONF_0_PRIORITY_SHIFT) |
		(coexc_mode_sr << COEXC_CCCONF_0_MODE_SHIFT);
	ccconf_config[5] = (SR_TX_CCCONF_PTI << COEXC_CCCONF_0_PRIORITY_SHIFT) |
		(coexc_mode_sr << COEXC_CCCONF_0_MODE_SHIFT);
	ccconf_config[6] = (EXT_RX_CCCONF_PTI << COEXC_CCCONF_0_PRIORITY_SHIFT) |
		(coexc_mode_ext << COEXC_CCCONF_0_MODE_SHIFT);
	ccconf_config[7] = (EXT_TX_CCCONF_PTI << COEXC_CCCONF_0_PRIORITY_SHIFT) |
		(coexc_mode_ext << COEXC_CCCONF_0_MODE_SHIFT);

	for (uint32_t client = 0U; client < COEXC_NUM_CLIENTS; client++) {
		ret = cd_coexc_reg_write(ABS_COEXC_CCCONF_0 +
					 (COEXC_OFFSET_BETWEEN_CLIENT_REGS * client),
					 ccconf_config[client]);
		if (ret != 0) {
			return ret;
		}
	}

	/* Block 3: TURNAROUND, one register per client. */
	for (uint32_t client = 0U; client < COEXC_NUM_CLIENTS; client++) {
		ret = cd_coexc_reg_write(ABS_COEXC_TURNAROUND_0 +
					 (COEXC_OFFSET_BETWEEN_CLIENT_REGS * client),
					 turnaround_config[client]);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}
