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
 * driver's private FMAC context.
 */

#ifndef NRF71_WIFI_COEX_H__
#define NRF71_WIFI_COEX_H__

#include <stddef.h>
#include <stdbool.h>

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

/**
 * @brief Check whether the Wi-Fi FMAC transport is ready for coex commands.
 *
 * Coexistence commands can only be forwarded to the Coexistence Manager once
 * the Wi-Fi driver has brought up the RPU and established the FMAC control
 * path. Callers should verify readiness before attempting to send.
 *
 * @retval true  The FMAC transport is ready.
 * @retval false The Wi-Fi driver/RPU is not (yet) ready.
 */
bool nrf71_wifi_coex_is_ready(void);

/**
 * @brief Send a Coexistence Driver -> Coexistence Manager command to the RPU.
 *
 * Forwards an opaque, already-marshalled CD2CM command buffer to the RPU over
 * the Wi-Fi FMAC control path. The command layout is defined by the CD2CM
 * message structures in the coexistence firmware interface header
 * (@c nrf71_coex_if.h).
 *
 * @param cmd Pointer to the command buffer to send.
 * @param len Length of the command buffer in bytes.
 *
 * @retval 0        On success.
 * @retval -EINVAL  Invalid argument.
 * @retval -ENODEV  The Wi-Fi FMAC transport is not ready
 *                  (see @ref nrf71_wifi_coex_is_ready).
 * @retval -EIO     The command could not be sent to the RPU.
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
