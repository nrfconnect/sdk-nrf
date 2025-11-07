/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <stdbool.h>

#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <nrf_ironside/call.h>

#include <psa/client.h>
#include <psa/error.h>

#include <tfm/ironside/se/ipc_service.h>

#include "bounce_buffers.h"

/* The correctness of the serialization depends on these asserts */
BUILD_ASSERT(4 == sizeof(psa_handle_t));
BUILD_ASSERT(4 == sizeof(int32_t));
BUILD_ASSERT(4 == sizeof(const psa_invec *));
BUILD_ASSERT(4 == sizeof(size_t));
BUILD_ASSERT(4 == sizeof(psa_outvec *));
BUILD_ASSERT(4 == sizeof(psa_status_t *));

static psa_status_t psa_call_buffered_and_flushed(psa_handle_t handle, int32_t type,
						  const psa_invec *in_vec, size_t in_len,
						  psa_outvec *out_vec, size_t out_len)
{
	struct ironside_call_buf *const buf = ironside_call_alloc();

	buf->id = IRONSIDE_CALL_ID_PSA_V1;

	buf->args[IRONSIDE_SE_IPC_INDEX_HANDLE] = handle;
	buf->args[IRONSIDE_SE_IPC_INDEX_IN_VEC] = (uint32_t)in_vec;
	buf->args[IRONSIDE_SE_IPC_INDEX_IN_LEN] = in_len;
	buf->args[IRONSIDE_SE_IPC_INDEX_OUT_VEC] = (uint32_t)out_vec;
	buf->args[IRONSIDE_SE_IPC_INDEX_OUT_LEN] = out_len;
	buf->args[IRONSIDE_SE_IPC_INDEX_TYPE] = type;

	ironside_call_dispatch(buf);

	psa_status_t status = PSA_ERROR_COMMUNICATION_FAILURE;

	if (buf->status == IRONSIDE_CALL_STATUS_RSP_SUCCESS) {
		status = buf->args[IRONSIDE_SE_IPC_INDEX_STATUS];
	}

	ironside_call_release(buf);

	return status;
}

/*
 * Addresses that the cpusec reads and/or writes must be flushed
 * before the PSA call.
 *
 * After the PSA call, any address that the cpusec may have written to
 * must be flushed and invalidated.
 */
static psa_status_t psa_call_buffered(psa_handle_t handle, int32_t type, const psa_invec *in_vec,
				      size_t in_len, psa_outvec *out_vec, size_t out_len)
{
	for (int i = 0; i < out_len; i++) {
		sys_cache_data_flush_range((void *)out_vec[i].base, out_vec[i].len);
	}

	for (int i = 0; i < in_len; i++) {
		sys_cache_data_flush_range((void *)in_vec[i].base, in_vec[i].len);
	}

	sys_cache_data_flush_range((void *)in_vec, in_len * sizeof(in_vec[0]));
	sys_cache_data_flush_range((void *)out_vec, out_len * sizeof(out_vec[0]));

	psa_status_t status =
		psa_call_buffered_and_flushed(handle, type, in_vec, in_len, out_vec, out_len);

	for (int i = 0; i < out_len; i++) {
		sys_cache_data_flush_and_invd_range(out_vec[i].base, out_vec[i].len);
	}

	/* cpusec may write the number of bytes writen to out_vec[i].len */
	sys_cache_data_flush_and_invd_range((void *)out_vec, out_len * sizeof(out_vec[0]));

	return status;
}

/*
 * Both the start address and end address of buffers that the cpusec
 * writes must be 4-byte aligned. bounce_buffers, when enabled with
 * NRF_IRONSIDE_SE_PSA_SERVICES_OUT_BOUNCE_BUFFERS, will correct the
 * alignment by buffering such buffers when necessary.
 */
psa_status_t psa_call(psa_handle_t handle, int32_t type, const psa_invec *in_vec, size_t in_len,
		      psa_outvec *out_vec, size_t out_len)
{
	if (in_len > PSA_MAX_IOVEC || out_len > PSA_MAX_IOVEC) {
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	psa_status_t status;

	psa_outvec out_vec_copy[out_len];

	memcpy(out_vec_copy, out_vec, sizeof(out_vec_copy));

	for (int i = 0; i < out_len; i++) {
		out_vec_copy[i].base = bounce_buffers_prepare(out_vec[i].base, out_vec[i].len);
		if (out_vec_copy[i].base == NULL) {
			status = PSA_ERROR_INSUFFICIENT_MEMORY;
			goto exit;
		}
	}

	status = psa_call_buffered(handle, type, in_vec, in_len, out_vec_copy, out_len);

exit:
	for (int i = 0; i < out_len; i++) {
		bounce_buffers_release(out_vec[i].base, out_vec_copy[i].base, out_vec[i].len);

		out_vec[i].len = out_vec_copy[i].len;
	}

	return status;
}
