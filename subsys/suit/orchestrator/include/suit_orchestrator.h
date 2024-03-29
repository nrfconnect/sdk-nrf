/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_ORCHESTRATOR_H__
#define SUIT_ORCHESTRATOR_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the SUIT orchestrator before it can be used.
 *
 * @return 0 on success, non-zero value otherwise.
 */
int suit_orchestrator_init(void);

/**
 * @brief Main function of the orchestrator, starting boot or update.
 *
 * @return 0 on success, negative value otherwise.
 */
int suit_orchestrator_entry(void);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_ORCHESTRATOR_H__ */
