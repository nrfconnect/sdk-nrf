/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** .. include_startingpoint_echo_source_rst */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <sdfw/sdfw_services/echo_service.h>

#include "echo_service_decode.h"
#include "echo_service_encode.h"
#include "echo_service_types.h"
#include <sdfw/sdfw_services/ssf_client.h>
#include "ssf_client_os.h"

SSF_CLIENT_SERVICE_DEFINE(echo_srvc, ECHO, cbor_encode_echo_service_req,
			  cbor_decode_echo_service_rsp);

int ssf_echo(char *str_in, char *str_out, size_t str_out_size)
{
	int err;
	int ret;
	struct zcbor_string req_data;
	struct echo_service_rsp rsp_data;
	const uint8_t *rsp_pkt; /* For freeing response packet after copying rsp_str. */

	const uint8_t *rsp_str;
	size_t rsp_str_len;

	req_data.value = (uint8_t *)str_in;
	req_data.len = strlen(str_in);

	err = ssf_client_send_request(&echo_srvc, &req_data, &rsp_data, &rsp_pkt);
	if (err != 0) {
		return err;
	}

	ret = rsp_data.echo_service_rsp_ret;
	if (ret != 0) {
		ssf_client_decode_done(rsp_pkt);
		return ret;
	}

	rsp_str = rsp_data.echo_service_rsp_str_out.value;
	rsp_str_len = rsp_data.echo_service_rsp_str_out.len;
	memcpy(str_out, rsp_str, SSF_CLIENT_MIN(str_out_size, rsp_str_len));

	ssf_client_decode_done(rsp_pkt);
	return 0;
}
/** .. include_endpoint_echo_source_rst */
