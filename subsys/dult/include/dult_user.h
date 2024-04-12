/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _DULT_USER_H_
#define _DULT_USER_H_

#include <stdint.h>
#include <stddef.h>

#include <dult.h>

/**
 * @defgroup dult_user Detecting Unwanted Location Trackers user
 * @brief Private API for DULT specification implementation DULT user module
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum length of a string parameter in the DULT user structure */
#define DULT_USER_STR_PARAM_LEN_MAX	64

/** @brief Check if provided DULT user is registered.
 *
 *  @param user	User structure to be checked.
 *
 *  @return True if the provided user is registered. Otherwise, False is returned.
 */
bool dult_user_is_registered(const struct dult_user *user);

/** @brief Get current registered DULT user structure.
 *
 *  @return Pointer to currently registered DULT user structure. If no user is registered,
 *	    NULL is returned.
 */
const struct dult_user *dult_user_get(void);

/** @brief Check if DULT is ready.
 *
 *  @return True if the DULT is ready. Otherwise, False is returned.
 */
bool dult_user_is_ready(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _DULT_USER_H_ */
