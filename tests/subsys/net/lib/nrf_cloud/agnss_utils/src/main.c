/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <zephyr/ztest.h>

#include "nrf_cloud_agnss_internal.h"
#include "nrf_cloud_agnss_schema_v1.h"
#include "nrf_cloud_pgps_schema_v1.h"
#include "testdata.h"

static size_t cb_count;

static int agnss_check_type_cb(const struct nrf_cloud_agnss_element *e)
{
	if (cb_count >= ARRAY_SIZE(agnss_types)) {
		return -1;
	}
	zassert_equal(agnss_types[cb_count], e->type);
	cb_count++;
	return 0;
}

static int pgps_check_type_cb(const struct nrf_cloud_agnss_element *e)
{
	if (cb_count >= ARRAY_SIZE(pgpgs_prediction_types)) {
		return -1;
	}
	zassert_equal(pgpgs_prediction_types[cb_count], e->type);
	cb_count++;
	return 0;
}

static int noop_cb(const struct nrf_cloud_agnss_element *e)
{
	ARG_UNUSED(e);
	return 0;
}

static int cb_return_error(const struct nrf_cloud_agnss_element *e)
{
	ARG_UNUSED(e);
	cb_count++;
	return -1;
}

ZTEST_SUITE(nrf_cloud_agnss_utils, NULL, NULL, NULL, NULL, NULL);

ZTEST(nrf_cloud_agnss_utils, test_parse_agnss_block_too_short_header)
{
	const char buf[2] = {0};
	int err = parse_agnss_block(buf, sizeof(buf), noop_cb);

	zassert_equal(err, -ENOENT, "Expected -ENOENT for short header, got %d", err);
}

ZTEST(nrf_cloud_agnss_utils, test_parse_agnss_block_invalid_type_bad_weather)
{
	const char buf[] = {NRF_CLOUD_AGNSS__LAST + 1, 0x01, 0x00};
	int err = parse_agnss_block(buf, sizeof(buf), noop_cb);

	zassert_equal(err, -ERANGE, "Expected -ERANGE for invalid type, got %d", err);
}

ZTEST(nrf_cloud_agnss_utils, test_parse_agnss_block_truncated_element_bad_weather)
{
	const char buf[] = {NRF_CLOUD_AGNSS_GPS_SYSTEM_CLOCK, 0x01, 0x00, 0x00};
	int err = parse_agnss_block(buf, sizeof(buf), noop_cb);

	zassert_equal(err, -ENOENT, "Expected -ENOENT for truncated element payload, got %d", err);
}

ZTEST(nrf_cloud_agnss_utils, test_parse_agnss_block_cb_returned_error_bad_weather)
{
	const char buf[NRF_CLOUD_AGNSS_BIN_TYPE_SIZE + NRF_CLOUD_AGNSS_BIN_COUNT_SIZE +
		       sizeof(struct nrf_cloud_agnss_utc)] = {NRF_CLOUD_AGNSS_GPS_UTC_PARAMETERS,
							      0x01, 0x00};

	cb_count = 0;

	int err = parse_agnss_block(buf, sizeof(buf), cb_return_error);

	zassert_equal(err, -EIO, "Expected -EIO when callback fails, got %d", err);
	zassert_equal(cb_count, 1, "Expected callback to be invoked once, got %d", cb_count);
}

ZTEST(nrf_cloud_agnss_utils, test_parse_agnss_block_agnss_blob_good_weather)
{
	cb_count = 0;

	int err = parse_agnss_block(
		(const char *)&agnss_bin[NRF_CLOUD_AGNSS_BIN_SCHEMA_VERSION_SIZE],
		agnss_bin_len - NRF_CLOUD_AGNSS_BIN_SCHEMA_VERSION_SIZE, agnss_check_type_cb);

	zassert_ok(err, "Expected AGNSS test blob to parse, got %d", err);
	zassert_equal(cb_count, ARRAY_SIZE(agnss_types), "Expected all elements to be parsed");
}

ZTEST(nrf_cloud_agnss_utils, test_parse_agnss_block_pgps_prediction_good_weather)
{
	struct nrf_cloud_pgps_header *header = (struct nrf_cloud_pgps_header *)pgnss_bin;

	size_t pos = sizeof(struct nrf_cloud_pgps_header);

	for (size_t i = 0; i < header->prediction_count; ++i) {
		cb_count = 0;
		/* P-GPS file starts with nrf_cloud_pgps_header; each prediction payload follows. */
		int err = parse_agnss_block((const char *)&pgnss_bin[pos], header->prediction_size,
					    pgps_check_type_cb);
		pos += header->prediction_size;
		zassert_ok(err, "Expected first P-GPS prediction chunk to parse, got %d", err);
		zassert_equal(cb_count, ARRAY_SIZE(pgpgs_prediction_types),
			      "Expected all elements to be parsed");
	}
	zassert_equal(pos, pgnss_bin_len, "Expected whole payload to be parsed");
}

ZTEST(nrf_cloud_agnss_utils, test_parse_agnss_block_trailing_byte)
{
	/* one klobuchar element and a trailing stray byte */
	const char buf[] = {
		NRF_CLOUD_AGNSS_KLOBUCHAR_CORRECTION,
		0x01, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x01
	};
	int err = parse_agnss_block(buf, sizeof(buf), noop_cb);

	zassert_equal(err, -ENOENT, "Expected -ENOENT for truncated element payload, got %d", err);
}
