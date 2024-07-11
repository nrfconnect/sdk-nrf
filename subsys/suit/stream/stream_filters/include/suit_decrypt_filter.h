/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_DECRYPT_FILTER_H__
#define SUIT_DECRYPT_FILTER_H__

#include <suit_sink.h>
#include <suit_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get decrypt filter object
 *
 * @param[out] dec_sink  Pointer to destination sink_stream to pass decrypted data
 * @param[in]  end_info  Pointer to the structure with encryption info.
 * @param[in]  enc_sink  Pointer to source sink_stream to be filled with encrypted data
 *
 * @return SUIT_PLAT_SUCCESS if success otherwise error code
 */
suit_plat_err_t suit_decrypt_filter_get(struct stream_sink *dec_sink,
					struct suit_encryption_info *enc_info,
					struct stream_sink *enc_sink);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_DECRYPT_FILTER_H__ */
