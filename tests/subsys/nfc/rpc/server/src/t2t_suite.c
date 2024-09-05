/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_nrf_rpc_transport.h>

#include <zephyr/fff.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#include <nfc_t2t_lib.h>

#include <nrf_rpc/nrf_rpc_cbkproxy.h>

#include <nfc_rpc_ids.h>
#include <nfc_rpc_common.h>
#include <test_rpc_env.h>

/* Fake functions */
DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, nfc_t2t_setup, nfc_t2t_callback_t, void *);
FAKE_VALUE_FUNC(int, nfc_t2t_parameter_set, nfc_t2t_param_id_t, void *, size_t);
FAKE_VALUE_FUNC(int, nfc_t2t_parameter_get, nfc_t2t_param_id_t, void *, size_t *);
FAKE_VALUE_FUNC(int, nfc_t2t_payload_set, const uint8_t *, size_t);
FAKE_VALUE_FUNC(int, nfc_t2t_payload_raw_set, const uint8_t *, size_t);
FAKE_VALUE_FUNC(int, nfc_t2t_internal_set, const uint8_t *, size_t);
FAKE_VALUE_FUNC(int, nfc_t2t_emulation_start);
FAKE_VALUE_FUNC(int, nfc_t2t_emulation_stop);
FAKE_VALUE_FUNC(int, nfc_t2t_done);
FAKE_VALUE_FUNC(void *, nrf_rpc_cbkproxy_out_get, int, void *);

#define FOREACH_FAKE(f)                                                                            \
	f(nfc_t2t_setup);                                                                          \
	f(nfc_t2t_parameter_set);                                                                  \
	f(nfc_t2t_parameter_get);                                                                  \
	f(nfc_t2t_payload_set);                                                                    \
	f(nfc_t2t_payload_raw_set);                                                                \
	f(nfc_t2t_internal_set);                                                                   \
	f(nfc_t2t_emulation_start);                                                                \
	f(nfc_t2t_emulation_stop);                                                                 \
	f(nfc_t2t_done);                                                                           \
	f(nrf_rpc_cbkproxy_out_get);

extern uint64_t nfc_t2t_cb_encoder(uint32_t callback_slot, uint32_t _rsv0, uint32_t _rsv1,
				   uint32_t _ret, void *context, nfc_t2t_event_t event,
				   const uint8_t *data, size_t data_length);

static void nrf_rpc_err_handler(const struct nrf_rpc_err_report *report)
{
	zassert_ok(report->code);
}

static void tc_setup(void *f)
{
	mock_nrf_rpc_tr_expect_add(RPC_INIT_REQ, RPC_INIT_RSP);
	zassert_ok(nrf_rpc_init(nrf_rpc_err_handler));
	mock_nrf_rpc_tr_expect_reset();

	FOREACH_FAKE(RESET_FAKE);
	FFF_RESET_HISTORY();
}

/* Test reception of nfc_t2t_setup command. */
ZTEST(nfc_rpc_t2t_srv, test_nfc_t2t_setup)
{
	nrf_rpc_cbkproxy_out_get_fake.return_val = (void *)0xfacecafe;
	nfc_t2t_setup_fake.return_val = 0;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(NFC_RPC_CMD_T2T_SETUP, 0, CBOR_UINT32(0xcafeface)));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(nfc_t2t_setup_fake.call_count, 1);
	zassert_equal_ptr(nfc_t2t_setup_fake.arg0_val, (void *)0xfacecafe);
	zassert_equal_ptr(nfc_t2t_setup_fake.arg1_val, (void *)0xcafeface);
}

static int nfc_t2t_parameter_set_custom_fake(nfc_t2t_param_id_t id, void *data, size_t data_length)
{
	uint8_t expected_data[DATA_SIZE] = {INT_SEQUENCE(DATA_SIZE)};

	zassert_equal(id, NFC_T2T_PARAM_NFCID1);
	zassert_mem_equal(data, expected_data, DATA_SIZE);
	zassert_equal(data_length, DATA_SIZE);

	return 0;
}

/* Test reception of nfc_t2t_parameter_set command. */
ZTEST(nfc_rpc_t2t_srv, test_nfc_t2t_parameter_set)
{
	nfc_t2t_parameter_set_fake.custom_fake = nfc_t2t_parameter_set_custom_fake;
	nfc_t2t_parameter_set_fake.return_val = 0;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0), NO_RSP);
	mock_nrf_rpc_tr_receive(
		RPC_CMD(NFC_RPC_CMD_T2T_PARAMETER_SET, NFC_T2T_PARAM_NFCID1, NFC_DATA));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(nfc_t2t_parameter_set_fake.call_count, 1);
}

static int nfc_t2t_parameter_get_custom_fake(nfc_t2t_param_id_t id, void *data,
					     size_t *max_data_length)
{
	uint8_t expected_data[DATA_SIZE] = {INT_SEQUENCE(DATA_SIZE)};

	zassert_equal(*max_data_length, DATA_SIZE);
	memcpy(data, expected_data, *max_data_length);

	return 0;
}

/* Test reception of nfc_t2t_parameter_get command. */
ZTEST(nfc_rpc_t2t_srv, test_nfc_t2t_parameter_get)
{
	nfc_t2t_parameter_get_fake.return_val = 0;
	nfc_t2t_parameter_get_fake.custom_fake = nfc_t2t_parameter_get_custom_fake;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(NFC_DATA), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(NFC_RPC_CMD_T2T_PARAMETER_GET, NFC_T2T_PARAM_FDT_MIN,
					CBOR_UINT8(DATA_SIZE)));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(nfc_t2t_parameter_get_fake.call_count, 1);
	zassert_equal(nfc_t2t_parameter_get_fake.arg0_val, NFC_T2T_PARAM_FDT_MIN);
	zassert_equal(*nfc_t2t_parameter_get_fake.arg2_val, DATA_SIZE);
}

/* Test reception of nfc_t2t_parameter_get command with returned error. */
ZTEST(nfc_rpc_t2t_srv, test_nfc_t2t_parameter_get_negative)
{
	nfc_t2t_parameter_get_fake.return_val = -NRF_EINVAL;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_NULL), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(NFC_RPC_CMD_T2T_PARAMETER_GET, NFC_T2T_PARAM_FDT_MIN,
					CBOR_UINT8(DATA_SIZE)));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(nfc_t2t_parameter_get_fake.call_count, 1);
	zassert_equal(nfc_t2t_parameter_get_fake.arg0_val, NFC_T2T_PARAM_FDT_MIN);
	zassert_equal(*nfc_t2t_parameter_get_fake.arg2_val, DATA_SIZE);
}

/* Test reception of nfc_t2t_payload_set command. */
ZTEST(nfc_rpc_t2t_srv, test_nfc_t2t_payload_set)
{
	mock_nrf_rpc_pkt_t expected_pkt;
	uint8_t expected_data[NFC_T2T_MAX_PAYLOAD_SIZE + 9] = {
		0x80, NFC_RPC_CMD_T2T_PAYLOAD_SET, 0xff, 0x00, 0x00, 0x59, 0x03, 0xdc};

	for (int i = 0; i < NFC_T2T_MAX_PAYLOAD_SIZE; i++) {
		expected_data[8 + i] = i + 1;
	}
	expected_data[NFC_T2T_MAX_PAYLOAD_SIZE + 8] = 0xf6;

	expected_pkt.data = expected_data;
	expected_pkt.len = sizeof(expected_data);

	nfc_t2t_payload_set_fake.return_val = 0;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0), NO_RSP);
	mock_nrf_rpc_tr_receive(expected_pkt);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(nfc_t2t_payload_set_fake.call_count, 1);
	zassert_mem_equal(nfc_t2t_payload_set_fake.arg0_val, expected_data + 8,
			  NFC_T2T_MAX_PAYLOAD_SIZE);
	zassert_equal(nfc_t2t_payload_set_fake.arg1_val, NFC_T2T_MAX_PAYLOAD_SIZE);
}

/* Test reception of nfc_t2t_payload_set command with NULL data. */
ZTEST(nfc_rpc_t2t_srv, test_nfc_t2t_payload_set_null)
{
	nfc_t2t_payload_set_fake.return_val = 0;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(NFC_RPC_CMD_T2T_PAYLOAD_SET, CBOR_NULL));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(nfc_t2t_payload_set_fake.call_count, 1);
	zassert_is_null(nfc_t2t_payload_set_fake.arg0_val);
}

/* Test reception of nfc_t2t_payload_set command with zero length. */
ZTEST(nfc_rpc_t2t_srv, test_nfc_t2t_payload_set_zero)
{
	nfc_t2t_payload_set_fake.return_val = 0;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(NFC_RPC_CMD_T2T_PAYLOAD_SET, 0x40));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(nfc_t2t_payload_set_fake.call_count, 1);
	zassert_not_null(nfc_t2t_payload_set_fake.arg0_val);
	zassert_equal(nfc_t2t_payload_set_fake.arg1_val, 0);
}

/* Test reception of nfc_t2t_payload_raw_set command. */
ZTEST(nfc_rpc_t2t_srv, test_nfc_t2t_payload_raw_set)
{
	mock_nrf_rpc_pkt_t expected_pkt;
	uint8_t expected_data[NFC_T2T_MAX_PAYLOAD_SIZE_RAW + 9] = {
		0x80, NFC_RPC_CMD_T2T_PAYLOAD_RAW_SET, 0xff, 0, 0, 0x59, 0x03, 0xf0};

	for (int i = 0; i < NFC_T2T_MAX_PAYLOAD_SIZE_RAW; i++) {
		expected_data[8 + i] = i + 1;
	}
	expected_data[NFC_T2T_MAX_PAYLOAD_SIZE_RAW + 8] = 0xf6;

	expected_pkt.data = expected_data;
	expected_pkt.len = sizeof(expected_data);

	nfc_t2t_payload_raw_set_fake.return_val = 0;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0), NO_RSP);
	mock_nrf_rpc_tr_receive(expected_pkt);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(nfc_t2t_payload_raw_set_fake.call_count, 1);
	zassert_mem_equal(nfc_t2t_payload_raw_set_fake.arg0_val, expected_data + 8,
			  NFC_T2T_MAX_PAYLOAD_SIZE_RAW);
	zassert_equal(nfc_t2t_payload_raw_set_fake.arg1_val, NFC_T2T_MAX_PAYLOAD_SIZE_RAW);
}

/* Test reception of nfc_t2t_payload_raw_set command with NULL data. */
ZTEST(nfc_rpc_t2t_srv, test_nfc_t2t_payload_raw_set_null)
{
	nfc_t2t_payload_raw_set_fake.return_val = 0;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(NFC_RPC_CMD_T2T_PAYLOAD_RAW_SET, CBOR_NULL));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(nfc_t2t_payload_raw_set_fake.call_count, 1);
	zassert_is_null(nfc_t2t_payload_raw_set_fake.arg0_val);
}

static int nfc_t2t_internal_set_custom_fake(const uint8_t *data, size_t data_length)
{
	uint8_t expected_data[DATA_SIZE] = {INT_SEQUENCE(DATA_SIZE)};

	zassert_mem_equal(data, expected_data, DATA_SIZE);
	zassert_equal(data_length, DATA_SIZE);

	return 0;
}

/* Test reception of nfc_t2t_internal_set command. */
ZTEST(nfc_rpc_t2t_srv, test_nfc_t2t_internal_set)
{
	nfc_t2t_internal_set_fake.return_val = 0;
	nfc_t2t_internal_set_fake.custom_fake = nfc_t2t_internal_set_custom_fake;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(NFC_RPC_CMD_T2T_INTERNAL_SET, NFC_DATA));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(nfc_t2t_internal_set_fake.call_count, 1);
}

/* Test reception of nfc_t2t_internal_set command with NULL data. */
ZTEST(nfc_rpc_t2t_srv, test_nfc_t2t_internal_set_null)
{
	nfc_t2t_internal_set_fake.return_val = 0;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(NFC_RPC_CMD_T2T_INTERNAL_SET, CBOR_NULL));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(nfc_t2t_internal_set_fake.call_count, 1);
	zassert_is_null(nfc_t2t_internal_set_fake.arg0_val);
}

/* Test reception of nfc_t2t_emulation_start command. */
ZTEST(nfc_rpc_t2t_srv, test_nfc_t2t_emulation_start)
{
	nfc_t2t_emulation_start_fake.return_val = 0;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(NFC_RPC_CMD_T2T_EMULATION_START));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(nfc_t2t_emulation_start_fake.call_count, 1);
}

/* Test reception of nfc_t2t_emulation_stop command. */
ZTEST(nfc_rpc_t2t_srv, test_nfc_t2t_emulation_stop)
{
	nfc_t2t_emulation_stop_fake.return_val = 0;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(NFC_RPC_CMD_T2T_EMULATION_STOP));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(nfc_t2t_emulation_stop_fake.call_count, 1);
}

/* Test reception of nfc_t2t_done command. */
ZTEST(nfc_rpc_t2t_srv, test_nfc_t2t_done)
{
	nfc_t2t_done_fake.return_val = 0;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(NFC_RPC_CMD_T2T_DONE));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(nfc_t2t_done_fake.call_count, 1);
}

/* Test sending some data over registered nfc t2t callback. */
ZTEST(nfc_rpc_t2t_srv, test_tx_t2t_cb)
{
	void *context = (void *)0xdeadbeef;
	nfc_t2t_event_t event = NFC_T2T_EVENT_STOPPED;
	uint8_t data[DATA_SIZE] = {INT_SEQUENCE(DATA_SIZE)};
	size_t data_length = DATA_SIZE;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(NFC_RPC_CMD_T2T_CB, CBOR_UINT32(0xdeadbeef),
					   NFC_T2T_EVENT_STOPPED, NFC_DATA, 0),
				   RPC_RSP());
	(void)nfc_t2t_cb_encoder(0, 0, 0, 0, context, event, data, data_length);
	mock_nrf_rpc_tr_expect_done();
}

/* Test sending NULL data over registered nfc t2t callback. */
ZTEST(nfc_rpc_t2t_srv, test_tx_t2t_cb_null)
{
	void *context = (void *)0xbabeface;
	nfc_t2t_event_t event = NFC_T2T_EVENT_FIELD_ON;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(NFC_RPC_CMD_T2T_CB, CBOR_UINT32(0xbabeface),
					   NFC_T2T_EVENT_FIELD_ON, CBOR_NULL, 0),
				   RPC_RSP());
	(void)nfc_t2t_cb_encoder(0, 0, 0, 0, context, event, NULL, DATA_SIZE);
	mock_nrf_rpc_tr_expect_done();
}

ZTEST_SUITE(nfc_rpc_t2t_srv, NULL, NULL, tc_setup, NULL, NULL);
