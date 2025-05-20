/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_nrf_rpc_transport.h>

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#include <nfc_t2t_lib.h>

#include <nrf_rpc/nrf_rpc_cbkproxy.h>

#include <nfc_rpc_ids.h>
#include <nfc_rpc_common.h>
#include <test_rpc_env.h>

static int slot_cnt;
static bool cb_called;

static void nrf_rpc_err_handler(const struct nrf_rpc_err_report *report)
{
	zassert_ok(report->code);
}

static void tc_setup(void *f)
{
	mock_nrf_rpc_tr_expect_add(RPC_INIT_REQ, RPC_INIT_RSP);
	zassert_ok(nrf_rpc_init(nrf_rpc_err_handler));
	mock_nrf_rpc_tr_expect_reset();
}

/* Test serialization of nfc_t2t_setup() */
ZTEST(nfc_rpc_t2t_cli, test_nfc_t2t_setup)
{
	int error;
	nfc_t2t_callback_t cb = (nfc_t2t_callback_t)0xdeadbeef;
	void *cb_ctx = (void *)0xcafeface;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(NFC_RPC_CMD_T2T_SETUP, slot_cnt, CBOR_UINT32(0xcafeface)), RPC_RSP(0));
	error = nfc_t2t_setup(cb, cb_ctx);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, 0);
}

/* Test serialization of nfc_t2t_parameter_set() */
ZTEST(nfc_rpc_t2t_cli, test_nfc_t2t_parameter_set)
{
	int error;
	uint8_t data[DATA_SIZE] = {INT_SEQUENCE(DATA_SIZE)};

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(NFC_RPC_CMD_T2T_PARAMETER_SET, NFC_T2T_PARAM_NFCID1, NFC_DATA), RPC_RSP(0));
	error = nfc_t2t_parameter_set(NFC_T2T_PARAM_NFCID1, data, DATA_SIZE);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, 0);
}

/* Test serialization of nfc_t2t_parameter_set() with NULL data */
ZTEST(nfc_rpc_t2t_cli, test_nfc_t2t_parameter_set_null)
{
	int error;

	error = nfc_t2t_parameter_set(NFC_T2T_PARAM_NFCID1, NULL, 10);
	zassert_equal(error, -NRF_EINVAL);
}

/* Test serialization of nfc_t2t_parameter_get() */
ZTEST(nfc_rpc_t2t_cli, test_nfc_t2t_parameter_get)
{
	int error;
	uint8_t expected_data[DATA_SIZE] = {INT_SEQUENCE(DATA_SIZE)};
	uint8_t data[DATA_SIZE];
	size_t max_data_length = DATA_SIZE;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(NFC_RPC_CMD_T2T_PARAMETER_GET, NFC_T2T_PARAM_FDT_MIN,
					   CBOR_UINT8(DATA_SIZE)),
				   RPC_RSP(NFC_DATA));
	error = nfc_t2t_parameter_get(NFC_T2T_PARAM_FDT_MIN, data, &max_data_length);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, 0);
	zassert_mem_equal(data, expected_data, DATA_SIZE);
}

/* Test serialization of nfc_t2t_parameter_get() with NULL data */
ZTEST(nfc_rpc_t2t_cli, test_nfc_t2t_parameter_get_null)
{
	int error;
	uint8_t data[DATA_SIZE];
	size_t max_data_length = DATA_SIZE;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(NFC_RPC_CMD_T2T_PARAMETER_GET, NFC_T2T_PARAM_FDT_MIN,
					   CBOR_UINT8(DATA_SIZE)),
				   RPC_RSP(CBOR_NULL));
	error = nfc_t2t_parameter_get(NFC_T2T_PARAM_FDT_MIN, data, &max_data_length);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, -NRF_EINVAL);
}

/* Test serialization of nfc_t2t_payload_set() */
ZTEST(nfc_rpc_t2t_cli, test_nfc_t2t_payload_set)
{
	int error;
	uint8_t data[NFC_T2T_MAX_PAYLOAD_SIZE];
	mock_nrf_rpc_pkt_t expected_pkt;
	uint8_t expected_data[NFC_T2T_MAX_PAYLOAD_SIZE + 9] = {
		0x80, NFC_RPC_CMD_T2T_PAYLOAD_SET, 0xff, 0x00, 0x00, 0x59, 0x03, 0xdc};

	for (int i = 0; i < NFC_T2T_MAX_PAYLOAD_SIZE; i++) {
		data[i] = i + 1;
		expected_data[8 + i] = i + 1;
	}
	expected_data[NFC_T2T_MAX_PAYLOAD_SIZE + 8] = 0xf6;

	expected_pkt.data = expected_data;
	expected_pkt.len = sizeof(expected_data);
	mock_nrf_rpc_tr_expect_add(expected_pkt, RPC_RSP(0));
	error = nfc_t2t_payload_set(data, NFC_T2T_MAX_PAYLOAD_SIZE);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, 0);
}

/* Test serialization of nfc_t2t_payload_set() with NULL data */
ZTEST(nfc_rpc_t2t_cli, test_nfc_t2t_payload_set_null)
{
	int error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(NFC_RPC_CMD_T2T_PAYLOAD_SET, CBOR_NULL), RPC_RSP(0));
	error = nfc_t2t_payload_set(NULL, NFC_T2T_MAX_PAYLOAD_SIZE);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, 0);
}

/* Test serialization of nfc_t2t_payload_set() with zero length */
ZTEST(nfc_rpc_t2t_cli, test_nfc_t2t_payload_set_zero)
{
	int error;
	uint8_t data[DATA_SIZE] = {INT_SEQUENCE(DATA_SIZE)};

	mock_nrf_rpc_tr_expect_add(RPC_CMD(NFC_RPC_CMD_T2T_PAYLOAD_SET, 0x40), RPC_RSP(0));
	error = nfc_t2t_payload_set(data, 0);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, 0);
}

/* Test serialization of nfc_t2t_payload_raw_set() */
ZTEST(nfc_rpc_t2t_cli, test_nfc_t2t_payload_raw_set)
{
	int error;
	uint8_t data[NFC_T2T_MAX_PAYLOAD_SIZE_RAW];
	mock_nrf_rpc_pkt_t expected_pkt;
	uint8_t expected_data[NFC_T2T_MAX_PAYLOAD_SIZE_RAW + 9] = {
		0x80, NFC_RPC_CMD_T2T_PAYLOAD_RAW_SET, 0xff, 0, 0, 0x59, 0x03, 0xf0};

	for (int i = 0; i < NFC_T2T_MAX_PAYLOAD_SIZE_RAW; i++) {
		data[i] = i + 1;
		expected_data[8 + i] = i + 1;
	}
	expected_data[NFC_T2T_MAX_PAYLOAD_SIZE_RAW + 8] = 0xf6;

	expected_pkt.data = expected_data;
	expected_pkt.len = sizeof(expected_data);
	mock_nrf_rpc_tr_expect_add(expected_pkt, RPC_RSP(0));
	error = nfc_t2t_payload_raw_set(data, NFC_T2T_MAX_PAYLOAD_SIZE_RAW);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, 0);
}

/* Test serialization of nfc_t2t_payload_raw_set() with NULL data */
ZTEST(nfc_rpc_t2t_cli, test_nfc_t2t_payload_raw_set_null)
{
	int error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(NFC_RPC_CMD_T2T_PAYLOAD_RAW_SET, CBOR_NULL), RPC_RSP(0));
	error = nfc_t2t_payload_raw_set(NULL, NFC_T2T_MAX_PAYLOAD_SIZE_RAW);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, 0);
}

/* Test serialization of nfc_t2t_internal_set() */
ZTEST(nfc_rpc_t2t_cli, test_nfc_t2t_internal_set)
{
	int error;
	uint8_t data[DATA_SIZE] = {INT_SEQUENCE(DATA_SIZE)};

	mock_nrf_rpc_tr_expect_add(RPC_CMD(NFC_RPC_CMD_T2T_INTERNAL_SET, NFC_DATA), RPC_RSP(0));
	error = nfc_t2t_internal_set(data, DATA_SIZE);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, 0);
}

/* Test serialization of nfc_t2t_internal_set() with NULL data */
ZTEST(nfc_rpc_t2t_cli, test_nfc_t2t_internal_set_null)
{
	int error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(NFC_RPC_CMD_T2T_INTERNAL_SET, CBOR_NULL), RPC_RSP(0));
	error = nfc_t2t_internal_set(NULL, DATA_SIZE);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, 0);
}

/* Test serialization of nfc_t2t_emulation_start() */
ZTEST(nfc_rpc_t2t_cli, test_nfc_t2t_emulation_start)
{
	int error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(NFC_RPC_CMD_T2T_EMULATION_START), RPC_RSP(0));
	error = nfc_t2t_emulation_start();
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, 0);
}

/* Test serialization of nfc_t2t_emulation_stop() */
ZTEST(nfc_rpc_t2t_cli, test_nfc_t2t_emulation_stop)
{
	int error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(NFC_RPC_CMD_T2T_EMULATION_STOP), RPC_RSP(0));
	error = nfc_t2t_emulation_stop();
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, 0);
}

/* Test serialization of nfc_t2t_done() */
ZTEST(nfc_rpc_t2t_cli, test_nfc_t2t_done)
{
	int error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(NFC_RPC_CMD_T2T_DONE), RPC_RSP(0));
	error = nfc_t2t_done();
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, 0);
}

static void t2t_cb(void *context, nfc_t2t_event_t event, const uint8_t *data, size_t data_length)
{
	uint8_t expected_data[DATA_SIZE] = {INT_SEQUENCE(DATA_SIZE)};

	cb_called = true;
	zassert_not_null(data);
	zassert_not_null(context);
	zassert_equal_ptr(context, (void *)0xdeadbeef);
	zassert_equal(event, NFC_T2T_EVENT_DATA_READ);
	zassert_equal(data_length, DATA_SIZE);
	zassert_mem_equal(data, expected_data, data_length);
}

/* Test reception of t2t callback. */
ZTEST(nfc_rpc_t2t_cli, test_t2t_cb_handler)
{

	int in_slot = nrf_rpc_cbkproxy_in_set(t2t_cb);

	slot_cnt++;
	cb_called = false;
	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(NFC_RPC_CMD_T2T_CB, CBOR_UINT32(0xdeadbeef),
					NFC_T2T_EVENT_DATA_READ, NFC_DATA, in_slot));
	mock_nrf_rpc_tr_expect_done();

	zassert_true(cb_called);
}

ZTEST_SUITE(nfc_rpc_t2t_cli, NULL, NULL, tc_setup, NULL, NULL);
