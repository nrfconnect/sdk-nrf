/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_AT_SOCKET_
#define SLM_AT_SOCKET_

/**@file slm_at_socket.h
 *
 * @brief Vendor-specific AT command for Socket service.
 * @{
 */

/**
 * @brief Bind socket to a local network address.
 *
 * @param socket Socket to bind.
 * @param family Socket family (AF_INET/AF_INET6).
 * @param port Port to bind to.
 *
 * @retval 0 If the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int slm_bind_to_local_addr(int socket, int family, uint16_t port);

/**
 * @brief Initialize socket AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_socket_init(void);

/**
 * @brief Uninitialize socket AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_socket_uninit(void);
/** @} */

#endif /* SLM_AT_SOCKET_ */
