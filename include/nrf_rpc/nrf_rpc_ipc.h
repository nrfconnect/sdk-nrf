/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_RPC_IPC_H_
#define NRF_RPC_IPC_H_

#include <zephyr/device.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/kernel.h>

#include <nrf_rpc.h>
#include <nrf_rpc_tr.h>

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_rpc_ipc nRF RPC IPC Service transport
 * @brief nRF RPC IPC Service transport.
 *
 * @{
 */

/*  nRF IPC Service transport API strucure. It contains all
 *  necessary functions required by the nRF RPC library.
 */
extern const struct nrf_rpc_tr_api nrf_rpc_ipc_service_api;

/** @brief nRF RPC IPC transport endpoint configuration. */
struct nrf_rpc_ipc_endpoint {
	/** IPC Service endpoint configuration. */
	struct ipc_ept_cfg ept_cfg;

	/** IPC Service endpoint structure. */
	struct ipc_ept ept;

	/** IPC Service endpoint bond event. */
	struct k_event ept_bond;

	/** The absolute value for binding timeout, started when bonding procedure is initialized */
	k_timeout_t timeout;
};

/** @brief nRF RPC IPC Service transport instance. */
struct nrf_rpc_ipc {
	const struct device *ipc;

	/** Endpoint configuration. */
	struct nrf_rpc_ipc_endpoint endpoint;

	/** Data received callback. It is called when data was received on given endpoint. */
	nrf_rpc_tr_receive_handler_t receive_cb;

	/** User context. */
	void *context;

#if defined(CONFIG_NRF_RPC_IPC_SERVICE_RX_THREAD)
	/** Queue of the received packets waiting for processing by the Rx thread. */
	struct k_msgq *rx_msgq;

	/** Rx thread object. */
	struct k_thread *rx_thread;

	/** Rx thread stack. */
	k_thread_stack_t *rx_stack;
#endif

	/** Current transport state. */
	uint8_t state;
};

#if defined(CONFIG_NRF_RPC_IPC_SERVICE_RX_THREAD)

/** @brief Descriptor of a received packet queued for processing by the Rx thread.
 *
 * For internal use only.
 */
struct nrf_rpc_ipc_rx_packet {
	/** Packet data located in the held IPC Service Rx buffer. */
	const void *data;

	/** Packet length. */
	size_t len;
};

#endif /* CONFIG_NRF_RPC_IPC_SERVICE_RX_THREAD */

/** @brief Extern nRF RPC IPC Service transport declaration.
 *
 * Can be used in header files. It is useful when several nRF RPC group
 * are defined amongst different source files but they can share the same
 * transport instance.
 *
 * @param[in] _name Name of the nRF RPC transport.
 */
#define NRF_RPC_IPC_TRANSPORT_DECLARE(_name) \
	extern const struct nrf_rpc_tr _name

/** @brief Defines the nRF IPC Transport instance.
 *
 * It creates the nRF RPC IPC Service transport instance. The @p _ipc parameter defines
 * the destination remote CPU. A single instance of this transport can be shared between
 * several nRF RPC groups. Thus, a single endpoint is shared by multiple nRF RPC groups.
 * It is also allowed to share a single IPC instance with a different endpoint name between
 * groups.
 *
 * Example:
 *
 *  * Two groups share the same IPC instance and the same endpoint:
 *
 *      NRF_RPC_IPC_TRANSPORT(nrf_rpc_1, DEVICE_DT_GET(DT_NODELABEL(ipc0)), "nrf_rpc_ept_1");
 *
 *      NRF_RPC_GROUP_DEFINE(group_1, "Group_1", &nrf_rpc_1, NULL, NULL, NULL);
 *      NRF_RPC_GROUP_DEFINE(group_2, "Group_2", &nrf_rpc_1, NULL, NULL, NULL);
 *
 *  * Two groups share the same IPC instance but endpoint is different for each group:
 *
 *      NRF_RPC_IPC_TRANSPORT(nrf_rpc_1, DEVICE_DT_GET(DT_NODELABEL(ipc0)), "nrf_rpc_ept_1");
 *      NRF_RPC_IPC_TRANSPORT(nrf_rpc_2, DEVICE_DT_GET(DT_NODELABEL(ipc0)), "nrf_rpc_ept_2");
 *
 *      NRF_RPC_GROUP_DEFINE(group_1, "Group_1", &nrf_rpc_1, NULL, NULL, NULL);
 *      NRF_RPC_GROUP_DEFINE(group_2, "Group_2", &nrf_rpc_2, NULL, NULL, NULL);
 *
 *  * Each group use different IPC instance, for example each IPC instance defined communication
 *    with different remote CPUs:
 *
 *      NRF_RPC_IPC_TRANSPORT(nrf_rpc_1, DEVICE_DT_GET(DT_NODELABEL(ipc0)), "nrf_rpc_ept");
 *      NRF_RPC_IPC_TRANSPORT(nrf_rpc_2, DEVICE_DT_GET(DT_NODELABEL(ipc1)), "nrf_rpc_ept");
 *
 *      NRF_RPC_GROUP_DEFINE(group_1, "Group_1", &nrf_rpc_1, NULL, NULL, NULL);
 *      NRF_RPC_GROUP_DEFINE(group_2, "Group_2", &nrf_rpc_2, NULL, NULL, NULL);
 *
 * If @kconfig{CONFIG_NRF_RPC_IPC_SERVICE_RX_THREAD} is enabled, this macro also allocates
 * the RX packet queue, and the RX thread object and stack for the transport instance.
 * The thread is created when the transport is initialized.
 *
 * @param[in] _name nRF RPC IPC Service transport instance name.
 * @param[in] _ipc The instance used for the IPC Service to transfer data between CPUs.
 * @param[in] _ept_name IPC Service endpoint name. The endpoint must have the same name on the
 *                      corresponding remote CPU.
 */
#define NRF_RPC_IPC_TRANSPORT(_name, _ipc, _ept_name)                        \
	IF_ENABLED(CONFIG_NRF_RPC_IPC_SERVICE_RX_THREAD, (                   \
		K_MSGQ_DEFINE(_name##_rx_msgq,                               \
			      sizeof(struct nrf_rpc_ipc_rx_packet),          \
			      CONFIG_NRF_RPC_IPC_SERVICE_RX_QUEUE_SIZE,      \
			      sizeof(void *));                               \
		static K_THREAD_STACK_DEFINE(_name##_rx_stack,               \
			CONFIG_NRF_RPC_IPC_SERVICE_RX_THREAD_STACK_SIZE);    \
		static struct k_thread _name##_rx_thread;                    \
	))                                                                   \
									     \
	static struct nrf_rpc_ipc _name##_instance = {                       \
	       .ipc = _ipc,                                                  \
	       .endpoint.ept_cfg.name = _ept_name,                           \
	       IF_ENABLED(CONFIG_NRF_RPC_IPC_SERVICE_RX_THREAD,              \
			  (.rx_msgq = &_name##_rx_msgq,                      \
			   .rx_thread = &_name##_rx_thread,                  \
			   .rx_stack = _name##_rx_stack,))                   \
	};                                                                   \
									     \
	const struct nrf_rpc_tr _name = {                                    \
		.api = &nrf_rpc_ipc_service_api,                             \
		.ctx = &_name##_instance                                     \
	}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* NRF_RPC_IPC_H_ */
