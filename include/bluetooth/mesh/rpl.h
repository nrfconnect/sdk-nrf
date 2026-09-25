/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief Bluetooth Mesh Replay Protection List persistence.
 * @defgroup bt_mesh_rpl Bluetooth Mesh Replay Protection List persistence
 * @{
 */

#ifndef BT_MESH_RPL_H__
#define BT_MESH_RPL_H__

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Initialize persistent storage for the Replay Protection List.
 *
 * Mounts the underlying storage and restores any previously persisted RPL
 * entries. This must be called once, after `bt_mesh_init()`, before the node
 * starts processing mesh traffic.
 *
 * @retval 0 Success.
 * @retval -ENOMEM The persisted data contains more entries than
 *                 @kconfig{CONFIG_BT_MESH_CRPL} can hold. CRPL number of
 *                 entries were restored, the rest were dropped. If this is not
 *                 expected, the application should consider treating this as a
 *                 fatal error.
 * @retval -EINVAL Some entries were found in the persisted data that could not
 *                 be restored, either because of read errors or because the
 *                 data was not consistent with the already restored RPL data.
 *                 The RPL was partially restored based on the valid entries,
 *                 but the application should consider treating this as a fatal
 *                 error because unreadable or inconsistent data were found.
 * @retval -EACCES The underlying storage could not be mounted. RPL is NOT
 *                 initialized or restored, and no future updates will be
 *                 persisted.
 * @retval -EIO    The underlying storage experienced an error while trying to
 *                 scan for data to restore. RPL might be partially restored,
 *                 but an unknown number of entries were dropped. The
 *                 application should consider treating this as a fatal error
 *                 because the RPL is in an unknown state.
 */
int bt_mesh_rpl_init(void);

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_RPL_H__ */

/** @} */
