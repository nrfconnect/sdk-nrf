/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BT_CONTENT_CTRL_H_
#define _BT_CONTENT_CTRL_H_

#include <zephyr/bluetooth/conn.h>

/**
 * @brief	Send the start request for content transmission.
 *
 * @param[in]	conn	Pointer to the connection to control; can be NULL.
 *
 * @return	0 for success, error otherwise.
 */
int bt_content_ctrl_start(struct bt_conn *conn);

/**
 * @brief	Send the stop request for content transmission.
 *
 * @param[in]	conn	Pointer to the connection to control; can be NULL.
 *
 * @return	0 for success, error otherwise.
 */
int bt_content_ctrl_stop(struct bt_conn *conn);

/**
 * @brief	Handle disconnected connection for the content control services.
 *
 * @param[in]	conn	Pointer to the disconnected connection.
 *
 * @return	0 for success, error otherwise.
 */
int bt_content_ctrl_conn_disconnected(struct bt_conn *conn);

/**
 * @brief	Discover the content control services for the given connection pointer.
 *
 * @param[in]	conn	Pointer to the connection on which to discover the services.
 *
 * @return	0 for success, error otherwise.
 */
int bt_content_ctrl_discover(struct bt_conn *conn);

/**
 * @brief	Put the UUIDs from this module into the buffer.
 *
 * @note	This partial data is used to build a complete extended advertising packet.
 *
 * @param[out]	uuid_buf	Buffer being populated with UUIDs.
 *
 * @return	0 for success, error otherwise.
 */
int bt_content_ctrl_uuid_populate(struct net_buf_simple *uuid_buf);

/**
 * @brief	Check if the media player is playing.
 *
 * @retval	true	Media player is in a playing state.
 * @retval	false	Media player is not in a playing state.
 */
bool bt_content_ctlr_media_state_playing(void);

/**
 * @brief	Initialize the content control module.
 *
 * @return	0 for success, error otherwise.
 */
int bt_content_ctrl_init(void);

#endif /* _BT_CONTENT_CTRL_H_ */
