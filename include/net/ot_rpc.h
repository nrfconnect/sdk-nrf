/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef OT_RPC_H_
#define OT_RPC_H_

#include <openthread/link.h>

/**
 * @file
 * @defgroup ot_rpc OpenThread over RPC API additions
 * @{
 * @brief API additions for OpenThread over RPC.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Set factory assigned IEEE EUI64 on the remote device.
 *
 * The function issues an nRF RPC command that configures the factory-assigned IEEE EUI64,
 * which can then be retrieved using the @c otLinkGetFactoryAssignedIeeeEui64 API.
 *
 * @param eui64 IEEE EUI64 identifier.
 *
 * @retval OT_ERROR_NONE        IEEE EUI64 saved successfully.
 * @retval OT_ERROR_NOT_CAPABLE Setting IEEE EUI64 unsupported by the remote device.
 * @retval OT_ERROR_FAILED      Failed to save IEEE EUI64.
 */
otError ot_rpc_set_factory_assigned_ieee_eui64(const otExtAddress * eui64);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* OT_RPC_H_ */
