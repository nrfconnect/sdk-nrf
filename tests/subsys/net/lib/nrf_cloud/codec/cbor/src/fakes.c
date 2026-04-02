/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Minimal fakes required to link nrf_cloud_coap_codec.c in the test
 * environment.
 *
 * nrf_cloud_coap_codec.c references several symbols from
 * nrf_cloud_codec_internal.c and nrf_cloud_mem.c.  Including those full
 * implementations would drag in the modem, FOTA, and REST
 * subsystems that are irrelevant for pure CBOR codec unit tests.
 *
 * Only three of those symbols are reachable in the CBOR path that is
 * exercised by these tests:
 *   - nrf_cloud_agnss_type_array_get  (called in coap_codec_agnss_encode)
 *   - nrf_cloud_calloc / nrf_cloud_free / nrf_cloud_malloc  (link-time deps)
 *
 * The remaining symbols (nrf_cloud_encode_message, nrf_cloud_error_msg_decode,
 * nrf_cloud_rest_fota_execution_decode) are only reachable via JSON paths that
 * the CBOR tests do not exercise; they are stubbed out with safe no-op or
 * error-returning implementations so that the linker is satisfied.
 *
 * NOTE: This file is intentionally a copy of (and closely mirrors) the fakes
 * in codec/json/src/fakes.c.  A future refactor can hoist the shared fakes
 * into a common location; that is deferred until the full codec test suite
 * shape is known.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <net/nrf_cloud.h>
#include <nrf_cloud_codec_internal.h>
#include <nrf_cloud_mem.h>

/* -------------------------------------------------------------------------
 * Memory wrappers (mirrors of nrf_cloud_mem.c)
 * -------------------------------------------------------------------------
 */

void *nrf_cloud_calloc(size_t count, size_t size)
{
	return calloc(count, size);
}

void *nrf_cloud_malloc(size_t size)
{
	return malloc(size);
}

void nrf_cloud_free(void *ptr)
{
	free(ptr);
}

/* -------------------------------------------------------------------------
 * JSON-path stubs — never called from the CBOR test path.
 * Returning an error code prevents silent mis-routing if they are ever
 * reached unexpectedly.
 * -------------------------------------------------------------------------
 */

int nrf_cloud_encode_message(const char *app_id, double value, const char *str_val,
			     const char *topic, int64_t ts, struct nrf_cloud_data *output)
{
	ARG_UNUSED(app_id);
	ARG_UNUSED(value);
	ARG_UNUSED(str_val);
	ARG_UNUSED(topic);
	ARG_UNUSED(ts);
	ARG_UNUSED(output);
	return -ENOTSUP;
}

int nrf_cloud_error_msg_decode(const char *const buf, const char *const app_id,
			       const char *const msg_type, enum nrf_cloud_error *const err)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(app_id);
	ARG_UNUSED(msg_type);
	ARG_UNUSED(err);
	/* Return non-zero: signals "no JSON error message found", which causes
	 * the callers (ground_fix / agnss / pgps decode) to return -EPROTO.
	 */
	return -ENOENT;
}

int nrf_cloud_rest_fota_execution_decode(const char *const response,
					 struct nrf_cloud_fota_job_info *const job)
{
	ARG_UNUSED(response);
	ARG_UNUSED(job);
	return -ENOTSUP;
}

/* -------------------------------------------------------------------------
 * nrf_cloud_agnss_type_array_get — called in the CBOR path of
 * coap_codec_agnss_encode when type == NRF_CLOUD_REST_AGNSS_REQ_CUSTOM.
 *
 * The real implementation parses an nrf_modem_gnss_agnss_data_frame bitmask
 * and fills in an array of nrf_cloud_agnss_type values.  For unit testing
 * purposes a fixed two-element response is sufficient; it keeps the test
 * hermetic and produces a deterministic, non-empty CBOR output.
 * -------------------------------------------------------------------------
 */

int nrf_cloud_agnss_type_array_get(const struct nrf_modem_gnss_agnss_data_frame *const request,
				   enum nrf_cloud_agnss_type *array, const size_t array_size)
{
	ARG_UNUSED(request);

	if (!array || array_size < 2) {
		return -EINVAL;
	}

	array[0] = NRF_CLOUD_AGNSS_GPS_UTC_PARAMETERS;
	array[1] = NRF_CLOUD_AGNSS_GPS_EPHEMERIDES;

	return 2;
}
