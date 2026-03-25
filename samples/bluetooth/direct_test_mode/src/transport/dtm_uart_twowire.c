/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <bluetooth/dtm_twowire/dtm_twowire_types.h>
#include <dtm.h>

#include "dtm_transport.h"

LOG_MODULE_REGISTER(dtm_tw_tr, CONFIG_DTM_TRANSPORT_LOG_LEVEL);

/* The DTM maximum wait time in milliseconds for the UART command second byte. */
#define DTM_UART_SECOND_BYTE_MAX_DELAY 5

#define DTM_UART DT_CHOSEN(ncs_dtm_uart)

#if DT_NODE_HAS_PROP(DTM_UART, current_speed)
/* UART Baudrate used to communicate with the DTM library. */
#define DTM_UART_BAUDRATE DT_PROP(DTM_UART, current_speed)

/* The UART poll cycle in micro seconds.
 * A baud rate of e.g. 19200 bits / second, and 8 data bits, 1 start/stop bit,
 * no flow control, give the time to transmit a byte:
 * 10 bits * 1/19200 = approx: 520 us.
 */
#define DTM_UART_POLL_CYCLE_US ((uint32_t) (10 * 1e6 / DTM_UART_BAUDRATE))
#else
#error "DTM UART node not found"
#endif /* DT_NODE_HAS_PROP(DTM_UART, currrent_speed) */

static const struct device *dtm_uart = DEVICE_DT_GET(DTM_UART);

/** Upper bits of packet length */
static uint8_t upper_len;

/** Currently configured PHY */
static enum dtm_phy dtm_phy = DTM_PHY_1M;

static int reset_dtm(uint8_t parameter)
{
	if (parameter > DTM_TW_RESET_MAX_RANGE) {
		return -EINVAL;
	}

	upper_len = 0;
	dtm_phy = DTM_PHY_1M;
	return dtm_setup_reset();
}

static int upper_set(uint8_t parameter)
{
	if (parameter > DTM_TW_SET_UPPER_BITS_MAX_RANGE) {
		return -EINVAL;
	}

	upper_len = (parameter << DTM_TW_UPPER_BITS_POS) & DTM_TW_UPPER_BITS_MASK;
	return 0;
}

static int phy_set(uint8_t parameter)
{
	int err;
	switch (parameter) {
	case DTM_TW_PHY_1M_MIN_RANGE ... DTM_TW_PHY_1M_MAX_RANGE:
		err = dtm_setup_set_phy(DTM_PHY_1M);
		if (err == 0) {
			dtm_phy = DTM_PHY_1M;
		}
		return err;
	case DTM_TW_PHY_2M_MIN_RANGE ... DTM_TW_PHY_2M_MAX_RANGE:
		err = dtm_setup_set_phy(DTM_PHY_2M);
		if (err == 0) {
			dtm_phy = DTM_PHY_2M;
		}
		return err;
	case DTM_TW_PHY_LE_CODED_S8_MIN_RANGE ... DTM_TW_PHY_LE_CODED_S8_MAX_RANGE:
		err = dtm_setup_set_phy(DTM_PHY_CODED_S8);
		if (err == 0) {
			dtm_phy = DTM_PHY_CODED_S8;
		}
		return err;
	case DTM_TW_PHY_LE_CODED_S2_MIN_RANGE ... DTM_TW_PHY_LE_CODED_S2_MAX_RANGE:
		err = dtm_setup_set_phy(DTM_PHY_CODED_S2);
		if (err == 0) {
			dtm_phy = DTM_PHY_CODED_S2;
		}
		return err;

	default:
		return -EINVAL;
	}
}

static int mod_set(uint8_t parameter)
{
	if (parameter >= DTM_TW_MODULATION_INDEX_STANDARD_MIN_RANGE &&
	    parameter <= DTM_TW_MODULATION_INDEX_STANDARD_MAX_RANGE) {
		return dtm_setup_set_modulation(DTM_MODULATION_STANDARD);
	} else if (parameter >= DTM_TW_MODULATION_INDEX_STABLE_MIN_RANGE &&
		   parameter <= DTM_TW_MODULATION_INDEX_STABLE_MAX_RANGE) {
		return dtm_setup_set_modulation(DTM_MODULATION_STABLE);
	} else {
		return -EINVAL;
	}
}

static int features_read(uint8_t parameter, uint16_t *ret)
{
	struct dtm_supp_features features;

	if (parameter > DTM_TW_FEATURE_READ_MAX_RANGE) {
		return -EINVAL;
	}

	features = dtm_setup_read_features();

	*ret = 0;
	*ret |= (features.data_len_ext ? DTM_TW_TEST_SETUP_DLE_SUPPORTED : 0);
	*ret |= (features.phy_2m ? DTM_TW_TEST_SETUP_2M_PHY_SUPPORTED : 0);
	*ret |= (features.stable_mod ? DTM_TW_TEST_SETUP_STABLE_MODULATION_SUPPORTED : 0);
	*ret |= (features.coded_phy ? DTM_TW_TEST_SETUP_CODED_PHY_SUPPORTED : 0);
	*ret |= (features.cte ? DTM_TW_TEST_SETUP_CTE_SUPPORTED : 0);
	*ret |= (features.ant_switching ? DTM_TW_TEST_SETUP_ANTENNA_SWITCH : 0);
	*ret |= (features.aod_1us_tx ? DTM_TW_TEST_SETUP_AOD_1US_TX : 0);
	*ret |= (features.aod_1us_rx ? DTM_TW_TEST_SETUP_AOD_1US_RX : 0);
	*ret |= (features.aoa_1us_rx ? DTM_TW_TEST_SETUP_AOA_1US_RX : 0);

	return 0;
}

static int read_max(uint8_t parameter, uint16_t *ret)
{
	int err;

	switch (parameter) {
	case DTM_TW_SUPPORTED_TX_OCTETS_MIN_RANGE ... DTM_TW_SUPPORTED_TX_OCTETS_MAX_RANGE:
		err = dtm_setup_read_max_supported_value(DTM_MAX_SUPPORTED_TX_OCTETS, ret);
		break;

	case DTM_TW_SUPPORTED_TX_TIME_MIN_RANGE ... DTM_TW_SUPPORTED_TX_TIME_MAX_RANGE:
		err = dtm_setup_read_max_supported_value(DTM_MAX_SUPPORTED_TX_TIME, ret);
		break;

	case DTM_TW_SUPPORTED_RX_OCTETS_MIN_RANGE ... DTM_TW_SUPPORTED_RX_OCTETS_MAX_RANGE:
		err = dtm_setup_read_max_supported_value(DTM_MAX_SUPPORTED_RX_OCTETS, ret);
		break;

	case DTM_TW_SUPPORTED_RX_TIME_MIN_RANGE ... DTM_TW_SUPPORTED_RX_TIME_MAX_RANGE:
		err = dtm_setup_read_max_supported_value(DTM_MAX_SUPPORTED_RX_TIME, ret);
		break;

	case DTM_TW_SUPPORTED_CTE_LENGTH:
		err = dtm_setup_read_max_supported_value(DTM_MAX_SUPPORTED_CTE_LENGTH, ret);
		break;

	default:
		return -EINVAL;
	}

	*ret = *ret << DTM_TW_EVENT_RESPONSE_POS;
	return err;
}

static int cte_set(uint8_t parameter)
{
	enum dtm_tw_cte_type_code type = (parameter >> DTM_TW_CTE_TYPE_POS) & DTM_TW_CTE_TYPE_MASK;
	uint8_t time = parameter & DTM_TW_CTE_CTETIME_MASK;

	if (!parameter) {
		return dtm_setup_set_cte_mode(DTM_CTE_TYPE_NONE, 0);
	}

	switch (type) {
	case DTM_TW_CTE_TYPE_AOA:
		return dtm_setup_set_cte_mode(DTM_CTE_TYPE_AOA, time);

	case DTM_TW_CTE_TYPE_AOD_1US:
		return dtm_setup_set_cte_mode(DTM_CTE_TYPE_AOD_1US, time);

	case DTM_TW_CTE_TYPE_AOD_2US:
		return dtm_setup_set_cte_mode(DTM_CTE_TYPE_AOD_2US, time);

	default:
		return -EINVAL;
	}
}

static int cte_slot_set(uint8_t parameter)
{
	enum dtm_tw_cte_type_code type = (parameter >> DTM_TW_CTE_TYPE_POS) & DTM_TW_CTE_TYPE_MASK;

	switch (type) {
	case DTM_TW_CTE_SLOT_1US:
		return dtm_setup_set_cte_slot(DTM_CTE_SLOT_DURATION_1US);

	case DTM_TW_CTE_SLOT_2US:
		return dtm_setup_set_cte_slot(DTM_CTE_SLOT_DURATION_2US);

	default:
		return -EINVAL;
	}
}

static int antenna_set(uint8_t parameter)
{
	static uint8_t pattern[DTM_TW_ANTENNA_NUMBER_MAX * 2];
	enum dtm_tw_antenna_pattern type = (parameter & DTM_TW_ANTENNA_SWITCH_PATTERN_MASK) >>
					   DTM_TW_ANTENNA_SWITCH_PATTERN_POS;
	uint8_t ant_count = (parameter & DTM_TW_ANTENNA_NUMBER_MASK);
	uint8_t length;
	size_t i;

	if ((ant_count < DTM_TW_ANTENNA_NUMBER_MIN) || (ant_count > DTM_TW_ANTENNA_NUMBER_MAX)) {
		return -EINVAL;
	}

	length = ant_count;

	switch (type) {
	case DTM_TW_ANTENNA_PATTERN_123N123N:
		for (i = 1; i <= length; i++) {
			pattern[i - 1] = i;
		}
		break;

	case DTM_TW_ANTENNA_PATTERN_123N2123:
		for (i = 1; i <= length; i++) {
			pattern[i - 1] = i;
		}
		for (i = 1; i < length; i++) {
			pattern[i + length - 1] = length - i;
		}

		length = (length * 2) - 1;
		break;

	default:
		return -EINVAL;
	}

	return dtm_setup_set_antenna_params(ant_count, pattern, length);
}

static int tx_power_set(int8_t parameter, uint16_t *ret)
{
	struct dtm_tx_power power;

	switch (parameter) {
	case DTM_TW_TRANSMIT_POWER_LVL_SET_MIN:
		power = dtm_setup_set_transmit_power(DTM_TX_POWER_REQUEST_MIN, 0, 0);
		break;

	case DTM_TW_TRANSMIT_POWER_LVL_SET_MAX:
		power = dtm_setup_set_transmit_power(DTM_TX_POWER_REQUEST_MAX, 0, 0);
		break;

	case DTM_TW_TRANSMIT_POWER_LVL_MIN ... DTM_TW_TRANSMIT_POWER_LVL_MAX:
		power = dtm_setup_set_transmit_power(DTM_TX_POWER_REQUEST_VAL, parameter, 0);
		break;

	default:
		return -EINVAL;
	}

	*ret = (power.power << DTM_TW_TRANSMIT_POWER_RESPONSE_LVL_POS) &
	       DTM_TW_TRANSMIT_POWER_RESPONSE_LVL_MASK;
	if (power.max) {
		*ret |= DTM_TW_TRANSMIT_POWER_MAX_LVL_BIT;
	}
	if (power.min) {
		*ret |= DTM_TW_TRANSMIT_POWER_MIN_LVL_BIT;
	}

	return 0;
}

static uint16_t on_test_setup_cmd(enum dtm_tw_ctrl_code control, uint8_t parameter)
{
	uint16_t ret = 0;
	int err;

	dtm_setup_prepare();

	switch (control) {
	case DTM_TW_TEST_SETUP_RESET:
		err = reset_dtm(parameter);
		break;

	case DTM_TW_TEST_SETUP_SET_UPPER:
		err = upper_set(parameter);
		break;

	case DTM_TW_TEST_SETUP_SET_PHY:
		err = phy_set(parameter);
		break;

	case DTM_TW_TEST_SETUP_SELECT_MODULATION:
		err = mod_set(parameter);
		break;

	case DTM_TW_TEST_SETUP_READ_SUPPORTED:
		err = features_read(parameter, &ret);
		break;

	case DTM_TW_TEST_SETUP_READ_MAX:
		err = read_max(parameter, &ret);
		break;

	case DTM_TW_TEST_SETUP_CONSTANT_TONE_EXTENSION:
		err = cte_set(parameter);
		break;

	case DTM_TW_TEST_SETUP_CONSTANT_TONE_EXTENSION_SLOT:
		err = cte_slot_set(parameter);
		break;

	case DTM_TW_TEST_SETUP_ANTENNA_ARRAY:
		err = antenna_set(parameter);
		break;

	case DTM_TW_TEST_SETUP_TRANSMIT_POWER:
		err = tx_power_set(parameter, &ret);
		break;

	default:
		err = -EINVAL;
		break;
	}

	return (err ? DTM_TW_EVENT_TEST_STATUS_ERROR : (DTM_TW_EVENT_TEST_STATUS_SUCCESS | ret));
}

static uint16_t on_test_end_cmd(enum dtm_tw_ctrl_code control, uint8_t parameter)
{
	uint16_t cnt;
	int err;

	if (control) {
		return DTM_TW_EVENT_TEST_STATUS_ERROR;
	}

	if (parameter > DTM_TW_TEST_END_MAX_RANGE) {
		return DTM_TW_EVENT_TEST_STATUS_ERROR;
	}

	err = dtm_test_end(&cnt);

	return err ? DTM_TW_EVENT_TEST_STATUS_ERROR : (DTM_TW_EVENT_PACKET_REPORTING | cnt);
}

static uint16_t on_test_rx_cmd(uint8_t chan)
{
	int err;

	err = dtm_test_receive(chan);

	return err ? DTM_TW_EVENT_TEST_STATUS_ERROR : DTM_TW_EVENT_TEST_STATUS_SUCCESS;
}

static uint16_t on_test_tx_cmd(uint8_t chan, uint8_t length, enum dtm_tw_pkt_type type)
{
	enum dtm_packet pkt;
	int err;

	switch (type) {
	case DTM_TW_PKT_PRBS9:
		pkt = DTM_PACKET_PRBS9;
		break;

	case DTM_TW_PKT_0X0F:
		pkt = DTM_PACKET_0F;
		break;

	case DTM_TW_PKT_0X55:
		pkt = DTM_PACKET_55;
		break;

	case DTM_TW_PKT_0XFF_OR_VS:
		pkt = DTM_PACKET_FF_OR_VENDOR;
		break;

	default:
		return DTM_TW_EVENT_TEST_STATUS_ERROR;
	}

	/* Add upper bits to length only if packet is not vendor specific */
	if (pkt != DTM_PACKET_FF_OR_VENDOR || (dtm_phy != DTM_PHY_1M && dtm_phy != DTM_PHY_2M)) {
		length = (length & ~DTM_TW_UPPER_BITS_MASK) | upper_len;
	}

	err = dtm_test_transmit(chan, length, pkt);

	return err ? DTM_TW_EVENT_TEST_STATUS_ERROR : DTM_TW_EVENT_TEST_STATUS_SUCCESS;
}

static uint16_t dtm_cmd_put(uint16_t cmd)
{
	enum dtm_tw_cmd_code cmd_code = (cmd >> 14) & 0x03;

	/* RX and TX test commands */
	uint8_t chan = (cmd >> 8) & 0x3F;
	uint8_t length = (cmd >> 2) & 0x3F;
	enum dtm_tw_pkt_type type = (enum dtm_tw_pkt_type)(cmd & 0x03);

	/* Setup and End commands */
	enum dtm_tw_ctrl_code control = (cmd >> 8) & 0x3F;
	uint8_t parameter = (uint8_t)cmd;

	switch (cmd_code) {
	case DTM_TW_CMD_TEST_SETUP:
		LOG_DBG("Executing test setup command. Control: %d Parameter: %d", control,
											parameter);
		return on_test_setup_cmd(control, parameter);

	case DTM_TW_CMD_TEST_END:
		LOG_DBG("Executing test end command. Control: %d Parameter: %d", control,
											parameter);
		return on_test_end_cmd(control, parameter);

	case DTM_TW_CMD_RECEIVER_TEST:
		LOG_DBG("Executing reception test command. Channel: %d", chan);
		return on_test_rx_cmd(chan);

	case DTM_TW_CMD_TRANSMITTER_TEST:
		LOG_DBG("Executing transmission test command. Channel: %d Length: %d Type: %d",
										chan, length, type);
		return on_test_tx_cmd(chan, length, type);

	default:
		LOG_ERR("Received unknown command code %d", cmd_code);
		return DTM_TW_EVENT_TEST_STATUS_ERROR;
	}
}

int dtm_tr_init(void)
{
	int err;

	if (!device_is_ready(dtm_uart)) {
		LOG_ERR("UART device not ready");
		return -EIO;
	}

#if defined(CONFIG_DTM_USB) && defined(CONFIG_SOC_NRF54H20_CPURAD)
	/* Enable RX path for the USB CDC ACM.
	 * uart_irq_rx_enable() -> cdc_acm_irq_rx_enable() -> cdc_acm_work_submit(rx_fifo_work)
	 * It is not needed for non CDC ACM UARTs.
	 */
	uart_irq_rx_enable(dtm_uart);
#endif /* defined(CONFIG_DTM_USB) && defined(CONFIG_SOC_NRF54H20_CPURAD) */

	err = dtm_init(NULL);
	if (err) {
		LOG_ERR("Error during DTM initialization: %d", err);
		return err;
	}

	return 0;
}

union dtm_tr_packet dtm_tr_get(void)
{
	bool is_msb_read = false;
	union dtm_tr_packet tmp;
	uint8_t rx_byte;
	uint16_t dtm_cmd = 0;
	int64_t msb_time = 0;
	int err;

	for (;;) {
		k_sleep(K_USEC(DTM_UART_POLL_CYCLE_US));

		err = uart_poll_in(dtm_uart, &rx_byte);
		if (err) {
			if (err != -1) {
				LOG_ERR("UART polling error: %d", err);
			}

			/* Nothing read from the UART */
			continue;
		}

		if (!is_msb_read) {
			/* This is first byte of two-byte command. */
			is_msb_read = true;
			dtm_cmd = rx_byte << 8;
			msb_time = k_uptime_get();

			/* Go back and wait for 2nd byte of command word. */
			continue;
		}

		/* This is the second byte read; combine it with the first and
		 * process command.
		 */
		if ((k_uptime_get() - msb_time) >
		    DTM_UART_SECOND_BYTE_MAX_DELAY) {
			/* More than ~5mS after msb: Drop old byte, take the
			 * new byte as MSB. The variable is_msb_read will
			 * remain true.
			 */
			dtm_cmd = rx_byte << 8;
			msb_time = k_uptime_get();
			/* Go back and wait for 2nd byte of command word. */
			LOG_DBG("Received byte discarded");
			continue;
		} else {
			dtm_cmd |= rx_byte;
			LOG_INF("Received 0x%04x command", dtm_cmd);
			tmp.twowire = dtm_cmd;
			return tmp;
		}
	}
}

int dtm_tr_process(union dtm_tr_packet cmd)
{
	uint16_t tmp = cmd.twowire;
	uint16_t ret;

	LOG_INF("Processing 0x%04x command", tmp);

	ret = dtm_cmd_put(tmp);
	LOG_INF("Sending 0x%04x response", ret);

	uart_poll_out(dtm_uart, (ret >> 8) & 0xFF);
	uart_poll_out(dtm_uart, ret & 0xFF);

	return 0;
}
