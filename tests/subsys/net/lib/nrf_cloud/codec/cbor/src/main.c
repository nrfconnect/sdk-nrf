/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Unit tests for the CoAP CBOR codec layer defined in
 * <coap_codec.h> (nrf_cloud_coap_codec.c).
 *
 * Scope — why the internal codec layer, not the public API:
 *   The public CoAP API (nrf_cloud_coap.h) requires an active CoAP connection
 *   to nRF Cloud and is therefore not suitable for unit testing on native_sim.
 *   The codec layer is where all encoding and decoding logic lives and can be
 *   tested deterministically without hardware or network dependencies.
 *   Public API correctness is validated through hardware-in-the-loop and
 *   system-level integration tests.
 *
 * Coverage: all nine public functions across seven functional groups.
 * Only the CBOR format path (COAP_CONTENT_FORMAT_APP_CBOR) is exercised;
 * the JSON fallback path is intentionally excluded — it is covered by the
 * codec/json/ suite via nrf_cloud_codec.h.
 *
 * Validation strategy:
 *  - Encode functions: verify return code, output length within expected
 *    bounds, and that known ASCII bytes (e.g. app_id) are present in the
 *    raw CBOR buffer.
 *  - Decode functions with hand-crafted CBOR: verify decoded field values
 *    match the encoded input.
 *  - Error paths: verify the documented error code is returned.
 *
 * A-GNSS response decode (coap_codec_agnss_resp_decode) is tested with a
 * synthetic octet-stream payload; the decode is a memcpy so any byte array
 * exercises the full code path.
 */

#include <zephyr/ztest.h>
/* nrf_cloud_codec.h defines struct nrf_cloud_obj_coap_cbor and
 * NRF_CLOUD_DATA_TYPE_* — must be included before coap_codec.h, which
 * uses the struct in its function signatures but does not include this header.
 */
#include <net/nrf_cloud_codec.h>
#include <net/nrf_cloud_location.h>   /* lte_lc.h, wifi_location_common.h, nrf_cloud.h */
#include <net/nrf_cloud_rest.h>        /* nrf_cloud_rest_agnss_request/result */
#include <net/nrf_cloud_pgps.h>        /* nrf_cloud_pgps_result, gps_pgps_request */
#include <zephyr/net/coap.h>           /* COAP_CONTENT_FORMAT_* */
#include "coap_codec.h"
#include <string.h>
#include <stdint.h>

/* =========================================================================
 * Helpers
 * =========================================================================
 */

/**
 * @brief Search for @p needle (length @p needle_len) inside @p haystack
 *        (length @p haystack_len).
 *
 * Used to spot-check that a known string (e.g. app_id) is present in the
 * raw CBOR output without hard-coding every byte.
 */
static bool buf_contains(const uint8_t *haystack, size_t haystack_len,
			  const uint8_t *needle, size_t needle_len)
{
	if (needle_len == 0 || haystack_len < needle_len) {
		return false;
	}
	for (size_t i = 0; i <= haystack_len - needle_len; i++) {
		if (memcmp(&haystack[i], needle, needle_len) == 0) {
			return true;
		}
	}
	return false;
}

/* =========================================================================
 * SUITE: coap_cbor_message_encode
 * Tests for coap_codec_message_encode().
 * =========================================================================
 */

ZTEST_SUITE(coap_cbor_message_encode, NULL, NULL, NULL, NULL, NULL);

ZTEST(coap_cbor_message_encode, test_str_type)
{
	uint8_t buf[MESSAGE_SEND_CBOR_MAX_SIZE];
	size_t len = sizeof(buf);
	struct nrf_cloud_obj_coap_cbor msg = {
		.app_id = "TEMP",
		.type = NRF_CLOUD_DATA_TYPE_STR,
		.str_val = "hello",
		.ts = 0,
	};

	zassert_equal(coap_codec_message_encode(&msg, buf, &len,
						COAP_CONTENT_FORMAT_APP_CBOR), 0);
	zassert_true(len > 0);
	zassert_true(len <= sizeof(buf));
	zassert_true(buf_contains(buf, len, (const uint8_t *)"TEMP", 4),
		     "app_id not found in CBOR output");
}

ZTEST(coap_cbor_message_encode, test_int_type)
{
	uint8_t buf[MESSAGE_SEND_CBOR_MAX_SIZE];
	size_t len = sizeof(buf);
	struct nrf_cloud_obj_coap_cbor msg = {
		.app_id = "CNT",
		.type = NRF_CLOUD_DATA_TYPE_INT,
		.int_val = 42,
		.ts = 0,
	};

	zassert_equal(coap_codec_message_encode(&msg, buf, &len,
						COAP_CONTENT_FORMAT_APP_CBOR), 0);
	zassert_true(len > 0);
	zassert_true(len <= sizeof(buf));
	zassert_true(buf_contains(buf, len, (const uint8_t *)"CNT", 3),
		     "app_id not found in CBOR output");
}

ZTEST(coap_cbor_message_encode, test_double_type)
{
	uint8_t buf[MESSAGE_SEND_CBOR_MAX_SIZE];
	size_t len = sizeof(buf);
	struct nrf_cloud_obj_coap_cbor msg = {
		.app_id = "HUM",
		.type = NRF_CLOUD_DATA_TYPE_DOUBLE,
		.double_val = 55.5,
		.ts = 1700000000LL,
	};

	zassert_equal(coap_codec_message_encode(&msg, buf, &len,
						COAP_CONTENT_FORMAT_APP_CBOR), 0);
	zassert_true(len > 0);
	zassert_true(len <= sizeof(buf));
	zassert_true(buf_contains(buf, len, (const uint8_t *)"HUM", 3),
		     "app_id not found in CBOR output");
}

ZTEST(coap_cbor_message_encode, test_pvt_type)
{
	uint8_t buf[MESSAGE_SEND_CBOR_MAX_SIZE];
	size_t len = sizeof(buf);
	struct nrf_cloud_gnss_pvt pvt = {
		.lat = 63.43,
		.lon = 10.39,
		.accuracy = 10.0f,
		.has_alt = 0,
		.has_speed = 0,
		.has_heading = 0,
	};
	struct nrf_cloud_obj_coap_cbor msg = {
		.app_id = "GNSS",
		.type = NRF_CLOUD_DATA_TYPE_PVT,
		.pvt = &pvt,
		.ts = 0,
	};

	zassert_equal(coap_codec_message_encode(&msg, buf, &len,
						COAP_CONTENT_FORMAT_APP_CBOR), 0);
	zassert_true(len > 0);
	zassert_true(len <= sizeof(buf));
	zassert_true(buf_contains(buf, len, (const uint8_t *)"GNSS", 4),
		     "app_id not found in CBOR output");
}

ZTEST(coap_cbor_message_encode, test_none_type_returns_einval)
{
	uint8_t buf[MESSAGE_SEND_CBOR_MAX_SIZE];
	size_t len = sizeof(buf);
	struct nrf_cloud_obj_coap_cbor msg = {
		.app_id = "X",
		.type = NRF_CLOUD_DATA_TYPE_NONE,
		.ts = 0,
	};

	zassert_equal(coap_codec_message_encode(&msg, buf, &len,
						COAP_CONTENT_FORMAT_APP_CBOR), -EINVAL);
}

ZTEST(coap_cbor_message_encode, test_buf_too_small)
{
	/* 1-byte buffer cannot hold any CBOR map */
	uint8_t buf[1];
	size_t len = sizeof(buf);
	struct nrf_cloud_obj_coap_cbor msg = {
		.app_id = "TEMP",
		.type = NRF_CLOUD_DATA_TYPE_STR,
		.str_val = "hello",
		.ts = 0,
	};

	zassert_equal(coap_codec_message_encode(&msg, buf, &len,
						COAP_CONTENT_FORMAT_APP_CBOR), -EINVAL);
	zassert_equal(len, 0);
}

/* =========================================================================
 * SUITE: coap_cbor_sensor_encode
 * Tests for coap_codec_sensor_encode().
 * =========================================================================
 */

ZTEST_SUITE(coap_cbor_sensor_encode, NULL, NULL, NULL, NULL, NULL);

ZTEST(coap_cbor_sensor_encode, test_valid)
{
	uint8_t buf[SENSOR_SEND_CBOR_MAX_SIZE];
	size_t len = sizeof(buf);

	zassert_equal(coap_codec_sensor_encode("TEMP", 25.5, 0, buf, &len,
					       COAP_CONTENT_FORMAT_APP_CBOR), 0);
	zassert_true(len > 0);
	zassert_true(len <= sizeof(buf));
	zassert_true(buf_contains(buf, len, (const uint8_t *)"TEMP", 4),
		     "app_id not found in CBOR output");
}

ZTEST(coap_cbor_sensor_encode, test_nonzero_timestamp)
{
	uint8_t buf[SENSOR_SEND_CBOR_MAX_SIZE];
	size_t len = sizeof(buf);

	/* Timestamp > 23 requires multi-byte CBOR integer encoding */
	zassert_equal(coap_codec_sensor_encode("HUM", 50.0, 1700000000LL, buf, &len,
					       COAP_CONTENT_FORMAT_APP_CBOR), 0);
	zassert_true(len > 0);
	zassert_true(len <= sizeof(buf));
}

ZTEST(coap_cbor_sensor_encode, test_buf_too_small)
{
	uint8_t buf[1];
	size_t len = sizeof(buf);

	zassert_equal(coap_codec_sensor_encode("TEMP", 25.5, 0, buf, &len,
					       COAP_CONTENT_FORMAT_APP_CBOR), -EINVAL);
	zassert_equal(len, 0);
}

/* =========================================================================
 * SUITE: coap_cbor_pvt_encode
 * Tests for coap_codec_pvt_encode().
 * =========================================================================
 */

ZTEST_SUITE(coap_cbor_pvt_encode, NULL, NULL, NULL, NULL, NULL);

ZTEST(coap_cbor_pvt_encode, test_basic)
{
	/* Minimal PVT: only lat, lon, accuracy (no optional fields) */
	uint8_t buf[MESSAGE_SEND_CBOR_MAX_SIZE];
	size_t len = sizeof(buf);
	struct nrf_cloud_gnss_pvt pvt = {
		.lat = 63.43,
		.lon = 10.39,
		.accuracy = 10.0f,
		.has_alt = 0,
		.has_speed = 0,
		.has_heading = 0,
	};

	zassert_equal(coap_codec_pvt_encode("GNSS", &pvt, 0, buf, &len,
					    COAP_CONTENT_FORMAT_APP_CBOR), 0);
	zassert_true(len > 0);
	zassert_true(len <= sizeof(buf));
	zassert_true(buf_contains(buf, len, (const uint8_t *)"GNSS", 4),
		     "app_id not found in CBOR output");
}

ZTEST(coap_cbor_pvt_encode, test_full_pvt)
{
	/* Full PVT with all optional fields set */
	uint8_t buf[MESSAGE_SEND_CBOR_MAX_SIZE];
	size_t len = sizeof(buf);
	struct nrf_cloud_gnss_pvt pvt = {
		.lat = 63.43,
		.lon = 10.39,
		.accuracy = 5.0f,
		.alt = 100.0f,
		.speed = 1.5f,
		.heading = 270.0f,
		.has_alt = 1,
		.has_speed = 1,
		.has_heading = 1,
	};

	zassert_equal(coap_codec_pvt_encode("GNSS", &pvt, 1700000000LL, buf, &len,
					    COAP_CONTENT_FORMAT_APP_CBOR), 0);
	zassert_true(len > 0);
	zassert_true(len <= sizeof(buf));
}

ZTEST(coap_cbor_pvt_encode, test_buf_too_small)
{
	uint8_t buf[1];
	size_t len = sizeof(buf);
	struct nrf_cloud_gnss_pvt pvt = {
		.lat = 0.0,
		.lon = 0.0,
		.accuracy = 1.0f,
	};

	zassert_equal(coap_codec_pvt_encode("GNSS", &pvt, 0, buf, &len,
					    COAP_CONTENT_FORMAT_APP_CBOR), -EINVAL);
	zassert_equal(len, 0);
}

/* =========================================================================
 * SUITE: coap_cbor_ground_fix
 * Tests for coap_codec_ground_fix_req_encode() and
 *           coap_codec_ground_fix_resp_decode().
 * =========================================================================
 */

ZTEST_SUITE(coap_cbor_ground_fix, NULL, NULL, NULL, NULL, NULL);

ZTEST(coap_cbor_ground_fix, test_req_encode_cell_only)
{
	uint8_t buf[LOCATION_GET_CBOR_MAX_SIZE];
	size_t len = sizeof(buf);
	struct lte_lc_cells_info cell_info = {
		.current_cell = {
			.mcc = 242,
			.mnc = 1,
			.id = 123,
			.tac = 0x1234,
			/* Omit optional fields to keep the CBOR minimal */
			.earfcn = NRF_CLOUD_LOCATION_CELL_OMIT_EARFCN,
			.timing_advance = NRF_CLOUD_LOCATION_CELL_OMIT_TIME_ADV,
			.rsrp = NRF_CLOUD_LOCATION_CELL_OMIT_RSRP,
			.rsrq = NRF_CLOUD_LOCATION_CELL_OMIT_RSRQ,
		},
		.ncells_count = 0,
	};

	zassert_equal(coap_codec_ground_fix_req_encode(&cell_info, NULL, buf, &len,
						       COAP_CONTENT_FORMAT_APP_CBOR), 0);
	zassert_true(len > 0);
	zassert_true(len <= sizeof(buf));
}

ZTEST(coap_cbor_ground_fix, test_req_encode_invalid_fmt)
{
	/* Only CBOR is supported for ground fix requests */
	uint8_t buf[LOCATION_GET_CBOR_MAX_SIZE];
	size_t len = sizeof(buf);
	struct lte_lc_cells_info cell_info = {
		.current_cell = {.id = 1, .mcc = 242, .mnc = 1, .tac = 1},
	};

	zassert_equal(coap_codec_ground_fix_req_encode(&cell_info, NULL, buf, &len,
						       COAP_CONTENT_FORMAT_APP_JSON), -ENOTSUP);
}

ZTEST(coap_cbor_ground_fix, test_req_encode_buf_too_small)
{
	uint8_t buf[1];
	size_t len = sizeof(buf);
	struct lte_lc_cells_info cell_info = {
		.current_cell = {.id = 1, .mcc = 242, .mnc = 1, .tac = 1},
	};

	zassert_equal(coap_codec_ground_fix_req_encode(&cell_info, NULL, buf, &len,
						       COAP_CONTENT_FORMAT_APP_CBOR), -EINVAL);
	zassert_equal(len, 0);
}

/*
 * Hand-crafted CBOR for a ground_fix_resp, mirroring a real nRF Cloud response.
 * Values taken from the nRF Cloud ground-fix API documentation example:
 *   { 1: 45.524098, 2: -122.688408, 3: 2000, 4: "SCELL" }
 *
 * Key mapping (from CDDL / ground_fix_decode.c):
 *   1 → lat (float64), 2 → lon (float64),
 *   3 → uncertainty (int or float), 4 → fulfilledWith (tstr)
 *
 * Expected decoded values:
 *   lat=45.524098, lon=-122.688408, unc=2000, type=LOCATION_TYPE_SINGLE_CELL
 */
static const uint8_t ground_fix_resp_scell[] = {
	0xA4,                                                   /* map(4)                  */
	0x01,                                                   /* key 1                   */
	0xFB, 0x40, 0x46, 0xC3, 0x15, 0xA4, 0xAC, 0xF3, 0x13, /* float64(45.524098)      */
	0x02,                                                   /* key 2                   */
	0xFB, 0xC0, 0x5E, 0xAC, 0x0E, 0xE0, 0x6D, 0x93, 0x81, /* float64(-122.688408)    */
	0x03,                                                   /* key 3                   */
	0x19, 0x07, 0xD0,                                       /* uint(2000)              */
	0x04,                                                   /* key 4                   */
	0x65, 0x53, 0x43, 0x45, 0x4C, 0x4C,                    /* tstr "SCELL"            */
};

/*
 * Hand-crafted CBOR for a ground_fix_resp, mirroring a real nRF Cloud response.
 * Values taken from the nRF Cloud ground-fix API documentation example:
 *   { 1: 45.524098, 2: -122.688408, 3: 300, 4: "MCELL" }
 *
 * Expected decoded values:
 *   lat=45.524098, lon=-122.688408, unc=300, type=LOCATION_TYPE_MULTI_CELL
 */
static const uint8_t ground_fix_resp_mcell[] = {
	0xA4,                                                   /* map(4)                  */
	0x01,                                                   /* key 1                   */
	0xFB, 0x40, 0x46, 0xC3, 0x15, 0xA4, 0xAC, 0xF3, 0x13, /* float64(45.524098)      */
	0x02,                                                   /* key 2                   */
	0xFB, 0xC0, 0x5E, 0xAC, 0x0E, 0xE0, 0x6D, 0x93, 0x81, /* float64(-122.688408)    */
	0x03,                                                   /* key 3                   */
	0x19, 0x01, 0x2C,                                       /* uint(300)               */
	0x04,                                                   /* key 4                   */
	0x65, 0x4D, 0x43, 0x45, 0x4C, 0x4C,                    /* tstr "MCELL"            */
};

ZTEST(coap_cbor_ground_fix, test_resp_decode_scell)
{
	struct nrf_cloud_location_result result = {0};

	zassert_equal(coap_codec_ground_fix_resp_decode(
			      &result, ground_fix_resp_scell,
			      sizeof(ground_fix_resp_scell),
			      COAP_CONTENT_FORMAT_APP_CBOR), 0);

	zassert_equal(result.type, LOCATION_TYPE_SINGLE_CELL);
	zassert_within(result.lat, 45.524098, 1e-6);
	zassert_within(result.lon, -122.688408, 1e-6);
	zassert_equal(result.unc, 2000);
}

ZTEST(coap_cbor_ground_fix, test_resp_decode_mcell)
{
	struct nrf_cloud_location_result result = {0};

	zassert_equal(coap_codec_ground_fix_resp_decode(
			      &result, ground_fix_resp_mcell,
			      sizeof(ground_fix_resp_mcell),
			      COAP_CONTENT_FORMAT_APP_CBOR), 0);

	zassert_equal(result.type, LOCATION_TYPE_MULTI_CELL);
	zassert_within(result.lat, 45.524098, 1e-6);
	zassert_within(result.lon, -122.688408, 1e-6);
	zassert_equal(result.unc, 300);
}

ZTEST(coap_cbor_ground_fix, test_resp_decode_invalid_fmt)
{
	/* JSON format: nrf_cloud_error_msg_decode stub returns -ENOENT
	 * → the function returns -EPROTO.
	 */
	struct nrf_cloud_location_result result = {0};
	const uint8_t dummy[] = {0x01};

	zassert_equal(coap_codec_ground_fix_resp_decode(&result, dummy, sizeof(dummy),
							COAP_CONTENT_FORMAT_APP_JSON), -EPROTO);
}

ZTEST(coap_cbor_ground_fix, test_resp_decode_malformed_cbor)
{
	struct nrf_cloud_location_result result = {0};
	/* Random bytes that cannot decode as a valid ground_fix_resp */
	const uint8_t garbage[] = {0xFF, 0xAA, 0xBB, 0xCC, 0xDD};

	zassert_equal(coap_codec_ground_fix_resp_decode(&result, garbage, sizeof(garbage),
							COAP_CONTENT_FORMAT_APP_CBOR), -EINVAL);
}

/* =========================================================================
 * SUITE: coap_cbor_agnss
 * Tests for coap_codec_agnss_encode() and coap_codec_agnss_resp_decode().
 *
 * Note: coap_codec_agnss_resp_decode() does not use CBOR — nRF Cloud returns
 * A-GNSS assistance data as raw binary in APP_OCTET_STREAM format, which the
 * function copies directly into result->buf.  It is tested here because it
 * belongs to nrf_cloud_coap_codec.c, the module under test for this suite.
 * =========================================================================
 */

ZTEST_SUITE(coap_cbor_agnss, NULL, NULL, NULL, NULL, NULL);

ZTEST(coap_cbor_agnss, test_encode_custom_type)
{
	uint8_t buf[AGNSS_GET_CBOR_MAX_SIZE];
	size_t len = sizeof(buf);
	/* nrf_modem_gnss_agnss_data_frame is a large struct; zero-init is fine
	 * because our stub ignores the content and returns fixed types.
	 */
	struct nrf_modem_gnss_agnss_data_frame agnss_data = {0};
	struct nrf_cloud_rest_agnss_request request = {
		.type = NRF_CLOUD_REST_AGNSS_REQ_CUSTOM,
		.agnss_req = &agnss_data,
		.net_info = NULL,
		.filtered = false,
		.mask_angle = 0,
	};

	zassert_equal(coap_codec_agnss_encode(&request, buf, &len,
					      COAP_CONTENT_FORMAT_APP_CBOR), 0);
	zassert_true(len > 0);
	zassert_true(len <= sizeof(buf));
}

ZTEST(coap_cbor_agnss, test_encode_with_net_info_and_filter)
{
	uint8_t buf[AGNSS_GET_CBOR_MAX_SIZE];
	size_t len = sizeof(buf);
	struct nrf_modem_gnss_agnss_data_frame agnss_data = {0};
	struct lte_lc_cells_info net_info = {
		.current_cell = {
			.id = 1000,  /* != LTE_LC_CELL_EUTRAN_ID_MAX */
			.mcc = 242,
			.mnc = 1,
			.tac = 0x1234,
			.rsrp = 50,  /* != LTE_LC_CELL_RSRP_INVALID */
		},
	};
	struct nrf_cloud_rest_agnss_request request = {
		.type = NRF_CLOUD_REST_AGNSS_REQ_CUSTOM,
		.agnss_req = &agnss_data,
		.net_info = &net_info,
		.filtered = true,
		.mask_angle = 10,  /* != DEFAULT_MASK_ANGLE (5), so mask field added */
	};

	zassert_equal(coap_codec_agnss_encode(&request, buf, &len,
					      COAP_CONTENT_FORMAT_APP_CBOR), 0);
	zassert_true(len > 0);
	zassert_true(len <= sizeof(buf));
}

ZTEST(coap_cbor_agnss, test_encode_non_custom_type_returns_enotsup)
{
	uint8_t buf[AGNSS_GET_CBOR_MAX_SIZE];
	size_t len = sizeof(buf);
	struct nrf_modem_gnss_agnss_data_frame agnss_data = {0};
	struct nrf_cloud_rest_agnss_request request = {
		.type = NRF_CLOUD_REST_AGNSS_REQ_ASSISTANCE,  /* not CUSTOM */
		.agnss_req = &agnss_data,
	};

	zassert_equal(coap_codec_agnss_encode(&request, buf, &len,
					      COAP_CONTENT_FORMAT_APP_CBOR), -ENOTSUP);
}

ZTEST(coap_cbor_agnss, test_encode_invalid_fmt_returns_enotsup)
{
	/* A-GNSS encode only supports CBOR */
	uint8_t buf[AGNSS_GET_CBOR_MAX_SIZE];
	size_t len = sizeof(buf);
	struct nrf_modem_gnss_agnss_data_frame agnss_data = {0};
	struct nrf_cloud_rest_agnss_request request = {
		.type = NRF_CLOUD_REST_AGNSS_REQ_CUSTOM,
		.agnss_req = &agnss_data,
	};

	zassert_equal(coap_codec_agnss_encode(&request, buf, &len,
					      COAP_CONTENT_FORMAT_APP_JSON), -ENOTSUP);
}

ZTEST(coap_cbor_agnss, test_decode_cbor_fmt_returns_enotsup)
{
	/* A-GNSS responses are binary octet-stream, not CBOR.
	 * Passing CBOR format must return -ENOTSUP.
	 */
	struct nrf_cloud_rest_agnss_result result = {0};
	const uint8_t dummy[] = {0x01, 0x02};

	zassert_equal(coap_codec_agnss_resp_decode(&result, dummy, sizeof(dummy),
						   COAP_CONTENT_FORMAT_APP_CBOR), -ENOTSUP);
}

ZTEST(coap_cbor_agnss, test_decode_octet_stream)
{
	/* The decode is a memcpy: any byte array is a valid octet-stream
	 * A-GNSS payload.  Verify the bytes land in result->buf unchanged.
	 */
	static const uint8_t payload[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03, 0x04};
	uint8_t out[sizeof(payload)];
	struct nrf_cloud_rest_agnss_result result = {
		.buf = out,
		.buf_sz = sizeof(out),
	};

	zassert_equal(coap_codec_agnss_resp_decode(&result, payload, sizeof(payload),
						   COAP_CONTENT_FORMAT_APP_OCTET_STREAM), 0);
	zassert_equal(result.agnss_sz, sizeof(payload));
	zassert_mem_equal(out, payload, sizeof(payload));
}

ZTEST(coap_cbor_agnss, test_decode_octet_stream_truncated)
{
	/* When result->buf_sz < response length the data is truncated to buf_sz. */
	static const uint8_t payload[] = {0x01, 0x02, 0x03, 0x04, 0x05};
	uint8_t out[3];
	struct nrf_cloud_rest_agnss_result result = {
		.buf = out,
		.buf_sz = sizeof(out),
	};

	zassert_equal(coap_codec_agnss_resp_decode(&result, payload, sizeof(payload),
						   COAP_CONTENT_FORMAT_APP_OCTET_STREAM), 0);
	zassert_equal(result.agnss_sz, sizeof(out));
	zassert_mem_equal(out, payload, sizeof(out));
}

/* =========================================================================
 * SUITE: coap_cbor_pgps
 * Tests for coap_codec_pgps_encode() and coap_codec_pgps_resp_decode().
 * =========================================================================
 */

ZTEST_SUITE(coap_cbor_pgps, NULL, NULL, NULL, NULL, NULL);

ZTEST(coap_cbor_pgps, test_encode_valid)
{
	uint8_t buf[PGPS_URL_GET_CBOR_MAX_SIZE];
	size_t len = sizeof(buf);
	struct gps_pgps_request pgps_req = {
		.prediction_count = 42,
		.prediction_period_min = 240,
		.gps_day = 3000,
		.gps_time_of_day = 43200,
	};
	struct nrf_cloud_rest_pgps_request request = {
		.pgps_req = &pgps_req,
	};

	zassert_equal(coap_codec_pgps_encode(&request, buf, &len,
					     COAP_CONTENT_FORMAT_APP_CBOR), 0);
	zassert_true(len > 0);
	zassert_true(len <= sizeof(buf));
}

ZTEST(coap_cbor_pgps, test_encode_invalid_fmt_returns_enotsup)
{
	/* P-GPS encode only supports CBOR */
	uint8_t buf[PGPS_URL_GET_CBOR_MAX_SIZE];
	size_t len = sizeof(buf);
	struct gps_pgps_request pgps_req = {
		.prediction_count = 1,
		.prediction_period_min = 120,
		.gps_day = 1,
		.gps_time_of_day = 0,
	};
	struct nrf_cloud_rest_pgps_request request = {.pgps_req = &pgps_req};

	zassert_equal(coap_codec_pgps_encode(&request, buf, &len,
					     COAP_CONTENT_FORMAT_APP_JSON), -ENOTSUP);
}

ZTEST(coap_cbor_pgps, test_encode_buf_too_small)
{
	uint8_t buf[1];
	size_t len = sizeof(buf);
	struct gps_pgps_request pgps_req = {
		.prediction_count = 42,
		.prediction_period_min = 240,
		.gps_day = 3000,
		.gps_time_of_day = 43200,
	};
	struct nrf_cloud_rest_pgps_request request = {.pgps_req = &pgps_req};

	zassert_equal(coap_codec_pgps_encode(&request, buf, &len,
					     COAP_CONTENT_FORMAT_APP_CBOR), -EINVAL);
	zassert_equal(len, 0);
}

/*
 * Hand-crafted CBOR for a pgps_resp, mirroring a real nRF Cloud P-GPS response:
 *   { 1: "pgnss.nrfcloud.com", 2: "public/15131-0_15135-72000.bin" }
 *
 * Key mapping (from CDDL / pgps_decode.c):
 *   1 → host (tstr), 2 → path (tstr)
 *
 * CBOR encoding:
 *   host (18 bytes): major type 3 + len 18 → 0x72
 *   path (30 bytes): major type 3 + additional info 24 → 0x78 0x1E
 */
static const uint8_t pgps_resp_cbor[] = {
	0xA2,                                           /* map(2)        */
	0x01,                                           /* key 1         */
	0x72,                                           /* tstr(18)      */
	0x70, 0x67, 0x6E, 0x73, 0x73, 0x2E, 0x6E, 0x72, /* "pgnss.nr   */
	0x66, 0x63, 0x6C, 0x6F, 0x75, 0x64, 0x2E, 0x63, /*  fcloud.c   */
	0x6F, 0x6D,                                     /*  om"          */
	0x02,                                           /* key 2         */
	0x78, 0x1E,                                     /* tstr(30)      */
	0x70, 0x75, 0x62, 0x6C, 0x69, 0x63, 0x2F,      /* "public/      */
	0x31, 0x35, 0x31, 0x33, 0x31, 0x2D, 0x30, 0x5F, /*  15131-0_   */
	0x31, 0x35, 0x31, 0x33, 0x35, 0x2D, 0x37, 0x32, /*  15135-72   */
	0x30, 0x30, 0x30, 0x2E, 0x62, 0x69, 0x6E,      /*  000.bin"     */
};

#define TEST_PGPS_HOST "pgnss.nrfcloud.com"
#define TEST_PGPS_PATH "public/15131-0_15135-72000.bin"

ZTEST(coap_cbor_pgps, test_decode_valid)
{
	char host[32] = {0};
	char path[32] = {0};
	struct nrf_cloud_pgps_result result = {
		.host = host,
		.host_sz = sizeof(host),
		.path = path,
		.path_sz = sizeof(path),
	};

	zassert_equal(coap_codec_pgps_resp_decode(&result, pgps_resp_cbor,
						  sizeof(pgps_resp_cbor),
						  COAP_CONTENT_FORMAT_APP_CBOR), 0);

	zassert_mem_equal(result.host, TEST_PGPS_HOST, strlen(TEST_PGPS_HOST));
	zassert_mem_equal(result.path, TEST_PGPS_PATH, strlen(TEST_PGPS_PATH));
	zassert_equal(result.host_sz, strlen(TEST_PGPS_HOST));
	zassert_equal(result.path_sz, strlen(TEST_PGPS_PATH));
}

ZTEST(coap_cbor_pgps, test_decode_invalid_fmt)
{
	/* JSON format: nrf_cloud_error_msg_decode stub returns -ENOENT
	 * → the function returns -EPROTO.
	 */
	char host[32] = {0};
	char path[32] = {0};
	struct nrf_cloud_pgps_result result = {
		.host = host,
		.host_sz = sizeof(host),
		.path = path,
		.path_sz = sizeof(path),
	};
	const uint8_t dummy[] = {0x01};

	zassert_equal(coap_codec_pgps_resp_decode(&result, dummy, sizeof(dummy),
						  COAP_CONTENT_FORMAT_APP_JSON), -EPROTO);
}

ZTEST(coap_cbor_pgps, test_decode_malformed_cbor)
{
	char host[32] = {0};
	char path[32] = {0};
	struct nrf_cloud_pgps_result result = {
		.host = host,
		.host_sz = sizeof(host),
		.path = path,
		.path_sz = sizeof(path),
	};
	const uint8_t garbage[] = {0xFF, 0xAA, 0xBB};

	zassert_equal(coap_codec_pgps_resp_decode(&result, garbage, sizeof(garbage),
						  COAP_CONTENT_FORMAT_APP_CBOR), -EINVAL);
}

/* =========================================================================
 * SUITE: coap_cbor_fota
 * Tests for coap_codec_fota_resp_decode().
 *
 * Note: coap_codec_fota_resp_decode() is a JSON-only function — it explicitly
 * rejects CBOR with -ENOTSUP and delegates all real work to
 * nrf_cloud_rest_fota_execution_decode() (JSON).  It lives in this suite
 * because it is part of nrf_cloud_coap_codec.c, which is the module under
 * test here.  The CLOUDMCU-108 JSON branch covers nrf_cloud_codec.h (a
 * different API), so there is no better home for this function's tests.
 * Only the CBOR-rejection path is exercised here; full JSON decode coverage
 * belongs to an integration test that can exercise the REST stack.
 * =========================================================================
 */

ZTEST_SUITE(coap_cbor_fota, NULL, NULL, NULL, NULL, NULL);

ZTEST(coap_cbor_fota, test_cbor_fmt_returns_enotsup)
{
	/* FOTA responses are JSON-only; passing CBOR format must be rejected. */
	struct nrf_cloud_fota_job_info job = {0};
	const uint8_t dummy[] = {0x01};

	zassert_equal(coap_codec_fota_resp_decode(&job, dummy, sizeof(dummy),
						  COAP_CONTENT_FORMAT_APP_CBOR), -ENOTSUP);
}
