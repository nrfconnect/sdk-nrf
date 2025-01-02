/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_DECRYPT_FILTER_H__
#define SUIT_DECRYPT_FILTER_H__

#include <suit_sink.h>
#include <suit_types.h>
#include <suit_metadata.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get decrypt filter object
 *
 * @param[out] in_sink   Pointer to input sink_stream to pass encrypted data
 * @param[in]  enc_info  Pointer to the structure with encryption info.
 * @param[in]  class_id  Pointer to the manifest class ID of the destination component
 * @param[in]  out_sink  Pointer to output sink_stream to be filled with decrypted data
 *
 * @return SUIT_PLAT_SUCCESS if success otherwise error code
 */
suit_plat_err_t suit_decrypt_filter_get(struct stream_sink *in_sink,
					struct suit_encryption_info *enc_info,
					const suit_manifest_class_id_t *class_id,
					struct stream_sink *out_sink);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_DECRYPT_FILTER_H__ */
