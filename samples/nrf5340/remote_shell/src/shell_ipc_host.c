/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/device.h>

#include "shell_ipc_host.h"

struct shell_ipc_host {
	struct ipc_ept ept;
	struct ipc_ept_cfg ept_cfg;
	struct k_sem ipc_bond_sem;
	shell_ipc_host_recv_cb cb;
	void *context;
};

static struct shell_ipc_host ipc_host;

static void bound(void *priv)
{
	struct shell_ipc_host *ipc = priv;

	k_sem_give(&ipc->ipc_bond_sem);
}

static void received(const void *data, size_t len, void *priv)
{
	struct shell_ipc_host *ipc = priv;

	if (!data || (len == 0)) {
		return;
	}

	if (ipc->cb) {
		ipc->cb(data, len, ipc->context);
	}
}

static int ipc_init(void)
{
	int err;
	const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_shell_ipc));

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	err = ipc_service_open_instance(dev);
	if (err && (err != -EALREADY)) {
		return err;
	}

	err = k_sem_init(&ipc_host.ipc_bond_sem, 0, 1);
	if (err) {
		return err;
	}

	ipc_host.ept_cfg.name = "remote shell";
	ipc_host.ept_cfg.cb.bound = bound;
	ipc_host.ept_cfg.cb.received = received;
	ipc_host.ept_cfg.priv = &ipc_host;

	err = ipc_service_register_endpoint(dev, &ipc_host.ept, &ipc_host.ept_cfg);
	if (err) {
		return err;
	}

	k_sem_take(&ipc_host.ipc_bond_sem, K_FOREVER);

	return 0;
}

int shell_ipc_host_init(shell_ipc_host_recv_cb cb, void *context)
{
	if (!cb) {
		return -EINVAL;
	}

	ipc_host.cb = cb;
	ipc_host.context = context;

	return ipc_init();
}

int shell_ipc_host_write(const uint8_t *data, size_t len)
{
	if (!data || (len == 0)) {
		return -EINVAL;
	}

	__ASSERT_NO_MSG(ipc_host.ept.instance);

	return ipc_service_send(&ipc_host.ept, data, len);
}
