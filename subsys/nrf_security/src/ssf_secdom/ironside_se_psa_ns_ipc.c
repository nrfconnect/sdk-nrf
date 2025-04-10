/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <stdbool.h>

#include <zephyr/ipc/ipc_service.h>

#include <psa/error.h>

static void ept_bound(void *priv)
{
}

static void ept_recv(const void *data, size_t len, void *priv)
{
}

static struct ipc_ept_cfg ept_cfg = {
	.name = "cpusec_cpuapp_ipc_ept",
	.cb = {
			.bound = ept_bound,
			.received = ept_recv,
		},
};

static struct ipc_ept ept;

psa_status_t ironside_se_psa_ns_ipc_setup(void)
{
	static bool initialized;

	if (initialized) {
		return PSA_SUCCESS;
	}

	const struct device *instance = DEVICE_DT_GET(DT_NODELABEL(cpusec_cpuapp_ipc));

	int ret = ipc_service_open_instance(instance);

	if (ret < 0) {
		return PSA_ERROR_COMMUNICATION_FAILURE;
	}

	ret = ipc_service_register_endpoint(instance, &ept, &ept_cfg);
	if (ret < 0) {
		return PSA_ERROR_COMMUNICATION_FAILURE;
	}

	initialized = true;

	return PSA_SUCCESS;
}

int32_t ironside_se_psa_ns_ipc_send(uint32_t *buf, size_t buf_len)
{
	return ipc_service_send(&ept, buf, buf_len);
}
