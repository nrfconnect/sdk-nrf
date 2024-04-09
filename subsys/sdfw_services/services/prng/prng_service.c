/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <sdfw/sdfw_services/prng_service.h>

#include "prng_service_decode.h"
#include "prng_service_encode.h"
#include "prng_service_types.h"
#include <sdfw/sdfw_services/ssf_client.h>
#include "ssf_client_os.h"

SSF_CLIENT_SERVICE_DEFINE(prng_srvc, PRNG, cbor_encode_prng_req, cbor_decode_prng_rsp);

int ssf_prng_get_random(uint8_t *buffer, size_t length)
{
	int err;
	int32_t ret;
	uint32_t req;
	struct prng_rsp decoded_rsp;
	const uint8_t *rsp_pkt;

	if (buffer == NULL) {
		return -SSF_EINVAL;
	}

	req = length;

	err = ssf_client_send_request(&prng_srvc, &req, &decoded_rsp, &rsp_pkt);
	if (err != 0) {
		return -SSF_EIO;
	}

	ret = decoded_rsp.prng_rsp_status;
	if (ret != 0) {
		ssf_client_decode_done(rsp_pkt);
		return ret;
	}

	/* Fail if the received number of random bytes is not enough to fill the output buffer. */
	if (decoded_rsp.prng_rsp_buffer.len < length) {
		ssf_client_decode_done(rsp_pkt);
		return -SSF_EMSGSIZE;
	}

	memcpy(buffer, decoded_rsp.prng_rsp_buffer.value, length);

	ssf_client_decode_done(rsp_pkt);
	return 0;
}
