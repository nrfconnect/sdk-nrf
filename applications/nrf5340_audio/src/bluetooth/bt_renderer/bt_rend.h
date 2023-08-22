/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BT_REND_H_
#define _BT_REND_H_

#include <zephyr/bluetooth/conn.h>

/**
 * @brief	Adjust volume up by one step.
 *
 * @return	0 if success, error otherwise.
 */
int bt_rend_volume_up(void);

/**
 * @brief	Adjust volume down by one step.
 *
 * @return	0 if success, error otherwise.
 */
int bt_rend_volume_down(void);

/**
 * @brief	Set the volume to the given @p volume value.
 *
 * @param[in]	volume		Value to set the volume to (0-255).
 * @param[in]	from_vcp	Describe if the function was called from a service
 *				or from somewhere else (buttons, shell, etc).
 *
 * @return	0 if success, error otherwise.
 */
int bt_rend_volume_set(uint8_t volume, bool from_vcp);

/**
 * @brief	Mute the volume.
 *
 * @param[in]	from_vcp	Describe if the function was called from a service
 *				or from somewhere else (buttons, shell, etc).
 *
 * @return	0 if success, error otherwise.
 */
int bt_rend_volume_mute(bool from_vcp);

/**
 * @brief	Unmute the volume.
 *
 * @return	0 if success, error otherwise.
 */
int bt_rend_volume_unmute(void);

/**
 * @brief	Discover the rendering services.
 *
 * @param[in]	conn	Pointer to the connection on which to do the discovery.
 *
 * @return	0 if success, error otherwise.
 */
int bt_rend_discover(struct bt_conn *conn);

/**
 * @brief	Put the UUIDs from this module into the buffer.
 *
 * @note	This partial data is used to build a complete extended advertising packet.
 *
 * @param[out]	uuid_buf	Buffer being populated with UUIDs.
 *
 * @return	0 for success, error otherwise.
 */
int bt_rend_uuid_populate(struct net_buf_simple *uuid_buf);

/**
 * @brief	Initialize the rendering services or profiles, or both.
 *
 * @return	0 if success, error otherwise.
 */
int bt_rend_init(void);

#endif /* _BT_REND_H_ */
