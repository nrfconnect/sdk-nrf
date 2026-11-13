/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief nRF71 Wi-Fi coexistence transport.
 *
 * Thin, stable wrapper that lets the host-side Coexistence Driver forward
 * CD2CM command buffers to the Coexistence Manager (running in RPU firmware)
 * over the Wi-Fi FMAC control path, without depending on the Wi-Fi driver's
 * private FMAC context layout. It also reports Wi-Fi admin and connection
 * state to the coexistence driver.
 *
 * Locking:
 *   - notify_lock serializes state callbacks against each other and against
 *     registration, so a replay on register can't interleave with a live event.
 *   - tx_lock guards admin_up and is held across the FMAC send, so a command
 *     can't be posted while the last interface is being torn down. It is never
 *     held while calling the state callback, so the consumer may post commands
 *     from other threads without deadlocking against an event.
 */

#include <errno.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <drivers/wifi/nrf71/nrf71_wifi_coex.h>

#include <system/core.h>
#include <system/coex_notify.h>
#include <common/fmac_api_common.h>
#include <common/fmac_structs_common.h>

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_NRF71_WIFI_LOG_LEVEL);

#define COEX_EID_HT_OPERATION      61U
#define COEX_HT_OP_INFO_IDX        3U /* [0] = ID, [1] = length, [2] = primary channel */
#define COEX_HT_OP_MIN_BODY_LEN    2U
#define COEX_HT_OP_ANY_WIDTH       0x04U
#define COEX_HT_OP_SEC_OFFSET_MASK 0x03U
#define COEX_HT_OP_SEC_ABOVE       1U
#define COEX_HT_OP_SEC_BELOW       3U
#define COEX_HT_40MHZ_CENTER_SHIFT 10U

static K_MUTEX_DEFINE(notify_lock);
static K_MUTEX_DEFINE(tx_lock);

static nrf71_wifi_coex_event_cb_t coex_event_cb;
static void *coex_event_cb_ctx;

static nrf71_wifi_coex_state_cb_t coex_state_cb;
static void *coex_state_cb_ctx;

/* Number of interfaces in the operational UP state, guarded by notify_lock. */
static unsigned int vif_up_cnt;
/* Mirrors vif_up_cnt != 0, guarded by tx_lock. */
static bool admin_up;

/* Station connection, guarded by notify_lock. */
static bool sta_connected;
static uint16_t sta_center_freq_mhz;
static uint16_t sta_bandwidth_mhz;

static struct nrf_wifi_ctx_zep *coex_rpu_ctx(void)
{
	return &rpu_drv_priv_zep.rpu_ctx_zep;
}

static void coex_state_emit(enum nrf71_wifi_coex_state_type type)
{
	struct nrf71_wifi_coex_state_evt evt = {
		.type = type,
	};

	if (type == NRF71_WIFI_COEX_STATE_CONNECTED) {
		evt.center_freq_mhz = sta_center_freq_mhz;
		evt.bandwidth_mhz = sta_bandwidth_mhz;
	}

	if (coex_state_cb != NULL) {
		coex_state_cb(&evt, coex_state_cb_ctx);
	}
}

static void coex_set_admin_up(bool up)
{
	k_mutex_lock(&tx_lock, K_FOREVER);
	admin_up = up;
	k_mutex_unlock(&tx_lock);
}

/* Called with notify_lock held. */
static void coex_admin_down_locked(void)
{
	/* The station can't stay connected across admin down, and there may be no
	 * deauth event when the interface is just stopped.
	 */
	if (sta_connected) {
		sta_connected = false;
		coex_state_emit(NRF71_WIFI_COEX_STATE_DISCONNECTED);
	}

	coex_set_admin_up(false);
	coex_state_emit(NRF71_WIFI_COEX_STATE_ADMIN_DOWN);
}

bool nrf71_wifi_coex_is_ready(void)
{
	bool up;

	k_mutex_lock(&tx_lock, K_FOREVER);
	up = admin_up;
	k_mutex_unlock(&tx_lock);

	return up;
}

int nrf71_wifi_coex_cmd_send(const void *cmd, size_t len)
{
	struct nrf_wifi_ctx_zep *rpu_ctx = coex_rpu_ctx();
	enum nrf_wifi_status status;
	int ret = 0;

	if ((cmd == NULL) || (len == 0)) {
		return -EINVAL;
	}

	k_mutex_lock(&tx_lock, K_FOREVER);

	if (!admin_up || (rpu_ctx->rpu_ctx == NULL)) {
		LOG_DBG("Wi-Fi is down, dropping coex command");
		ret = -ENETDOWN;
		goto out;
	}

	/* nrf_wifi_fmac_conf_srcoex() takes a non-const buffer; the FMAC layer
	 * copies it into the command message and does not modify the caller's
	 * data.
	 */
	status = nrf_wifi_fmac_conf_srcoex(rpu_ctx->rpu_ctx, (void *)cmd, (unsigned int)len);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("Failed to send coex command to RPU");
		ret = -EIO;
		goto out;
	}

	LOG_DBG("CD2CM command posted to RPU (%u bytes)", (unsigned int)len);
out:
	k_mutex_unlock(&tx_lock);

	return ret;
}

int nrf71_wifi_coex_register_event_cb(nrf71_wifi_coex_event_cb_t cb, void *ctx)
{
	coex_event_cb = cb;
	coex_event_cb_ctx = ctx;

	return 0;
}

int nrf71_wifi_coex_register_state_cb(nrf71_wifi_coex_state_cb_t cb, void *ctx)
{
	int ret = 0;

	k_mutex_lock(&notify_lock, K_FOREVER);

	if ((cb != NULL) && (coex_state_cb != NULL)) {
		ret = -EALREADY;
		goto out;
	}

	coex_state_cb = cb;
	coex_state_cb_ctx = ctx;

	if ((cb != NULL) && (vif_up_cnt > 0)) {
		coex_state_emit(NRF71_WIFI_COEX_STATE_ADMIN_UP);
		if (sta_connected) {
			coex_state_emit(NRF71_WIFI_COEX_STATE_CONNECTED);
		}
	}
out:
	k_mutex_unlock(&notify_lock);

	return ret;
}

void nrf71_wifi_coex_on_event(const void *event, size_t len)
{
	if (coex_event_cb != NULL) {
		LOG_DBG("Forwarding CM2CD event to coex driver (%zu bytes)", len);
		coex_event_cb(coex_event_cb_ctx, event, len);
		LOG_DBG("Coex driver handler returned");
	} else {
		LOG_WRN("Coex event dropped (%zu bytes): no handler registered", len);
	}
}

void nrf_wifi_coex_vif_up(void)
{
	k_mutex_lock(&notify_lock, K_FOREVER);

	if (vif_up_cnt++ == 0) {
		coex_set_admin_up(true);
		coex_state_emit(NRF71_WIFI_COEX_STATE_ADMIN_UP);
	}

	k_mutex_unlock(&notify_lock);
}

void nrf_wifi_coex_vif_down(void)
{
	k_mutex_lock(&notify_lock, K_FOREVER);

	if (vif_up_cnt == 0) {
		LOG_WRN("Coex: unbalanced interface down");
	} else if (--vif_up_cnt == 0) {
		coex_admin_down_locked();
	}

	k_mutex_unlock(&notify_lock);
}

void nrf_wifi_coex_dev_rem(void)
{
	k_mutex_lock(&notify_lock, K_FOREVER);

	if (vif_up_cnt > 0) {
		vif_up_cnt = 0;
		coex_admin_down_locked();
	}

	k_mutex_unlock(&notify_lock);
}

/*
 * The association response only gives the primary channel. The width comes from
 * the HT Operation element, because the only other source is a blocking firmware
 * command that can't run in this event context. Without that element, or if it
 * says 20 MHz, the channel is taken as 20 MHz wide.
 */
void nrf_wifi_coex_sta_connected(unsigned int primary_freq_mhz, const uint8_t *ies,
				 size_t ies_len)
{
	unsigned int center_freq_mhz = primary_freq_mhz;
	unsigned int bandwidth_mhz = 20U;

	while ((ies != NULL) && (ies_len >= 2U)) {
		size_t elem_len = ies[1];

		if ((elem_len + 2U) > ies_len) {
			break;
		}

		if ((ies[0] == COEX_EID_HT_OPERATION) && (elem_len >= COEX_HT_OP_MIN_BODY_LEN)) {
			uint8_t info = ies[COEX_HT_OP_INFO_IDX];

			if ((info & COEX_HT_OP_ANY_WIDTH) != 0U) {
				switch (info & COEX_HT_OP_SEC_OFFSET_MASK) {
				case COEX_HT_OP_SEC_ABOVE:
					center_freq_mhz += COEX_HT_40MHZ_CENTER_SHIFT;
					bandwidth_mhz = 40U;
					break;
				case COEX_HT_OP_SEC_BELOW:
					center_freq_mhz -= COEX_HT_40MHZ_CENTER_SHIFT;
					bandwidth_mhz = 40U;
					break;
				default:
					break;
				}
			}
			break;
		}

		ies += elem_len + 2U;
		ies_len -= elem_len + 2U;
	}

	k_mutex_lock(&notify_lock, K_FOREVER);

	sta_connected = true;
	sta_center_freq_mhz = (uint16_t)center_freq_mhz;
	sta_bandwidth_mhz = (uint16_t)bandwidth_mhz;
	coex_state_emit(NRF71_WIFI_COEX_STATE_CONNECTED);

	k_mutex_unlock(&notify_lock);
}

void nrf_wifi_coex_sta_disconnected(void)
{
	k_mutex_lock(&notify_lock, K_FOREVER);

	if (sta_connected) {
		sta_connected = false;
		coex_state_emit(NRF71_WIFI_COEX_STATE_DISCONNECTED);
	}

	k_mutex_unlock(&notify_lock);
}
