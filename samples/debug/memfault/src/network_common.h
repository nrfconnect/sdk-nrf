/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Initialize the network interface and establish a network connection.
 *
 */
void network_init(void);

/**
 * @brief A function to be called when the network connection has been established and IP address
 *        received. The application can establish a TCP connection when this function is called by
 *        the network implementation.
 */
void network_connected(void);

/**
 * @brief A function to be called when the network connection has been terminated.
 */
void network_disconnected(void);
