/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file dfu_target_smp_bt.h
 *
 * @defgroup dfu_target_smp_bt SMP DFU target Bluetooth LE transport
 * @{
 * @brief Bluetooth LE (GATT client) transport for the SMP DFU target.
 *
 * This module bridges the transport agnostic mcumgr SMP client used by the
 * SMP DFU target with the DFU SMP client
 * (@ref bt_dfu_smp). It allows firmware updates to be pushed over Bluetooth LE
 * to a remote device that exposes the mcumgr SMP service.
 */

#ifndef DFU_TARGET_SMP_BT_H__
#define DFU_TARGET_SMP_BT_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration */
struct bt_dfu_smp;

/**
 * @brief Register the Bluetooth LE SMP client transport.
 *
 * The application is responsible for establishing the connection, running
 * service discovery and assigning the handles to @p smp_client (see
 * @ref bt_dfu_smp_handles_assign) before calling this function.
 *
 * @param[in] smp_client Initialized and discovered DFU SMP Client instance.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p smp_client is NULL.
 * @retval -errno negative error code on failure.
 */
int dfu_target_smp_bt_transport_register(struct bt_dfu_smp *smp_client);

#ifdef __cplusplus
}
#endif

#endif /* DFU_TARGET_SMP_BT_H__ */

/** @} */
