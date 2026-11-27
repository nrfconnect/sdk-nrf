/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/ztest.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/bluetooth/hci_types.h>

#if defined(CONFIG_DTM_TWOWIRE_TO_HCI_SDC_VS_COMMANDS)
#include <sdc_hci_vs.h>
#endif /* CONFIG_DTM_TWOWIRE_TO_HCI_SDC_VS_COMMANDS */

#include <bluetooth/dtm_twowire/dtm_twowire_types.h>
#include <bluetooth/dtm_twowire/dtm_twowire_to_hci.h>

#define RX_TEST_VERSION CONFIG_DTM_TWOWIRE_TO_HCI_RECEIVER_TEST_VERSION
#define TX_TEST_VERSION CONFIG_DTM_TWOWIRE_TO_HCI_TRANSMITTER_TEST_VERSION

/* Fits the largest HCI packet in use: the H4 Command Complete event of
 * HCI_LE_Read_All_Local_Supported_Features.
 */
#define HCI_BUF_SIZE 260

/* Offsets in an H4 HCI event packet */
#define H4_PKT_INDICATOR_OFFSET 0
#define H4_EVT_CODE_OFFSET	1

/* The HCI command buffer is filled with this value before every call to the library,
 * so that command fields left unset by the library are detected.
 */
#define BUF_POISON 0xA5

/* Initial value of the 2-wire event output, used to detect if the library has written to it */
#define TW_EVENT_UNSET 0x5A5A

/* Transmit power range reported by the fake controller */
#define CTLR_TX_POWER_MIN (-40)
#define CTLR_TX_POWER_MAX 8

/* DTM 2-wire commands, as specified in Core v6.2, Vol 6, Part F, 3.3 */
#define TW_SETUP(control, parameter)                                                               \
	((uint16_t)((DTM_TW_CMD_TEST_SETUP << 14) | (((control) & 0x3F) << 8) |                    \
		    ((parameter) & 0xFF)))
#define TW_END(control, parameter)                                                                 \
	((uint16_t)((DTM_TW_CMD_TEST_END << 14) | (((control) & 0x3F) << 8) | ((parameter) & 0xFF)))
#define TW_RX(channel) ((uint16_t)((DTM_TW_CMD_RECEIVER_TEST << 14) | (((channel) & 0x3F) << 8)))
#define TW_TX(channel, length, pkt_type)                                                           \
	((uint16_t)((DTM_TW_CMD_TRANSMITTER_TEST << 14) | (((channel) & 0x3F) << 8) |              \
		    (((length) & 0x3F) << 2) | ((pkt_type) & 0x03)))

/* LE_Test_Status event reporting success with a value in the response field */
#define TW_STATUS_RESPONSE(value)                                                                  \
	((uint16_t)(DTM_TW_EVENT_TEST_STATUS_SUCCESS | ((value) << DTM_TW_EVENT_RESPONSE_POS)))

/* LE_Test_Status event reporting success for the Transmit Power setup command */
#define TW_STATUS_TX_POWER(tx_power, flags)                                                        \
	((uint16_t)(DTM_TW_EVENT_TEST_STATUS_SUCCESS | (flags) |                                   \
		    ((uint8_t)(tx_power) << DTM_TW_TRANSMIT_POWER_RESPONSE_LVL_POS)))

NET_BUF_POOL_DEFINE(hci_pool, 2, HCI_BUF_SIZE, 0, NULL);

static struct net_buf *hci_cmd;
static struct net_buf *hci_evt;

static dtm_tw_to_hci_status_t process_tw_cmd(uint16_t tw_cmd, uint16_t *tw_event)
{
	net_buf_reset(hci_cmd);
	memset(hci_cmd->data, BUF_POISON, net_buf_tailroom(hci_cmd));

	return dtm_tw_to_hci_process_tw_cmd(tw_cmd, hci_cmd, tw_event);
}

/* Process a 2-wire command that is expected to be answered directly with a 2-wire event */
static uint16_t tw_cmd_to_tw_event(uint16_t tw_cmd)
{
	uint16_t tw_event = TW_EVENT_UNSET;
	dtm_tw_to_hci_status_t status = process_tw_cmd(tw_cmd, &tw_event);

	zassert_equal(status, DTM_TW_TO_HCI_STATUS_TW_EVENT,
		      "2-wire command 0x%04X: unexpected status %d", tw_cmd, status);

	return tw_event;
}

/* Process a 2-wire command that is expected to be converted into the given HCI command.
 * Returns the parameters of the generated HCI command.
 */
static const uint8_t *tw_cmd_to_hci_cmd(uint16_t tw_cmd, uint16_t opcode, uint8_t param_len)
{
	uint16_t tw_event = TW_EVENT_UNSET;
	dtm_tw_to_hci_status_t status = process_tw_cmd(tw_cmd, &tw_event);

	zassert_equal(status, DTM_TW_TO_HCI_STATUS_HCI_CMD,
		      "2-wire command 0x%04X: unexpected status %d", tw_cmd, status);
	zassert_equal(hci_cmd->len, BT_HCI_CMD_HDR_SIZE + param_len,
		      "2-wire command 0x%04X: unexpected HCI command length %d", tw_cmd,
		      hci_cmd->len);
	zassert_equal(sys_get_le16(hci_cmd->data), opcode,
		      "2-wire command 0x%04X: unexpected HCI opcode 0x%04X", tw_cmd,
		      sys_get_le16(hci_cmd->data));
	zassert_equal(hci_cmd->data[sizeof(uint16_t)], param_len,
		      "2-wire command 0x%04X: unexpected HCI parameter length %d", tw_cmd,
		      hci_cmd->data[sizeof(uint16_t)]);

	return &hci_cmd->data[BT_HCI_CMD_HDR_SIZE];
}

static void assert_tw_event(uint16_t tw_cmd, uint16_t expected_tw_event)
{
	uint16_t tw_event = tw_cmd_to_tw_event(tw_cmd);

	zassert_equal(tw_event, expected_tw_event,
		      "2-wire command 0x%04X: unexpected 2-wire event 0x%04X", tw_cmd, tw_event);
}

static void assert_rx_test_cmd(uint16_t tw_cmd, uint8_t channel, uint8_t phy, uint8_t mod_index)
{
#if RX_TEST_VERSION == 1
	const uint16_t opcode = BT_HCI_OP_LE_RX_TEST;
	const uint8_t expected[] = {channel};

	ARG_UNUSED(phy);
	ARG_UNUSED(mod_index);
#elif RX_TEST_VERSION == 2
	const uint16_t opcode = BT_HCI_OP_LE_ENH_RX_TEST;
	const uint8_t expected[] = {channel, phy, mod_index};
#else
	const uint16_t opcode = BT_HCI_OP_LE_RX_TEST_V3;
	/* CTE is not supported, so the CTE and antenna switching parameters are all zero */
	const uint8_t expected[] = {channel,
				    phy,
				    mod_index,
				    BT_HCI_LE_TEST_CTE_DISABLED,
				    BT_HCI_LE_TEST_CTE_TYPE_ANY,
				    BT_HCI_LE_TEST_SLOT_DURATION_ANY,
				    BT_HCI_LE_TEST_SWITCH_PATTERN_LEN_ANY};
#endif
	const uint8_t *params = tw_cmd_to_hci_cmd(tw_cmd, opcode, sizeof(expected));

	zassert_mem_equal(params, expected, sizeof(expected),
			  "2-wire command 0x%04X: unexpected HCI parameters", tw_cmd);
}

static void assert_tx_test_cmd(uint16_t tw_cmd, uint8_t channel, uint8_t length, uint8_t payload,
			       uint8_t phy, int8_t tx_power)
{
#if TX_TEST_VERSION == 1
	const uint16_t opcode = BT_HCI_OP_LE_TX_TEST;
	const uint8_t expected[] = {channel, length, payload};

	ARG_UNUSED(phy);
	ARG_UNUSED(tx_power);
#elif TX_TEST_VERSION == 2
	const uint16_t opcode = BT_HCI_OP_LE_ENH_TX_TEST;
	const uint8_t expected[] = {channel, length, payload, phy};

	ARG_UNUSED(tx_power);
#elif TX_TEST_VERSION == 3
	const uint16_t opcode = BT_HCI_OP_LE_TX_TEST_V3;
	/* CTE is not supported, so the CTE and antenna switching parameters are all zero */
	const uint8_t expected[] = {channel,
				    length,
				    payload,
				    phy,
				    BT_HCI_LE_TEST_CTE_DISABLED,
				    BT_HCI_LE_TEST_CTE_TYPE_ANY,
				    BT_HCI_LE_TEST_SWITCH_PATTERN_LEN_ANY};

	ARG_UNUSED(tx_power);
#else
	const uint16_t opcode = BT_HCI_OP_LE_TX_TEST_V4;
	/* CTE is not supported, so the CTE and antenna switching parameters are all zero */
	const uint8_t expected[] = {channel,
				    length,
				    payload,
				    phy,
				    BT_HCI_LE_TEST_CTE_DISABLED,
				    BT_HCI_LE_TEST_CTE_TYPE_ANY,
				    BT_HCI_LE_TEST_SWITCH_PATTERN_LEN_ANY,
				    (uint8_t)tx_power};
#endif
	const uint8_t *params = tw_cmd_to_hci_cmd(tw_cmd, opcode, sizeof(expected));

	zassert_mem_equal(params, expected, sizeof(expected),
			  "2-wire command 0x%04X: unexpected HCI parameters", tw_cmd);
}

static void hci_evt_cmd_complete(uint16_t opcode, const void *return_params, uint8_t len)
{
	net_buf_reset(hci_evt);
	net_buf_add_u8(hci_evt, BT_HCI_H4_EVT);
	net_buf_add_u8(hci_evt, BT_HCI_EVT_CMD_COMPLETE);
	net_buf_add_u8(hci_evt, sizeof(struct bt_hci_evt_cmd_complete) + len);
	net_buf_add_u8(hci_evt, 1); /* Num_HCI_Command_Packets */
	net_buf_add_le16(hci_evt, opcode);
	net_buf_add_mem(hci_evt, return_params, len);
}

static void hci_evt_cmd_complete_status(uint16_t opcode, uint8_t status)
{
	hci_evt_cmd_complete(opcode, &status, sizeof(status));
}

static void hci_evt_cmd_status(uint16_t opcode, uint8_t status)
{
	net_buf_reset(hci_evt);
	net_buf_add_u8(hci_evt, BT_HCI_H4_EVT);
	net_buf_add_u8(hci_evt, BT_HCI_EVT_CMD_STATUS);
	net_buf_add_u8(hci_evt, sizeof(struct bt_hci_evt_cmd_status));
	net_buf_add_u8(hci_evt, status);
	net_buf_add_u8(hci_evt, 1); /* Num_HCI_Command_Packets */
	net_buf_add_le16(hci_evt, opcode);
}

/* Process the prepared HCI event hci_evt (global), which is expected to be converted into a 2-wire
 * event as a response to the 2-wire command tw_cmd
 */
static uint16_t hci_evt_to_tw_event(uint16_t tw_cmd)
{
	uint16_t tw_event = TW_EVENT_UNSET;
	dtm_tw_to_hci_status_t status = dtm_tw_to_hci_process_hci_event(tw_cmd, hci_evt, &tw_event);

	zassert_equal(status, DTM_TW_TO_HCI_STATUS_TW_EVENT,
		      "2-wire command 0x%04X: unexpected status %d for the HCI event", tw_cmd,
		      status);

	return tw_event;
}

static void assert_hci_evt_unhandled(void)
{
	uint16_t tw_event = TW_EVENT_UNSET;

	zassert_equal(dtm_tw_to_hci_process_hci_event(TW_END(0, 0), hci_evt, &tw_event),
		      DTM_TW_TO_HCI_STATUS_UNHANDLED);
	zassert_equal(tw_event, TW_EVENT_UNSET, "A 2-wire event was generated");
}

/* Run the full Transmit Power setup sequence against a controller supporting the
 * CTLR_TX_POWER_MIN to CTLR_TX_POWER_MAX range. Returns the resulting 2-wire event.
 */
static uint16_t setup_tx_power(uint8_t parameter)
{
	const uint16_t tw_cmd = TW_SETUP(DTM_TW_TEST_SETUP_TRANSMIT_POWER, parameter);
	const struct bt_hci_rp_le_read_tx_power rp = {
		.status = BT_HCI_ERR_SUCCESS,
		.min_tx_power = CTLR_TX_POWER_MIN,
		.max_tx_power = CTLR_TX_POWER_MAX,
	};

	tw_cmd_to_hci_cmd(tw_cmd, BT_HCI_OP_LE_READ_TX_POWER, 0);
	hci_evt_cmd_complete(BT_HCI_OP_LE_READ_TX_POWER, &rp, sizeof(rp));

	return hci_evt_to_tw_event(tw_cmd);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	hci_cmd = net_buf_alloc(&hci_pool, K_NO_WAIT);
	hci_evt = net_buf_alloc(&hci_pool, K_NO_WAIT);
	zassert_not_null(hci_cmd);
	zassert_not_null(hci_evt);

	/* The library buffers the test parameters, so restore the defaults for every test */
	assert_tw_event(TW_SETUP(DTM_TW_TEST_SETUP_RESET, DTM_TW_RESET_MIN_RANGE),
			DTM_TW_EVENT_TEST_STATUS_SUCCESS);
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	if (hci_cmd) {
		net_buf_unref(hci_cmd);
		hci_cmd = NULL;
	}
	if (hci_evt) {
		net_buf_unref(hci_evt);
		hci_evt = NULL;
	}
}

ZTEST_SUITE(dtm_twowire_to_hci, NULL, NULL, before, after, NULL);

ZTEST(dtm_twowire_to_hci, test_tw_cmd_invalid_args)
{
	const uint16_t tw_cmd = TW_SETUP(DTM_TW_TEST_SETUP_RESET, DTM_TW_RESET_MIN_RANGE);
	uint16_t tw_event = TW_EVENT_UNSET;

	zassert_equal(dtm_tw_to_hci_process_tw_cmd(tw_cmd, NULL, &tw_event),
		      DTM_TW_TO_HCI_STATUS_ERROR);
	zassert_equal(tw_event, TW_EVENT_UNSET, "A 2-wire event was generated");

	zassert_equal(process_tw_cmd(tw_cmd, NULL), DTM_TW_TO_HCI_STATUS_ERROR);
	zassert_equal(hci_cmd->len, 0, "An HCI command was generated");
}

ZTEST(dtm_twowire_to_hci, test_hci_event_invalid_args)
{
	const uint16_t tw_cmd = TW_TX(0, 0, DTM_TW_PKT_PRBS9);
	const struct net_buf no_data = {0};
	uint16_t tw_event = TW_EVENT_UNSET;

	hci_evt_cmd_complete_status(BT_HCI_OP_LE_TX_TEST, BT_HCI_ERR_SUCCESS);

	zassert_equal(dtm_tw_to_hci_process_hci_event(tw_cmd, NULL, &tw_event),
		      DTM_TW_TO_HCI_STATUS_ERROR);
	zassert_equal(dtm_tw_to_hci_process_hci_event(tw_cmd, &no_data, &tw_event),
		      DTM_TW_TO_HCI_STATUS_ERROR);
	zassert_equal(tw_event, TW_EVENT_UNSET, "A 2-wire event was generated");

	zassert_equal(dtm_tw_to_hci_process_hci_event(tw_cmd, hci_evt, NULL),
		      DTM_TW_TO_HCI_STATUS_ERROR);
}

ZTEST(dtm_twowire_to_hci, test_setup_reset)
{
	for (uint8_t param = DTM_TW_RESET_MIN_RANGE; param <= DTM_TW_RESET_MAX_RANGE; param++) {
		assert_tw_event(TW_SETUP(DTM_TW_TEST_SETUP_SET_UPPER, 0x0C),
				DTM_TW_EVENT_TEST_STATUS_SUCCESS);
		assert_tw_event(TW_SETUP(DTM_TW_TEST_SETUP_SET_PHY, DTM_TW_PHY_2M_MIN_RANGE),
				DTM_TW_EVENT_TEST_STATUS_SUCCESS);
		assert_tw_event(TW_SETUP(DTM_TW_TEST_SETUP_SELECT_MODULATION,
					 DTM_TW_MODULATION_INDEX_STABLE_MIN_RANGE),
				DTM_TW_EVENT_TEST_STATUS_SUCCESS);
		zassert_equal(setup_tx_power(4), TW_STATUS_TX_POWER(4, 0));

		assert_tx_test_cmd(TW_TX(1, 10, DTM_TW_PKT_PRBS9), 1, 0xC0 | 10,
				   BT_HCI_TEST_PKT_PAYLOAD_PRBS9, BT_HCI_LE_TX_PHY_2M, 4);
		assert_rx_test_cmd(TW_RX(1), 1, BT_HCI_LE_RX_PHY_2M, BT_HCI_LE_MOD_INDEX_STABLE);

		assert_tw_event(TW_SETUP(DTM_TW_TEST_SETUP_RESET, param),
				DTM_TW_EVENT_TEST_STATUS_SUCCESS);

		assert_tx_test_cmd(TW_TX(1, 10, DTM_TW_PKT_PRBS9), 1, 10,
				   BT_HCI_TEST_PKT_PAYLOAD_PRBS9, BT_HCI_LE_TX_PHY_1M, 0);
		assert_rx_test_cmd(TW_RX(1), 1, BT_HCI_LE_RX_PHY_1M, BT_HCI_LE_MOD_INDEX_STANDARD);
	}
}

ZTEST(dtm_twowire_to_hci, test_setup_reset_invalid_parameter)
{
	assert_tw_event(TW_SETUP(DTM_TW_TEST_SETUP_RESET, DTM_TW_RESET_MAX_RANGE + 1),
			DTM_TW_EVENT_TEST_STATUS_ERROR);
	assert_tw_event(TW_SETUP(DTM_TW_TEST_SETUP_RESET, 0xFF), DTM_TW_EVENT_TEST_STATUS_ERROR);
}

ZTEST(dtm_twowire_to_hci, test_setup_upper_length_bits)
{
	static const struct {
		uint8_t parameter;
		uint8_t upper_bits;
	} vectors[] = {
		{0x00, 0x00},
		{0x04, 0x40},
		{0x08, 0x80},
		{0x0C, 0xC0},
		/* Only bits 2 and 3 of the parameter carry the upper length bits */
		{0x03, 0x00},
		{0x0F, 0xC0},
	};

	ARRAY_FOR_EACH_PTR(vectors, v) {
		assert_tw_event(TW_SETUP(DTM_TW_TEST_SETUP_SET_UPPER, v->parameter),
				DTM_TW_EVENT_TEST_STATUS_SUCCESS);

		assert_tx_test_cmd(TW_TX(0, 0x00, DTM_TW_PKT_PRBS9), 0, v->upper_bits,
				   BT_HCI_TEST_PKT_PAYLOAD_PRBS9, BT_HCI_LE_TX_PHY_1M, 0);
		assert_tx_test_cmd(TW_TX(0, 0x3F, DTM_TW_PKT_PRBS9), 0, v->upper_bits | 0x3F,
				   BT_HCI_TEST_PKT_PAYLOAD_PRBS9, BT_HCI_LE_TX_PHY_1M, 0);
	}

	/* An out of range parameter is rejected and does not affect the length */
	assert_tw_event(TW_SETUP(DTM_TW_TEST_SETUP_SET_UPPER, DTM_TW_SET_UPPER_BITS_MAX_RANGE + 1),
			DTM_TW_EVENT_TEST_STATUS_ERROR);
	assert_tx_test_cmd(TW_TX(0, 0x3F, DTM_TW_PKT_PRBS9), 0, 0xFF, BT_HCI_TEST_PKT_PAYLOAD_PRBS9,
			   BT_HCI_LE_TX_PHY_1M, 0);
}

ZTEST(dtm_twowire_to_hci, test_setup_phy)
{
	static const struct {
		uint8_t parameter;
		uint8_t tx_phy;
		uint8_t rx_phy;
	} vectors[] = {
		{DTM_TW_PHY_1M_MIN_RANGE, BT_HCI_LE_TX_PHY_1M, BT_HCI_LE_RX_PHY_1M},
		{DTM_TW_PHY_1M_MAX_RANGE, BT_HCI_LE_TX_PHY_1M, BT_HCI_LE_RX_PHY_1M},
		{DTM_TW_PHY_2M_MIN_RANGE, BT_HCI_LE_TX_PHY_2M, BT_HCI_LE_RX_PHY_2M},
		{DTM_TW_PHY_2M_MAX_RANGE, BT_HCI_LE_TX_PHY_2M, BT_HCI_LE_RX_PHY_2M},
		{DTM_TW_PHY_LE_CODED_S8_MIN_RANGE, BT_HCI_LE_TX_PHY_CODED_S8,
		 BT_HCI_LE_RX_PHY_CODED},
		{DTM_TW_PHY_LE_CODED_S8_MAX_RANGE, BT_HCI_LE_TX_PHY_CODED_S8,
		 BT_HCI_LE_RX_PHY_CODED},
		{DTM_TW_PHY_LE_CODED_S2_MIN_RANGE, BT_HCI_LE_TX_PHY_CODED_S2,
		 BT_HCI_LE_RX_PHY_CODED},
		{DTM_TW_PHY_LE_CODED_S2_MAX_RANGE, BT_HCI_LE_TX_PHY_CODED_S2,
		 BT_HCI_LE_RX_PHY_CODED},
	};
	static const uint8_t invalid_parameters[] = {
		0x00,
		DTM_TW_PHY_1M_MIN_RANGE - 1,
		DTM_TW_PHY_LE_CODED_S2_MAX_RANGE + 1,
		0xFF,
	};

	ARRAY_FOR_EACH_PTR(vectors, v) {
		assert_tw_event(TW_SETUP(DTM_TW_TEST_SETUP_SET_PHY, v->parameter),
				DTM_TW_EVENT_TEST_STATUS_SUCCESS);

		assert_tx_test_cmd(TW_TX(0, 0, DTM_TW_PKT_PRBS9), 0, 0,
				   BT_HCI_TEST_PKT_PAYLOAD_PRBS9, v->tx_phy, 0);
		assert_rx_test_cmd(TW_RX(0), 0, v->rx_phy, BT_HCI_LE_MOD_INDEX_STANDARD);
	}

	/* An out of range parameter is rejected and does not affect the PHY */
	assert_tw_event(TW_SETUP(DTM_TW_TEST_SETUP_SET_PHY, DTM_TW_PHY_2M_MIN_RANGE),
			DTM_TW_EVENT_TEST_STATUS_SUCCESS);

	ARRAY_FOR_EACH_PTR(invalid_parameters, param) {
		assert_tw_event(TW_SETUP(DTM_TW_TEST_SETUP_SET_PHY, *param),
				DTM_TW_EVENT_TEST_STATUS_ERROR);
	}

	assert_tx_test_cmd(TW_TX(0, 0, DTM_TW_PKT_PRBS9), 0, 0, BT_HCI_TEST_PKT_PAYLOAD_PRBS9,
			   BT_HCI_LE_TX_PHY_2M, 0);
	assert_rx_test_cmd(TW_RX(0), 0, BT_HCI_LE_RX_PHY_2M, BT_HCI_LE_MOD_INDEX_STANDARD);
}

ZTEST(dtm_twowire_to_hci, test_setup_modulation_index)
{
	static const struct {
		uint8_t parameter;
		uint8_t mod_index;
	} vectors[] = {
		{DTM_TW_MODULATION_INDEX_STABLE_MIN_RANGE, BT_HCI_LE_MOD_INDEX_STABLE},
		{DTM_TW_MODULATION_INDEX_STANDARD_MIN_RANGE, BT_HCI_LE_MOD_INDEX_STANDARD},
		{DTM_TW_MODULATION_INDEX_STABLE_MAX_RANGE, BT_HCI_LE_MOD_INDEX_STABLE},
		{DTM_TW_MODULATION_INDEX_STANDARD_MAX_RANGE, BT_HCI_LE_MOD_INDEX_STANDARD},
	};

	ARRAY_FOR_EACH_PTR(vectors, v) {
		assert_tw_event(TW_SETUP(DTM_TW_TEST_SETUP_SELECT_MODULATION, v->parameter),
				DTM_TW_EVENT_TEST_STATUS_SUCCESS);

		assert_rx_test_cmd(TW_RX(0), 0, BT_HCI_LE_RX_PHY_1M, v->mod_index);
	}

	/* An out of range parameter is rejected and does not affect the modulation index */
	assert_tw_event(TW_SETUP(DTM_TW_TEST_SETUP_SELECT_MODULATION,
				 DTM_TW_MODULATION_INDEX_STABLE_MIN_RANGE),
			DTM_TW_EVENT_TEST_STATUS_SUCCESS);
	assert_tw_event(TW_SETUP(DTM_TW_TEST_SETUP_SELECT_MODULATION,
				 DTM_TW_MODULATION_INDEX_STABLE_MAX_RANGE + 1),
			DTM_TW_EVENT_TEST_STATUS_ERROR);
	assert_rx_test_cmd(TW_RX(0), 0, BT_HCI_LE_RX_PHY_1M, BT_HCI_LE_MOD_INDEX_STABLE);
}

ZTEST(dtm_twowire_to_hci, test_setup_read_supported_features_cmd)
{
	for (uint8_t param = DTM_TW_FEATURE_READ_MIN_RANGE; param <= DTM_TW_FEATURE_READ_MAX_RANGE;
	     param++) {
		tw_cmd_to_hci_cmd(TW_SETUP(DTM_TW_TEST_SETUP_READ_SUPPORTED, param),
				  BT_HCI_OP_LE_READ_ALL_LOCAL_SUPPORTED_FEATURES, 0);
	}

	assert_tw_event(
		TW_SETUP(DTM_TW_TEST_SETUP_READ_SUPPORTED, DTM_TW_FEATURE_READ_MAX_RANGE + 1),
		DTM_TW_EVENT_TEST_STATUS_ERROR);
}

ZTEST(dtm_twowire_to_hci, test_setup_read_supported_features_event)
{
	static const struct {
		uint8_t hci_feature_bit;
		uint16_t tw_feature;
	} vectors[] = {
		{BT_LE_FEAT_BIT_DLE, DTM_TW_TEST_SETUP_DLE_SUPPORTED},
		{BT_LE_FEAT_BIT_PHY_2M, DTM_TW_TEST_SETUP_2M_PHY_SUPPORTED},
		{BT_LE_FEAT_BIT_SMI_TX, DTM_TW_TEST_SETUP_STABLE_MODULATION_SUPPORTED},
		{BT_LE_FEAT_BIT_PHY_CODED, DTM_TW_TEST_SETUP_CODED_PHY_SUPPORTED},
	};
	static struct bt_hci_rp_le_read_all_local_supported_features rp;
	const uint16_t tw_cmd = TW_SETUP(DTM_TW_TEST_SETUP_READ_SUPPORTED, 0);
	uint16_t all_tw_features = 0;

	memset(&rp, 0, sizeof(rp));
	rp.status = BT_HCI_ERR_SUCCESS;

	hci_evt_cmd_complete(BT_HCI_OP_LE_READ_ALL_LOCAL_SUPPORTED_FEATURES, &rp, sizeof(rp));
	zassert_equal(hci_evt_to_tw_event(tw_cmd), DTM_TW_EVENT_TEST_STATUS_SUCCESS);

	ARRAY_FOR_EACH_PTR(vectors, v) {
		memset(rp.features, 0, sizeof(rp.features));
		rp.features[v->hci_feature_bit / 8] = BIT(v->hci_feature_bit % 8);

		hci_evt_cmd_complete(BT_HCI_OP_LE_READ_ALL_LOCAL_SUPPORTED_FEATURES, &rp,
				     sizeof(rp));
		zassert_equal(hci_evt_to_tw_event(tw_cmd),
			      DTM_TW_EVENT_TEST_STATUS_SUCCESS | v->tw_feature,
			      "HCI feature bit %d", v->hci_feature_bit);

		all_tw_features |= v->tw_feature;
	}

	/* Features that are irrelevant to DTM, or that the library does not support (CTE and
	 * antenna switching), are not reported.
	 */
	memset(rp.features, 0xFF, sizeof(rp.features));
	hci_evt_cmd_complete(BT_HCI_OP_LE_READ_ALL_LOCAL_SUPPORTED_FEATURES, &rp, sizeof(rp));
	zassert_equal(hci_evt_to_tw_event(tw_cmd),
		      DTM_TW_EVENT_TEST_STATUS_SUCCESS | all_tw_features);

	rp.status = BT_HCI_ERR_UNKNOWN_CMD;
	hci_evt_cmd_complete(BT_HCI_OP_LE_READ_ALL_LOCAL_SUPPORTED_FEATURES, &rp, sizeof(rp));
	zassert_equal(hci_evt_to_tw_event(tw_cmd), DTM_TW_EVENT_TEST_STATUS_ERROR);
}

ZTEST(dtm_twowire_to_hci, test_setup_read_max_data_length)
{
	const struct bt_hci_rp_le_read_max_data_len rp = {
		.status = BT_HCI_ERR_SUCCESS,
		.max_tx_octets = sys_cpu_to_le16(251),
		.max_tx_time = sys_cpu_to_le16(17040),
		.max_rx_octets = sys_cpu_to_le16(200),
		.max_rx_time = sys_cpu_to_le16(2120),
	};
	struct bt_hci_rp_le_read_max_data_len failed_rp = rp;
	/* The 2-wire event reports octets as is and times in units of 2 microseconds */
	static const struct {
		uint8_t parameter;
		uint16_t response;
	} vectors[] = {
		{DTM_TW_SUPPORTED_TX_OCTETS_MIN_RANGE, 251},
		{DTM_TW_SUPPORTED_TX_OCTETS_MAX_RANGE, 251},
		{DTM_TW_SUPPORTED_TX_TIME_MIN_RANGE, 8520},
		{DTM_TW_SUPPORTED_TX_TIME_MAX_RANGE, 8520},
		{DTM_TW_SUPPORTED_RX_OCTETS_MIN_RANGE, 200},
		{DTM_TW_SUPPORTED_RX_OCTETS_MAX_RANGE, 200},
		{DTM_TW_SUPPORTED_RX_TIME_MIN_RANGE, 1060},
		{DTM_TW_SUPPORTED_RX_TIME_MAX_RANGE, 1060},
	};

	ARRAY_FOR_EACH_PTR(vectors, v) {
		const uint16_t tw_cmd = TW_SETUP(DTM_TW_TEST_SETUP_READ_MAX, v->parameter);

		tw_cmd_to_hci_cmd(tw_cmd, BT_HCI_OP_LE_READ_MAX_DATA_LEN, 0);

		hci_evt_cmd_complete(BT_HCI_OP_LE_READ_MAX_DATA_LEN, &rp, sizeof(rp));
		zassert_equal(hci_evt_to_tw_event(tw_cmd), TW_STATUS_RESPONSE(v->response),
			      "Parameter 0x%02X", v->parameter);
	}

	failed_rp.status = BT_HCI_ERR_UNKNOWN_CMD;
	hci_evt_cmd_complete(BT_HCI_OP_LE_READ_MAX_DATA_LEN, &failed_rp, sizeof(failed_rp));
	zassert_equal(hci_evt_to_tw_event(TW_SETUP(DTM_TW_TEST_SETUP_READ_MAX,
						   DTM_TW_SUPPORTED_TX_OCTETS_MIN_RANGE)),
		      DTM_TW_EVENT_TEST_STATUS_ERROR);
}

ZTEST(dtm_twowire_to_hci, test_setup_read_max_cte_length)
{
	const uint16_t tw_cmd = TW_SETUP(DTM_TW_TEST_SETUP_READ_MAX, DTM_TW_SUPPORTED_CTE_LENGTH);
	struct bt_hci_rp_le_read_ant_info rp = {
		.status = BT_HCI_ERR_SUCCESS,
		.switch_sample_rates = 0x07,
		.num_ant = 12,
		.max_switch_pattern_len = 38,
		.max_cte_len = 20,
	};

	tw_cmd_to_hci_cmd(tw_cmd, BT_HCI_OP_LE_READ_ANT_INFO, 0);

	hci_evt_cmd_complete(BT_HCI_OP_LE_READ_ANT_INFO, &rp, sizeof(rp));
	zassert_equal(hci_evt_to_tw_event(tw_cmd), TW_STATUS_RESPONSE(20));

	rp.status = BT_HCI_ERR_UNKNOWN_CMD;
	hci_evt_cmd_complete(BT_HCI_OP_LE_READ_ANT_INFO, &rp, sizeof(rp));
	zassert_equal(hci_evt_to_tw_event(tw_cmd), DTM_TW_EVENT_TEST_STATUS_ERROR);
}

ZTEST(dtm_twowire_to_hci, test_setup_read_max_invalid_parameter)
{
	assert_tw_event(TW_SETUP(DTM_TW_TEST_SETUP_READ_MAX, DTM_TW_SUPPORTED_CTE_LENGTH + 1),
			DTM_TW_EVENT_TEST_STATUS_ERROR);
	assert_tw_event(TW_SETUP(DTM_TW_TEST_SETUP_READ_MAX, 0xFF), DTM_TW_EVENT_TEST_STATUS_ERROR);
}

ZTEST(dtm_twowire_to_hci, test_setup_tx_power)
{
	static const struct {
		uint8_t parameter;
		int8_t tx_power;
		uint16_t flags;
	} vectors[] = {
		{0, 0, 0},
		{(uint8_t)-20, -20, 0},
		{(uint8_t)(CTLR_TX_POWER_MIN - 1), CTLR_TX_POWER_MIN,
		 DTM_TW_TRANSMIT_POWER_MIN_LVL_BIT},
		{(uint8_t)DTM_TW_TRANSMIT_POWER_LVL_MIN, CTLR_TX_POWER_MIN,
		 DTM_TW_TRANSMIT_POWER_MIN_LVL_BIT},
		{DTM_TW_TRANSMIT_POWER_LVL_SET_MIN, CTLR_TX_POWER_MIN,
		 DTM_TW_TRANSMIT_POWER_MIN_LVL_BIT},
		{CTLR_TX_POWER_MAX + 1, CTLR_TX_POWER_MAX, DTM_TW_TRANSMIT_POWER_MAX_LVL_BIT},
		{DTM_TW_TRANSMIT_POWER_LVL_MAX, CTLR_TX_POWER_MAX,
		 DTM_TW_TRANSMIT_POWER_MAX_LVL_BIT},
		{DTM_TW_TRANSMIT_POWER_LVL_SET_MAX, CTLR_TX_POWER_MAX,
		 DTM_TW_TRANSMIT_POWER_MAX_LVL_BIT},
	};

	ARRAY_FOR_EACH_PTR(vectors, v) {
		zassert_equal(setup_tx_power(v->parameter),
			      TW_STATUS_TX_POWER(v->tx_power, v->flags), "Parameter 0x%02X",
			      v->parameter);

		/* The transmit power accepted by the controller is used for the next test */
		assert_tx_test_cmd(TW_TX(0, 0, DTM_TW_PKT_PRBS9), 0, 0,
				   BT_HCI_TEST_PKT_PAYLOAD_PRBS9, BT_HCI_LE_TX_PHY_1M, v->tx_power);
	}
}

ZTEST(dtm_twowire_to_hci, test_setup_tx_power_read_failed)
{
	const uint16_t tw_cmd = TW_SETUP(DTM_TW_TEST_SETUP_TRANSMIT_POWER, 0);
	const struct bt_hci_rp_le_read_tx_power rp = {
		.status = BT_HCI_ERR_UNKNOWN_CMD,
	};

	tw_cmd_to_hci_cmd(tw_cmd, BT_HCI_OP_LE_READ_TX_POWER, 0);

	hci_evt_cmd_complete(BT_HCI_OP_LE_READ_TX_POWER, &rp, sizeof(rp));
	zassert_equal(hci_evt_to_tw_event(tw_cmd), DTM_TW_EVENT_TEST_STATUS_ERROR);
}

ZTEST(dtm_twowire_to_hci, test_setup_cte_not_supported)
{
	assert_tw_event(TW_SETUP(DTM_TW_TEST_SETUP_CONSTANT_TONE_EXTENSION, 20),
			DTM_TW_EVENT_TEST_STATUS_ERROR);
	assert_tw_event(
		TW_SETUP(DTM_TW_TEST_SETUP_CONSTANT_TONE_EXTENSION_SLOT, DTM_TW_CTE_SLOT_1US),
		DTM_TW_EVENT_TEST_STATUS_ERROR);
	assert_tw_event(TW_SETUP(DTM_TW_TEST_SETUP_ANTENNA_ARRAY, DTM_TW_ANTENNA_NUMBER_MIN),
			DTM_TW_EVENT_TEST_STATUS_ERROR);
}

ZTEST(dtm_twowire_to_hci, test_setup_unknown_control)
{
	assert_tw_event(TW_SETUP(DTM_TW_TEST_SETUP_TRANSMIT_POWER + 1, 0),
			DTM_TW_EVENT_TEST_STATUS_ERROR);
	assert_tw_event(TW_SETUP(0x3F, 0), DTM_TW_EVENT_TEST_STATUS_ERROR);
}

ZTEST(dtm_twowire_to_hci, test_rx)
{
	static const uint8_t channels[] = {
		DTM_TW_CHANNEL_MIN_RANGE,
		19,
		DTM_TW_CHANNEL_MAX_RANGE,
	};

	ARRAY_FOR_EACH_PTR(channels, channel) {
		assert_rx_test_cmd(TW_RX(*channel), *channel, BT_HCI_LE_RX_PHY_1M,
				   BT_HCI_LE_MOD_INDEX_STANDARD);
	}

	assert_tw_event(TW_RX(DTM_TW_CHANNEL_MAX_RANGE + 1), DTM_TW_EVENT_TEST_STATUS_ERROR);
	assert_tw_event(TW_RX(0x3F), DTM_TW_EVENT_TEST_STATUS_ERROR);
}

ZTEST(dtm_twowire_to_hci, test_tx)
{
	static const struct {
		uint8_t channel;
		uint8_t length;
		enum dtm_tw_pkt_type pkt_type;
		uint8_t hci_payload;
	} vectors[] = {
		{DTM_TW_CHANNEL_MIN_RANGE, 0, DTM_TW_PKT_PRBS9, BT_HCI_TEST_PKT_PAYLOAD_PRBS9},
		{19, 37, DTM_TW_PKT_0X0F, BT_HCI_TEST_PKT_PAYLOAD_11110000},
		{DTM_TW_CHANNEL_MAX_RANGE, 63, DTM_TW_PKT_0X55, BT_HCI_TEST_PKT_PAYLOAD_10101010},
	};

	ARRAY_FOR_EACH_PTR(vectors, v) {
		assert_tx_test_cmd(TW_TX(v->channel, v->length, v->pkt_type), v->channel, v->length,
				   v->hci_payload, BT_HCI_LE_TX_PHY_1M, 0);
	}

	assert_tw_event(TW_TX(DTM_TW_CHANNEL_MAX_RANGE + 1, 37, DTM_TW_PKT_PRBS9),
			DTM_TW_EVENT_TEST_STATUS_ERROR);
	assert_tw_event(TW_TX(0x3F, 37, DTM_TW_PKT_PRBS9), DTM_TW_EVENT_TEST_STATUS_ERROR);
}

ZTEST(dtm_twowire_to_hci, test_tx_0xff_payload_on_coded_phy)
{
	static const struct {
		uint8_t parameter;
		uint8_t tx_phy;
	} vectors[] = {
		{DTM_TW_PHY_LE_CODED_S8_MIN_RANGE, BT_HCI_LE_TX_PHY_CODED_S8},
		{DTM_TW_PHY_LE_CODED_S2_MIN_RANGE, BT_HCI_LE_TX_PHY_CODED_S2},
	};

	ARRAY_FOR_EACH_PTR(vectors, v) {
		assert_tw_event(TW_SETUP(DTM_TW_TEST_SETUP_SET_PHY, v->parameter),
				DTM_TW_EVENT_TEST_STATUS_SUCCESS);

		assert_tx_test_cmd(TW_TX(5, 37, DTM_TW_PKT_0XFF_OR_VS), 5, 37,
				   BT_HCI_TEST_PKT_PAYLOAD_11111111, v->tx_phy, 0);

		assert_tw_event(TW_TX(DTM_TW_CHANNEL_MAX_RANGE + 1, 37, DTM_TW_PKT_0XFF_OR_VS),
				DTM_TW_EVENT_TEST_STATUS_ERROR);
	}
}

#if defined(CONFIG_DTM_TWOWIRE_TO_HCI_SDC_VS_COMMANDS)
ZTEST(dtm_twowire_to_hci, test_tx_vendor_specific_carrier_test)
{
	static const uint8_t phy_parameters[] = {
		DTM_TW_PHY_1M_MIN_RANGE,
		DTM_TW_PHY_2M_MIN_RANGE,
	};
	const uint8_t carrier_test_subcmd = 0;
	const uint8_t expected[] = {
		SDC_HCI_VS_DTM_COMMAND_OPCODE_TRANSMITTER_CARRIER_TEST,
		DTM_TW_CHANNEL_MAX_RANGE,
		(uint8_t)-20,
	};

	zassert_equal(setup_tx_power((uint8_t)-20), TW_STATUS_TX_POWER(-20, 0));

	ARRAY_FOR_EACH_PTR(phy_parameters, param) {
		const uint8_t *params;

		assert_tw_event(TW_SETUP(DTM_TW_TEST_SETUP_SET_PHY, *param),
				DTM_TW_EVENT_TEST_STATUS_SUCCESS);

		params = tw_cmd_to_hci_cmd(
			TW_TX(DTM_TW_CHANNEL_MAX_RANGE, carrier_test_subcmd, DTM_TW_PKT_0XFF_OR_VS),
			SDC_HCI_OPCODE_CMD_VS_DTM_COMMAND, sizeof(expected));
		zassert_mem_equal(params, expected, sizeof(expected));

		assert_tw_event(TW_TX(DTM_TW_CHANNEL_MAX_RANGE + 1, carrier_test_subcmd,
				      DTM_TW_PKT_0XFF_OR_VS),
				DTM_TW_EVENT_TEST_STATUS_ERROR);
	}
}

ZTEST(dtm_twowire_to_hci, test_tx_vendor_specific_unknown_subcmd)
{
	assert_tw_event(TW_TX(0, 1, DTM_TW_PKT_0XFF_OR_VS), DTM_TW_EVENT_TEST_STATUS_ERROR);
	assert_tw_event(TW_TX(0, 0x3F, DTM_TW_PKT_0XFF_OR_VS), DTM_TW_EVENT_TEST_STATUS_ERROR);
}
#else
ZTEST(dtm_twowire_to_hci, test_tx_vendor_specific_not_supported)
{
	static const uint8_t phy_parameters[] = {
		DTM_TW_PHY_1M_MIN_RANGE,
		DTM_TW_PHY_2M_MIN_RANGE,
	};

	ARRAY_FOR_EACH_PTR(phy_parameters, param) {
		assert_tw_event(TW_SETUP(DTM_TW_TEST_SETUP_SET_PHY, *param),
				DTM_TW_EVENT_TEST_STATUS_SUCCESS);

		assert_tw_event(TW_TX(0, 0, DTM_TW_PKT_0XFF_OR_VS), DTM_TW_EVENT_TEST_STATUS_ERROR);
	}
}
#endif /* CONFIG_DTM_TWOWIRE_TO_HCI_SDC_VS_COMMANDS */

ZTEST(dtm_twowire_to_hci, test_end_cmd)
{
	for (uint8_t param = DTM_TW_TEST_END_MIN_RANGE; param <= DTM_TW_TEST_END_MAX_RANGE;
	     param++) {
		tw_cmd_to_hci_cmd(TW_END(0, param), BT_HCI_OP_LE_TEST_END, 0);
	}

	assert_tw_event(TW_END(0, DTM_TW_TEST_END_MAX_RANGE + 1), DTM_TW_EVENT_TEST_STATUS_ERROR);
	assert_tw_event(TW_END(1, DTM_TW_TEST_END_MIN_RANGE), DTM_TW_EVENT_TEST_STATUS_ERROR);
}

ZTEST(dtm_twowire_to_hci, test_end_event)
{
	static const uint16_t packet_counts[] = {0, 1500, DTM_TW_PACKET_COUNT_MASK};
	const uint16_t tw_cmd = TW_END(0, DTM_TW_TEST_END_MIN_RANGE);
	struct bt_hci_rp_le_test_end rp = {
		.status = BT_HCI_ERR_SUCCESS,
	};

	ARRAY_FOR_EACH_PTR(packet_counts, count) {
		rp.rx_pkt_count = sys_cpu_to_le16(*count);

		hci_evt_cmd_complete(BT_HCI_OP_LE_TEST_END, &rp, sizeof(rp));
		zassert_equal(hci_evt_to_tw_event(tw_cmd), DTM_TW_EVENT_PACKET_REPORTING | *count,
			      "Packet count %d", *count);
	}

	rp.status = BT_HCI_ERR_CMD_DISALLOWED;
	hci_evt_cmd_complete(BT_HCI_OP_LE_TEST_END, &rp, sizeof(rp));
	zassert_equal(hci_evt_to_tw_event(tw_cmd), DTM_TW_EVENT_TEST_STATUS_ERROR);
}

ZTEST(dtm_twowire_to_hci, test_test_start_event)
{
	static const uint16_t opcodes[] = {
		BT_HCI_OP_LE_RX_TEST,
		BT_HCI_OP_LE_ENH_RX_TEST,
		BT_HCI_OP_LE_RX_TEST_V3,
		BT_HCI_OP_LE_TX_TEST,
		BT_HCI_OP_LE_ENH_TX_TEST,
		BT_HCI_OP_LE_TX_TEST_V3,
		BT_HCI_OP_LE_TX_TEST_V4,
#if defined(CONFIG_DTM_TWOWIRE_TO_HCI_SDC_VS_COMMANDS)
		SDC_HCI_OPCODE_CMD_VS_DTM_COMMAND,
#endif /* CONFIG_DTM_TWOWIRE_TO_HCI_SDC_VS_COMMANDS */
	};
	const uint16_t tw_cmd = TW_TX(0, 37, DTM_TW_PKT_PRBS9);

	ARRAY_FOR_EACH_PTR(opcodes, opcode) {
		hci_evt_cmd_complete_status(*opcode, BT_HCI_ERR_SUCCESS);
		zassert_equal(hci_evt_to_tw_event(tw_cmd), DTM_TW_EVENT_TEST_STATUS_SUCCESS,
			      "Opcode 0x%04X", *opcode);

		hci_evt_cmd_complete_status(*opcode, BT_HCI_ERR_INVALID_PARAM);
		zassert_equal(hci_evt_to_tw_event(tw_cmd), DTM_TW_EVENT_TEST_STATUS_ERROR,
			      "Opcode 0x%04X", *opcode);
	}
}

ZTEST(dtm_twowire_to_hci, test_command_status_event)
{
	hci_evt_cmd_status(BT_HCI_OP_LE_TX_TEST_V4, BT_HCI_ERR_UNKNOWN_CMD);

	zassert_equal(hci_evt_to_tw_event(TW_TX(0, 37, DTM_TW_PKT_PRBS9)),
		      DTM_TW_EVENT_TEST_STATUS_ERROR);
}

ZTEST(dtm_twowire_to_hci, test_unhandled_hci_packets)
{
	/* A packet that is not an HCI event */
	hci_evt_cmd_complete_status(BT_HCI_OP_LE_TX_TEST, BT_HCI_ERR_SUCCESS);
	hci_evt->data[H4_PKT_INDICATOR_OFFSET] = BT_HCI_H4_ACL;
	assert_hci_evt_unhandled();

	/* An HCI event that is neither Command Complete nor Command Status */
	hci_evt_cmd_complete_status(BT_HCI_OP_LE_TX_TEST, BT_HCI_ERR_SUCCESS);
	hci_evt->data[H4_EVT_CODE_OFFSET] = BT_HCI_EVT_LE_META_EVENT;
	assert_hci_evt_unhandled();

	/* A Command Complete event of a command that is not generated by the library */
	hci_evt_cmd_complete_status(BT_HCI_OP_RESET, BT_HCI_ERR_SUCCESS);
	assert_hci_evt_unhandled();
}
