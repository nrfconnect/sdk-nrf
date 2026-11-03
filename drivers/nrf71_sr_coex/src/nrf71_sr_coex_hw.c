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
 *   NRF71_SR_COEX_ANTENNA_SHARED    Wi-Fi and SR share one physical antenna
 *   NRF71_SR_COEX_ANTENNA_SEPARATE  radios use separate antennas
 *
 * COEXC sits in the always-on global domain and is memory-mapped on the
 * application processor, so it is written directly, independent of the Wi-Fi
 * driver and the RPU, and its contents survive an RPU reset.
 */

#include <stdint.h>

#include <nrfx.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <drivers/nrf71_sr_coex/nrf71_sr_coex.h>

#include "nrf71_sr_coex_internal.h"

LOG_MODULE_DECLARE(nrf71_sr_coex, CONFIG_NRF71_SR_COEX_DRIVER_LOG_LEVEL);

/** COEXC clock frequency used for turnaround register programming (MHz). */
#define COEXC_CLK_FREQ_MHZ 16U

#define COEXC_NUM_CLIENTS COEXC_CCCONF_MaxCount
/* Modes used by the CCMALLOW tables below; higher modes are left at reset. */
#define COEXC_NUM_MODES   4U

BUILD_ASSERT(COEXC_CCMALLOW_MaxCount == COEXC_NUM_CLIENTS);
BUILD_ASSERT(COEXC_TURNAROUND_MaxCount == COEXC_NUM_CLIENTS);

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
#define TURNAROUND_WIFI_RX_IN_US 1U
#define TURNAROUND_WIFI_TX_IN_US 1U
#define TURNAROUND_SR_RX_IN_US   1U
#define TURNAROUND_SR_TX_IN_US   1U
#define TURNAROUND_EXT_RX_IN_US  1U
#define TURNAROUND_EXT_TX_IN_US  1U

#define TURNAROUND_WIFI_RX (TURNAROUND_WIFI_RX_IN_US * COEXC_CLK_FREQ_MHZ)
#define TURNAROUND_WIFI_TX (TURNAROUND_WIFI_TX_IN_US * COEXC_CLK_FREQ_MHZ)
#define TURNAROUND_SR_RX   (TURNAROUND_SR_RX_IN_US * COEXC_CLK_FREQ_MHZ)
#define TURNAROUND_SR_TX   (TURNAROUND_SR_TX_IN_US * COEXC_CLK_FREQ_MHZ)
#define TURNAROUND_EXT_RX  (TURNAROUND_EXT_RX_IN_US * COEXC_CLK_FREQ_MHZ)
#define TURNAROUND_EXT_TX  (TURNAROUND_EXT_TX_IN_US * COEXC_CLK_FREQ_MHZ)

/*
 * CCCONF mode field. Only Wi-Fi is band dependent; SR and external use 0.
 * The Wi-Fi mode written here is only the initial value (2.4 GHz). The
 * Coexistence Manager firmware updates it at runtime when Wi-Fi changes band.
 */
#define COEXC_MODE_WIFI_INIT 0U
#define COEXC_MODE_SR        0U
#define COEXC_MODE_EXT       0U

#define CCCONF(pti, mode)                                                                          \
	((((pti) << COEXC_CCCONF_PRIORITY_Pos) & COEXC_CCCONF_PRIORITY_Msk) |                      \
	 (((mode) << COEXC_CCCONF_MODE_Pos) & COEXC_CCCONF_MODE_Msk))

static const uint32_t ccconf_config[COEXC_NUM_CLIENTS] = {
	CCCONF(WLAN_HIGH_PTI_RX_CCCONF_PTI, COEXC_MODE_WIFI_INIT),
	CCCONF(WLAN_HIGH_PTI_TX_CCCONF_PTI, COEXC_MODE_WIFI_INIT),
	CCCONF(WLAN_LOW_PTI_RX_CCCONF_PTI, COEXC_MODE_WIFI_INIT),
	CCCONF(WLAN_LOW_PTI_TX_CCCONF_PTI, COEXC_MODE_WIFI_INIT),
	CCCONF(SR_RX_CCCONF_PTI, COEXC_MODE_SR),
	CCCONF(SR_TX_CCCONF_PTI, COEXC_MODE_SR),
	CCCONF(EXT_RX_CCCONF_PTI, COEXC_MODE_EXT),
	CCCONF(EXT_TX_CCCONF_PTI, COEXC_MODE_EXT),
};

static const uint32_t turnaround_config[COEXC_NUM_CLIENTS] = {
	TURNAROUND_WIFI_RX, TURNAROUND_WIFI_TX, TURNAROUND_WIFI_RX, TURNAROUND_WIFI_TX,
	TURNAROUND_SR_RX,   TURNAROUND_SR_TX,   TURNAROUND_EXT_RX,  TURNAROUND_EXT_TX,
};

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

/*
 * Same layout for the shared-antenna configuration.
 *
 * Clients 0 (Wi-Fi HP Rx) and 2 (Wi-Fi LP Rx) must be allowed together in
 * mode 0 (Wi-Fi 2.4 GHz). The firmware SW request raises all four Wi-Fi clients
 * to the same priority and requests on client 0 while client 2 holds the grant,
 * so without that the request is refused forever. Mode 1 (5 GHz) already allows
 * the Wi-Fi clients together.
 */
static const uint32_t ccmallow_sha_ant[COEXC_NUM_CLIENTS][COEXC_NUM_MODES] = {
	{0x00010101UL, 0x00022222UL, 0x00044444UL, 0x00000000UL},
	{0x00000000UL, 0x00202222UL, 0x00404444UL, 0x00000000UL},
	{0x00010101UL, 0x00022222UL, 0x00044444UL, 0x00000000UL},
	{0x00000000UL, 0x00202222UL, 0x00404444UL, 0x00000000UL},
	{0x00110101UL, 0x00220202UL, 0x00440404UL, 0x00000000UL},
	{0x00110000UL, 0x00222020UL, 0x44000000UL, 0x00000000UL},
	{0x11000000UL, 0x22000000UL, 0x44000000UL, 0x00000000UL},
	{0x11000000UL, 0x22000000UL, 0x44000000UL, 0x00000000UL},
};

void cd_coexc_configure(enum nrf71_sr_coex_antenna antenna)
{
	const uint32_t (*ccmallow)[COEXC_NUM_MODES] =
		(antenna == NRF71_SR_COEX_ANTENNA_SEPARATE) ? ccmallow_sep_ant : ccmallow_sha_ant;

	LOG_DBG("Configuring COEXC for %s antenna",
		(antenna == NRF71_SR_COEX_ANTENNA_SEPARATE) ? "separate" : "shared");

	for (uint32_t client = 0U; client < COEXC_NUM_CLIENTS; client++) {
		for (uint32_t mode = 0U; mode < COEXC_NUM_MODES; mode++) {
			NRF_COEXC->CCMALLOW[client].MODE[mode] = ccmallow[client][mode];
		}
	}

	for (uint32_t client = 0U; client < COEXC_NUM_CLIENTS; client++) {
		NRF_COEXC->CCCONF[client] = ccconf_config[client];
	}

	for (uint32_t client = 0U; client < COEXC_NUM_CLIENTS; client++) {
		NRF_COEXC->TURNAROUND[client] = turnaround_config[client];
	}
}
