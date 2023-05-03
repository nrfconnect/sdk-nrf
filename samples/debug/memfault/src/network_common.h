/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Initialize network interface and establish network connection.
 *
 */
void network_init(void);

/**
 * @brief To be called when network connection has been established and IP address received.
 *        The application can establish TCP connection when this function is called by the network
 *        implementation.
 */
void network_connected(void);

/**
 * @brief To be called when network connection has been terminated.
 */
void network_disconnected(void);
