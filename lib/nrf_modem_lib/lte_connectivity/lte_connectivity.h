/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LTE_CONNECTIVITY_H__
#define LTE_CONNECTIVITY_H__

#include <zephyr/types.h>
#include <zephyr/net/conn_mgr_connectivity.h>

/**
 * @defgroup lte_connectivity LTE Connectivity layer
 * @{
 * @brief Library that provides connectivity for nRF91 based boards.
 *	  The library is intended to be integrated with and bound to a Zephyr network interface via
 *	  the Connection Manager binding structure conn_mgr_conn_api.
 *	  It can be managed via Zephyr's network management API and the
 *	  Connection Manager Connectivity API using calls such as net_if_down(), net_if_up(), and
 *	  conn_mgr_if_connect().
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Options used to specify desired behavior when the network interface is brough down by
 *	   calling net_if_down().
 */
enum lte_connectivity_if_down_options {
	/** Shutdown modem - GNSS functionality will not be preserved. */
	LTE_CONNECTIVITY_IF_DOWN_MODEM_SHUTDOWN = 1,

	/** Disconnect from LTE - GNSS functionality will be preserved. */
	LTE_CONNECTIVITY_IF_DOWN_LTE_DISCONNECT
};

/** @brief Option types used to specify library behavior. */
enum lte_connectivity_options {
	/** Sets desired behavior when lte_connectivity_disable() / net_if_down() is called. */
	LTE_CONNECTIVITY_IF_DOWN = 1,
};

/** @brief Macro used to define a private Connection Manager connectivity context type.
 *         Required but not implemented.
 */
#define LTE_CONNECTIVITY_CTX_TYPE void *

/** @brief Initialize the LTE Connectivity layer.
 *
 *  @details Bind to the network interface via the passed in binding structure.
 *	     Setup appropriate internal handlers and mark the network interface as dormant.
 *	     Set default options for connection timeout and persistency.
 *
 *  @param if_conn Pointer to the Connection Manager binding structure.
 */
void lte_connectivity_init(struct conn_mgr_conn_binding * const if_conn);

/** @brief Enable the LTE Connectivity layer.
 *
 *  @details Initialize the nRF Modem and Link controller libraries.
 *	     Modem reinitialization is carried out if required by the nRF Modem library.
 *	     Modem reinitialization is typically required after Modem DFU (FOTA) has been performed
 *	     in order to apply the new image. Refer to the nRF Modem library (nrfxlib) documentation
 *	     for more information.
 *
 *  @returns 0 on success, nonzero value on failure.
 *	     If a positive value is returned, modem reinitialization failed. (DFU related)
 *	     DFU related error codes are listed in nrf_modem.h prefixed NRF_MODEM_DFU_RESULT.
 */
int lte_connectivity_enable(void);

/** @brief Disable the LTE Connectivity layer.
 *
 *  @details The behavior of this function changes depending on library options:
 *
 *	     If LTE_CONNECTIVITY_IF_DOWN_MODEM_SHUTDOWN is set:
 *
 *	     Modem is completely shutdown and no modem functionality is preserved.
 *	     This is useful after modem DFU when the modem needs to be reinitialized which requires
 *	     that the modem is shutdown first. Default option.
 *
 *	     If LTE_CONNECTIVITY_IF_DOWN_LTE_DISCONNECT is set:
 *
 *	     Deactivates the LTE stack on the modem and cancels any ongoing LTE connection attempts.
 *	     The network interface will be marked as dormant if a pre-existing
 *	     PDN connection is deactivated, in addition, any IP address that is associated with the
 *	     network interface is removed.
 *	     The application can subscribe to the event NET_EVENT_L4_DISCONNECTED to get notified
 *	     when IP is no longer available.
 *
 *  @returns 0 on success, negative value on failure.
 */
int lte_connectivity_disable(void);

/** @brief Connect to LTE. This is a non-blocking function.
 *
 *  @details Activates the LTE stack on the modem. The modem will connect with the default network
 *	     mode set by the CONFIG_LTE_NETWORK_MODE link controller choice option.
 *	     If the modem is not able to establish a connection before the connection times out,
 *	     the event NET_EVENT_CONN_IF_TIMEOUT is notified.
 *	     When a timeout occurs LTE is deactivated, and it is up to the application to call
 *	     conn_mgr_if_connect() to start a new connection attempt.
 *	     When a connection is established, the IP addresses associated with the default PDP
 *	     (Packet Data Protocol) context is registered to the network interface and
 *	     the network interface is marked non-dormant.
 *	     The application can subscribe to the event NET_EVENT_L4_CONNECTED to get notified
 *	     when IP has been obtained.
 *
 *  @param if_conn Pointer to the Connection Manager binding structure.
 *
 *  @returns 0 on success, negative value on failure.
 */
int lte_connectivity_connect(struct conn_mgr_conn_binding * const if_conn);

/** @brief Disconnect from LTE.
 *
 *  @details Deactivates the LTE stack on the modem and cancels any ongoing LTE connection attempts.
 *	     The Network interface will be marked as dormant if any pre-existing
 *	     LTE connection is aborted, in addition, any IP address that is associated with the
 *	     network interface is removed.
 *	     The application can subscribe to the event NET_EVENT_L4_DISCONNECTED to get notified
 *	     when IP is no longer available.
 *
 *  @param if_conn Pointer to the Connection Manager binding structure.
 *
 *  @returns 0 on success, negative value on failure.
 */
int lte_connectivity_disconnect(struct conn_mgr_conn_binding * const if_conn);

/** @brief Set connectivity options.
 *
 *  @param if_conn Pointer to the Connection Manager binding structure.
 *  @param name Integer value representing the option to set.
 *  @param value Pointer to the value that will be assigned to the option.
 *  @param length Length (in bytes) of the value parameter.
 *
 *  @returns 0 on success, negative value on failure.
 *  @retval -ENOBUFS if length is too long.
 *  @retval -ENOPROTOOPT if name is not recognized.
 *  @retval -EBADF if the passed in option value is not recognized.
 */
int lte_connectivity_options_set(struct conn_mgr_conn_binding * const if_conn,
				   int name,
				   const void *value,
				   size_t length);

/** @brief Get connectivity options.
 *
 *  @param if_conn Pointer to the Connection Manager binding structure.
 *  @param name Integer value representing the option to get.
 *  @param value Pointer to a variable / buffer that will store the retrieved option value.
 *  @param length Pointer to a variable to store number of bytes written to the variable / buffer.
 *
 *  @returns 0 on success, negative value on failure.
 *  @retval -ENOPROTOOPT if name is not recognized.
 */
int lte_connectivity_options_get(struct conn_mgr_conn_binding * const if_conn,
				   int name,
				   void *value,
				   size_t *length);

#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /* LTE_CONNECTIVITY_H__ */
