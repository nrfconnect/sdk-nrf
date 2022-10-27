/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BLE_AUDIO_SERVICES_H_
#define _BLE_AUDIO_SERVICES_H_

#include <zephyr/bluetooth/conn.h>

/**
 * @brief  Callback for changing stream state.
 *
 * @param  play Differentiate between play command and pause command.
 */
typedef void (*ble_mcs_play_pause_cb)(bool play);

/**
 * @brief  Set volume to a specific value.
 * @param  volume The absolute volume to set.
 *         If the current device is gateway, the target device
 *         will be headset, if the current device is headset then
 *         the target device will be itself.
 *
 * @return 0 for success,
 *          -ENXIO if the feature is disabled.
 *          Other errors from underlying drivers.
 */
int ble_vcs_vol_set(uint8_t volume);

/**
 * @brief  Turn the volume up by one step.
 *
 *         If the current device is gateway, the target device
 *         will be headset, if the current device is headset then
 *         the target device will be itself.
 *
 * @return 0 for success,
 *         -ENXIO if the feature is disabled.
 *         Other errors from underlying drivers.
 */
int ble_vcs_volume_up(void);

/**
 * @brief  Turn the volume down by one step.
 *
 *         If the current device is gateway, the target device
 *         will be headset, if the current device is headset then
 *         the target device will be itself.
 *
 * @return 0 for success,
 *         -ENXIO if the feature is disabled.
 *         Other errors from underlying drivers.
 */
int ble_vcs_volume_down(void);

/**
 * @brief  Mute the output volume of the device.
 *
 *         If the current device is gateway, the target device
 *         will be headset, if the current device is headset then
 *         the target device will be itself.
 *
 * @return 0 for success,
 *         -ENXIO if the feature is disabled.
 *         Other errors from underlying drivers.
 */
int ble_vcs_volume_mute(void);

/**
 * @brief  Unmute the output volume of the device.
 *
 *         If the current device is gateway, the target device
 *         will be headset, if the current device is headset then
 *         the target device will be itself.
 *
 * @return 0 for success,
 *         -ENXIO if the feature is disabled.
 *         Other errors from underlying drivers.
 */
int ble_vcs_volume_unmute(void);

/**
 * @brief  Discover VCS and included services.
 *
 * @param  conn Pointer for peer connection information.
 * @param  channel_num The number of the remote device.
 *
 * This will start a GATT discovery and setup handles and subscriptions for
 * VCS and included services.
 * This shall be called once before any other actions related with VCS.
 *
 * @return 0 for success, error otherwise.
 */
int ble_vcs_discover(struct bt_conn *conn, uint8_t channel_num);

/**
 * @brief  Discover MCS and included services.
 *
 * @param  conn  Pointer to the active connection, only valid for client.
 *
 * @return 0 for success, error otherwise.
 */
int ble_mcs_discover(struct bt_conn *conn);

/**
 * @brief  Get the current state of the media player, only valid for client.
 *
 * @param  conn  Pointer to the active connection.
 *
 * @return 0 for success, error otherwise.
 */
int ble_mcs_state_update(struct bt_conn *conn);

/**
 * @brief  Send play/pause command to the media player,
 *         depending on the current state.
 *
 * @param  conn  Pointer to the active connection, only valid for client.
 *               Shall be NULL if called from server.
 *
 * @return 0 for success, error otherwise.
 */
int ble_mcs_play_pause(struct bt_conn *conn);

/**
 * @brief Initialize the Volume Control Service client.
 */
int ble_vcs_client_init(void);

/**
 * @brief  Initialize the Volume Control Service server.
 *
 * @return 0 for success, error otherwise.
 */
int ble_vcs_server_init(void);

/**
 * @brief  Initialize the Media Control Client.
 *
 * @return 0 for success, error otherwise.
 */
int ble_mcs_client_init(void);

/**
 * @brief  Initialize the Media Control Server.
 *
 * @return 0 for success, error otherwise.
 */
int ble_mcs_server_init(ble_mcs_play_pause_cb le_audio_play_pause_cb);

#endif /* _BLE_AUDIO_SERVICES_H_ */
