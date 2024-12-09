/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** .. include_startingpoint_device_info_source_rst */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <sdfw/sdfw_services/device_info_service.h>

#include "device_info_service_decode.h"
#include "device_info_service_encode.h"
#include "device_info_service_types.h"
#include <sdfw/sdfw_services/ssf_client.h>
#include "ssf_client_os.h"

#define UUID_BYTES_LENGTH 16UL

SSF_CLIENT_LOG_REGISTER(device_info_service, CONFIG_SSF_DEVICE_INFO_SERVICE_LOG_LEVEL);

SSF_CLIENT_SERVICE_DEFINE(device_info_srvc, DEVICE_INFO, cbor_encode_device_info_req,
			  cbor_decode_device_info_resp);

int ssf_device_info_get_uuid(uint32_t *uuid_words, const size_t uuid_words_count)
{
	int err = -1;

	if ((NULL != uuid_words) && (uuid_words_count >= UUID_BYTES_LENGTH)) {
		int ret = -1;
		const uint8_t *rsp_pkt;

		struct read_req uuid_read_request = {
			.read_req_target.entity_choice = entity_UUID_c,
		};

		struct device_info_req read_request = {.req_msg_choice = req_msg_read_req_m_c,
						       .req_msg_read_req_m = uuid_read_request};

		struct device_info_resp read_response;

		err = ssf_client_send_request(&device_info_srvc, &read_request, &read_response,
					      &rsp_pkt);
		if (err != 0) {
			return err;
		}

		struct read_resp uuid_read_response = read_response.resp_msg_read_resp_m;

		if (read_response.resp_msg_choice == resp_msg_read_resp_m_c) {
			if (entity_UUID_c == uuid_read_response.read_resp_target.entity_choice) {
				ret = read_response.resp_msg_read_resp_m.read_resp_status
					      .stat_choice;
				if (stat_SUCCESS_c != ret) {
					ssf_client_decode_done(rsp_pkt);
					return ret;
				}
			} else {
				/* the received response message is not a read UUID response */
				ssf_client_decode_done(rsp_pkt);
				return stat_INTERNAL_ERROR_c;
			}

		} else {
			/* the received response message is not a read response */
			ssf_client_decode_done(rsp_pkt);
			return stat_INTERNAL_ERROR_c;
		}

		size_t uuid_words_len = uuid_read_response.read_resp_data_uint_count;

		memcpy(uuid_words, uuid_read_response.read_resp_data_uint,
		       uuid_words_len * sizeof(uint32_t));

		ssf_client_decode_done(rsp_pkt);
	}

	return err;
}

/** .. include_endpoint_device_info_source_rst */
