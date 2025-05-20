/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUITFU_MGMT_H_
#define SUITFU_MGMT_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Registers the SUIT-based update management command group handler.
 */
void img_mgmt_register_group(void);

/**
 * @brief Unregisters the SUIT-based update management command group handler.
 */
void img_mgmt_unregister_group(void);

#ifdef __cplusplus
}
#endif

#endif /* SUITFU_MGMT_H_ */
