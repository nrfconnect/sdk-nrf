/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Structure used to populate and describe the JSON payload sent to the AWS IoT Shadow Service. */

struct cluster_onoff {
	uint32_t onoff;
	uint32_t level_control;
};

struct payload {
	struct {
		struct {
			struct {
				struct cluster_onoff onoff;
			} node;
		} reported;
	} state;
};

/* @brief Functions that encodes a JSON string based on the payload structure.
 *
 * @param[out]	message Pointer to a buffer that the JSON string is written to.
 * @param[in]	size    Size of the output buffer, message.
 * @param[in]	payload Pointer to a payload structure that will be used
 *			to populate the JSON message.
 *
 * @return 0 on success, otherwise a negative value is returned.
 */
int codec_json_encode_update_message(char *message, size_t size, struct payload *payload);

/* @brief Function that decodes a JSON string received from the update/delta topic
 *	  and populates the payload structure.
 *
 * @param[in]	message	    Pointer to a buffer with the encoded JSON string.
 * @param[in]	size        Size of the output buffer, message.
 * @param[out]	payload Pointer to a payload structure that will be populated.
 *
 * @return 0 on success, otherwise a negative value is returned.
 */
int codec_json_decode_delta_message(char *message, size_t size, struct payload *payload);

#ifdef __cplusplus
}
#endif
