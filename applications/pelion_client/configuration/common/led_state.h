/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
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

#ifdef __cplusplus
extern "C" {
#endif

enum led_id {
	LED_ID_NET_STATE,
	LED_ID_PELION_STATE,

	LED_ID_COUNT
};

#define LED_UNAVAILABLE 0xFF

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _LED_STATE_H_ */
