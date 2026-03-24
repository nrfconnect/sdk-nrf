/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <assert.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/hci_types.h>

#include <bluetooth/dtm_twowire/dtm_twowire_types.h>
#include <bluetooth/dtm_twowire/dtm_twowire_to_hci.h>

LOG_MODULE_REGISTER(dtm_tw_to_hci, CONFIG_DTM_TWOWIRE_TO_HCI_LOG_LEVEL);

/* Return value for internal functions that return 2-wire events to signal
 * that an HCI command was generated instead.
 */
#define HCI_GENERATED 0xFFFF

/* HCI H4 command offsets */
#define HCI_H4_PKT_INDICATOR_OFFSET 0
#define HCI_H4_PKT_OFFSET 1

/* Helper macros to parse DTM 2-wire commands */
#define TW_CMD_CODE(tw_cmd) ((tw_cmd >> 14) & 0x03)
#define TW_CMD_CHANNEL(tw_cmd) ((tw_cmd >> 8) & 0x3F)
#define TW_CMD_LENGTH(tw_cmd) ((tw_cmd >> 2) & 0x3F)
#define TW_CMD_PKT_TYPE(tw_cmd) (tw_cmd & 0x03)
#define TW_CMD_CONTROL(tw_cmd) ((tw_cmd >> 8) & 0x3F)
#define TW_CMD_PARAMETER(tw_cmd) (tw_cmd & 0xFF)

/* Helper macros to check if a DTM feature is marked as supported in the
 * HCI_LE_Read_All_Local_Supported_Features command response
 */
#define FEATURE_DLE(features) \
	BT_FEAT_LE_DLE(features)
#define FEATURE_2M_PHY(features) \
	BT_FEAT_LE_PHY_2M(features)
#define FEATURE_TX_STABLE_MOD(features) \
	BT_LE_FEAT_TEST(features, BT_LE_FEAT_BIT_SMI_TX)
#define FEATURE_CODED_PHY(features) \
	BT_FEAT_LE_PHY_CODED(features)
/* TODO: DRGN-27910 add CTE and antenna switching support */
#define FEATURE_CTE(features) 0
#define FEATURE_ANT_SWITCHING(features) 0
#define FEATURE_AOD_1US_TX(features) 0
#define FEATURE_AOD_1US_RX(features) 0
#define FEATURE_AOA_1US_RX(features) 0

/* DTM parameters buffered from 2-wire Test Setup commands to be used in HCI commands */
struct {
	uint8_t test_data_length;   /**< Length of the payload for Transmitter Test HCI command */
	uint8_t tx_phy;             /**< PHY for Transmitter Test HCI command */
	uint8_t rx_phy;             /**< PHY for Receiver Test HCI command */
	uint8_t modulation_index;   /**< Modulation index for Receiver Test HCI command */
	int8_t transmit_power;      /**< Transmit power for Transmitter Test HCI command */
} dtm_hci_parameters;

static uint16_t reset_dtm(uint8_t parameter)
{
	if (parameter < DTM_TW_RESET_MIN_RANGE || parameter > DTM_TW_RESET_MAX_RANGE) {
		return DTM_TW_EVENT_TEST_STATUS_ERROR;
	}

	/* Reset DTM parameters to default values as specified in Core v6.2, Vol 6, Part F, 3.3.2 */
	memset(&dtm_hci_parameters, 0, sizeof(dtm_hci_parameters));
	dtm_hci_parameters.tx_phy = BT_HCI_LE_TX_PHY_1M;
	dtm_hci_parameters.modulation_index = BT_HCI_LE_MOD_INDEX_STANDARD;
	return DTM_TW_EVENT_TEST_STATUS_SUCCESS;
}

static uint16_t set_upper_length_bits(uint8_t parameter)
{
	if (parameter < DTM_TW_SET_UPPER_BITS_MIN_RANGE ||
	    parameter > DTM_TW_SET_UPPER_BITS_MAX_RANGE) {
		return DTM_TW_EVENT_TEST_STATUS_ERROR;
	}

	dtm_hci_parameters.test_data_length = (parameter << DTM_TW_UPPER_BITS_POS) &
					       DTM_TW_UPPER_BITS_MASK;
	return DTM_TW_EVENT_TEST_STATUS_SUCCESS;
}

static uint16_t set_phy(uint8_t parameter)
{
	if (parameter >= DTM_TW_PHY_1M_MIN_RANGE && parameter <= DTM_TW_PHY_1M_MAX_RANGE) {
		dtm_hci_parameters.tx_phy = BT_HCI_LE_TX_PHY_1M;
		dtm_hci_parameters.rx_phy = BT_HCI_LE_RX_PHY_1M;
	} else if (parameter >= DTM_TW_PHY_2M_MIN_RANGE && parameter <= DTM_TW_PHY_2M_MAX_RANGE) {
		dtm_hci_parameters.tx_phy = BT_HCI_LE_TX_PHY_2M;
		dtm_hci_parameters.rx_phy = BT_HCI_LE_RX_PHY_2M;
	} else if (parameter >= DTM_TW_PHY_LE_CODED_S8_MIN_RANGE &&
		   parameter <= DTM_TW_PHY_LE_CODED_S8_MAX_RANGE) {
		dtm_hci_parameters.tx_phy = BT_HCI_LE_TX_PHY_CODED_S8;
		dtm_hci_parameters.rx_phy = BT_HCI_LE_RX_PHY_CODED;
	} else if (parameter >= DTM_TW_PHY_LE_CODED_S2_MIN_RANGE &&
		   parameter <= DTM_TW_PHY_LE_CODED_S2_MAX_RANGE) {
		dtm_hci_parameters.tx_phy = BT_HCI_LE_TX_PHY_CODED_S2;
		dtm_hci_parameters.rx_phy = BT_HCI_LE_RX_PHY_CODED;
	} else {
		return DTM_TW_EVENT_TEST_STATUS_ERROR;
	}

	return DTM_TW_EVENT_TEST_STATUS_SUCCESS;
}

static uint16_t set_modulation_index(uint8_t parameter)
{
	if (parameter >= DTM_TW_MODULATION_INDEX_STANDARD_MIN_RANGE &&
	    parameter <= DTM_TW_MODULATION_INDEX_STANDARD_MAX_RANGE) {
		dtm_hci_parameters.modulation_index = BT_HCI_LE_MOD_INDEX_STANDARD;
	} else if (parameter >= DTM_TW_MODULATION_INDEX_STABLE_MIN_RANGE &&
		   parameter <= DTM_TW_MODULATION_INDEX_STABLE_MAX_RANGE) {
		dtm_hci_parameters.modulation_index = BT_HCI_LE_MOD_INDEX_STABLE;
	} else {
		return DTM_TW_EVENT_TEST_STATUS_ERROR;
	}

	return DTM_TW_EVENT_TEST_STATUS_SUCCESS;
}

static uint16_t read_supported_features(uint8_t parameter, struct net_buf *hci_cmd)
{
	if (parameter < DTM_TW_FEATURE_READ_MIN_RANGE ||
	    parameter > DTM_TW_FEATURE_READ_MAX_RANGE) {
		return DTM_TW_EVENT_TEST_STATUS_ERROR;
	}

	/* Generate HCI command to read local features */
	struct bt_hci_cmd_hdr *cmd_hdr = net_buf_add(hci_cmd, BT_HCI_CMD_HDR_SIZE);

	cmd_hdr->opcode = BT_HCI_OP_LE_READ_ALL_LOCAL_SUPPORTED_FEATURES;
	cmd_hdr->param_len = 0;
	return HCI_GENERATED;
}

static uint16_t read_max_value(uint8_t parameter, struct net_buf *hci_cmd)
{
	struct bt_hci_cmd_hdr *cmd_hdr = net_buf_add(hci_cmd, BT_HCI_CMD_HDR_SIZE);

	if (parameter >= DTM_TW_SUPPORTED_TX_OCTETS_MIN_RANGE &&
	    parameter <= DTM_TW_SUPPORTED_RX_TIME_MAX_RANGE) {
		/* This range covers TX Octets, TX Time, RX Octets, and RX Time,
		 * all of which are read by the same HCI command
		 */
		cmd_hdr->opcode = BT_HCI_OP_LE_READ_MAX_DATA_LEN;
		cmd_hdr->param_len = 0;
	} else if (parameter == DTM_TW_SUPPORTED_CTE_LENGTH) {
		cmd_hdr->opcode = BT_HCI_OP_LE_READ_ANT_INFO;
		cmd_hdr->param_len = 0;
	} else {
		return DTM_TW_EVENT_TEST_STATUS_ERROR;
	}

	return HCI_GENERATED;
}

static uint16_t set_cte(uint8_t parameter)
{
	/* TODO: DRGN-27910 add CTE and antenna switching support */
	ARG_UNUSED(parameter);
	return DTM_TW_EVENT_TEST_STATUS_ERROR;
}

static uint16_t set_cte_slot(uint8_t parameter)
{
	/* TODO: DRGN-27910 add CTE and antenna switching support */
	ARG_UNUSED(parameter);
	return DTM_TW_EVENT_TEST_STATUS_ERROR;
}

static uint16_t set_antenna_pattern(uint8_t parameter)
{
	/* TODO: DRGN-27910 add CTE and antenna switching support */
	ARG_UNUSED(parameter);
	return DTM_TW_EVENT_TEST_STATUS_ERROR;
}

static uint16_t set_tx_power(int8_t parameter, struct net_buf *hci_cmd)
{
	dtm_hci_parameters.transmit_power = parameter;

	/* Generate HCI command to read min/max TX power levels */
	struct bt_hci_cmd_hdr *cmd_hdr = net_buf_add(hci_cmd, BT_HCI_CMD_HDR_SIZE);

	cmd_hdr->opcode = BT_HCI_OP_LE_READ_TX_POWER;
	cmd_hdr->param_len = 0;
	return HCI_GENERATED;
}

static uint16_t on_test_setup_cmd(enum dtm_tw_ctrl_code control, uint8_t parameter,
				  struct net_buf *hci_cmd)
{
	switch (control) {
	case DTM_TW_TEST_SETUP_RESET:
		return reset_dtm(parameter);

	case DTM_TW_TEST_SETUP_SET_UPPER:
		return set_upper_length_bits(parameter);

	case DTM_TW_TEST_SETUP_SET_PHY:
		return set_phy(parameter);

	case DTM_TW_TEST_SETUP_SELECT_MODULATION:
		return set_modulation_index(parameter);

	case DTM_TW_TEST_SETUP_READ_SUPPORTED:
		return read_supported_features(parameter, hci_cmd);

	case DTM_TW_TEST_SETUP_READ_MAX:
		return read_max_value(parameter, hci_cmd);

	case DTM_TW_TEST_SETUP_CONSTANT_TONE_EXTENSION:
		return set_cte(parameter);

	case DTM_TW_TEST_SETUP_CONSTANT_TONE_EXTENSION_SLOT:
		return set_cte_slot(parameter);

	case DTM_TW_TEST_SETUP_ANTENNA_ARRAY:
		return set_antenna_pattern(parameter);

	case DTM_TW_TEST_SETUP_TRANSMIT_POWER:
		return set_tx_power(parameter, hci_cmd);

	default:
		return DTM_TW_EVENT_TEST_STATUS_ERROR;
	}
}

static uint16_t on_test_end_cmd(enum dtm_tw_ctrl_code control, uint8_t parameter,
				struct net_buf *hci_cmd)
{
	if (control) {
		return DTM_TW_EVENT_TEST_STATUS_ERROR;
	}

	if (parameter > DTM_TW_TEST_END_MAX_RANGE) {
		return DTM_TW_EVENT_TEST_STATUS_ERROR;
	}

	struct bt_hci_cmd_hdr *cmd_hdr = net_buf_add(hci_cmd, BT_HCI_CMD_HDR_SIZE);

	cmd_hdr->opcode = BT_HCI_OP_LE_TEST_END;
	cmd_hdr->param_len = 0;

	return HCI_GENERATED;
}

static int get_hci_param_channel(uint8_t channel_tw, uint8_t *channel_hci)
{
	if (channel_tw < DTM_TW_CHANNEL_MIN_RANGE || channel_tw > DTM_TW_CHANNEL_MAX_RANGE) {
		return -EINVAL;
	}

	*channel_hci = channel_tw;
	return 0;
}

static int get_hci_param_pkt_payload(enum dtm_tw_pkt_type pkt_type_tw, uint8_t *pkt_payload_hci)
{
	switch (pkt_type_tw) {
	case DTM_TW_PKT_PRBS9:
		*pkt_payload_hci = BT_HCI_TEST_PKT_PAYLOAD_PRBS9;
		break;

	case DTM_TW_PKT_0X0F:
		*pkt_payload_hci = BT_HCI_TEST_PKT_PAYLOAD_11110000;
		break;

	case DTM_TW_PKT_0X55:
		*pkt_payload_hci = BT_HCI_TEST_PKT_PAYLOAD_10101010;
		break;

	case DTM_TW_PKT_0XFF_OR_VS:
		if (dtm_hci_parameters.tx_phy == BT_HCI_LE_TX_PHY_CODED_S8 ||
		    dtm_hci_parameters.tx_phy == BT_HCI_LE_TX_PHY_CODED_S2) {
			*pkt_payload_hci = BT_HCI_TEST_PKT_PAYLOAD_11111111;
		} else {
			/* For non-Coded PHY, the 0xFF packet type is vendor specific.
			 * We do not support vendor specific packet types in this implementation.
			 */
			return -EINVAL;
		}
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static uint16_t on_test_rx_cmd(uint8_t channel, struct net_buf *hci_cmd)
{
	uint8_t hci_param_channel;

	if (get_hci_param_channel(channel, &hci_param_channel)) {
		return DTM_TW_EVENT_TEST_STATUS_ERROR;
	}

	struct bt_hci_cmd_hdr *cmd_hdr = net_buf_add(hci_cmd, BT_HCI_CMD_HDR_SIZE);

#if CONFIG_DTM_TWOWIRE_TO_HCI_RECEIVER_TEST_VERSION == 3
	cmd_hdr->opcode = BT_HCI_OP_LE_RX_TEST_V3;
	/* TODO: DRGN-27910 add CTE and antenna switching support */
	cmd_hdr->param_len = sizeof(struct bt_hci_cp_le_rx_test_v3);
	struct bt_hci_cp_le_rx_test_v3 *cp = net_buf_add(hci_cmd,
							 sizeof(struct bt_hci_cp_le_rx_test_v3));

#elif CONFIG_DTM_TWOWIRE_TO_HCI_RECEIVER_TEST_VERSION == 2
	cmd_hdr->opcode = BT_HCI_OP_LE_ENH_RX_TEST;
	cmd_hdr->param_len = sizeof(struct bt_hci_cp_le_enh_rx_test);
	struct bt_hci_cp_le_enh_rx_test *cp = net_buf_add(hci_cmd,
							  sizeof(struct bt_hci_cp_le_enh_rx_test));

#elif CONFIG_DTM_TWOWIRE_TO_HCI_RECEIVER_TEST_VERSION == 1
	cmd_hdr->opcode = BT_HCI_OP_LE_RX_TEST;
	cmd_hdr->param_len = sizeof(struct bt_hci_cp_le_rx_test);
	struct bt_hci_cp_le_rx_test *cp = net_buf_add(hci_cmd,
						      sizeof(struct bt_hci_cp_le_rx_test));

#else
	#error "Unsupported HCI_LE_Receiver_Test version selected"
#endif
#if CONFIG_DTM_TWOWIRE_TO_HCI_RECEIVER_TEST_VERSION >= 1
	cp->rx_ch = hci_param_channel;
#endif
#if CONFIG_DTM_TWOWIRE_TO_HCI_RECEIVER_TEST_VERSION >= 2
	cp->phy = dtm_hci_parameters.rx_phy;
	cp->mod_index = dtm_hci_parameters.modulation_index;
#endif
#if CONFIG_DTM_TWOWIRE_TO_HCI_RECEIVER_TEST_VERSION >= 3
	/* TODO: DRGN-27910 add CTE and antenna switching support */
	cp->expected_cte_len = 0;
	cp->expected_cte_type = 0;
	cp->switch_pattern_len = 0;
#endif

	return HCI_GENERATED;
}

static uint16_t on_test_tx_cmd(uint8_t channel, uint8_t length, enum dtm_tw_pkt_type type,
			       struct net_buf *hci_cmd)
{
	uint8_t hci_param_channel, hci_param_pkt_payload;

	if (get_hci_param_channel(channel, &hci_param_channel) ||
	    get_hci_param_pkt_payload(type, &hci_param_pkt_payload)) {
		return DTM_TW_EVENT_TEST_STATUS_ERROR;
	}

	uint8_t hci_param_length = (length & ~DTM_TW_UPPER_BITS_MASK) |
				   dtm_hci_parameters.test_data_length;

	struct bt_hci_cmd_hdr *cmd_hdr = net_buf_add(hci_cmd, BT_HCI_CMD_HDR_SIZE);

#if CONFIG_DTM_TWOWIRE_TO_HCI_TRANSMITTER_TEST_VERSION == 4
	cmd_hdr->opcode = BT_HCI_OP_LE_TX_TEST_V4;
	/* TODO: DRGN-27910 add CTE and antenna switching support */
	cmd_hdr->param_len = sizeof(struct bt_hci_cp_le_tx_test_v4) +
			     sizeof(struct bt_hci_cp_le_tx_test_v4_tx_power);
	struct bt_hci_cp_le_tx_test_v4 *cp = net_buf_add(hci_cmd,
							 sizeof(struct bt_hci_cp_le_tx_test_v4));

#elif CONFIG_DTM_TWOWIRE_TO_HCI_TRANSMITTER_TEST_VERSION == 3
	cmd_hdr->opcode = BT_HCI_OP_LE_TX_TEST_V3;
	/* TODO: DRGN-27910 add CTE and antenna switching support */
	cmd_hdr->param_len = sizeof(struct bt_hci_cp_le_tx_test_v3);
	struct bt_hci_cp_le_tx_test_v3 *cp = net_buf_add(hci_cmd,
							 sizeof(struct bt_hci_cp_le_tx_test_v3));

#elif CONFIG_DTM_TWOWIRE_TO_HCI_TRANSMITTER_TEST_VERSION == 2
	cmd_hdr->opcode = BT_HCI_OP_LE_ENH_TX_TEST;
	cmd_hdr->param_len = sizeof(struct bt_hci_cp_le_enh_tx_test);
	struct bt_hci_cp_le_enh_tx_test *cp = net_buf_add(hci_cmd,
							  sizeof(struct bt_hci_cp_le_enh_tx_test));

#elif CONFIG_DTM_TWOWIRE_TO_HCI_TRANSMITTER_TEST_VERSION == 1
	cmd_hdr->opcode = BT_HCI_OP_LE_TX_TEST;
	cmd_hdr->param_len = sizeof(struct bt_hci_cp_le_tx_test);
	struct bt_hci_cp_le_tx_test *cp = net_buf_add(hci_cmd,
						      sizeof(struct bt_hci_cp_le_tx_test));

#else
	#error "Unsupported HCI_LE_Transmitter_Test version selected"
#endif
#if CONFIG_DTM_TWOWIRE_TO_HCI_TRANSMITTER_TEST_VERSION >= 1
	cp->tx_ch = hci_param_channel;
	cp->test_data_len = hci_param_length;
	cp->pkt_payload = hci_param_pkt_payload;
#endif
#if CONFIG_DTM_TWOWIRE_TO_HCI_TRANSMITTER_TEST_VERSION >= 2
	cp->phy = dtm_hci_parameters.tx_phy;
#endif
#if CONFIG_DTM_TWOWIRE_TO_HCI_TRANSMITTER_TEST_VERSION >= 3
	/* TODO: DRGN-27910 add CTE and antenna switching support */
	cp->cte_len = 0;
	cp->cte_type = 0;
	cp->switch_pattern_len = 0;
#endif
#if CONFIG_DTM_TWOWIRE_TO_HCI_TRANSMITTER_TEST_VERSION >= 4
	struct bt_hci_cp_le_tx_test_v4_tx_power *cp_pwr = net_buf_add(
		hci_cmd,
		sizeof(struct bt_hci_cp_le_tx_test_v4_tx_power)
	);

	cp_pwr->tx_power = dtm_hci_parameters.transmit_power;
#endif

	return HCI_GENERATED;
}

static uint16_t on_cc_supported_features(
	const struct bt_hci_rp_le_read_all_local_supported_features *features_rp)
{
	if (features_rp->status != BT_HCI_ERR_SUCCESS) {
		LOG_ERR("Read Local Supported Features command failed with status 0x%02X",
			features_rp->status);
		return DTM_TW_EVENT_TEST_STATUS_ERROR;
	}

	uint16_t event = DTM_TW_EVENT_TEST_STATUS_SUCCESS;

	event |= (FEATURE_DLE(features_rp->features) ? DTM_TW_TEST_SETUP_DLE_SUPPORTED : 0);
	event |= (FEATURE_2M_PHY(features_rp->features) ? DTM_TW_TEST_SETUP_2M_PHY_SUPPORTED : 0);
	event |= (FEATURE_TX_STABLE_MOD(features_rp->features) ?
		  DTM_TW_TEST_SETUP_STABLE_MODULATION_SUPPORTED : 0);
	event |= (FEATURE_CODED_PHY(features_rp->features) ?
		  DTM_TW_TEST_SETUP_CODED_PHY_SUPPORTED : 0);
	event |= (FEATURE_CTE(features_rp->features) ? DTM_TW_TEST_SETUP_CTE_SUPPORTED : 0);
	event |= (FEATURE_ANT_SWITCHING(features_rp->features) ?
		  DTM_TW_TEST_SETUP_ANTENNA_SWITCH : 0);
	event |= (FEATURE_AOD_1US_TX(features_rp->features) ? DTM_TW_TEST_SETUP_AOD_1US_TX : 0);
	event |= (FEATURE_AOD_1US_RX(features_rp->features) ? DTM_TW_TEST_SETUP_AOD_1US_RX : 0);
	event |= (FEATURE_AOA_1US_RX(features_rp->features) ? DTM_TW_TEST_SETUP_AOA_1US_RX : 0);
	return event;
}

static uint16_t on_cc_read_max(const struct bt_hci_rp_le_read_max_data_len *max_data_len_rp,
			       const uint16_t tw_cmd)
{
	if (max_data_len_rp->status != BT_HCI_ERR_SUCCESS) {
		LOG_ERR("Read Max Data Length command failed with status 0x%02X",
			max_data_len_rp->status);
		return DTM_TW_EVENT_TEST_STATUS_ERROR;
	}
	/* The max data length event encodes both RX and TX octets and time, so we need to check
	 * which parameter the 2-wire command was querying for and return the appropriate
	 * value.
	 */
	uint16_t ret;
	uint8_t parameter = TW_CMD_PARAMETER(tw_cmd);

	if (parameter >= DTM_TW_SUPPORTED_TX_OCTETS_MIN_RANGE &&
	    parameter <= DTM_TW_SUPPORTED_TX_OCTETS_MAX_RANGE) {
		ret = max_data_len_rp->max_tx_octets;
	} else if (parameter >= DTM_TW_SUPPORTED_TX_TIME_MIN_RANGE &&
		   parameter <= DTM_TW_SUPPORTED_TX_TIME_MAX_RANGE) {
		/* The HCI command returns the max TX time in microseconds,
		 * while the 2-wire event returns the max TX time in microseconds divided by 2.
		 */
		ret = max_data_len_rp->max_tx_time >> 1;
	} else if (parameter >= DTM_TW_SUPPORTED_RX_OCTETS_MIN_RANGE &&
		   parameter <= DTM_TW_SUPPORTED_RX_OCTETS_MAX_RANGE) {
		ret = max_data_len_rp->max_rx_octets;
	} else if (parameter >= DTM_TW_SUPPORTED_RX_TIME_MIN_RANGE &&
		   parameter <= DTM_TW_SUPPORTED_RX_TIME_MAX_RANGE) {
		/* The HCI command returns the max RX time in microseconds,
		 * while the 2-wire event returns the max RX time in microseconds divided by 2.
		 */
		ret = max_data_len_rp->max_rx_time >> 1;
	} else {
		return DTM_TW_EVENT_TEST_STATUS_ERROR;
	}

	return DTM_TW_EVENT_TEST_STATUS_SUCCESS |
	       ((ret << DTM_TW_EVENT_RESPONSE_POS) & DTM_TW_EVENT_RESPONSE_MASK);
}

static uint16_t on_cc_read_ant_info(const struct bt_hci_rp_le_read_ant_info *ant_info_rp)
{
	if (ant_info_rp->status != BT_HCI_ERR_SUCCESS) {
		LOG_ERR("Read Antenna Information command failed with status 0x%02X",
			ant_info_rp->status);
		return DTM_TW_EVENT_TEST_STATUS_ERROR;
	}

	return DTM_TW_EVENT_TEST_STATUS_SUCCESS |
	       ((ant_info_rp->max_cte_len << DTM_TW_EVENT_RESPONSE_POS) &
		DTM_TW_EVENT_RESPONSE_MASK);
}

static uint16_t on_cc_read_tx_power(const struct bt_hci_rp_le_read_tx_power *tx_power_rp)
{
	if (tx_power_rp->status != BT_HCI_ERR_SUCCESS) {
		LOG_ERR("Read TX Power command failed with status 0x%02X", tx_power_rp->status);
		return DTM_TW_EVENT_TEST_STATUS_ERROR;
	}

	uint16_t event = DTM_TW_EVENT_TEST_STATUS_SUCCESS;

	if (dtm_hci_parameters.transmit_power < tx_power_rp->min_tx_power ||
			dtm_hci_parameters.transmit_power == DTM_TW_TRANSMIT_POWER_LVL_SET_MIN) {
		dtm_hci_parameters.transmit_power = tx_power_rp->min_tx_power;
		event |= DTM_TW_TRANSMIT_POWER_MIN_LVL_BIT;
	}
	if (dtm_hci_parameters.transmit_power > tx_power_rp->max_tx_power ||
		 dtm_hci_parameters.transmit_power == DTM_TW_TRANSMIT_POWER_LVL_SET_MAX) {
		dtm_hci_parameters.transmit_power = tx_power_rp->max_tx_power;
		event |= DTM_TW_TRANSMIT_POWER_MAX_LVL_BIT;
	}

	/* Since we do not communicate the transmit power level to the controller until the test is
	 * started, we have to assume the current value of the transmit_power parameter will be
	 * accepted.
	 */
	event |= ((dtm_hci_parameters.transmit_power << DTM_TW_TRANSMIT_POWER_RESPONSE_LVL_POS) &
						DTM_TW_TRANSMIT_POWER_RESPONSE_LVL_MASK);
	return event;
}

static uint16_t on_cc_test_start(const struct bt_hci_evt_cc_status *status_rp)
{
	if (status_rp->status != BT_HCI_ERR_SUCCESS) {
		LOG_ERR("Test start command failed with status 0x%02X", status_rp->status);
		return DTM_TW_EVENT_TEST_STATUS_ERROR;
	}
	return DTM_TW_EVENT_TEST_STATUS_SUCCESS;
}

static uint16_t on_cc_test_end(const struct bt_hci_rp_le_test_end *test_end_rp)
{
	if (test_end_rp->status != BT_HCI_ERR_SUCCESS) {
		LOG_ERR("LE Test End command failed with status 0x%02X", test_end_rp->status);
		return DTM_TW_EVENT_TEST_STATUS_ERROR;
	}

	return DTM_TW_EVENT_PACKET_REPORTING |
	       (test_end_rp->rx_pkt_count & DTM_TW_PACKET_COUNT_MASK);
}

static int dtm_tw_to_hci_init(void)
{
	reset_dtm(DTM_TW_RESET_MIN_RANGE);

	return 0;
}

dtm_tw_to_hci_status_t dtm_tw_to_hci_process_tw_cmd(const uint16_t tw_cmd, struct net_buf *hci_cmd,
						    uint16_t *tw_event)
{
	if (!tw_event || !hci_cmd) {
		return DTM_TW_TO_HCI_STATUS_ERROR;
	}

	enum dtm_tw_cmd_code cmd_code = TW_CMD_CODE(tw_cmd);

	/* RX and TX test commands */
	uint8_t channel = TW_CMD_CHANNEL(tw_cmd);
	uint8_t length = TW_CMD_LENGTH(tw_cmd);
	enum dtm_tw_pkt_type pkt_type = TW_CMD_PKT_TYPE(tw_cmd);

	/* Setup and End commands */
	enum dtm_tw_ctrl_code control = TW_CMD_CONTROL(tw_cmd);
	uint8_t parameter = TW_CMD_PARAMETER(tw_cmd);

	switch (cmd_code) {
	case DTM_TW_CMD_TEST_SETUP:
		LOG_DBG("Executing test setup command. Control: %d Parameter: %d", control,
			parameter);
		*tw_event = on_test_setup_cmd(control, parameter, hci_cmd);
		break;

	case DTM_TW_CMD_TEST_END:
		LOG_DBG("Executing test end command. Control: %d Parameter: %d", control,
			parameter);
		*tw_event = on_test_end_cmd(control, parameter, hci_cmd);
		break;

	case DTM_TW_CMD_RECEIVER_TEST:
		LOG_DBG("Executing reception test command. Channel: %d", channel);
		*tw_event = on_test_rx_cmd(channel, hci_cmd);
		break;

	case DTM_TW_CMD_TRANSMITTER_TEST:
		LOG_DBG("Executing transmission test command. Channel: %d Length: %d Type: %d",
			channel, length, pkt_type);
		*tw_event = on_test_tx_cmd(channel, length, pkt_type, hci_cmd);
		break;

	default:
		LOG_ERR("Received unknown command code %d", cmd_code);
		*tw_event = DTM_TW_EVENT_TEST_STATUS_ERROR;
	}

	if (*tw_event == HCI_GENERATED) {
		return DTM_TW_TO_HCI_STATUS_HCI_CMD;
	} else {
		return DTM_TW_TO_HCI_STATUS_TW_EVENT;
	}
}

dtm_tw_to_hci_status_t dtm_tw_to_hci_process_hci_event(const uint16_t tw_cmd,
						       const struct net_buf *hci_event,
						       uint16_t *tw_event)
{
	if (!hci_event || !hci_event->data || !tw_event) {
		return DTM_TW_TO_HCI_STATUS_ERROR;
	}

	/* Verify that the received packet is an HCI event */
	uint8_t packet_indicator = hci_event->data[HCI_H4_PKT_INDICATOR_OFFSET];

	if (packet_indicator != BT_HCI_H4_EVT) {
		LOG_WRN("Received non-event HCI packet 0x%02X", packet_indicator);
		return DTM_TW_TO_HCI_STATUS_UNHANDLED;
	}

	const struct bt_hci_evt_hdr *event_hdr = (void *)(&hci_event->data[HCI_H4_PKT_OFFSET]);

	/* Check for Command Status event, which is only expected for the DTM HCI commands if
	 * the command failed to be processed.
	 */
	if (event_hdr->evt == BT_HCI_EVT_CMD_STATUS) {
		const struct bt_hci_evt_cmd_status *status_event = (void *)event_hdr->data;

		LOG_ERR("HCI command 0x%04X failed with status 0x%02X", status_event->opcode,
			status_event->status);
		*tw_event = DTM_TW_EVENT_TEST_STATUS_ERROR;
		return DTM_TW_TO_HCI_STATUS_TW_EVENT;
	}

	/* Verify that the received event is a Command Complete event */
	if (event_hdr->evt != BT_HCI_EVT_CMD_COMPLETE) {
		LOG_WRN("Unhandled HCI event 0x%02X", event_hdr->evt);
		return DTM_TW_TO_HCI_STATUS_UNHANDLED;
	}

	const struct bt_hci_evt_cmd_complete *cc_event = (void *)event_hdr->data;

	/* The completed command's return parameters are at the end of the CC event */
	const void *cc_event_return_params =
		&event_hdr->data[sizeof(struct bt_hci_evt_cmd_complete)];
	const int16_t param_len = event_hdr->len - sizeof(struct bt_hci_evt_cmd_complete);

	switch (cc_event->opcode) {
	case BT_HCI_OP_LE_READ_MAX_DATA_LEN:
		__ASSERT_NO_MSG(param_len == sizeof(struct bt_hci_rp_le_read_max_data_len));
		const struct bt_hci_rp_le_read_max_data_len *max_data_len_rp =
			cc_event_return_params;

		*tw_event = on_cc_read_max(max_data_len_rp, tw_cmd);
		return DTM_TW_TO_HCI_STATUS_TW_EVENT;

	case BT_HCI_OP_LE_READ_ANT_INFO:
		__ASSERT_NO_MSG(param_len == sizeof(struct bt_hci_rp_le_read_ant_info));
		const struct bt_hci_rp_le_read_ant_info *ant_info_rp = cc_event_return_params;

		*tw_event = on_cc_read_ant_info(ant_info_rp);
		return DTM_TW_TO_HCI_STATUS_TW_EVENT;

	case BT_HCI_OP_LE_READ_ALL_LOCAL_SUPPORTED_FEATURES:
		__ASSERT_NO_MSG(param_len ==
				sizeof(struct bt_hci_rp_le_read_all_local_supported_features));
		const struct bt_hci_rp_le_read_all_local_supported_features *features_rp =
			cc_event_return_params;

		*tw_event = on_cc_supported_features(features_rp);
		return DTM_TW_TO_HCI_STATUS_TW_EVENT;

	case BT_HCI_OP_LE_READ_TX_POWER:
		__ASSERT_NO_MSG(param_len == sizeof(struct bt_hci_rp_le_read_tx_power));
		const struct bt_hci_rp_le_read_tx_power *tx_power_rp = cc_event_return_params;

		*tw_event = on_cc_read_tx_power(tx_power_rp);
		return DTM_TW_TO_HCI_STATUS_TW_EVENT;

	case BT_HCI_OP_LE_RX_TEST:
	case BT_HCI_OP_LE_ENH_RX_TEST:
	case BT_HCI_OP_LE_RX_TEST_V3:
	case BT_HCI_OP_LE_TX_TEST:
	case BT_HCI_OP_LE_ENH_TX_TEST:
	case BT_HCI_OP_LE_TX_TEST_V3:
	case BT_HCI_OP_LE_TX_TEST_V4:
		__ASSERT_NO_MSG(param_len == sizeof(struct bt_hci_evt_cc_status));
		const struct bt_hci_evt_cc_status *status_rp = cc_event_return_params;

		*tw_event = on_cc_test_start(status_rp);
		return DTM_TW_TO_HCI_STATUS_TW_EVENT;

	case BT_HCI_OP_LE_TEST_END:
		__ASSERT_NO_MSG(param_len == sizeof(struct bt_hci_rp_le_test_end));
		const struct bt_hci_rp_le_test_end *test_end_rp = cc_event_return_params;

		*tw_event = on_cc_test_end(test_end_rp);
		return DTM_TW_TO_HCI_STATUS_TW_EVENT;

	default:
		LOG_WRN("Unhandled HCI command complete event for opcode 0x%04X", cc_event->opcode);
		return DTM_TW_TO_HCI_STATUS_UNHANDLED;
	}
}

SYS_INIT(dtm_tw_to_hci_init, APPLICATION, 0);
