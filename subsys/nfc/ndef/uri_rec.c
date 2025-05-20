/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <errno.h>
#include <nfc/ndef/uri_rec.h>

const uint8_t nfc_ndef_uri_rec_type = 'U'; /**< URI Record type. */

/**
 * @brief Function for constructing the payload for a URI record.
 *
 * This function encodes the payload according to the URI record definition.
 * It implements an API compatible with @ref payload_constructor_t.
 *
 * @param input Pointer to the description of the payload.
 * @param buff Pointer to payload destination. If NULL, function will calculate
 * the expected size of the URI record payload.
 *
 * @param len Size of available memory to write as input. Size of generated
 * payload as output.
 *
 * @return 0 If the payload was encoded successfully.
 * @return -ENOMEM If the predicted payload size is bigger than the provided
 * buffer space.
 */
int nfc_ndef_uri_rec_payload_encode(struct nfc_ndef_uri_rec_payload *input,
				    uint8_t *buff,
				    uint32_t *len)
{
	if (buff) {
		/* Verify if there is enough available memory */
		if (input->uri_data_len >= *len) {
			return -ENOMEM;
		}
		/* Copy descriptor content into the buffer */
		*buff = input->uri_id_code;
		buff++;
		memcpy(buff, input->uri_data, input->uri_data_len);
	}

	*len = input->uri_data_len + 1;

	return 0;
}
