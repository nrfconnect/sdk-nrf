/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_RPC_IPC_H_
#define NRF_RPC_IPC_H_

#include <zephyr/device.h>
#include <zephyr/ipc/ipc_service.h>

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

	/** Current transport state. */
	uint8_t state;
};

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
 * @param[in] _name nRF RPC IPC Service transport instance name.
 * @param[in] _ipc The instance used for the IPC Service to transfer data between CPUs.
 * @param[in] _ept_name IPC Service endpoint name. The endpoint must have the same name on the
 *                      corresponding remote CPU.
 */
#define NRF_RPC_IPC_TRANSPORT(_name, _ipc, _ept_name)                        \
	static struct nrf_rpc_ipc _name##_instance = {                       \
	       .ipc = _ipc,                                                  \
	       .endpoint.ept_cfg.name = _ept_name,                           \
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
