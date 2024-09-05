/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_nrf_rpc_transport.h>

#include <zephyr/fff.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#include <nfc_t4t_lib.h>

#include <nrf_rpc/nrf_rpc_cbkproxy.h>

#include <nfc_rpc_ids.h>
#include <nfc_rpc_common.h>
#include <test_rpc_env.h>

/* Fake functions */
FAKE_VALUE_FUNC(int, nfc_t4t_setup, nfc_t4t_callback_t, void *);
FAKE_VALUE_FUNC(int, nfc_t4t_ndef_rwpayload_set, uint8_t *, size_t);
FAKE_VALUE_FUNC(int, nfc_t4t_ndef_staticpayload_set, const uint8_t *, size_t);
FAKE_VALUE_FUNC(int, nfc_t4t_response_pdu_send, const uint8_t *, size_t);
FAKE_VALUE_FUNC(int, nfc_t4t_parameter_set, nfc_t4t_param_id_t, void *, size_t);
FAKE_VALUE_FUNC(int, nfc_t4t_parameter_get, nfc_t4t_param_id_t, void *, size_t *);
FAKE_VALUE_FUNC(int, nfc_t4t_emulation_start);
FAKE_VALUE_FUNC(int, nfc_t4t_emulation_stop);
FAKE_VALUE_FUNC(int, nfc_t4t_done);
DECLARE_FAKE_VALUE_FUNC(void *, nrf_rpc_cbkproxy_out_get, int, void *);

#define FOREACH_FAKE(f)                                                                            \
	f(nfc_t4t_setup);                                                                          \
	f(nfc_t4t_ndef_rwpayload_set);                                                             \
	f(nfc_t4t_ndef_staticpayload_set);                                                         \
	f(nfc_t4t_response_pdu_send);                                                              \
	f(nfc_t4t_parameter_set);                                                                  \
	f(nfc_t4t_parameter_get);                                                                  \
	f(nfc_t4t_emulation_start);                                                                \
	f(nfc_t4t_emulation_stop);                                                                 \
	f(nfc_t4t_done);

extern uint64_t nfc_t4t_cb_encoder(uint32_t callback_slot, uint32_t _rsv0, uint32_t _rsv1,
				   uint32_t _ret, void *context, nfc_t4t_event_t event,
				   const uint8_t *data, size_t data_length, uint32_t flags);

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

/* Test reception of nfc_t4t_setup command. */
ZTEST(nfc_rpc_t4t_srv, test_nfc_t4t_setup)
{
	nrf_rpc_cbkproxy_out_get_fake.return_val = (void *)0xfacecafe;
	nfc_t4t_setup_fake.return_val = 0;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(NFC_RPC_CMD_T4T_SETUP, 0, CBOR_UINT32(0xcafeface)));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(nfc_t4t_setup_fake.call_count, 1);
	zassert_equal_ptr(nfc_t4t_setup_fake.arg0_val, (void *)0xfacecafe);
	zassert_equal_ptr(nfc_t4t_setup_fake.arg1_val, (void *)0xcafeface);
}

static int nfc_t4t_parameter_set_custom_fake(nfc_t4t_param_id_t id, void *data, size_t data_length)
{
	uint8_t expected_data[DATA_SIZE] = {INT_SEQUENCE(DATA_SIZE)};

	zassert_equal(id, NFC_T4T_PARAM_SELRES);
	zassert_mem_equal(data, expected_data, DATA_SIZE);
	zassert_equal(data_length, DATA_SIZE);

	return 0;
}

/* Test reception of nfc_t4t_parameter_set command. */
ZTEST(nfc_rpc_t4t_srv, test_nfc_t4t_parameter_set)
{
	nfc_t4t_parameter_set_fake.custom_fake = nfc_t4t_parameter_set_custom_fake;
	nfc_t4t_parameter_set_fake.return_val = 0;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0), NO_RSP);
	mock_nrf_rpc_tr_receive(
		RPC_CMD(NFC_RPC_CMD_T4T_PARAMETER_SET, NFC_T4T_PARAM_SELRES, NFC_DATA));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(nfc_t4t_parameter_set_fake.call_count, 1);
}

static int nfc_t4t_parameter_get_custom_fake(nfc_t4t_param_id_t id, void *data,
					     size_t *max_data_length)
{
	uint8_t expected_data[DATA_SIZE] = {INT_SEQUENCE(DATA_SIZE)};

	zassert_equal(*max_data_length, DATA_SIZE);
	memcpy(data, expected_data, *max_data_length);

	return 0;
}

/* Test reception of nfc_t4t_parameter_get command. */
ZTEST(nfc_rpc_t4t_srv, test_nfc_t4t_parameter_get)
{
	nfc_t4t_parameter_get_fake.return_val = 0;
	nfc_t4t_parameter_get_fake.custom_fake = nfc_t4t_parameter_get_custom_fake;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(NFC_DATA), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(NFC_RPC_CMD_T4T_PARAMETER_GET, NFC_T4T_PARAM_FWI,
					CBOR_UINT8(DATA_SIZE)));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(nfc_t4t_parameter_get_fake.call_count, 1);
	zassert_equal(nfc_t4t_parameter_get_fake.arg0_val, NFC_T4T_PARAM_FWI);
	zassert_equal(*nfc_t4t_parameter_get_fake.arg2_val, DATA_SIZE);
}

/* Test reception of nfc_t4t_parameter_get command with returned error. */
ZTEST(nfc_rpc_t4t_srv, test_nfc_t4t_parameter_get_negative)
{
	nfc_t4t_parameter_get_fake.return_val = -NRF_EINVAL;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_NULL), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(NFC_RPC_CMD_T4T_PARAMETER_GET, NFC_T4T_PARAM_FDT_MIN,
					CBOR_UINT8(DATA_SIZE)));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(nfc_t4t_parameter_get_fake.call_count, 1);
	zassert_equal(nfc_t4t_parameter_get_fake.arg0_val, NFC_T4T_PARAM_FDT_MIN);
	zassert_equal(*nfc_t4t_parameter_get_fake.arg2_val, DATA_SIZE);
}

/* Test reception of nfc_t4t_ndef_rwpayload_set command. */
ZTEST(nfc_rpc_t4t_srv, test_nfc_t4t_ndef_rwpayload_set)
{
	mock_nrf_rpc_pkt_t expected_pkt;
	uint8_t expected_data[CONFIG_NDEF_FILE_SIZE + 9] = {
		0x80, NFC_RPC_CMD_T4T_NDEF_RWPAYLOAD_SET, 0xff, 0x00, 0x00, 0x59, 0x04, 0x00};

	for (int i = 0; i < CONFIG_NDEF_FILE_SIZE; i++) {
		expected_data[8 + i] = i + 1;
	}
	expected_data[CONFIG_NDEF_FILE_SIZE + 8] = 0xf6;

	expected_pkt.data = expected_data;
	expected_pkt.len = sizeof(expected_data);

	nfc_t4t_ndef_rwpayload_set_fake.return_val = 0;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0), NO_RSP);
	mock_nrf_rpc_tr_receive(expected_pkt);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(nfc_t4t_ndef_rwpayload_set_fake.call_count, 1);
	zassert_mem_equal(nfc_t4t_ndef_rwpayload_set_fake.arg0_val, expected_data + 8,
			  CONFIG_NDEF_FILE_SIZE);
	zassert_equal(nfc_t4t_ndef_rwpayload_set_fake.arg1_val, CONFIG_NDEF_FILE_SIZE);
}

/* Test reception of nfc_t4t_ndef_staticpayload_set command. */
ZTEST(nfc_rpc_t4t_srv, test_nfc_t4t_ndef_staticpayload_set)
{
	mock_nrf_rpc_pkt_t expected_pkt;
	uint8_t expected_data[CONFIG_NDEF_FILE_SIZE + 9] = {
		0x80, NFC_RPC_CMD_T4T_NDEF_STATICPAYLOAD_SET, 0xff, 0, 0, 0x59, 0x04, 0x00};

	for (int i = 0; i < CONFIG_NDEF_FILE_SIZE; i++) {
		expected_data[8 + i] = i + 1;
	}
	expected_data[CONFIG_NDEF_FILE_SIZE + 8] = 0xf6;

	expected_pkt.data = expected_data;
	expected_pkt.len = sizeof(expected_data);

	nfc_t4t_ndef_staticpayload_set_fake.return_val = 0;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0), NO_RSP);
	mock_nrf_rpc_tr_receive(expected_pkt);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(nfc_t4t_ndef_staticpayload_set_fake.call_count, 1);
	zassert_mem_equal(nfc_t4t_ndef_staticpayload_set_fake.arg0_val, expected_data + 8,
			  CONFIG_NDEF_FILE_SIZE);
	zassert_equal(nfc_t4t_ndef_staticpayload_set_fake.arg1_val, CONFIG_NDEF_FILE_SIZE);
}

static int nfc_t4t_internal_set_custom_fake(const uint8_t *data, size_t data_length)
{
	uint8_t expected_data[DATA_SIZE] = {INT_SEQUENCE(DATA_SIZE)};

	zassert_mem_equal(data, expected_data, DATA_SIZE);
	zassert_equal(data_length, DATA_SIZE);

	return 0;
}

/* Test reception of nfc_t4t_response_pdu_send command. */
ZTEST(nfc_rpc_t4t_srv, test_nfc_t4t_response_pdu_send)
{
	nfc_t4t_response_pdu_send_fake.return_val = 0;
	nfc_t4t_response_pdu_send_fake.custom_fake = nfc_t4t_internal_set_custom_fake;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(NFC_RPC_CMD_T4T_RESPONSE_PDU_SEND, NFC_DATA));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(nfc_t4t_response_pdu_send_fake.call_count, 1);
}

/* Test reception of nfc_t4t_response_pdu_send command with NULL data. */
ZTEST(nfc_rpc_t4t_srv, test_nfc_t4t_response_pdu_send_null)
{
	nfc_t4t_response_pdu_send_fake.return_val = 0;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(NFC_RPC_CMD_T4T_RESPONSE_PDU_SEND, CBOR_NULL));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(nfc_t4t_response_pdu_send_fake.call_count, 1);
	zassert_is_null(nfc_t4t_response_pdu_send_fake.arg0_val);
}

/* Test reception of nfc_t4t_emulation_start command. */
ZTEST(nfc_rpc_t4t_srv, test_nfc_t4t_emulation_start)
{
	nfc_t4t_emulation_start_fake.return_val = 0;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(NFC_RPC_CMD_T4T_EMULATION_START));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(nfc_t4t_emulation_start_fake.call_count, 1);
}

/* Test reception of nfc_t4t_emulation_stop command. */
ZTEST(nfc_rpc_t4t_srv, test_nfc_t4t_emulation_stop)
{
	nfc_t4t_emulation_stop_fake.return_val = 0;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(NFC_RPC_CMD_T4T_EMULATION_STOP));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(nfc_t4t_emulation_stop_fake.call_count, 1);
}

/* Test reception of nfc_t4t_done command. */
ZTEST(nfc_rpc_t4t_srv, test_nfc_t4t_done)
{
	nfc_t4t_done_fake.return_val = 0;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(NFC_RPC_CMD_T4T_DONE));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(nfc_t4t_done_fake.call_count, 1);
}

/* Test sending some data over registered nfc t4t callback. */
ZTEST(nfc_rpc_t4t_srv, test_tx_t4t_cb)
{
	void *context = (void *)0xdeadbeef;
	nfc_t4t_event_t event = NFC_T4T_EVENT_DATA_IND;
	uint8_t data[DATA_SIZE] = {INT_SEQUENCE(DATA_SIZE)};
	size_t data_length = DATA_SIZE;
	uint32_t flags = 0xcafebabe;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(NFC_RPC_CMD_T4T_CB, CBOR_UINT32(0xdeadbeef),
					   NFC_T4T_EVENT_DATA_IND, NFC_DATA,
					   CBOR_UINT32(0xcafebabe), 0),
				   RPC_RSP());
	(void)nfc_t4t_cb_encoder(0, 0, 0, 0, context, event, data, data_length, flags);
	mock_nrf_rpc_tr_expect_done();
}

/* Test sending NULL data over registered nfc t4t callback. */
ZTEST(nfc_rpc_t4t_srv, test_tx_t4t_cb_null)
{
	void *context = (void *)0xbabeface;
	nfc_t4t_event_t event = NFC_T4T_EVENT_DATA_IND;
	uint32_t flags = 0xcafebabe;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(NFC_RPC_CMD_T4T_CB, CBOR_UINT32(0xbabeface),
					   NFC_T4T_EVENT_DATA_IND, CBOR_NULL,
					   CBOR_UINT32(0xcafebabe), 0),
				   RPC_RSP());
	(void)nfc_t4t_cb_encoder(0, 0, 0, 0, context, event, NULL, DATA_SIZE, flags);
	mock_nrf_rpc_tr_expect_done();
}

ZTEST_SUITE(nfc_rpc_t4t_srv, NULL, NULL, tc_setup, NULL, NULL);
