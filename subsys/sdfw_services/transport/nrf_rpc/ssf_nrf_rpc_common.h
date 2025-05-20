/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SSF_NRF_RPC_COMMON_H__
#define SSF_NRF_RPC_COMMON_H__

#include <sdfw/sdfw_services/ssf_errno.h>

#include <nrf_rpc_errno.h>

static int ssf_translate_error(int nrf_rpc_err)
{
	switch (nrf_rpc_err) {
	case -NRF_EIO:
		return -SSF_EIO;
	case -NRF_EFAULT:
		return -SSF_EFAULT;
	case -NRF_EINVAL:
		return -SSF_EINVAL;
	case -NRF_EBADMSG:
		return -SSF_EBADMSG;
	case -NRF_EALREADY:
		return -SSF_EALREADY;
	default:
		if (nrf_rpc_err != 0) {
			return -SSF_EIO;
		}
		break;
	}

	return 0;
}

#endif /* SSF_NRF_RPC_COMMON_H__ */
