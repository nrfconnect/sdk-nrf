/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef IPC_BT_H_
#define IPC_BT_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Initialize the IPC Bluetooth interface.
 *
 * The function might not have any functionality if the interface
 * does not need to be initialized (or is initialized automatically).
 * When this function is not needed, it should be implemented as an empty
 * function and return -ENOSYS.
 *
 * @retval -ENOSYS The function is not implemented.
 * @return 0 in case of success or negative value in case of error.
 */
int ipc_bt_init(void);

/** @brief Give processing time to the IPC Bluetooth interface.
 *
 * The function might not have any functionality if the interface
 * does not need to be given processing time (or is processed automatically).
 * When this function is not needed, it should be implemented as an empty
 * function and return -ENOSYS.
 *
 * @retval -ENOSYS The function is not implemented.
 * @return 0 in case of success or negative value in case of error.
 */
int ipc_bt_process(void);

#ifdef __cplusplus
}
#endif

#endif /* IPC_BT_H_ */
