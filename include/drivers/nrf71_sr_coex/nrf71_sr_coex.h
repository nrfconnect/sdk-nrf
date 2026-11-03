/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief nRF71 Wi-Fi / Short-Range coexistence driver application API.
 *
 * The coexistence driver works with built-in defaults. An application that
 * needs different coexistence preferences calls nrf71_sr_coex_init() once.
 * The driver keeps its own copy of the preferences and applies them to the
 * Coexistence Manager every time Wi-Fi comes up.
 */

#ifndef NRF71_SR_COEX_H__
#define NRF71_SR_COEX_H__

#include <stddef.h>
#include <stdint.h>

#include <drivers/nrf71_sr_coex/nrf71_sr_coex_sr.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf71_sr_coex nRF71 coexistence driver
 * @{
 */

/** Largest coexistence parameter blob accepted in @ref nrf71_sr_coex_cfg. */
#define NRF71_SR_COEX_PARAMS_MAX_LEN 128U

/** @brief Antenna layout of the board. */
enum nrf71_sr_coex_antenna {
	/** Wi-Fi and the SR radio share one antenna. */
	NRF71_SR_COEX_ANTENNA_SHARED,
	/** Wi-Fi and the SR radio have separate antennas. */
	NRF71_SR_COEX_ANTENNA_SEPARATE,
};

/** @brief Wi-Fi client priority ranges. */
struct nrf71_sr_coex_wifi_prio {
	/** Wi-Fi software request range. */
	struct nrf71_sr_coex_pti_range sw_request;
	/** High-priority Wi-Fi Rx client range. */
	struct nrf71_sr_coex_pti_range hp_rx;
	/** High-priority Wi-Fi Tx client range. */
	struct nrf71_sr_coex_pti_range hp_tx;
	/** Low-priority Wi-Fi Rx client range. */
	struct nrf71_sr_coex_pti_range lp_rx;
	/** Low-priority Wi-Fi Tx client range. */
	struct nrf71_sr_coex_pti_range lp_tx;
	/** Index of the value to use within the hardware client ranges. */
	uint8_t level;
};

/** @brief Wi-Fi and SR client priority ranges. */
struct nrf71_sr_coex_prio {
	/** Wi-Fi client ranges. */
	struct nrf71_sr_coex_wifi_prio wifi;
	/** SR client ranges. */
	struct nrf71_sr_coex_sr_prio sr;
};

/** @brief Shared antenna allocation. */
enum nrf71_sr_coex_ant_alloc {
	/** The Coexistence Manager allocates the antenna dynamically. */
	NRF71_SR_COEX_ANT_ALLOC_DYNAMIC,
	/** The antenna is always connected to Wi-Fi. */
	NRF71_SR_COEX_ANT_ALLOC_STATIC_WIFI,
	/** The antenna is always connected to the SR radio. */
	NRF71_SR_COEX_ANT_ALLOC_STATIC_SR,
};

/**
 * @brief Coexistence user preferences.
 *
 * The protection values are the probability, in percent (0-100), that the
 * Coexistence Manager protects the given activity against the other radio.
 */
struct nrf71_sr_coex_user_params {
	/** SR Rx protection on the Wi-Fi power save listen to inactive transition. */
	uint8_t sr_rx_prot_ps_listen2inactive;
	/** SR Rx protection on the Wi-Fi power save inactive to listen transition. */
	uint8_t sr_rx_prot_ps_inactive2listen;
	/** SR Rx protection on the Wi-Fi calibration listen to inactive transition. */
	uint8_t sr_rx_prot_calib_listen2inactive;
	/** SR Rx protection on the Wi-Fi calibration inactive to listen transition. */
	uint8_t sr_rx_prot_calib_inactive2listen;
	/** Wi-Fi scan protection. */
	uint8_t wifi_scan_prot;
	/** Wi-Fi beacon protection. */
	uint8_t wifi_beacon_prot;
	/** Wi-Fi connection protection. */
	uint8_t wifi_conn_prot;
	/** Wi-Fi calibration protection. */
	uint8_t wifi_calib_prot;
	/** Shared antenna allocation. */
	enum nrf71_sr_coex_ant_alloc ant_alloc;
};

/**
 * @brief Coexistence configuration.
 *
 * A NULL member selects the built-in default for that block. Nothing is
 * referenced after nrf71_sr_coex_init() returns.
 */
struct nrf71_sr_coex_cfg {
	/** Antenna layout. The driver assumes a shared antenna until this is set. */
	enum nrf71_sr_coex_antenna antenna;
	/** Priority ranges. */
	const struct nrf71_sr_coex_prio *prio;
	/** User preferences. */
	const struct nrf71_sr_coex_user_params *user;
	/** Coexistence Manager tuning parameters, as provided by Nordic. */
	const uint8_t *coex_params;
	/** Length of @c coex_params in bytes. */
	size_t coex_params_len;
};

/**
 * @brief Set the coexistence configuration.
 *
 * Can be called once, before or after Wi-Fi is up. If Wi-Fi is already up,
 * the Coexistence Manager is reconfigured in the background.
 *
 * @param cfg Configuration.
 *
 * @retval 0         On success.
 * @retval -EINVAL   Invalid configuration.
 * @retval -EALREADY Already called.
 */
int nrf71_sr_coex_init(const struct nrf71_sr_coex_cfg *cfg);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* NRF71_SR_COEX_H__ */
