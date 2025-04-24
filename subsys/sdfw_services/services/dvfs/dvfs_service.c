/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <sdfw/sdfw_services/dvfs_service.h>

#include "dvfs_service_decode.h"
#include "dvfs_service_encode.h"
#include "dvfs_service_types.h"
#include <sdfw/sdfw_services/ssf_client.h>
#include <sdfw/sdfw_services/ssf_errno.h>
#include "ssf_client_os.h"

SSF_CLIENT_SERVICE_DEFINE(dvfs_srvc, DVFS, cbor_encode_dvfs_oppoint_req,
	cbor_decode_dvfs_oppoint_rsp);

int ssf_dvfs_set_oppoint(uint8_t opp)
{
	int ret = -SSF_ENODATA;

	if (opp >= dvfs_oppoint_DVFS_FREQ_HIGH_c && opp <= dvfs_oppoint_DVFS_FREQ_LOW_c) {
		const uint8_t *rsp_pkt;

		int stat = stat_INTERNAL_ERROR_c;

		struct dvfs_oppoint_req dvfs_request = {
			.dvfs_oppoint_req_data
				.dvfs_oppoint_choice = opp,
		};

		struct dvfs_oppoint_rsp dvfs_response;

		ret = ssf_client_send_request(&dvfs_srvc, &dvfs_request,
					      &dvfs_response, &rsp_pkt);
		if (ret == 0) {
			if (dvfs_response.dvfs_oppoint_rsp_status.stat_choice ==
			    stat_SUCCESS_c) {
				stat = dvfs_response.dvfs_oppoint_rsp_status.stat_choice;
				if (stat == stat_SUCCESS_c) {
					printk("DVFS scaling done, new frequency setting %d\n",
					 dvfs_response.dvfs_oppoint_rsp_data
					 .dvfs_oppoint_choice);
					ret = 0;
				} else {
					ret = -SSF_EPROTO;
				}
			} else {
				ret = -SSF_EBADMSG;
			}

			ssf_client_decode_done(rsp_pkt);
		}
	}

	return ret;
}
