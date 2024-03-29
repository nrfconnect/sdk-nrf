/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_ENVELOPE_INFO_H__
#define SUIT_ENVELOPE_INFO_H__

#include <stdint.h>
#include <stddef.h>
#include <suit_plat_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inform the module that the SUIT candidate envelope was stored.
 *        This will extract the information such as the size of the envelope.
 *        If after calling this function the envelope is modified, the
 *        state of this module must be reset using @ref suit_envelope_info_reset
 *        and this function must be called again when the modifications to the
 *        candidate envelope are finished.
 *
 * @param[in] address The address of the stored envelope.
 * @param[in] max_size The maximum supported envelope size (larger envelope will cause an error).
 *
 * @return suit_plat_success on success, error code otherwise.
 */
suit_plat_err_t suit_envelope_info_candidate_stored(const uint8_t *address, size_t max_size);

/**
 * @brief Resets the module.
 *
 */
void suit_envelope_info_reset(void);

/**
 * @brief Gets information about the stored candidate envelope.
 *
 * @param[out] address The address of the stored envelope.
 * @param[out] size The size of the stored envelope.
 *
 * @return suit_plat_success on success, error code otherwise.
 */
suit_plat_err_t suit_envelope_info_get(const uint8_t **address, size_t *size);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_ENVELOPE_INFO_H__ */
