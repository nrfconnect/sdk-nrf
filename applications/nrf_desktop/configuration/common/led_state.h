/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Header file with LED states.
 */

#ifndef _LED_STATE_H_
#define _LED_STATE_H_

#include <zephyr/types.h>
#include <caf/events/led_event.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LED State
 * @defgroup nrf_desktop_led_state LED State
 * @{
 */

/** @brief System states represented by LED. */
enum led_system_state {
	/** Device is idle. */
	LED_SYSTEM_STATE_IDLE,
	/** Device is charging. */
	LED_SYSTEM_STATE_CHARGING,
	/** Device in error state. */
	LED_SYSTEM_STATE_ERROR,

	/** Number of system states. */
	LED_SYSTEM_STATE_COUNT
};

/** @brief Peer states represented by LED. */
enum led_peer_state {
	/** Peer is disconnected. */
	LED_PEER_STATE_DISCONNECTED,
	/** Peer is connected. */
	LED_PEER_STATE_CONNECTED,
	/** Device searches for peer. */
	LED_PEER_STATE_PEER_SEARCH,
	/** Device waits for user to confirm peer switch. */
	LED_PEER_STATE_CONFIRM_SELECT,
	/** Device waits for user to confirm bond erase. */
	LED_PEER_STATE_CONFIRM_ERASE,
	/** Device searches for new peer before bond erase. */
	LED_PEER_STATE_ERASE_ADV,

	/** Number of peer states. */
	LED_PEER_STATE_COUNT
};

/** @brief LED identification. */
enum led_id_nrf_desktop {
	/** LED representing system state. */
	LED_ID_SYSTEM_STATE,
	/** LED representing peer state. */
	LED_ID_PEER_STATE,

	/** Number of LEDs. */
	LED_ID_COUNT
};

#define LED_UNAVAILABLE 0xFF

#if defined(CONFIG_DESKTOP_BT_PERIPHERAL)
/* By default, peripheral uses a separate Bluetooth local identity per every Bluetooth peer.
 * The default local identity cannot be used, because application cannot reset it.
 * One Bluetooth local identity is reserved for the erase advertising.
 */
BUILD_ASSERT(CONFIG_BT_ID_MAX >= 3);
#define LED_PEER_COUNT (CONFIG_BT_ID_MAX - 2)
#else
#define LED_PEER_COUNT 1
#endif

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* _LED_STATE_H_ */
