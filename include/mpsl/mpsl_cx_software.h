/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file mpsl_cx_software.h
 *
 * @defgroup mpsl_cx_software Multiprotocol Service Layer Software Coexistence
 *
 * @brief MPSL Software Coexistence
 * @{
 */

#ifndef MPSL_CX_SOFTWARE__
#define MPSL_CX_SOFTWARE__

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/sys_clock.h>

#include <stdint.h>

#ifdef CONFIG_MPSL_CX_SOFTWARE_RPC_CLIENT
/*
 * MPSL Software Coexistence RPC client provides serialization of the MPSL Software Coexistence
 * APIs, but it does not pull the actual MPSL Radio Coexistence library into the build system.
 * Hence the 'mpsl_cx_op_map_t' must be redefined below.
 */
enum mpsl_cx_op_t {
	MPSL_CX_OP_IDLE_LISTEN = 0x01,
	MPSL_CX_OP_RX = 0x02,
	MPSL_CX_OP_TX = 0x04,
};
typedef uint8_t mpsl_cx_op_map_t;
#else
#include <protocol/mpsl_cx_protocol_api.h>
#endif

/**
 * @brief Configures granted radio operations.
 *
 * By default, the MPSL Software Coexistence implementation grants access to all radio operations.
 * You can use this function to override the default granted operations.
 *
 * @param[in] ops     The bitmap of granted radio operations defined in mpsl_cx_op_t.
 * @param[in] timeout The time after which the granted operations are reset to defaults.
 *
 * @return - 0 on success negative error code otherwhise.
 */
int mpsl_cx_software_set_granted_ops(mpsl_cx_op_map_t ops, k_timeout_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* MPSL_CX_SOFTWARE__ */

/**@} */
