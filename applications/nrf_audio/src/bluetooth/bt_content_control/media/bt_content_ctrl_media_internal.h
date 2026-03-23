/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BT_CONTENT_CTRL_MEDIA_INTERNAL_H_
#define _BT_CONTENT_CTRL_MEDIA_INTERNAL_H_

#include <zephyr/bluetooth/conn.h>

/**
 * @brief	Callback for changing the stream state.
 *
 * @param[in]	play	Differentiate between the play command and the pause command.
 */
typedef void (*bt_content_ctrl_media_play_pause_cb)(bool play);

/**
 * @brief	Discover Media Control Service and the included services.
 *
 * @note	Only valid for client.
 *
 * @param[in]	conn	Pointer to the active connection.
 *
 * @return	0 for success, error otherwise.
 */
int bt_content_ctrl_media_discover(struct bt_conn *conn);

/**
 * @brief	Get the current state of the media player.
 *
 * @note	Only valid for client.
 *
 * @param[in]	conn	Pointer to the active connection.
 *
 * @return	0 for success, error otherwise.
 */
int bt_content_ctrl_media_state_update(struct bt_conn *conn);

/**
 * @brief	Send a play command to the media player,
 *		depending on the current state.
 *
 * @param[in]	conn	Pointer to the connection to control; can be NULL.
 *
 * @note	If @p conn is NULL, play will be sent to all mcc_peers discovered.
 *
 * @return	0 for success, error otherwise.
 */
int bt_content_ctrl_media_play(struct bt_conn *conn);

/**
 * @brief	Send a pause command to the media player,
 *		depending on the current state.
 *
 * @param[in]	conn	Pointer to the connection to control; can be NULL.
 *
 * @note	If @p conn is NULL, pause will be sent to all mcc_peers discovered.
 *
 * @return	0 for success, error otherwise.
 */
int bt_content_ctrl_media_pause(struct bt_conn *conn);

/**
 * @brief	Reset the media control peer's discovered state
 *
 * @note	Only valid for client.
 *
 * @param[in]	conn	Pointer to the active connection.
 *
 * @return	0 for success, error otherwise.
 */
int bt_content_ctrl_media_conn_disconnected(struct bt_conn *conn);

/**
 * @brief	Initialize the Media Control Client.
 *
 * @return	0 for success, error otherwise.
 */
int bt_content_ctrl_media_client_init(void);

/**
 * @brief	Initialize the Media Control Server.
 *
 * @param[in]	play_pause_cb	Callback for received play/pause commands.
 *
 * @return	0 for success, error otherwise.
 */
int bt_content_ctrl_media_server_init(bt_content_ctrl_media_play_pause_cb play_pause_cb);

#endif /* _BT_CONTENT_CTRL_MEDIA_INTERNAL_H_ */
