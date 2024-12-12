/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** .. include_startingpoint_device_info_source_rst */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <sdfw/sdfw_services/device_info_service.h>

#include "device_info_service_decode.h"
#include "device_info_service_encode.h"
#include "device_info_service_types.h"
#include <sdfw/sdfw_services/ssf_client.h>
#include "ssf_client_os.h"

/* Not exposed in header because not used by interface */
#define TESTIMPRINT_BYTES_LENGTH 32UL

SSF_CLIENT_LOG_REGISTER(device_info_service, CONFIG_SSF_DEVICE_INFO_SERVICE_LOG_LEVEL);

SSF_CLIENT_SERVICE_DEFINE(device_info_srvc, DEVICE_INFO, cbor_encode_device_info_req,
			  cbor_decode_device_info_resp);

int ssf_device_info_get_uuid(uint8_t *uuid_buff, const size_t uuid_buff_size)
{
	int err = -ENODATA;
	if ((NULL != uuid_buff) && (uuid_buff_size >= UUID_BYTES_LENGTH)) {
		int ret = -ENODATA;
		const uint8_t *rsp_pkt;

		struct device_info_req device_info_request = {
			.device_info_req_msg.read_req_action.operation_choice =
				operation_READ_DEVICE_INFO_c,
		};

		uint8_t uuid_bytes[UUID_BYTES_LENGTH] = {0};
		uint8_t testimprint_bytes[TESTIMPRINT_BYTES_LENGTH] = {0};

		struct read_resp response_message = {
			.read_resp_action.operation_choice = operation_UNSUPPORTED_c,
			.read_resp_status.stat_choice = stat_UNPROGRAMMED_c,
			.read_resp_data = {
				.device_info_partno = 0,
				.device_info_type = 0,
				.device_info_hwrevision = 0,
				.device_info_productionrevision = 0,
				.device_info_testimprint.value = testimprint_bytes,
				.device_info_testimprint.len = TESTIMPRINT_BYTES_LENGTH,
				.device_info_uuid.value = uuid_bytes,
				.device_info_uuid.len = UUID_BYTES_LENGTH,
			}};

		struct device_info_resp device_info_response = {
			.device_info_resp_msg = response_message,
		};

		err = ssf_client_send_request(&device_info_srvc, &device_info_request,
					      &device_info_response, &rsp_pkt);
		if (err != 0) {
			/* return ssf error value */
			return err;
		}

		if (device_info_response.device_info_resp_msg.read_resp_action.operation_choice ==
		    operation_READ_DEVICE_INFO_c) {
			ret = device_info_response.device_info_resp_msg.read_resp_status
				      .stat_choice;
			if (ret == stat_SUCCESS_c) {

				if (device_info_response.device_info_resp_msg.read_resp_data
					    .device_info_uuid.len == UUID_BYTES_LENGTH) {
					memcpy(uuid_buff,
					       device_info_response.device_info_resp_msg
						       .read_resp_data.device_info_uuid.value,
					       device_info_response.device_info_resp_msg
						       .read_resp_data.device_info_uuid.len);
				} else {
					/* message has the wrong size */
					ssf_client_decode_done(rsp_pkt);
					return -EMSGSIZE;
				}
			} else {
				/* operation failed on server side */
				ssf_client_decode_done(rsp_pkt);
				return -EPROTO;
			}
		} else {
			/* the received response message is not a read device info response */
			ssf_client_decode_done(rsp_pkt);
			return -EBADMSG;
		}

		ssf_client_decode_done(rsp_pkt);
	}

	return err;
}

/** .. include_endpoint_device_info_source_rst */
