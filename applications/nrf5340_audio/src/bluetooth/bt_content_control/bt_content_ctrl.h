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
 * @brief	Initialize the content control module.
 *
 * @return	0 for success, error otherwise.
 */
int bt_content_ctrl_init(void);

#endif /* _BT_CONTENT_CTRL_H_ */
