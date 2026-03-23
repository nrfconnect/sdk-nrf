/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief IPC service helper.
 */

#ifndef VPR_OFFLOADING_IPC_COMM_H__
#define VPR_OFFLOADING_IPC_COMM_H__

#include <zephyr/device.h>
#include <zephyr/ipc/ipc_service.h>

/** @brief Callback with received message.
 *
 * @param user_data User data provided when IPC communication helper was initialized.
 * @param msg Pointer to the message buffer.
 * @param size Message size.
 */
typedef void (*ipc_comm_cb_t)(void *user_data, const void *msg, size_t size);

/** @brief Data structure used by the IPC communication helper. */
struct ipc_comm_data {
	/** Endpoint. */
	struct ipc_ept ep;
#if defined(CONFIG_MULTITHREADING)
	/** Semaphore used for bounding. */
	struct k_sem sem;
#endif
	/** Flag indicating that communication is established. */
	volatile bool bounded;
};

/** @brief Structure used by the IPC communication helper. */
struct ipc_comm {
	/** Pointer to the data structure for the helper. */
	struct ipc_comm_data *data;
	/** Pointer to the IPC device that is used. */
	const struct device *ipc_dev;
	/** User data that is passed in the callback. */
	void *user_data;
	/** Callback called when message is received from the remote core. */
	ipc_comm_cb_t cb;
	/** Endpoint configuration structure. */
	struct ipc_ept_cfg ep_cfg;
};

/** @brief Macro used for initialization of the helper.
 *
 * Structure can be constant. Use forward declaration to get the address of the
 * variable that is being initialized.
 *
 * @param _ep_name Endpoint name.
 * @param _cb Callback.
 * @param _user_data User data passed to the callback.
 * @param _ipc_dev IPC device.
 * @param _ipc_comm_data Pointer to the data structure.
 * @param _self Pointer to the structure that is being initialized by this macro.
 */
#define IPC_COMM_INIT(_ep_name, _cb, _user_data, _ipc_dev, _ipc_comm_data, _self)                  \
	{                                                                                          \
		.data = _ipc_comm_data,                                                            \
		.ipc_dev = _ipc_dev,                                                               \
		.user_data = (void *)_user_data,                                                   \
		.cb = _cb,                                                                         \
		.ep_cfg =                                                                          \
			{                                                                          \
				.name = _ep_name,                                                  \
				.cb =                                                              \
					{                                                          \
						.bound = z_ipc_comm_ep_bound,                      \
						.received = z_ipc_comm_ep_recv,                    \
					},                                                         \
				.priv = (void *)_self,                                             \
			},                                                                         \
	}

/** @brief Internal function used for bounding.
 *
 * @param priv Endpoint data.
 */
void z_ipc_comm_ep_bound(void *priv);

/** @brief Internal callback used for the IPC service.
 *
 * @param buf Message buffer.
 * @param len Message length.
 * @param priv Endpoint data.
 */
void z_ipc_comm_ep_recv(const void *buf, size_t len, void *priv);

/** @brief Function for sending a message to the remote core.
 *
 * @param ipc_comm_data Data structure for the helper.
 * @param msg Message.
 * @param size Message size.
 *
 * @retval 0 Success.
 * @retval -EAGAIN Communication is not established.
 * @retval negative Failure. Error code returned by the IPC service API.
 */
int ipc_comm_send(const struct ipc_comm *ipc_helper, void *msg, size_t size);

/** @brief Initialization of the IPC communication helper.
 *
 * @param ipc_helper IPC helper instance.
 *
 * @retval 0 Success.
 * @retval negative Failure. Code returned by the IPC service API.
 */
int ipc_comm_init(const struct ipc_comm *ipc_helper);

#endif /* VPR_OFFLOADING_IPC_COMM_H__ */
