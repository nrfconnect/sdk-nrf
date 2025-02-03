/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _LED_ASSIGNMENTS_H_
#define _LED_ASSIGNMENTS_H_

#include <zephyr/devicetree.h>
#include <stdint.h>

#define LED_AUDIO_NOT_ASSIGNED 0xFF

/**
 * @brief Mapping of LEDs on board to indication.
 */
#if DT_NODE_EXISTS(DT_ALIAS(led_device_type))
#define LED_AUDIO_DEVICE_TYPE_INDEX DT_NODE_CHILD_IDX(DT_ALIAS(led_device_type))
#else
#define LED_AUDIO_DEVICE_TYPE_INDEX LED_AUDIO_NOT_ASSIGNED
#endif

#if DT_NODE_EXISTS(DT_ALIAS(led_net_status))
#define LED_AUDIO_NET_STATUS_INDEX DT_NODE_CHILD_IDX(DT_ALIAS(led_net_status))
#else
#define LED_AUDIO_NET_STATUS_INDEX LED_AUDIO_NOT_ASSIGNED
#endif

#if DT_NODE_EXISTS(DT_ALIAS(led_conn_status))
#define LED_AUDIO_CONN_STATUS_INDEX DT_NODE_CHILD_IDX(DT_ALIAS(led_conn_status))
#else
#define LED_AUDIO_CONN_STATUS_INDEX LED_AUDIO_NOT_ASSIGNED
#endif

#if DT_NODE_EXISTS(DT_ALIAS(led_app_status))
#define LED_AUDIO_APP_STATUS_INDEX DT_NODE_CHILD_IDX(DT_ALIAS(led_app_status))
#else
#define LED_AUDIO_APP_STATUS_INDEX LED_AUDIO_NOT_ASSIGNED
#endif

#if DT_NODE_EXISTS(DT_ALIAS(led_sync_status))
#define LED_AUDIO_SYNC_STATUS_INDEX DT_NODE_CHILD_IDX(DT_ALIAS(led_sync_status))
#else
#define LED_AUDIO_SYNC_STATUS_INDEX LED_AUDIO_NOT_ASSIGNED
#endif

#endif /* _LED_ASSIGNMENTS_H_ */
