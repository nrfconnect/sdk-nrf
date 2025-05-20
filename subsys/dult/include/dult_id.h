/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _DULT_ID_H_
#define _DULT_ID_H_

#include <stdint.h>
#include <stddef.h>

/**
 * @defgroup dult_id Detecting Unwanted Location Trackers identifier
 * @brief Private API for DULT specification implementation of the identifier module
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Check if DULT is in identifier read state.
 *
 *  @return True if the DULT is in identifier read state. Otherwise, False is returned.
 */
bool dult_id_is_in_read_state(void);

/** @brief Get identifier payload string.
 *
 *  Thin wrapper for @ref payload_get callback from @ref dult_id_read_state_cb structure.
 *
 *  @param[out]    buf	Pointer to the buffer used to store identifier payload.
 *  @param[in,out] len	Pointer to the length (in bytes) of the buffer used to store
 *			identifier payload. A negative error code shall be returned
 *			if this value is too small. If the operation was successful, the
 *			length of the identifier payload shall be written to this pointer.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is
 *	    returned.
 */
int dult_id_payload_get(uint8_t *buf, size_t *len);

/** @brief Enable DULT identifier.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int dult_id_enable(void);

/** @brief Reset DULT identifier.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int dult_id_reset(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _DULT_ID_H_ */
