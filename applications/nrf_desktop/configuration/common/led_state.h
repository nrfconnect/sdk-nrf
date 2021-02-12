/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _LED_STATE_H_
#define _LED_STATE_H_

/**
 * @brief LED State
 * @defgroup led_state LED State
 * @{
 */

#include <zephyr/types.h>
#include "led_event.h"

#ifdef __cplusplus
extern "C" {
#endif

enum led_system_state {
	LED_SYSTEM_STATE_IDLE,
	LED_SYSTEM_STATE_CHARGING,
	LED_SYSTEM_STATE_ERROR,

	LED_SYSTEM_STATE_COUNT
};

enum led_peer_state {
	LED_PEER_STATE_DISCONNECTED,
	LED_PEER_STATE_CONNECTED,
	LED_PEER_STATE_PEER_SEARCH,
	LED_PEER_STATE_CONFIRM_SELECT,
	LED_PEER_STATE_CONFIRM_ERASE,
	LED_PEER_STATE_ERASE_ADV,

	LED_PEER_STATE_COUNT
};

enum led_id {
	LED_ID_SYSTEM_STATE,
	LED_ID_PEER_STATE,

	LED_ID_COUNT
};

#define LED_UNAVAILABLE CONFIG_DESKTOP_LED_COUNT

#if (defined(CONFIG_BT_PERIPHERAL) && defined(CONFIG_BT_CENTRAL)) || \
    (!defined(CONFIG_BT_PERIPHERAL) && !defined(CONFIG_BT_CENTRAL))
#error Device should be either Bluetooth peripheral or central
#endif

#if defined(CONFIG_BT_PERIPHERAL)
#define LED_PEER_COUNT (CONFIG_BT_MAX_PAIRED - 1)
#else
#define LED_PEER_COUNT 1
#endif

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _LED_STATE_H_ */
