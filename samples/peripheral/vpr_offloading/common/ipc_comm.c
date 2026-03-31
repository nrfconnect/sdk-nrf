/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ipc_comm.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ipc_comm);

/* Module is a helper for IPC service communication. It simplifies adding IPC communication
 * to other modules.
 */
void z_ipc_comm_ep_bound(void *priv)
{
	const struct ipc_comm *ipc_helper = priv;

#if defined(CONFIG_MULTITHREADING)
	k_sem_give(&ipc_helper->data->sem);
#endif
	ipc_helper->data->bounded = true;
}

void z_ipc_comm_ep_recv(const void *buf, size_t len, void *priv)
{
	const struct ipc_comm *ipc_helper = priv;

	ipc_helper->cb(ipc_helper->user_data, buf, len);
}

int ipc_comm_send(const struct ipc_comm *ipc_helper, void *msg, size_t size)
{
	int rv;

	if (ipc_helper->data->bounded == false) {
		/* Wait for endpoint to be bound */
#if defined(CONFIG_MULTITHREADING)
		rv = k_sem_take(&ipc_helper->data->sem, K_MSEC(1000));
#else
		while (ipc_helper->data->bounded != 0) {
		};
		rv = 0;
#endif
	}

	rv = ipc_service_send(&ipc_helper->data->ep, msg, size);
	return rv >= 0 ? 0 : rv;
}

int ipc_comm_init(const struct ipc_comm *ipc_helper)
{
	int rv;

#if defined(CONFIG_MULTITHREADING)
	k_sem_init(&ipc_helper->data->sem, 0, 1);
#endif
	rv = ipc_service_open_instance(ipc_helper->ipc_dev);
	if ((rv < 0) && (rv != -EALREADY)) {
		LOG_ERR("IPC communication failed to open: %d", rv);
		return rv;
	}

	rv = ipc_service_register_endpoint(ipc_helper->ipc_dev, &ipc_helper->data->ep,
					   &ipc_helper->ep_cfg);
	if (rv < 0) {
		return rv;
	}

	return rv;
}
