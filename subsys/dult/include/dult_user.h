/*
 * Copyright (c) 2024-2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _DULT_USER_H_
#define _DULT_USER_H_

#include <stdint.h>
#include <stddef.h>

#include <dult/dult.h>

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

/** @brief Get the currently associated DULT user.
 *
 *  This is the associated user, that is the user that called
 *  @ref dult_enable. It returns NULL while there is no associated user (before
 *  @ref dult_enable, after @ref dult_reset, and during the multi-user
 *  pre-association window).
 *
 *  @return Pointer to the currently associated DULT user, or NULL if there is none.
 */
const struct dult_user *dult_user_get_associated(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _DULT_USER_H_ */
