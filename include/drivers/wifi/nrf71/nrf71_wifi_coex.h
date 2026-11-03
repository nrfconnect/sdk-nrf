/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief nRF71 Wi-Fi driver coexistence transport API.
 *
 * The nRF71 is a single-die Wi-Fi + Short-Range (BLE) combo SoC. Coexistence
 * between the two radios is arbitrated on-chip by the Coexistence Controller
 * (COEXC) hardware and a Coexistence Manager (CM) that runs in the RPU/LMAC
 * firmware.
 *
 * Commands from the host-side Coexistence Driver (CD) to the CM are carried
 * over the Wi-Fi FMAC host<->RPU control path, which is owned by this Wi-Fi
 * driver. This header exposes a small, stable transport so the coexistence
 * driver (drivers/nrf71_sr_coex) does not need to reach into the Wi-Fi
 * driver's private FMAC context, plus a state notifier so the CD learns when
 * that transport comes and goes.
 */

#ifndef NRF71_WIFI_COEX_H__
#define NRF71_WIFI_COEX_H__

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Coexistence event callback.
 *
 * Invoked when a Coexistence Manager to Coexistence Driver (CM2CD) event is
 * received from the RPU over the Wi-Fi FMAC path.
 *
 * @param ctx   Opaque context registered with the callback.
 * @param event Pointer to the received event payload.
 * @param len   Length of the event payload in bytes.
 */
typedef void (*nrf71_wifi_coex_event_cb_t)(void *ctx, const void *event, size_t len);

/** @brief Wi-Fi state changes reported to the coexistence driver. */
enum nrf71_wifi_coex_state_type {
	/**
	 * The first Wi-Fi interface is up. FMAC, IPC and the CM are usable and
	 * CD2CM commands can be sent.
	 */
	NRF71_WIFI_COEX_STATE_ADMIN_UP,
	/**
	 * The last Wi-Fi interface is going down. Sent before the interface is
	 * torn down. Once the callback returns, CD2CM commands fail with
	 * -ENETDOWN until the next ADMIN_UP.
	 */
	NRF71_WIFI_COEX_STATE_ADMIN_DOWN,
	/** The station associated. The channel fields of the event are valid. */
	NRF71_WIFI_COEX_STATE_CONNECTED,
	/** The station left the channel reported in the last CONNECTED. */
	NRF71_WIFI_COEX_STATE_DISCONNECTED,
};

/** @brief Wi-Fi state event passed to @ref nrf71_wifi_coex_state_cb_t. */
struct nrf71_wifi_coex_state_evt {
	/** Type of the state change. */
	enum nrf71_wifi_coex_state_type type;
	/** Center frequency of the operating channel in MHz, CONNECTED only. */
	uint16_t center_freq_mhz;
	/** Operating channel width in MHz (20 or 40), CONNECTED only. */
	uint16_t bandwidth_mhz;
};

/**
 * @brief Wi-Fi state callback.
 *
 * Runs in the Wi-Fi driver context, possibly with driver locks held. It must
 * not block and must not call back into any nrf71_wifi_coex_* function.
 *
 * @param evt State event. Only valid for the duration of the call.
 * @param ctx Opaque context registered with the callback.
 */
typedef void (*nrf71_wifi_coex_state_cb_t)(const struct nrf71_wifi_coex_state_evt *evt,
					   void *ctx);

/**
 * @brief Check whether the Wi-Fi FMAC transport is ready for coex commands.
 *
 * True between @ref NRF71_WIFI_COEX_STATE_ADMIN_UP and
 * @ref NRF71_WIFI_COEX_STATE_ADMIN_DOWN.
 *
 * @retval true  At least one Wi-Fi interface is up.
 * @retval false Wi-Fi is down.
 */
bool nrf71_wifi_coex_is_ready(void);

/**
 * @brief Send a Coexistence Driver -> Coexistence Manager command to the RPU.
 *
 * Forwards an opaque, already-marshalled CD2CM command buffer to the RPU over
 * the Wi-Fi FMAC control path. The command layout is defined by the CD2CM
 * message structures in the coexistence firmware interface header
 * (@c nrf71_coex_if.h). Only posts the command; the CM reply arrives through
 * the callback registered with @ref nrf71_wifi_coex_register_event_cb.
 *
 * @param cmd Pointer to the command buffer to send.
 * @param len Length of the command buffer in bytes.
 *
 * @retval 0          On success.
 * @retval -EINVAL    Invalid argument.
 * @retval -ENETDOWN  Wi-Fi is down (see @ref nrf71_wifi_coex_is_ready).
 * @retval -EIO       The command could not be sent to the RPU.
 */
int nrf71_wifi_coex_cmd_send(const void *cmd, size_t len);

/**
 * @brief Register a callback for received coexistence (CM2CD) events.
 *
 * The coexistence driver registers a handler that is invoked from
 * @ref nrf71_wifi_coex_on_event when the RPU delivers a coexistence event.
 *
 * @param cb  Callback to invoke, or NULL to unregister.
 * @param ctx Opaque context passed back to the callback.
 *
 * @retval 0 On success.
 */
int nrf71_wifi_coex_register_event_cb(nrf71_wifi_coex_event_cb_t cb, void *ctx);

/**
 * @brief Register for Wi-Fi state changes.
 *
 * There is a single consumer. Registration replays the current state before it
 * returns: ADMIN_UP if Wi-Fi is already up, followed by CONNECTED if the
 * station is associated. This makes the consumer independent of the init
 * order between the two drivers.
 *
 * @param cb  Callback to invoke, or NULL to unregister.
 * @param ctx Opaque context passed back to the callback.
 *
 * @retval 0         On success.
 * @retval -EALREADY Another callback is already registered.
 */
int nrf71_wifi_coex_register_state_cb(nrf71_wifi_coex_state_cb_t cb, void *ctx);

/**
 * @brief Deliver a received coexistence (CM2CD) event to the registered handler.
 *
 * This is the integration point for the Wi-Fi FMAC event path: it shall be
 * called when a coexistence event (NRF_WIFI_EVENT_COEX_CONFIG) is received from
 * the RPU, so that the event reaches the coexistence driver.
 *
 * @param event Pointer to the received event payload.
 * @param len   Length of the event payload in bytes.
 */
void nrf71_wifi_coex_on_event(const void *event, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* NRF71_WIFI_COEX_H__ */
