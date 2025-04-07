/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __CONFIG_SDFW_PSA_NS_IPC_H__
#define __CONFIG_SDFW_PSA_NS_IPC_H__

#include <stdint.h>

#include <psa/error.h>

/* Provides IPC services to ironside_se_ps_ns_api.c */

/*
 * setup must be called before send.
 *
 * successive calls to setup will have no effect.
 */
psa_status_t ironside_se_psa_ns_ipc_setup(void);

/*
 * A thin wrapper on top of ipc_service_send.
 *
 * See ipc_service_send for return codes etc.
 */
int32_t ironside_se_psa_ns_ipc_send(uint32_t *buf, size_t buf_len);

#endif /* __CONFIG_SDFW_PSA_NS_IPC_H__ */
