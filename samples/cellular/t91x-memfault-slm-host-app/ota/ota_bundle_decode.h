/*
 * Generated using zcbor version 0.8.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 3
 */

#ifndef OTA_BUNDLE_DECODE_H__
#define OTA_BUNDLE_DECODE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "ota_bundle_decode_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if DEFAULT_MAX_QTY != 3
#error "The type file was generated with a different default_max_qty than this file"
#endif


/**
 * Decode the CBOR header at the start of an OTA bundle.
 *
 * The element payload bytes follow the decoded CBOR map and are not decoded
 * or verified by this function. On success, payload_len_out reports how many
 * bytes of payload were consumed by the header, so the caller can locate the
 * first element's data. The caller must separately verify the payload size
 * and SHA-256 hash.
 *
 * @param payload         Start of the OTA bundle.
 * @param payload_len     Number of available bytes in payload.
 * @param result          Receives the decoded bundle metadata.
 * @param payload_len_out Optional; receives the number of bytes consumed by
 *                        the CBOR header.
 * @return ZCBOR_SUCCESS on success, otherwise a zcbor error code.
 */
int cbor_decode_ota_bundle(
		const uint8_t *payload, size_t payload_len,
		struct ota_bundle *result,
		size_t *payload_len_out);


#ifdef __cplusplus
}
#endif

#endif /* OTA_BUNDLE_DECODE_H__ */
