/*
 * Copyright (c) 2022 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <tfm_ns_interface.h>

#include "secure_peripheral_partition.h"

#include "psa/client.h"
#include "psa_manifest/sid.h"

psa_status_t spp_process(void)
{
	psa_status_t status;
	psa_handle_t handle = TFM_SPP_PROCESS_HANDLE;

	status = psa_call(handle, PSA_IPC_CALL, NULL, 0, NULL, 0);

	return status;
}

psa_status_t spp_send(void)
{
	psa_status_t status;
	psa_handle_t handle = TFM_SPP_SEND_HANDLE;

	status = psa_call(handle, PSA_IPC_CALL, NULL, 0, NULL, 0);

	return status;
}
