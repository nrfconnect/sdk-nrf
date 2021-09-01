/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: interface part of radio module */

#include <stddef.h>
#include <stdint.h>

#include "ctrl/ptt_ctrl.h"
#include "ctrl/ptt_trace.h"
#include "rf/ptt_rf.h"
#include "ptt.h"
#include "nrf_802154.h"

#include "ptt_rf_api.h"
#include "ptt_rf_internal.h"

struct ptt_rf_ctx_s ptt_rf_ctx;

static enum ptt_ret ptt_rf_try_lock(ptt_evt_id_t evt_id);
static inline bool ptt_rf_is_locked(void);
static inline bool ptt_rf_is_locked_by(ptt_evt_id_t evt_id);
static inline ptt_evt_id_t ptt_rf_unlock(void);

static enum ptt_ret ptt_rf_promiscuous_set(bool value);

static void ptt_rf_stat_inc(int8_t rssi, uint8_t lqi);

static void ptt_rf_set_default_channel(void);
static void ptt_rf_set_default_power(void);
static void ptt_rf_set_default_antenna(void);
#if CONFIG_PTT_ANTENNA_DIVERSITY
static void ptt_rf_set_ant_div_mode(void);
#endif

void ptt_rf_init(void)
{
	PTT_TRACE("%s ->\n", __func__);
	ptt_rf_ctx = (struct ptt_rf_ctx_s){ 0 };
	ptt_rf_unlock();

	ptt_rf_set_default_channel();
	ptt_rf_set_default_power();
	ptt_rf_set_default_antenna();

#if CONFIG_PTT_ANTENNA_DIVERSITY
	ptt_rf_set_ant_div_mode();
#endif

	/* we need raw PHY packets without any filtering */
	ptt_rf_promiscuous_set(true);
	ptt_rf_receive_ext();
	PTT_TRACE("%s -<\n", __func__);
}

void ptt_rf_uninit(void)
{
	PTT_TRACE("%s\n", __func__);
}

void ptt_rf_reset(void)
{
	PTT_TRACE("%s ->\n", __func__);
	ptt_rf_uninit();
	ptt_rf_init();
	PTT_TRACE("%s -<\n", __func__);
}

static void ptt_rf_set_default_channel(void)
{
	uint32_t channel_mask;
	enum ptt_ret ret = PTT_RET_SUCCESS;

	/* try to set channel from prod config */
	if (ptt_get_channel_mask_ext(&channel_mask)) {
		ret = ptt_rf_set_channel_mask(PTT_RF_EVT_UNLOCKED, channel_mask);
		if (ret != PTT_RET_SUCCESS) {
			/* set default channel */
			ptt_rf_set_channel(PTT_RF_EVT_UNLOCKED, PTT_RF_DEFAULT_CHANNEL);
		}
	} else {
		/* set default channel */
		ptt_rf_set_channel(PTT_RF_EVT_UNLOCKED, PTT_RF_DEFAULT_CHANNEL);
	}
}

static void ptt_rf_set_default_power(void)
{
	int8_t power = PTT_RF_DEFAULT_POWER;

	/* set power from prod config */
	if (ptt_get_power_ext(&power)) {
		ptt_rf_set_power(PTT_RF_EVT_UNLOCKED, power);
	} else {
		ptt_rf_set_power(PTT_RF_EVT_UNLOCKED, PTT_RF_DEFAULT_POWER);
	}
}

static void ptt_rf_set_default_antenna(void)
{
	uint8_t antenna = PTT_RF_DEFAULT_ANTENNA;

	/* set antenna from prod config */
	if (ptt_get_antenna_ext(&antenna)) {
		ptt_rf_set_antenna(PTT_RF_EVT_UNLOCKED, antenna);
	} else {
		ptt_rf_set_antenna(PTT_RF_EVT_UNLOCKED, PTT_RF_DEFAULT_ANTENNA);
	}
}

#if CONFIG_PTT_ANTENNA_DIVERSITY
static void ptt_rf_set_ant_div_mode(void)
{
#if CONFIG_PTT_ANT_MODE_AUTO
	nrf_802154_antenna_diversity_rx_mode_set(NRF_802154_SL_ANT_DIV_MODE_AUTO);
	nrf_802154_antenna_diversity_tx_mode_set(NRF_802154_SL_ANT_DIV_MODE_AUTO);
#elif CONFIG_PTT_ANT_MODE_MANUAL
	nrf_802154_antenna_diversity_rx_mode_set(NRF_802154_SL_ANT_DIV_MODE_AUTO);
	nrf_802154_antenna_diversity_tx_mode_set(NRF_802154_SL_ANT_DIV_MODE_AUTO);

	nrf_802154_antenna_diversity_rx_mode_set(NRF_802154_SL_ANT_DIV_MODE_MANUAL);
	nrf_802154_antenna_diversity_tx_mode_set(NRF_802154_SL_ANT_DIV_MODE_MANUAL);
#endif
}
#endif

void ptt_rf_push_packet(const uint8_t *pkt, ptt_pkt_len_t len, int8_t rssi, uint8_t lqi)
{
	ptt_rf_stat_inc(rssi, lqi);
	ptt_ctrl_rf_push_packet(pkt, len, rssi, lqi);
}

void ptt_rf_tx_finished(void)
{
	if (ptt_rf_is_locked()) {
		/* have to unlock before passing control out of RF module */
		ptt_evt_id_t evt_id = ptt_rf_unlock();

		ptt_ctrl_rf_tx_finished(evt_id);
	} else {
		PTT_TRACE("%s: called, but rf module is not locked\n", __func__);
		/* we get event although we didn't send a packet, just pass it */
	}
}

void ptt_rf_tx_failed(ptt_rf_tx_error_t tx_error)
{
	if (ptt_rf_is_locked()) {
		/* have to unlock before passing control out of RF module */
		ptt_evt_id_t evt_id = ptt_rf_unlock();

		ptt_ctrl_rf_tx_failed(evt_id, tx_error);
	} else {
		PTT_TRACE("%s: called, but rf module is not locked\n", __func__);
		/* we get event although we didn't send a packet, just pass it */
	}
}

void ptt_rf_rx_failed(ptt_rf_rx_error_t rx_error)
{
	ptt_ctrl_rf_rx_failed(rx_error);
}

void ptt_rf_cca_done(bool result)
{
	if (ptt_rf_is_locked()) {
		/* have to unlock before passing control out of RF module */
		ptt_evt_id_t evt_id = ptt_rf_unlock();

		ptt_ctrl_rf_cca_done(evt_id, result);
	} else {
		PTT_TRACE("%s: called, but rf module is not locked\n", __func__);
		/* we get event although we didn't send a packet, just pass it */
	}
}

void ptt_rf_cca_failed(void)
{
	if (ptt_rf_is_locked()) {
		/* have to unlock before passing control out of RF module */
		ptt_evt_id_t evt_id = ptt_rf_unlock();

		ptt_ctrl_rf_cca_failed(evt_id);
	} else {
		PTT_TRACE("%s: called, but rf module is not locked\n", __func__);
		/* we get event although we didn't send a packet, just pass it */
	}
}

enum ptt_ret ptt_rf_cca(ptt_evt_id_t evt_id, uint8_t mode)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;

	if (ptt_rf_try_lock(evt_id) == PTT_RET_SUCCESS) {
		/* will be unlocked inside ptt_rf_cca_done/ptt_rf_cca_failed functions */
		if (false == ptt_rf_cca_ext(mode)) {
			ret = PTT_RET_BUSY;
		}

		if (ret != PTT_RET_SUCCESS) {
			ptt_rf_unlock();
		}
	} else {
		ret = PTT_RET_BUSY;
	}

	return ret;
}

enum ptt_ret ptt_rf_set_cca(ptt_evt_id_t evt_id, uint8_t activate)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;

	if (ptt_rf_try_lock(evt_id) == PTT_RET_SUCCESS) {
		if ((activate != 0 && activate != 1)) {
			ret = PTT_RET_INVALID_VALUE;
		}

		if (ret == PTT_RET_SUCCESS) {
			ptt_rf_ctx.cca_on_tx = activate;
		}

		ptt_rf_unlock();
	} else {
		ret = PTT_RET_BUSY;
	}

	return ret;
}

void ptt_rf_ed_detected(ptt_ed_t result)
{
	if (ptt_rf_is_locked()) {
		/* have to unlock before passing control out of RF module */
		ptt_evt_id_t evt_id = ptt_rf_unlock();

		ptt_ctrl_rf_ed_detected(evt_id, result);
	} else {
		PTT_TRACE("%s: called, but rf module does not locked\n", __func__);
		/* we get event although we didn't send a packet, just pass it */
	}
}

void ptt_rf_ed_failed(void)
{
	if (ptt_rf_is_locked()) {
		/* have to unlock before passing control out of RF module */
		ptt_evt_id_t evt_id = ptt_rf_unlock();

		ptt_ctrl_rf_ed_failed(evt_id);
	} else {
		PTT_TRACE("%s: called, but rf module does not locked\n", __func__);
		/* we get event although we didn't send a packet, just pass it */
	}
}

enum ptt_ret ptt_rf_ed(ptt_evt_id_t evt_id, uint32_t time_us)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;

	if (ptt_rf_try_lock(evt_id) == PTT_RET_SUCCESS) {
		/* will be unlocked inside ptt_rf_ed_detected/ptt_rf_ed_failed functions */
		if (false == ptt_rf_ed_ext(time_us)) {
			ret = PTT_RET_BUSY;
		}

		if (ret != PTT_RET_SUCCESS) {
			ptt_rf_unlock();
		}
	} else {
		ret = PTT_RET_BUSY;
	}

	return ret;
}

enum ptt_ret ptt_rf_rssi_measure_begin(ptt_evt_id_t evt_id)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;

	if (ptt_rf_try_lock(evt_id) == PTT_RET_SUCCESS) {
		if (false == ptt_rf_rssi_measure_begin_ext()) {
			ret = PTT_RET_BUSY;
		}

		ptt_rf_unlock();
	} else {
		ret = PTT_RET_BUSY;
	}

	return ret;
}

enum ptt_ret ptt_rf_rssi_last_get(ptt_evt_id_t evt_id, ptt_rssi_t *rssi)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;

	if (ptt_rf_try_lock(evt_id) == PTT_RET_SUCCESS) {
		*rssi = ptt_rf_rssi_last_get_ext();

		ptt_rf_unlock();
	} else {
		ret = PTT_RET_BUSY;
	}

	return ret;
}

enum ptt_ret ptt_rf_send_packet(ptt_evt_id_t evt_id, const uint8_t *pkt, ptt_pkt_len_t len)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;

	if (ptt_rf_try_lock(evt_id) == PTT_RET_SUCCESS) {
		if ((pkt == NULL) || (len == 0)) {
			ret = PTT_RET_INVALID_VALUE;
		}

		if (ret == PTT_RET_SUCCESS) {
			/* will be unlocked inside ptt_rf_tx_finished/ptt_rf_tx_failed functions */
			if (false == ptt_rf_send_packet_ext(pkt, len, ptt_rf_ctx.cca_on_tx)) {
				ret = PTT_RET_BUSY;
			}
		}

		if (ret != PTT_RET_SUCCESS) {
			ptt_rf_unlock();
		}
	} else {
		ret = PTT_RET_BUSY;
	}

	return ret;
}

enum ptt_ret ptt_rf_set_channel_mask(ptt_evt_id_t evt_id, uint32_t mask)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;

	uint8_t channel = ptt_rf_convert_channel_mask_to_num(mask);

	if ((channel < PTT_CHANNEL_MIN) || (channel > PTT_CHANNEL_MAX)) {
		ret = PTT_RET_INVALID_VALUE;
	} else {
		ret = ptt_rf_set_channel(evt_id, channel);
	}

	return ret;
}

enum ptt_ret ptt_rf_set_channel(ptt_evt_id_t evt_id, uint8_t channel)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;

	if (ptt_rf_try_lock(evt_id) == PTT_RET_SUCCESS) {
		if ((channel < PTT_CHANNEL_MIN) || (channel > PTT_CHANNEL_MAX)) {
			ret = PTT_RET_INVALID_VALUE;
		}

		if (ret == PTT_RET_SUCCESS) {
			ptt_rf_ctx.channel = channel;
			ptt_rf_set_channel_ext(ptt_rf_ctx.channel);
		}

		ptt_rf_unlock();
	} else {
		ret = PTT_RET_BUSY;
	}

	return ret;
}

enum ptt_ret ptt_rf_set_short_address(ptt_evt_id_t evt_id, const uint8_t *short_addr)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;

	if (ptt_rf_try_lock(evt_id) == PTT_RET_SUCCESS) {
		ptt_rf_set_short_address_ext(short_addr);

		ptt_rf_unlock();
	} else {
		ret = PTT_RET_BUSY;
	}

	return ret;
}

enum ptt_ret ptt_rf_set_extended_address(ptt_evt_id_t evt_id, const uint8_t *extended_addr)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;

	if (ptt_rf_try_lock(evt_id) == PTT_RET_SUCCESS) {
		ptt_rf_set_extended_address_ext(extended_addr);

		ptt_rf_unlock();
	} else {
		ret = PTT_RET_BUSY;
	}

	return ret;
}

enum ptt_ret ptt_rf_set_pan_id(ptt_evt_id_t evt_id, const uint8_t *pan_id)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;

	if (ptt_rf_try_lock(evt_id) == PTT_RET_SUCCESS) {
		ptt_rf_set_pan_id_ext(pan_id);

		ptt_rf_unlock();
	} else {
		ret = PTT_RET_BUSY;
	}

	return ret;
}

uint8_t ptt_rf_convert_channel_mask_to_num(uint32_t mask)
{
	uint8_t channel_num = 0;

	for (uint8_t i = PTT_CHANNEL_MIN; i <= PTT_CHANNEL_MAX; i++) {
		if (((mask >> i) & 1u) == 1u) {
			channel_num = i;
			break;
		}
	}

	return channel_num;
}

uint8_t ptt_rf_get_channel(void)
{
	return ptt_rf_ctx.channel;
}

enum ptt_ret ptt_rf_set_power(ptt_evt_id_t evt_id, int8_t power)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;

	if (ptt_rf_try_lock(evt_id) == PTT_RET_SUCCESS) {
		ptt_rf_set_power_ext(power);
		ptt_rf_ctx.power = power;

		ptt_rf_unlock();
	} else {
		ret = PTT_RET_BUSY;
	}

	return ret;
}

int8_t ptt_rf_get_power(void)
{
	ptt_rf_ctx.power = ptt_rf_get_power_ext();
	return ptt_rf_ctx.power;
}

enum ptt_ret ptt_rf_set_antenna(ptt_evt_id_t evt_id, uint8_t antenna)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;

	if (ptt_rf_try_lock(evt_id) == PTT_RET_SUCCESS) {
		ptt_rf_set_antenna_ext(antenna);
		ptt_rf_ctx.antenna = antenna;

		ptt_rf_unlock();
	} else {
		ret = PTT_RET_BUSY;
	}

	return ret;
}

enum ptt_ret ptt_rf_set_tx_antenna(ptt_evt_id_t evt_id, uint8_t antenna)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;

	if (ptt_rf_try_lock(evt_id) == PTT_RET_SUCCESS) {
		ptt_rf_set_tx_antenna_ext(antenna);

		ptt_rf_unlock();
	} else {
		ret = PTT_RET_BUSY;
	}

	return ret;
}

enum ptt_ret ptt_rf_set_rx_antenna(ptt_evt_id_t evt_id, uint8_t antenna)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;

	if (ptt_rf_try_lock(evt_id) == PTT_RET_SUCCESS) {
		ptt_rf_set_rx_antenna_ext(antenna);

		ptt_rf_unlock();
	} else {
		ret = PTT_RET_BUSY;
	}

	return ret;
}

uint8_t ptt_rf_get_rx_antenna(void)
{
	ptt_rf_ctx.antenna = ptt_rf_get_rx_antenna_ext();
	return ptt_rf_ctx.antenna;
}

uint8_t ptt_rf_get_tx_antenna(void)
{
	ptt_rf_ctx.antenna = ptt_rf_get_tx_antenna_ext();
	return ptt_rf_ctx.antenna;
}

uint8_t ptt_rf_get_last_rx_best_antenna(void)
{
	ptt_rf_ctx.antenna = ptt_rf_get_lat_rx_best_antenna_ext();
	return ptt_rf_ctx.antenna;
}

enum ptt_ret ptt_rf_start_statistic(ptt_evt_id_t evt_id)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;

	if (ptt_rf_try_lock(evt_id) == PTT_RET_SUCCESS) {
		ptt_rf_ctx.stat = (struct ptt_rf_stat_s){ 0 };
		ptt_rf_ctx.stat_enabled = true;
	} else {
		ret = PTT_RET_BUSY;
	}

	return ret;
}

enum ptt_ret ptt_rf_end_statistic(ptt_evt_id_t evt_id)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;

	if (ptt_rf_is_locked_by(evt_id)) {
		ptt_rf_ctx.stat_enabled = false;

		ptt_rf_unlock();
	} else {
		ret = PTT_RET_BUSY;
	}

	return ret;
}

struct ptt_rf_stat_s ptt_rf_get_stat_report(void)
{
	return ptt_rf_ctx.stat;
}

enum ptt_ret ptt_rf_start_modulated_stream(ptt_evt_id_t evt_id, const uint8_t *pkt,
					   ptt_pkt_len_t len)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;

	if (ptt_rf_try_lock(evt_id) == PTT_RET_SUCCESS) {
		if ((pkt == NULL) || (len == 0)) {
			ret = PTT_RET_INVALID_VALUE;
		}

		if (ret == PTT_RET_SUCCESS) {
			/* will be unlocked inside ptt_rf_stop_modulated_stream */
			if (false == ptt_rf_modulated_stream_ext(pkt, len)) {
				ret = PTT_RET_BUSY;
			}
		}

		if (ret != PTT_RET_SUCCESS) {
			ptt_rf_unlock();
		}
	} else {
		ret = PTT_RET_BUSY;
	}

	return ret;
}

enum ptt_ret ptt_rf_stop_modulated_stream(ptt_evt_id_t evt_id)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;

	if (ptt_rf_is_locked_by(evt_id)) {
		if (false == ptt_rf_receive_ext()) {
			ret = PTT_RET_BUSY;
		}

		ptt_rf_unlock();
	} else {
		ret = PTT_RET_BUSY;
	}

	return ret;
}

enum ptt_ret ptt_rf_start_continuous_carrier(ptt_evt_id_t evt_id)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;

	if (ptt_rf_try_lock(evt_id) == PTT_RET_SUCCESS) {
		/* will be unlocked inside ptt_rf_stop_continuous_stream */
		if (false == ptt_rf_continuous_carrier_ext()) {
			ret = PTT_RET_BUSY;

			ptt_rf_unlock();
		}
	} else {
		ret = PTT_RET_BUSY;
	}

	return ret;
}

enum ptt_ret ptt_rf_stop_continuous_carrier(ptt_evt_id_t evt_id)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;

	if (ptt_rf_is_locked_by(evt_id)) {
		if (false == ptt_rf_receive_ext()) {
			ret = PTT_RET_BUSY;
		}

		ptt_rf_unlock();
	} else {
		ret = PTT_RET_BUSY;
	}

	return ret;
}

static enum ptt_ret ptt_rf_promiscuous_set(bool value)
{
	ptt_rf_promiscuous_set_ext(value);

	return 0;
}

enum ptt_ret ptt_rf_receive(void)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;

	if (ptt_rf_is_locked() || (false == ptt_rf_receive_ext())) {
		ret = PTT_RET_BUSY;
	}

	return ret;
}

enum ptt_ret ptt_rf_sleep(void)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;

	if (ptt_rf_is_locked() || (false == ptt_rf_sleep_ext())) {
		ret = PTT_RET_BUSY;
	}

	return ret;
}

static inline ptt_evt_id_t ptt_rf_unlock(void)
{
	ptt_evt_id_t evt_id = ptt_rf_ctx.evt_lock;

	ptt_rf_ctx.evt_lock = PTT_RF_EVT_UNLOCKED;

	return evt_id;
}

static inline bool ptt_rf_is_locked(void)
{
	return (ptt_rf_ctx.evt_lock == PTT_RF_EVT_UNLOCKED) ? false : true;
}

static inline bool ptt_rf_is_locked_by(ptt_evt_id_t evt_id)
{
	return (evt_id == ptt_rf_ctx.evt_lock) ? true : false;
}

static enum ptt_ret ptt_rf_try_lock(ptt_evt_id_t evt_id)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;

	if (ptt_rf_is_locked()) {
		ret = PTT_RET_BUSY;
	}

	if (ret == PTT_RET_SUCCESS) {
		ptt_rf_ctx.evt_lock = evt_id;
	}

	return ret;
}

static void ptt_rf_stat_inc(int8_t rssi, uint8_t lqi)
{
	if (ptt_rf_ctx.stat_enabled) {
		ptt_rf_ctx.stat.total_pkts++;
		/* according to B.3.10:
		 * Assumes no RSSI values measured that are greater than zero dBm
		 */
		if (rssi > 0) {
			rssi = 0;
		}
		ptt_rf_ctx.stat.total_rssi += (rssi * (-1));
		ptt_rf_ctx.stat.total_lqi += lqi;
	}
}

inline struct ptt_ltx_payload_s *ptt_rf_get_custom_ltx_payload(void)
{
	return &(ptt_rf_ctx.ltx_payload);
}
