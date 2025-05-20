/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <sdfw/sdfw_services/ssf_client.h>
#include <sdfw/sdfw_services/ssf_client_notif.h>
#include <sdfw/sdfw_services/ssf_errno.h>

#include <nrf_rpc_errno.h>
#include <zcbor_common.h>
#include <zephyr/toolchain.h>

#include <unity.h>
#include "cmock_ssf_client_transport.h"

#define CBOR_INF_ARRAY_START 0x9F
#define CBOR_INF_ARRAY_END 0xFF

void ssf_client_notif_handler(const uint8_t *pkt, size_t pkt_len);

const uint8_t CLIENT_DOMAIN = 2;
static uint8_t *req_hdr_expected;
static size_t req_hdr_len_expected;
static int32_t srvc1_req_expected;
static size_t srvc1_req_len_expected;
static uint8_t *srvc2_req_expected;
static size_t srvc2_req_len_expected;

static int srvc1_encode_req(uint8_t *payload, size_t payload_len, int32_t *input,
			    size_t *payload_len_out)
{
	if (payload_len < sizeof(int32_t)) {
		return ZCBOR_ERR_UNKNOWN;
	}
	memcpy(payload, (void *)input, sizeof(int32_t));

	if (payload_len_out != NULL) {
		*payload_len_out = sizeof(int32_t);
	}

	return ZCBOR_SUCCESS;
}

static int srvc1_decode_rsp(const uint8_t *payload, size_t payload_len, int32_t *result,
			    size_t *payload_len_out)
{
	if (payload_len < sizeof(int32_t)) {
		return ZCBOR_ERR_UNKNOWN;
	}
	memcpy((void *)result, payload, sizeof(int32_t));

	if (payload_len_out != NULL) {
		*payload_len_out = sizeof(int32_t);
	}

	return ZCBOR_SUCCESS;
}

#define CONFIG_SSF_SRVC1_SERVICE_ID 0x10
#define CONFIG_SSF_SRVC1_SERVICE_VERSION 1
#define CONFIG_SSF_SRVC1_SERVICE_BUFFER_SIZE 32
SSF_CLIENT_SERVICE_DEFINE(srvc1, SRVC1, srvc1_encode_req, srvc1_decode_rsp);

#define CONFIG_SSF_SRVC2_SERVICE_ID 0x20
#define CONFIG_SSF_SRVC2_SERVICE_VERSION 5
#define CONFIG_SSF_SRVC2_SERVICE_BUFFER_SIZE 32
SSF_CLIENT_SERVICE_DEFINE(srvc2, SRVC2, NULL, NULL);

#define CONFIG_SSF_SRVC3_SERVICE_ID 0x3
#define CONFIG_SSF_SRVC3_SERVICE_VERSION 3
#define CONFIG_SSF_SRVC3_SERVICE_BUFFER_SIZE 3
SSF_CLIENT_SERVICE_DEFINE(srvc3, SRVC3, srvc1_encode_req, srvc1_decode_rsp);

#define CONFIG_SSF_SRVC4_SERVICE_ID 0x5
#define CONFIG_SSF_SRVC4_SERVICE_VERSION 5
#define CONFIG_SSF_SRVC4_SERVICE_BUFFER_SIZE 7
SSF_CLIENT_SERVICE_DEFINE(srvc4, SRVC4, srvc1_encode_req, srvc1_decode_rsp);

static int stub_srvc1_transport_request_send(uint8_t *pkt, size_t pkt_len, const uint8_t **rsp_pkt,
					     size_t *rsp_pkt_len, int cmock_num_calls)
{
	ARG_UNUSED(rsp_pkt);
	ARG_UNUSED(rsp_pkt_len);
	ARG_UNUSED(cmock_num_calls);

	TEST_ASSERT_EQUAL(req_hdr_len_expected + srvc1_req_len_expected, pkt_len);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(req_hdr_expected, pkt, req_hdr_len_expected);

	int32_t *req_ptr = (int32_t *)&pkt[req_hdr_len_expected];

	TEST_ASSERT_EQUAL(srvc1_req_expected, *req_ptr);

	return 0;
}

static int stub_srvc2_transport_request_send(uint8_t *pkt, size_t pkt_len, const uint8_t **rsp_pkt,
					     size_t *rsp_pkt_len, int cmock_num_calls)
{
	ARG_UNUSED(rsp_pkt);
	ARG_UNUSED(rsp_pkt_len);
	ARG_UNUSED(cmock_num_calls);

	TEST_ASSERT_EQUAL(req_hdr_len_expected + srvc2_req_len_expected, pkt_len);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(req_hdr_expected, pkt, req_hdr_len_expected);

	uint8_t *req_ptr = &pkt[req_hdr_len_expected];
	size_t req_len = pkt_len - req_hdr_len_expected;

	TEST_ASSERT_EQUAL(srvc2_req_len_expected, req_len);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(srvc2_req_expected, req_ptr, req_len);

	return 0;
}

static void init_client(void)
{
	__cmock_ssf_client_transport_init_ExpectAndReturn(ssf_client_notif_handler, 0);

	int err = ssf_client_init();

	TEST_ASSERT_EQUAL(0, err);
}

void test_client_init_success(void)
{
	init_client();
}

void test_client_init_transport_init_error(void)
{
	__cmock_ssf_client_transport_init_ExpectAndReturn(ssf_client_notif_handler, -NRF_EINVAL);

	int err = ssf_client_init();

	TEST_ASSERT_EQUAL(-SSF_EINVAL, err);
}

void test_client_send_request_success(void)
{
	int32_t req = 0xCACE;
	int32_t rsp;
	uint8_t hdr_expected[] = { CBOR_INF_ARRAY_START, CLIENT_DOMAIN, CONFIG_SSF_SRVC1_SERVICE_ID,
				   CONFIG_SSF_SRVC1_SERVICE_VERSION, CBOR_INF_ARRAY_END };

	uint8_t shmem_tx[CONFIG_SSF_SRVC1_SERVICE_BUFFER_SIZE];
	uint8_t *shmem_tx_ptr = &shmem_tx[0];

	__cmock_ssf_client_transport_alloc_tx_buf_ExpectAndReturn(
		NULL, CONFIG_SSF_SRVC1_SERVICE_BUFFER_SIZE, 0);
	__cmock_ssf_client_transport_alloc_tx_buf_IgnoreArg_buf();
	__cmock_ssf_client_transport_alloc_tx_buf_ReturnThruPtr_buf(&shmem_tx_ptr);

	req_hdr_expected = hdr_expected;
	req_hdr_len_expected = sizeof(hdr_expected);
	srvc1_req_expected = req;
	srvc1_req_len_expected = sizeof(req);
	__cmock_ssf_client_transport_request_send_AddCallback(stub_srvc1_transport_request_send);

	const uint8_t shmem_rx[] = {
		CBOR_INF_ARRAY_START, 0x00, CBOR_INF_ARRAY_END, 0xEF, 0xBE, 0x00, 0x00
	};
	const uint8_t *shmem_rx_ptr = &shmem_rx[0];
	size_t shmem_rx_len = sizeof(shmem_rx);

	__cmock_ssf_client_transport_request_send_ExpectAndReturn(
		shmem_tx_ptr, sizeof(hdr_expected) + sizeof(req), NULL, NULL, 0);
	__cmock_ssf_client_transport_request_send_IgnoreArg_rsp_pkt();
	__cmock_ssf_client_transport_request_send_IgnoreArg_rsp_pkt_len();
	__cmock_ssf_client_transport_request_send_ReturnThruPtr_rsp_pkt(&shmem_rx_ptr);
	__cmock_ssf_client_transport_request_send_ReturnThruPtr_rsp_pkt_len(&shmem_rx_len);

	__cmock_ssf_client_transport_decoding_done_Expect(shmem_rx_ptr);

	int err = ssf_client_send_request(&srvc1, &req, &rsp, NULL);

	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(0xBEEF, rsp);
}

void test_client_send_request_success_free_rsp_later(void)
{
	int32_t req = 0xCACE;
	int32_t rsp;
	const uint8_t *rsp_pkt;
	uint8_t hdr_expected[] = { CBOR_INF_ARRAY_START, CLIENT_DOMAIN, CONFIG_SSF_SRVC1_SERVICE_ID,
				   CONFIG_SSF_SRVC1_SERVICE_VERSION, CBOR_INF_ARRAY_END };

	uint8_t shmem_tx[CONFIG_SSF_SRVC1_SERVICE_BUFFER_SIZE];
	uint8_t *shmem_tx_ptr = &shmem_tx[0];

	__cmock_ssf_client_transport_alloc_tx_buf_ExpectAndReturn(
		NULL, CONFIG_SSF_SRVC1_SERVICE_BUFFER_SIZE, 0);
	__cmock_ssf_client_transport_alloc_tx_buf_IgnoreArg_buf();
	__cmock_ssf_client_transport_alloc_tx_buf_ReturnThruPtr_buf(&shmem_tx_ptr);

	req_hdr_expected = hdr_expected;
	req_hdr_len_expected = sizeof(hdr_expected);
	srvc1_req_expected = req;
	srvc1_req_len_expected = sizeof(req);
	__cmock_ssf_client_transport_request_send_AddCallback(stub_srvc1_transport_request_send);

	const uint8_t shmem_rx[] = {
		CBOR_INF_ARRAY_START, 0x00, CBOR_INF_ARRAY_END, 0xEF, 0xBE, 0x00, 0x00
	};
	const uint8_t *shmem_rx_ptr = &shmem_rx[0];
	size_t shmem_rx_len = sizeof(shmem_rx);

	__cmock_ssf_client_transport_request_send_ExpectAndReturn(
		shmem_tx_ptr, sizeof(hdr_expected) + sizeof(req), NULL, NULL, 0);
	__cmock_ssf_client_transport_request_send_IgnoreArg_rsp_pkt();
	__cmock_ssf_client_transport_request_send_IgnoreArg_rsp_pkt_len();
	__cmock_ssf_client_transport_request_send_ReturnThruPtr_rsp_pkt(&shmem_rx_ptr);
	__cmock_ssf_client_transport_request_send_ReturnThruPtr_rsp_pkt_len(&shmem_rx_len);

	int err = ssf_client_send_request(&srvc1, &req, &rsp, &rsp_pkt);

	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(0xBEEF, rsp);
	TEST_ASSERT_EQUAL_PTR(shmem_rx_ptr, rsp_pkt);
}

void test_client_send_request_srvc_null(void)
{
	int32_t req = 0xCACE;
	int32_t rsp;

	int err = ssf_client_send_request(NULL, &req, &rsp, NULL);

	TEST_ASSERT_EQUAL(-SSF_EINVAL, err);
}

void test_client_send_request_req_null(void)
{
	int32_t rsp;

	int err = ssf_client_send_request(&srvc1, NULL, &rsp, NULL);

	TEST_ASSERT_EQUAL(-SSF_EINVAL, err);
}

void test_client_send_request_rsp_null(void)
{
	int32_t req = 0xCACE;

	int err = ssf_client_send_request(&srvc1, &req, NULL, NULL);

	TEST_ASSERT_EQUAL(-SSF_EINVAL, err);
}

void test_client_send_request_encode_and_decode_func_null(void)
{
	int32_t req = 0xCACE;
	int32_t rsp;

	int err = ssf_client_send_request(&srvc2, &req, &rsp, NULL);

	TEST_ASSERT_EQUAL(-SSF_EPROTO, err);
}

void test_client_send_request_alloc_tx_error(void)
{
	int32_t req = 0xCACE;
	int32_t rsp;

	uint8_t shmem_tx[CONFIG_SSF_SRVC1_SERVICE_BUFFER_SIZE];
	uint8_t *shmem_tx_ptr = &shmem_tx[0];

	__cmock_ssf_client_transport_alloc_tx_buf_ExpectAndReturn(
		NULL, CONFIG_SSF_SRVC1_SERVICE_BUFFER_SIZE, -SSF_ENOMEM);
	__cmock_ssf_client_transport_alloc_tx_buf_IgnoreArg_buf();
	__cmock_ssf_client_transport_alloc_tx_buf_ReturnThruPtr_buf(&shmem_tx_ptr);

	int err = ssf_client_send_request(&srvc1, &req, &rsp, NULL);

	TEST_ASSERT_EQUAL(-SSF_ENOMEM, err);
}

void test_client_send_request_encode_hdr_error(void)
{
	int32_t req = 0xCACE;
	int32_t rsp;

	/* Have encoding of request header fail by allocating less memory than needed by header. */
	uint8_t shmem_tx[CONFIG_SSF_SRVC3_SERVICE_BUFFER_SIZE];
	uint8_t *shmem_tx_ptr = &shmem_tx[0];

	__cmock_ssf_client_transport_alloc_tx_buf_ExpectAndReturn(
		NULL, CONFIG_SSF_SRVC3_SERVICE_BUFFER_SIZE, 0);
	__cmock_ssf_client_transport_alloc_tx_buf_IgnoreArg_buf();
	__cmock_ssf_client_transport_alloc_tx_buf_ReturnThruPtr_buf(&shmem_tx_ptr);

	__cmock_ssf_client_transport_free_tx_buf_Expect(shmem_tx_ptr);

	int err = ssf_client_send_request(&srvc3, &req, &rsp, NULL);

	TEST_ASSERT_EQUAL(-SSF_EPROTO, err);
}

void test_client_send_request_encode_req_error(void)
{
	int32_t req = 0xCACE;
	int32_t rsp;
	uint8_t hdr_expected[] = { CBOR_INF_ARRAY_START, CLIENT_DOMAIN, CONFIG_SSF_SRVC4_SERVICE_ID,
				   CONFIG_SSF_SRVC4_SERVICE_VERSION, CBOR_INF_ARRAY_END };

	/*
	 * Have encoding of request payload fail by allocating enough memory for
	 * header, but not for the payload.
	 */
	uint8_t shmem_tx[CONFIG_SSF_SRVC4_SERVICE_BUFFER_SIZE];
	uint8_t *shmem_tx_ptr = &shmem_tx[0];

	__cmock_ssf_client_transport_alloc_tx_buf_ExpectAndReturn(
		NULL, CONFIG_SSF_SRVC4_SERVICE_BUFFER_SIZE, 0);
	__cmock_ssf_client_transport_alloc_tx_buf_IgnoreArg_buf();
	__cmock_ssf_client_transport_alloc_tx_buf_ReturnThruPtr_buf(&shmem_tx_ptr);

	__cmock_ssf_client_transport_free_tx_buf_Expect(shmem_tx_ptr);

	int err = ssf_client_send_request(&srvc4, &req, &rsp, NULL);

	TEST_ASSERT_EQUAL_UINT8_ARRAY(hdr_expected, shmem_tx, sizeof(hdr_expected));
	TEST_ASSERT_EQUAL(-SSF_EPROTO, err);
}

void test_client_send_request_send_pkt_too_large(void)
{
	int32_t req = 0xCACE;
	int32_t rsp;

	uint8_t shmem_tx[CONFIG_SSF_SRVC1_SERVICE_BUFFER_SIZE];
	uint8_t *shmem_tx_ptr = &shmem_tx[0];

	__cmock_ssf_client_transport_alloc_tx_buf_ExpectAndReturn(
		NULL, CONFIG_SSF_SRVC1_SERVICE_BUFFER_SIZE, 0);
	__cmock_ssf_client_transport_alloc_tx_buf_IgnoreArg_buf();
	__cmock_ssf_client_transport_alloc_tx_buf_ReturnThruPtr_buf(&shmem_tx_ptr);

	/* Have underlying transport return -SSF_EMSGSIZE, indicating a too big package. */
	__cmock_ssf_client_transport_request_send_ExpectAnyArgsAndReturn(-SSF_EMSGSIZE);

	int err = ssf_client_send_request(&srvc1, &req, &rsp, NULL);

	TEST_ASSERT_EQUAL(-SSF_EMSGSIZE, err);
}

void test_client_send_request_send_error(void)
{
	int32_t req = 0xCACE;
	int32_t rsp;
	const uint8_t hdr_expected[] = { CBOR_INF_ARRAY_START, CLIENT_DOMAIN,
					 CONFIG_SSF_SRVC1_SERVICE_ID,
					 CONFIG_SSF_SRVC1_SERVICE_VERSION, CBOR_INF_ARRAY_END };
	const uint8_t pl_expected[] = { 0xCE, 0xCA, 0x00, 0x00 };

	uint8_t shmem_tx[CONFIG_SSF_SRVC1_SERVICE_BUFFER_SIZE];
	uint8_t *shmem_tx_ptr = &shmem_tx[0];

	__cmock_ssf_client_transport_alloc_tx_buf_ExpectAndReturn(
		NULL, CONFIG_SSF_SRVC1_SERVICE_BUFFER_SIZE, 0);
	__cmock_ssf_client_transport_alloc_tx_buf_IgnoreArg_buf();
	__cmock_ssf_client_transport_alloc_tx_buf_ReturnThruPtr_buf(&shmem_tx_ptr);

	__cmock_ssf_client_transport_request_send_ExpectAnyArgsAndReturn(-NRF_EIO);

	int err = ssf_client_send_request(&srvc1, &req, &rsp, NULL);

	TEST_ASSERT_EQUAL(-SSF_EIO, err);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(hdr_expected, &shmem_tx[0], sizeof(hdr_expected));
	TEST_ASSERT_EQUAL_UINT8_ARRAY(pl_expected, &shmem_tx[sizeof(hdr_expected)],
				      sizeof(pl_expected));
}

void test_client_send_request_decode_rsp_hdr_error(void)
{
	int32_t req = 0xCACE;
	int32_t rsp;
	uint8_t hdr_expected[] = { CBOR_INF_ARRAY_START, CLIENT_DOMAIN, CONFIG_SSF_SRVC1_SERVICE_ID,
				   CONFIG_SSF_SRVC1_SERVICE_VERSION, CBOR_INF_ARRAY_END };

	uint8_t shmem_tx[CONFIG_SSF_SRVC1_SERVICE_BUFFER_SIZE];
	uint8_t *shmem_tx_ptr = &shmem_tx[0];

	__cmock_ssf_client_transport_alloc_tx_buf_ExpectAndReturn(
		NULL, CONFIG_SSF_SRVC1_SERVICE_BUFFER_SIZE, 0);
	__cmock_ssf_client_transport_alloc_tx_buf_IgnoreArg_buf();
	__cmock_ssf_client_transport_alloc_tx_buf_ReturnThruPtr_buf(&shmem_tx_ptr);

	req_hdr_expected = hdr_expected;
	req_hdr_len_expected = sizeof(hdr_expected);
	srvc1_req_expected = req;
	srvc1_req_len_expected = sizeof(req);
	__cmock_ssf_client_transport_request_send_AddCallback(stub_srvc1_transport_request_send);

	/* Have header decoding fail by passing CBOR_INF_ARRAY_END instead
	 * of CBOR_INF_ARRAY_START.
	 */
	const uint8_t shmem_rx[] = {
		CBOR_INF_ARRAY_END, 0x0, CBOR_INF_ARRAY_END, 0xEF, 0xBE, 0x00, 0x00
	};
	const uint8_t *shmem_rx_ptr = &shmem_rx[0];
	size_t shmem_rx_len = sizeof(shmem_rx);

	__cmock_ssf_client_transport_request_send_ExpectAndReturn(
		shmem_tx_ptr, sizeof(hdr_expected) + sizeof(req), NULL, NULL, 0);
	__cmock_ssf_client_transport_request_send_IgnoreArg_rsp_pkt();
	__cmock_ssf_client_transport_request_send_IgnoreArg_rsp_pkt_len();
	__cmock_ssf_client_transport_request_send_ReturnThruPtr_rsp_pkt(&shmem_rx_ptr);
	__cmock_ssf_client_transport_request_send_ReturnThruPtr_rsp_pkt_len(&shmem_rx_len);

	__cmock_ssf_client_transport_decoding_done_Expect(shmem_rx_ptr);

	int err = ssf_client_send_request(&srvc1, &req, &rsp, NULL);

	TEST_ASSERT_EQUAL(-SSF_EPROTO, err);
}

void test_client_send_request_rsp_is_error(void)
{
	int32_t req = 0xCACE;
	int32_t rsp;
	uint8_t hdr_expected[] = { CBOR_INF_ARRAY_START, CLIENT_DOMAIN, CONFIG_SSF_SRVC1_SERVICE_ID,
				   CONFIG_SSF_SRVC1_SERVICE_VERSION, CBOR_INF_ARRAY_END };

	uint8_t shmem_tx[CONFIG_SSF_SRVC1_SERVICE_BUFFER_SIZE];
	uint8_t *shmem_tx_ptr = &shmem_tx[0];

	__cmock_ssf_client_transport_alloc_tx_buf_ExpectAndReturn(
		NULL, CONFIG_SSF_SRVC1_SERVICE_BUFFER_SIZE, 0);
	__cmock_ssf_client_transport_alloc_tx_buf_IgnoreArg_buf();
	__cmock_ssf_client_transport_alloc_tx_buf_ReturnThruPtr_buf(&shmem_tx_ptr);

	req_hdr_expected = hdr_expected;
	req_hdr_len_expected = sizeof(hdr_expected);
	srvc1_req_expected = req;
	srvc1_req_len_expected = sizeof(req);
	__cmock_ssf_client_transport_request_send_AddCallback(stub_srvc1_transport_request_send);

	const uint8_t ret_err = 0x24; /* Return -5 (0x24 with CBOR) as error. */
	const uint8_t shmem_rx[] = {
		CBOR_INF_ARRAY_START, ret_err, CBOR_INF_ARRAY_END, 0xEF, 0xBE, 0x00, 0x00
	};
	const uint8_t *shmem_rx_ptr = &shmem_rx[0];
	size_t shmem_rx_len = sizeof(shmem_rx);

	__cmock_ssf_client_transport_request_send_ExpectAndReturn(
		shmem_tx_ptr, sizeof(hdr_expected) + sizeof(req), NULL, NULL, 0);
	__cmock_ssf_client_transport_request_send_IgnoreArg_rsp_pkt();
	__cmock_ssf_client_transport_request_send_IgnoreArg_rsp_pkt_len();
	__cmock_ssf_client_transport_request_send_ReturnThruPtr_rsp_pkt(&shmem_rx_ptr);
	__cmock_ssf_client_transport_request_send_ReturnThruPtr_rsp_pkt_len(&shmem_rx_len);

	__cmock_ssf_client_transport_decoding_done_Expect(shmem_rx_ptr);

	int err = ssf_client_send_request(&srvc1, &req, &rsp, NULL);

	TEST_ASSERT_EQUAL(-5, err);
}

void test_client_send_request_decode_rsp_error(void)
{
	int32_t req = 0xCACE;
	int32_t rsp;
	uint8_t hdr_expected[] = { CBOR_INF_ARRAY_START, CLIENT_DOMAIN, CONFIG_SSF_SRVC1_SERVICE_ID,
				   CONFIG_SSF_SRVC1_SERVICE_VERSION, CBOR_INF_ARRAY_END };

	uint8_t shmem_tx[CONFIG_SSF_SRVC1_SERVICE_BUFFER_SIZE];
	uint8_t *shmem_tx_ptr = &shmem_tx[0];

	__cmock_ssf_client_transport_alloc_tx_buf_ExpectAndReturn(
		NULL, CONFIG_SSF_SRVC1_SERVICE_BUFFER_SIZE, 0);
	__cmock_ssf_client_transport_alloc_tx_buf_IgnoreArg_buf();
	__cmock_ssf_client_transport_alloc_tx_buf_ReturnThruPtr_buf(&shmem_tx_ptr);

	req_hdr_expected = hdr_expected;
	req_hdr_len_expected = sizeof(hdr_expected);
	srvc1_req_expected = req;
	srvc1_req_len_expected = sizeof(req);
	__cmock_ssf_client_transport_request_send_AddCallback(stub_srvc1_transport_request_send);

	/* Have payload decoding fail by giving fewer bytes than is necessary for an int32_t. */
	const uint8_t shmem_rx[] = {
		CBOR_INF_ARRAY_START, 0x0, CBOR_INF_ARRAY_END, 0xEF, 0xBE, 0x00
	};
	const uint8_t *shmem_rx_ptr = &shmem_rx[0];
	size_t shmem_rx_len = sizeof(shmem_rx);

	__cmock_ssf_client_transport_request_send_ExpectAndReturn(
		shmem_tx_ptr, sizeof(hdr_expected) + sizeof(req), NULL, NULL, 0);
	__cmock_ssf_client_transport_request_send_IgnoreArg_rsp_pkt();
	__cmock_ssf_client_transport_request_send_IgnoreArg_rsp_pkt_len();
	__cmock_ssf_client_transport_request_send_ReturnThruPtr_rsp_pkt(&shmem_rx_ptr);
	__cmock_ssf_client_transport_request_send_ReturnThruPtr_rsp_pkt_len(&shmem_rx_len);

	__cmock_ssf_client_transport_decoding_done_Expect(shmem_rx_ptr);

	int err = ssf_client_send_request(&srvc1, &req, &rsp, NULL);

	TEST_ASSERT_EQUAL(-SSF_EPROTO, err);
}

void test_client_send_raw_request_success(void)
{
	uint8_t req_data[] = { 0xCE, 0xCA, 0x00, 0x00 };
	const uint8_t rsp_data_expected[] = { 0xEF, 0xBE, 0x00, 0x00 };
	uint8_t rsp_data[sizeof(rsp_data_expected)];
	struct ssf_client_raw_data req = { .data = req_data, .len = sizeof(req_data) };
	struct ssf_client_raw_data rsp = { .data = rsp_data, .len = sizeof(rsp_data) };

	/* 0x18 0x20 is cbor for 0x20 */
	uint8_t hdr_expected[] = { CBOR_INF_ARRAY_START, CLIENT_DOMAIN, 0x18, 0x20, 0x5,
				   CBOR_INF_ARRAY_END };

	uint8_t shmem_tx[CONFIG_SSF_SRVC2_SERVICE_BUFFER_SIZE];
	uint8_t *shmem_tx_ptr = &shmem_tx[0];

	__cmock_ssf_client_transport_alloc_tx_buf_ExpectAndReturn(
		NULL, CONFIG_SSF_SRVC2_SERVICE_BUFFER_SIZE, 0);
	__cmock_ssf_client_transport_alloc_tx_buf_IgnoreArg_buf();
	__cmock_ssf_client_transport_alloc_tx_buf_ReturnThruPtr_buf(&shmem_tx_ptr);

	req_hdr_expected = hdr_expected;
	req_hdr_len_expected = sizeof(hdr_expected);
	srvc2_req_expected = req_data;
	srvc2_req_len_expected = sizeof(req_data);
	__cmock_ssf_client_transport_request_send_AddCallback(stub_srvc2_transport_request_send);

	const uint8_t shmem_rx[] = {
		CBOR_INF_ARRAY_START, 0x00, CBOR_INF_ARRAY_END, 0xEF, 0xBE, 0x00, 0x00
	};
	const uint8_t *shmem_rx_ptr = &shmem_rx[0];
	size_t shmem_rx_len = sizeof(shmem_rx);

	__cmock_ssf_client_transport_request_send_ExpectAndReturn(
		shmem_tx_ptr, sizeof(hdr_expected) + sizeof(req_data), NULL, NULL, 0);
	__cmock_ssf_client_transport_request_send_IgnoreArg_rsp_pkt();
	__cmock_ssf_client_transport_request_send_IgnoreArg_rsp_pkt_len();
	__cmock_ssf_client_transport_request_send_ReturnThruPtr_rsp_pkt(&shmem_rx_ptr);
	__cmock_ssf_client_transport_request_send_ReturnThruPtr_rsp_pkt_len(&shmem_rx_len);

	__cmock_ssf_client_transport_decoding_done_Expect(shmem_rx_ptr);

	int err = ssf_client_send_raw_request(&srvc2, req, &rsp);

	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(rsp_data_expected, rsp.data, rsp.len);
}

void test_client_send_raw_request_srvc_null(void)
{
	uint8_t req_data[] = { 0xCE, 0xCA, 0x00, 0x00 };
	uint8_t rsp_data[sizeof(req_data)];
	struct ssf_client_raw_data req = { .data = req_data, .len = sizeof(req_data) };
	struct ssf_client_raw_data rsp = { .data = rsp_data, .len = sizeof(rsp_data) };

	int err = ssf_client_send_raw_request(NULL, req, &rsp);

	TEST_ASSERT_EQUAL(-SSF_EINVAL, err);
}

void test_client_send_raw_request_rsp_null(void)
{
	uint8_t req_data[] = { 0xCE, 0xCA, 0x00, 0x00 };
	struct ssf_client_raw_data req = { .data = req_data, .len = sizeof(req_data) };

	int err = ssf_client_send_raw_request(&srvc2, req, NULL);

	TEST_ASSERT_EQUAL(-SSF_EINVAL, err);
}

void test_client_decode_done_success(void)
{
	const uint8_t rsp_pkt[] = { 0xFF };

	__cmock_ssf_client_transport_decoding_done_Expect(rsp_pkt);

	ssf_client_decode_done(rsp_pkt);
}

extern int unity_main(void);

int main(void)
{
	(void)unity_main();

	return 0;
}
