/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <sdfw/sdfw_services/device_info_service.h>

#include "device_info_service_decode.h"
#include "device_info_service_encode.h"
#include "device_info_service_types.h"
#include <sdfw/sdfw_services/ssf_client.h>
#include <sdfw/sdfw_services/ssf_errno.h>
#include "ssf_client_os.h"

/* Not exposed in header because not used by interface */
#define TESTIMPRINT_BYTES_LENGTH 32UL

SSF_CLIENT_SERVICE_DEFINE(device_info_srvc, DEVICE_INFO, cbor_encode_device_info_req,
			  cbor_decode_device_info_resp);

static int get_device_info_data(struct device_info *device_info_data)
{
	int ret = -SSF_ENODATA;

	if (device_info_data != NULL) {
		const uint8_t *rsp_pkt;

		int stat = stat_INTERNAL_ERROR_c;

		struct device_info_req device_info_request = {
			.device_info_req_msg.read_req_action.operation_choice =
				operation_READ_DEVICE_INFO_c,
		};

		struct device_info_resp device_info_response;

		ret = ssf_client_send_request(&device_info_srvc, &device_info_request,
					      &device_info_response, &rsp_pkt);
		if (ret == 0) {
			if (device_info_response.device_info_resp_msg.read_resp_action
				    .operation_choice == operation_READ_DEVICE_INFO_c) {
				stat = device_info_response.device_info_resp_msg.read_resp_status
					       .stat_choice;
				if (stat == stat_SUCCESS_c) {

					/* copy device info data to provided struction */
					device_info_data->device_info_partno =
						device_info_response.device_info_resp_msg
							.read_resp_data.device_info_partno;

					device_info_data->device_info_type =
						device_info_response.device_info_resp_msg
							.read_resp_data.device_info_type;

					device_info_data->device_info_hwrevision =
						device_info_response.device_info_resp_msg
							.read_resp_data.device_info_hwrevision;

					device_info_data->device_info_productionrevision =
						device_info_response.device_info_resp_msg
							.read_resp_data
							.device_info_productionrevision;

					memcpy((uint8_t *)(device_info_data->device_info_testimprint
								   .value),
					       (uint8_t *)(device_info_response.device_info_resp_msg
								   .read_resp_data
								   .device_info_testimprint.value),
					       device_info_response.device_info_resp_msg
						       .read_resp_data.device_info_testimprint.len);

					device_info_data->device_info_testimprint.len =
						device_info_response.device_info_resp_msg
							.read_resp_data.device_info_testimprint.len;

					memcpy((uint8_t *)(device_info_data->device_info_uuid
								   .value),
					       (uint8_t *)(device_info_response.device_info_resp_msg
								   .read_resp_data.device_info_uuid
								   .value),
					       device_info_response.device_info_resp_msg
						       .read_resp_data.device_info_uuid.len);

					device_info_data->device_info_uuid.len =
						device_info_response.device_info_resp_msg
							.read_resp_data.device_info_uuid.len;

					/* operation is successful */
					ret = 0;
				} else {
					/* operation failed on server side */
					ret = -SSF_EPROTO;
				}
			} else {
				/* the received response message is not a read device info response
				 */
				ret = -SSF_EBADMSG;
			}

			/* at least a response message was received, free response packet buffer */
			ssf_client_decode_done(rsp_pkt);
		} else {
			/* ssf error, no response received, no need to free response packet */
			/* return value will be returned by ssf_client_send_request call */
		}
	}

	return ret;
}

int ssf_device_info_get_uuid(uint8_t *uuid_buff)
{
	int err = -SSF_ENODATA;

	if (NULL != uuid_buff) {

		uint8_t testimprint_bytes[TESTIMPRINT_BYTES_LENGTH] = {0};

		struct device_info device_info_data = {
			.device_info_partno = 0,
			.device_info_type = 0,
			.device_info_hwrevision = 0,
			.device_info_productionrevision = 0,
			.device_info_testimprint.value = testimprint_bytes,
			.device_info_testimprint.len = TESTIMPRINT_BYTES_LENGTH,
			/* caller provided buffer for UUID value */
			.device_info_uuid.value = uuid_buff,
			.device_info_uuid.len = UUID_BYTES_LENGTH,
		};

		err = get_device_info_data(&device_info_data);
	} else {
		err = -SSF_ENOBUFS;
	}

	return err;
}
