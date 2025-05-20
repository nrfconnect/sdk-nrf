/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_nrf_rpc_transport.h>

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#include <nfc_t4t_lib.h>

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

/* Test serialization of nfc_t4t_setup() */
ZTEST(nfc_rpc_t4t_cli, test_nfc_t4t_setup)
{
	int error;
	nfc_t4t_callback_t cb = (nfc_t4t_callback_t)0xdeadbeef;
	void *cb_ctx = (void *)0xcafeface;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(NFC_RPC_CMD_T4T_SETUP, slot_cnt, CBOR_UINT32(0xcafeface)), RPC_RSP(0));
	error = nfc_t4t_setup(cb, cb_ctx);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, 0);
}

/* Test serialization of nfc_t4t_parameter_set() */
ZTEST(nfc_rpc_t4t_cli, test_nfc_t4t_parameter_set)
{
	int error;
	uint8_t data[DATA_SIZE] = {INT_SEQUENCE(DATA_SIZE)};

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(NFC_RPC_CMD_T4T_PARAMETER_SET, NFC_T4T_PARAM_NFCID1, NFC_DATA), RPC_RSP(0));
	error = nfc_t4t_parameter_set(NFC_T4T_PARAM_NFCID1, data, DATA_SIZE);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, 0);
}

/* Test serialization of nfc_t4t_parameter_set() with NULL data */
ZTEST(nfc_rpc_t4t_cli, test_nfc_t4t_parameter_set_null)
{
	int error;

	error = nfc_t4t_parameter_set(NFC_T4T_PARAM_NFCID1, NULL, 10);
	zassert_equal(error, -NRF_EINVAL);
}

/* Test serialization of nfc_t4t_parameter_get() */
ZTEST(nfc_rpc_t4t_cli, test_nfc_t4t_parameter_get)
{
	int error;
	uint8_t expected_data[DATA_SIZE] = {INT_SEQUENCE(DATA_SIZE)};
	uint8_t data[DATA_SIZE];
	size_t max_data_length = DATA_SIZE;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(NFC_RPC_CMD_T4T_PARAMETER_GET, NFC_T4T_PARAM_FDT_MIN,
					   CBOR_UINT8(DATA_SIZE)),
				   RPC_RSP(NFC_DATA));
	error = nfc_t4t_parameter_get(NFC_T4T_PARAM_FDT_MIN, data, &max_data_length);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, 0);
	zassert_mem_equal(data, expected_data, DATA_SIZE);
}

/* Test serialization of nfc_t4t_parameter_get() with NULL data */
ZTEST(nfc_rpc_t4t_cli, test_nfc_t4t_parameter_get_null)
{
	int error;
	uint8_t data[DATA_SIZE];
	size_t max_data_length = DATA_SIZE;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(NFC_RPC_CMD_T4T_PARAMETER_GET, NFC_T4T_PARAM_FDT_MIN,
					   CBOR_UINT8(DATA_SIZE)),
				   RPC_RSP(CBOR_NULL));
	error = nfc_t4t_parameter_get(NFC_T4T_PARAM_FDT_MIN, data, &max_data_length);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, -NRF_EINVAL);
}

static void payload_test(uint8_t cmd)
{
	int error;
	uint8_t data[CONFIG_NDEF_FILE_SIZE];
	mock_nrf_rpc_pkt_t expected_pkt;
	uint8_t expected_data[CONFIG_NDEF_FILE_SIZE + 9] = {0x80, cmd,  0xff, 0x00,
							    0x00, 0x59, 0x04, 0x00};

	for (int i = 0; i < CONFIG_NDEF_FILE_SIZE; i++) {
		data[i] = i + 1;
		expected_data[8 + i] = i + 1;
	}
	expected_data[CONFIG_NDEF_FILE_SIZE + 8] = 0xf6;

	expected_pkt.data = expected_data;
	expected_pkt.len = sizeof(expected_data);
	mock_nrf_rpc_tr_expect_add(expected_pkt, RPC_RSP(0));

	switch (cmd) {
	case NFC_RPC_CMD_T4T_NDEF_RWPAYLOAD_SET:
		error = nfc_t4t_ndef_rwpayload_set(data, CONFIG_NDEF_FILE_SIZE);
		break;
	case NFC_RPC_CMD_T4T_NDEF_STATICPAYLOAD_SET:
		error = nfc_t4t_ndef_staticpayload_set(data, CONFIG_NDEF_FILE_SIZE);
		break;
	case NFC_RPC_CMD_T4T_RESPONSE_PDU_SEND:
		error = nfc_t4t_response_pdu_send(data, CONFIG_NDEF_FILE_SIZE);
		break;
	default:
		zassert_unreachable();
		break;
	}

	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, 0);
}

/* Test serialization of nfc_t4t_ndef_rwpayload_set() */
ZTEST(nfc_rpc_t4t_cli, test_nfc_t4t_ndef_rwpayload_set)
{
	payload_test(NFC_RPC_CMD_T4T_NDEF_RWPAYLOAD_SET);
}

/* Test serialization of nfc_t4t_ndef_staticpayload_set() */
ZTEST(nfc_rpc_t4t_cli, test_nfc_t4t_ndef_staticpayload_set)
{
	payload_test(NFC_RPC_CMD_T4T_NDEF_STATICPAYLOAD_SET);
}

/* Test serialization of nfc_t4t_response_pdu_send() */
ZTEST(nfc_rpc_t4t_cli, test_nfc_t4t_response_pdu_send)
{
	payload_test(NFC_RPC_CMD_T4T_RESPONSE_PDU_SEND);
}

/* Test serialization of nfc_t4t_emulation_start() */
ZTEST(nfc_rpc_t4t_cli, test_nfc_t4t_emulation_start)
{
	int error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(NFC_RPC_CMD_T4T_EMULATION_START), RPC_RSP(0));
	error = nfc_t4t_emulation_start();
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, 0);
}

/* Test serialization of nfc_t4t_emulation_stop() */
ZTEST(nfc_rpc_t4t_cli, test_nfc_t4t_emulation_stop)
{
	int error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(NFC_RPC_CMD_T4T_EMULATION_STOP), RPC_RSP(0));
	error = nfc_t4t_emulation_stop();
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, 0);
}

/* Test serialization of nfc_t4t_done() */
ZTEST(nfc_rpc_t4t_cli, test_nfc_t4t_done)
{
	int error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(NFC_RPC_CMD_T4T_DONE), RPC_RSP(0));
	error = nfc_t4t_done();
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, 0);
}

static void t4t_cb(void *context, nfc_t4t_event_t event, const uint8_t *data, size_t data_length,
		   uint32_t flags)
{
	uint8_t expected_data[DATA_SIZE] = {INT_SEQUENCE(DATA_SIZE)};

	cb_called = true;
	zassert_not_null(data);
	zassert_not_null(context);
	zassert_equal_ptr(context, (void *)0xdeadbeef);
	zassert_equal(event, NFC_T4T_EVENT_DATA_IND);
	zassert_equal(data_length, DATA_SIZE);
	zassert_mem_equal(data, expected_data, data_length);
	zassert_equal(flags, 0xcafebabe);
}

/* Test reception of t4t callback. */
ZTEST(nfc_rpc_t4t_cli, test_t4t_cb_handler)
{

	int in_slot = nrf_rpc_cbkproxy_in_set(t4t_cb);

	slot_cnt++;
	cb_called = false;
	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(NFC_RPC_CMD_T4T_CB, CBOR_UINT32(0xdeadbeef),
					NFC_T4T_EVENT_DATA_IND, NFC_DATA, CBOR_UINT32(0xcafebabe),
					in_slot));
	mock_nrf_rpc_tr_expect_done();

	zassert_true(cb_called);
}

ZTEST_SUITE(nfc_rpc_t4t_cli, NULL, NULL, tc_setup, NULL, NULL);
