/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include <stdint.h>
#include "tfm_platform_api.h"
#include "tfm_ioctl_api.h"

__attribute__((section("SFN")))
enum tfm_platform_err_t tfm_platform_mem_read(void *destination, uint32_t addr,
					      size_t len, uint32_t *result)
{
	psa_status_t ret;
	psa_invec in_vec;
	psa_outvec out_vec;
	struct tfm_read_service_args_t args;
	struct tfm_read_service_out_t out;

	in_vec.base = (const void *)&args;
	in_vec.len = sizeof(args);

	out_vec.base = (void *)&out;
	out_vec.len = sizeof(out);

	args.destination = destination;
	args.addr = addr;
	args.len = len;

	ret = tfm_platform_ioctl(TFM_PLATFORM_IOCTL_READ_SERVICE, &in_vec,
				 &out_vec);

	return (enum tfm_platform_err_t) ret;
}
