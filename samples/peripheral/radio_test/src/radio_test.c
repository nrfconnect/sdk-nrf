/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "radio_test.h"

#include <string.h>
#include <hal/nrf_nvmc.h>
#include <hal/nrf_power.h>
#include <hal/nrf_rng.h>
#include <nrfx_timer.h>
#include <zephyr.h>

#if CONFIG_NRF21540_FEM
#include "nrf21540.h"
#endif

/* IEEE 802.15.4 default frequency. */
#define IEEE_DEFAULT_FREQ         (5)
/* Length on air of the LENGTH field. */
#define RADIO_LENGTH_LENGTH_FIELD (8UL)

/* Frequency calculation for a given channel in the IEEE 802.15.4 radio
 * mode.
 */
#define IEEE_FREQ_CALC(_channel) (IEEE_DEFAULT_FREQ + \
				 (IEEE_DEFAULT_FREQ * \
				 ((_channel) - IEEE_MIN_CHANNEL)))
/* Frequency calculation for a given channel. */
#define CHAN_TO_FREQ(_channel) (2400 + _channel)

/* Buffer for the radio TX packet */
static uint8_t tx_packet[RADIO_MAX_PAYLOAD_LEN];
/* Buffer for the radio RX packet. */
static uint8_t rx_packet[RADIO_MAX_PAYLOAD_LEN];
/* Number of transmitted packets. */
static uint32_t tx_packet_cnt;
/* Number of received packets with valid CRC. */
static uint32_t rx_packet_cnt;

/* Radio current channel (frequency). */
static uint8_t current_channel;

/* Timer used for channel sweeps and tx with duty cycle. */
static const nrfx_timer_t timer = NRFX_TIMER_INSTANCE(0);

static bool sweep_proccesing;


#if CONFIG_NRF21540_FEM
static struct radio_test_nrf21540 nrf21540;

static int nrf21540_configure(bool rx, nrf_radio_mode_t mode,
			      struct radio_test_nrf21540 *nrf21540)
{
	int err;

	/* nRF21540 is kept powered during sweeping */
	if (!sweep_proccesing) {
		err = nrf21540_power_up();
		if (err) {
			return err;
		}
	}

	if (nrf21540->active_delay == 0) {
		nrf21540->active_delay =
			nrf21540_default_active_delay_calculate(false, mode);
	}

	if (rx) {
		return nrf21540_rx_configure(NRF21540_EXECUTE_NOW,
					     nrf_radio_event_address_get(
						NRF_RADIO,
						NRF_RADIO_EVENT_DISABLED),
					     nrf21540->active_delay);
	}

	/* Sweeping is done from the interrupt context do not trigger SPI
	 * transfer in the interrupt.
	 */
	if ((nrf21540->gain != NRF21540_USE_DEFAULT_GAIN) &&
	    !sweep_proccesing) {
		err = nrf21540_tx_gain_set(nrf21540->gain);
		if (err) {
			return err;
		}
	}

	return nrf21540_tx_configure(NRF21540_EXECUTE_NOW,
				     nrf_radio_event_address_get(NRF_RADIO,
						NRF_RADIO_EVENT_DISABLED),
				     nrf21540->active_delay);
}
#endif /* CONFIG_NRF21540_FEM */

static uint8_t rnd8(void)
{
	nrf_rng_event_clear(NRF_RNG, NRF_RNG_EVENT_VALRDY);

	while (!nrf_rng_event_check(NRF_RNG, NRF_RNG_EVENT_VALRDY)) {
		/* Do nothing. */
	}
	return nrf_rng_random_value_get(NRF_RNG);
}

static void radio_channel_set(nrf_radio_mode_t mode, uint8_t channel)
{
#if defined(RADIO_MODE_MODE_Ieee802154_250Kbit)
	if (mode == NRF_RADIO_MODE_IEEE802154_250KBIT) {
		if ((channel >= IEEE_MIN_CHANNEL) &&
			(channel <= IEEE_MAX_CHANNEL)) {
			nrf_radio_frequency_set(
				NRF_RADIO,
				CHAN_TO_FREQ(IEEE_FREQ_CALC(channel)));
		} else {
			nrf_radio_frequency_set(
				NRF_RADIO,
				CHAN_TO_FREQ(IEEE_DEFAULT_FREQ));
		}
	} else {
		nrf_radio_frequency_set(NRF_RADIO, CHAN_TO_FREQ(channel));
	}
#else
	nrf_radio_frequency_set(NRF_RADIO, CHAN_TO_FREQ(channel));
#endif /* defined(RADIO_MODE_MODE_Ieee802154_250Kbit) */
}

static void radio_config(nrf_radio_mode_t mode, enum transmit_pattern pattern)
{
	nrf_radio_packet_conf_t packet_conf;

	/* Reset Radio ramp-up time. */
	nrf_radio_modecnf0_set(NRF_RADIO, false, RADIO_MODECNF0_DTX_Center);
	nrf_radio_crc_configure(NRF_RADIO, RADIO_CRCCNF_LEN_Disabled,
				NRF_RADIO_CRC_ADDR_INCLUDE, 0);

	/* Set the device address 0 to use when transmitting. */
	nrf_radio_txaddress_set(NRF_RADIO, 0);
	/* Enable the device address 0 to use to select which addresses to
	 * receive
	 */
	nrf_radio_rxaddresses_set(NRF_RADIO, 1);

	/* Set the address according to the transmission pattern. */
	switch (pattern) {
	case TRANSMIT_PATTERN_RANDOM:
		nrf_radio_prefix0_set(NRF_RADIO, 0xAB);
		nrf_radio_base0_set(NRF_RADIO, 0xABABABAB);
		break;

	case TRANSMIT_PATTERN_11001100:
		nrf_radio_prefix0_set(NRF_RADIO, 0xCC);
		nrf_radio_base0_set(NRF_RADIO, 0xCCCCCCCC);
		break;

	case TRANSMIT_PATTERN_11110000:
		nrf_radio_prefix0_set(NRF_RADIO, 0x6A);
		nrf_radio_base0_set(NRF_RADIO, 0x58FE811B);
		break;

	default:
		return;
	}

	/* Packet configuration:
	 * payload length size = 8 bits,
	 * 0-byte static length, max 255-byte payload,
	 * 4-byte base address length (5-byte full address length),
	 * Bit 24: 1 Big endian,
	 * Bit 25: 1 Whitening enabled.
	 */
	memset(&packet_conf, 0, sizeof(packet_conf));
	packet_conf.lflen = RADIO_LENGTH_LENGTH_FIELD;
	packet_conf.maxlen = (sizeof(tx_packet) - 1);
	packet_conf.statlen = 0;
	packet_conf.balen = 4;
	packet_conf.big_endian = true;
	packet_conf.whiteen = true;

	switch (mode) {
#if defined(RADIO_MODE_MODE_Ieee802154_250Kbit)
	case NRF_RADIO_MODE_IEEE802154_250KBIT:
		/* Packet configuration:
		 * S1 size = 0 bits,
		 * S0 size = 0 bytes,
		 * 32-bit preamble.
		 */
		packet_conf.plen = NRF_RADIO_PREAMBLE_LENGTH_32BIT_ZERO;
		packet_conf.maxlen = IEEE_MAX_PAYLOAD_LEN;
		packet_conf.balen = 0;
		packet_conf.big_endian = false;
		packet_conf.whiteen = false;

		/* Set fast ramp-up time. */
		nrf_radio_modecnf0_set(NRF_RADIO, true,
				       RADIO_MODECNF0_DTX_Center);
		break;
#endif /* defined(RADIO_MODE_MODE_Ieee802154_250Kbit) */

#if defined(RADIO_MODE_MODE_Ble_LR125Kbit) || \
	defined(RADIO_MODE_MODE_Ble_LR500Kbit)

	case NRF_RADIO_MODE_BLE_LR500KBIT:
	case NRF_RADIO_MODE_BLE_LR125KBIT:
		/* Packet configuration:
		 * S1 size = 0 bits,
		 * S0 size = 0 bytes,
		 * 10-bit preamble.
		 */
		packet_conf.plen = NRF_RADIO_PREAMBLE_LENGTH_LONG_RANGE;
		packet_conf.maxlen = IEEE_MAX_PAYLOAD_LEN;
		packet_conf.cilen = 2;
		packet_conf.termlen = 3;
		packet_conf.big_endian = false;
		packet_conf.balen = 3;

		/* Set fast ramp-up time. */
		nrf_radio_modecnf0_set(NRF_RADIO, true,
				       RADIO_MODECNF0_DTX_Center);

		/* Set CRC length; CRC calculation does not include the address
		 * field.
		 */
		nrf_radio_crc_configure(NRF_RADIO, RADIO_CRCCNF_LEN_Three,
					NRF_RADIO_CRC_ADDR_SKIP, 0);
		break;

#endif /* defined(RADIO_MODE_MODE_Ble_LR125Kbit) ||
	* defined(RADIO_MODE_MODE_Ble_LR500Kbit)
	*/

	case NRF_RADIO_MODE_BLE_2MBIT:
		/* Packet configuration:
		 * S1 size = 0 bits,
		 * S0 size = 0 bytes,
		 * 16-bit preamble.
		 */
		packet_conf.plen = NRF_RADIO_PREAMBLE_LENGTH_16BIT;
		break;

	default:
		/* Packet configuration:
		 * S1 size = 0 bits,
		 * S0 size = 0 bytes,
		 * 8-bit preamble.
		 */
		packet_conf.plen = NRF_RADIO_PREAMBLE_LENGTH_8BIT;
		break;
	}

	nrf_radio_packet_configure(NRF_RADIO, &packet_conf);
}

static void generate_modulated_rf_packet(uint8_t mode,
					 enum transmit_pattern pattern)
{
	radio_config(mode, pattern);

	/* One byte used for size, actual size is SIZE-1 */
#if defined(RADIO_MODE_MODE_Ieee802154_250Kbit)
	if (mode == NRF_RADIO_MODE_IEEE802154_250KBIT) {
		tx_packet[0] = IEEE_MAX_PAYLOAD_LEN - 1;
	} else {
		tx_packet[0] = sizeof(tx_packet) - 1;
	}
#else
	tx_packet[0] = sizeof(tx_packet) - 1;
#endif /* defined(RADIO_MODE_MODE_Ieee802154_250Kbit) */

	/* Fill payload with random data. */
	for (uint8_t i = 0; i < sizeof(tx_packet) - 1; i++) {
		if (pattern == TRANSMIT_PATTERN_RANDOM) {
			tx_packet[i + 1] = rnd8();
		} else if (pattern == TRANSMIT_PATTERN_11001100) {
			tx_packet[i + 1] = 0xCC;
		} else if (pattern == TRANSMIT_PATTERN_11110000) {
			tx_packet[i + 1] = 0xF0;
		} else {
			/* Do nothing. */
		}
	}
	nrf_radio_packetptr_set(NRF_RADIO, tx_packet);
}

static void radio_disable(void)
{
	nrf_radio_shorts_set(NRF_RADIO, 0);
	nrf_radio_int_disable(NRF_RADIO, ~0);
	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_DISABLED);

	nrf_radio_task_trigger(NRF_RADIO, NRF_RADIO_TASK_DISABLE);
	while (!nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_DISABLED)) {
		/* Do nothing */
	}
	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_DISABLED);

#if CONFIG_NRF21540_FEM
	(void) nrf21540_txrx_configuration_clear();
	(void) nrf21540_txrx_stop();

	/* Do not powerdown nRF21540 during sweeping. */
	if (!sweep_proccesing) {
		(void) nrf21540_power_down();
	}
#endif /* CONFIG_NRF21540_FEM */
}

static void radio_unmodulated_tx_carrier(uint8_t mode, uint8_t txpower, uint8_t channel)
{
	radio_disable();

	nrf_radio_mode_set(NRF_RADIO, mode);
	nrf_radio_shorts_enable(NRF_RADIO, NRF_RADIO_SHORT_READY_START_MASK);
	nrf_radio_txpower_set(NRF_RADIO, txpower);

	radio_channel_set(mode, channel);

#if CONFIG_NRF21540_FEM
	(void) nrf21540_configure(false, mode, &nrf21540);
#else
	nrf_radio_task_trigger(NRF_RADIO, NRF_RADIO_TASK_TXEN);
#endif /* CONFIG_NRF21540_FEM */
}

static void radio_modulated_tx_carrier(uint8_t mode, uint8_t txpower, uint8_t channel,
				       enum transmit_pattern pattern)
{
	radio_disable();
	generate_modulated_rf_packet(mode, pattern);

	switch (mode) {
#if defined(RADIO_MODE_MODE_Ieee802154_250Kbit) || \
	defined(RADIO_MODE_MODE_Ble_LR500Kbit) || \
	defined(RADIO_MODE_MODE_Ble_LR125Kbit)

	case NRF_RADIO_MODE_IEEE802154_250KBIT:
	case NRF_RADIO_MODE_BLE_LR125KBIT:
	case NRF_RADIO_MODE_BLE_LR500KBIT:
		nrf_radio_shorts_enable(NRF_RADIO,
					NRF_RADIO_SHORT_READY_START_MASK |
					NRF_RADIO_SHORT_PHYEND_START_MASK);
		break;

#endif /* defined(RADIO_MODE_MODE_Ieee802154_250Kbit) ||
	* defined(RADIO_MODE_MODE_Ble_LR500Kbit) ||
	* defined(RADIO_MODE_MODE_Ble_LR125Kbit)
	*/

	case NRF_RADIO_MODE_BLE_1MBIT:
	case NRF_RADIO_MODE_BLE_2MBIT:
	case NRF_RADIO_MODE_NRF_1MBIT:
	case NRF_RADIO_MODE_NRF_2MBIT:
	default:
#if defined(RADIO_MODE_MODE_Nrf_250Kbit)
	case NRF_RADIO_MODE_NRF_250KBIT:
#endif /* defined(RADIO_MODE_MODE_Nrf_250Kbit) */
		nrf_radio_shorts_enable(NRF_RADIO,
					NRF_RADIO_SHORT_READY_START_MASK |
					NRF_RADIO_SHORT_END_START_MASK);
		break;
	}

	nrf_radio_mode_set(NRF_RADIO, mode);
	nrf_radio_txpower_set(NRF_RADIO, txpower);

	radio_channel_set(mode, channel);

	tx_packet_cnt = 0;

	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_END);
	nrf_radio_int_enable(NRF_RADIO, NRF_RADIO_INT_END_MASK);

#if CONFIG_NRF21540_FEM
	(void) nrf21540_configure(false, mode, &nrf21540);
#else
	nrf_radio_task_trigger(NRF_RADIO, NRF_RADIO_TASK_TXEN);
#endif /* CONFIG_NRF21540_FEM */
	while (!nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_END)) {
		/* Do nothing */
	}
}

static void radio_rx(uint8_t mode, uint8_t channel, enum transmit_pattern pattern)
{
	radio_disable();

	nrf_radio_mode_set(NRF_RADIO, mode);
	nrf_radio_shorts_enable(NRF_RADIO,
				NRF_RADIO_SHORT_READY_START_MASK |
				NRF_RADIO_SHORT_END_START_MASK);
	nrf_radio_packetptr_set(NRF_RADIO, rx_packet);

	radio_config(mode, pattern);
	radio_channel_set(mode, channel);

	rx_packet_cnt = 0;

	nrf_radio_int_enable(NRF_RADIO, NRF_RADIO_INT_CRCOK_MASK);

#if CONFIG_NRF21540_FEM
	(void) nrf21540_configure(true, mode, &nrf21540);
#else
	nrf_radio_task_trigger(NRF_RADIO, NRF_RADIO_TASK_RXEN);
#endif
}

static void radio_sweep_start(uint8_t channel, uint32_t delay_ms)
{
	current_channel = channel;

#if CONFIG_NRF21540_FEM
	(void) nrf21540_power_up();

	if (nrf21540.gain != NRF21540_USE_DEFAULT_GAIN) {
		(void) nrf21540_tx_gain_set(nrf21540.gain);
	}
#endif

	nrfx_timer_disable(&timer);
	nrf_timer_shorts_disable(timer.p_reg, ~0);
	nrf_timer_int_disable(timer.p_reg, ~0);

	nrfx_timer_extended_compare(&timer,
		NRF_TIMER_CC_CHANNEL0,
		nrfx_timer_ms_to_ticks(&timer, delay_ms),
		NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK,
		true);

	nrfx_timer_clear(&timer);
	nrfx_timer_enable(&timer);
}

static void radio_modulated_tx_carrier_duty_cycle(uint8_t mode, uint8_t txpower,
						  uint8_t channel,
						  enum transmit_pattern pattern,
						  uint32_t duty_cycle)
{
	/* Lookup table with time per byte in each radio MODE
	 * Mapped per NRF_RADIO->MODE available on nRF5-series devices
	 */
	static const uint8_t time_in_us_per_byte[16] = {
		8, 4, 32, 8, 4, 64, 16, 0, 0, 0, 0, 0, 0, 0, 0, 32
	};

	/* 1 byte preamble, 5 byte address (BALEN + PREFIX),
	 * and sizeof(payload), no CRC
	 */
	const uint32_t total_payload_size     = 1 + 5 + sizeof(tx_packet);
	const uint32_t total_time_per_payload =
		time_in_us_per_byte[mode] * total_payload_size;

	/* Duty cycle = 100 * Time_on / (time_on + time_off),
	 * we need to calculate "time_off" for delay.
	 * In addition, the timer includes the "total_time_per_payload",
	 * so we need to add this to the total timer cycle.
	 */
	uint32_t delay_time = total_time_per_payload +
			   ((100 * total_time_per_payload -
			   (total_time_per_payload * duty_cycle)) /
			   duty_cycle);

	radio_disable();
	generate_modulated_rf_packet(mode, pattern);

	nrf_radio_mode_set(NRF_RADIO, mode);
	nrf_radio_shorts_enable(NRF_RADIO,
				NRF_RADIO_SHORT_READY_START_MASK |
				NRF_RADIO_SHORT_END_DISABLE_MASK);
	nrf_radio_txpower_set(NRF_RADIO, txpower);
	radio_channel_set(mode, channel);

#if CONFIG_NRF21540_FEM
	(void) nrf21540_power_up();

	if (nrf21540.gain != NRF21540_USE_DEFAULT_GAIN) {
		(void) nrf21540_tx_gain_set(nrf21540.gain);
	}

	if (nrf21540.active_delay == 0) {
		nrf21540.active_delay =
			nrf21540_default_active_delay_calculate(false, mode);
	}

#endif /* CONFIG_NRF21540_FEM */

	/* We let the TIMER start the radio transmission again. */
	nrfx_timer_disable(&timer);
	nrf_timer_shorts_disable(timer.p_reg, ~0);
	nrf_timer_int_disable(timer.p_reg, ~0);

	nrfx_timer_extended_compare(&timer,
		NRF_TIMER_CC_CHANNEL1,
		nrfx_timer_us_to_ticks(&timer, delay_time),
		NRF_TIMER_SHORT_COMPARE1_CLEAR_MASK,
		true);

	nrfx_timer_clear(&timer);
	nrfx_timer_enable(&timer);
}

void radio_test_start(const struct radio_test_config *config)
{
#if CONFIG_NRF21540_FEM
	nrf21540.active_delay = config->nrf21540.active_delay;
	nrf21540.gain = config->nrf21540.gain;
#endif

	switch (config->type) {
	case UNMODULATED_TX:
		radio_unmodulated_tx_carrier(config->mode,
			config->params.unmodulated_tx.txpower,
			config->params.unmodulated_tx.channel);
		break;
	case MODULATED_TX:
		radio_modulated_tx_carrier(config->mode,
			config->params.modulated_tx.txpower,
			config->params.modulated_tx.channel,
			config->params.modulated_tx.pattern);
		break;
	case RX:
		radio_rx(config->mode,
			config->params.rx.channel,
			config->params.rx.pattern);
		break;
	case TX_SWEEP:
		radio_sweep_start(config->params.tx_sweep.channel_start,
			config->params.tx_sweep.delay_ms);
		break;
	case RX_SWEEP:
		radio_sweep_start(config->params.rx_sweep.channel_start,
			config->params.rx_sweep.delay_ms);
		break;
	case MODULATED_TX_DUTY_CYCLE:
		radio_modulated_tx_carrier_duty_cycle(config->mode,
			config->params.modulated_tx_duty_cycle.txpower,
			config->params.modulated_tx_duty_cycle.channel,
			config->params.modulated_tx_duty_cycle.pattern,
			config->params.modulated_tx_duty_cycle.duty_cycle);
		break;
	}
}

void radio_test_cancel(void)
{
	nrfx_timer_disable(&timer);
	radio_disable();
}

void radio_rx_stats_get(struct radio_rx_stats *rx_stats)
{
	size_t size;

#if defined(RADIO_MODE_MODE_Ieee802154_250Kbit)
	nrf_radio_mode_t radio_mode;

	radio_mode = nrf_radio_mode_get(NRF_RADIO);
	if (radio_mode == NRF_RADIO_MODE_IEEE802154_250KBIT) {
		size = IEEE_MAX_PAYLOAD_LEN;
	} else {
		size = sizeof(rx_packet);
	}
#else
	size = sizeof(rx_packet);
#endif /* defined(RADIO_MODE_MODE_Ieee802154_250Kbit) */

	rx_stats->last_packet.buf = rx_packet;
	rx_stats->last_packet.len = size;
	rx_stats->packet_cnt = rx_packet_cnt;
}

#if NRF_POWER_HAS_DCDCEN_VDDH || NRF_POWER_HAS_DCDCEN
void toggle_dcdc_state(uint8_t dcdc_state)
{
	bool is_enabled;

#if NRF_POWER_HAS_DCDCEN_VDDH
	if (dcdc_state == 0) {
		is_enabled = nrf_power_dcdcen_vddh_get(NRF_POWER);
		nrf_power_dcdcen_vddh_set(NRF_POWER, !is_enabled);
		return;
	}
#endif /* NRF_POWER_HAS_DCDCEN_VDDH */

#if NRF_POWER_HAS_DCDCEN
	if (dcdc_state <= 1) {
		is_enabled = nrf_power_dcdcen_get(NRF_POWER);
		nrf_power_dcdcen_set(NRF_POWER, !is_enabled);
		return;
	}
#endif /* NRF_POWER_HAS_DCDCEN */
}
#endif /* NRF_POWER_HAS_DCDCEN_VDDH || NRF_POWER_HAS_DCDCEN */

static void timer_handler(nrf_timer_event_t event_type, void *context)
{
	const struct radio_test_config *config =
		(const struct radio_test_config *) context;

	if (event_type == NRF_TIMER_EVENT_COMPARE0) {
		uint8_t channel_start;
		uint8_t channel_end;

		if (config->type == TX_SWEEP) {
			sweep_proccesing = true;
			radio_unmodulated_tx_carrier(config->mode,
				config->params.tx_sweep.txpower,
				current_channel);

			channel_start = config->params.tx_sweep.channel_start;
			channel_end = config->params.tx_sweep.channel_end;
		} else if (config->type == RX_SWEEP) {
			sweep_proccesing = true;
			radio_rx(config->mode,
				current_channel,
				config->params.rx.pattern);

			channel_start = config->params.rx_sweep.channel_start;
			channel_end = config->params.rx_sweep.channel_end;
		} else {
			printk("Unexpected test type: %d\n", config->type);
			return;
		}

		sweep_proccesing = false;

		current_channel++;
		if (current_channel > channel_end) {
			current_channel = channel_start;
		}
	}

	if (event_type == NRF_TIMER_EVENT_COMPARE1) {
#if CONFIG_NRF21540_FEM
		(void) nrf21540_txrx_configuration_clear();
		(void) nrf21540_txrx_stop();
		(void) nrf21540_tx_configure(NRF21540_EXECUTE_NOW,
				nrf_radio_event_address_get(NRF_RADIO,
						NRF_RADIO_EVENT_DISABLED),
				nrf21540.active_delay);
#else
		nrf_radio_task_trigger(NRF_RADIO, NRF_RADIO_TASK_TXEN);
#endif /* CONFIG_NRF21540_FEM */
	}
}

static void timer_init(const struct radio_test_config *config)
{
	nrfx_err_t          err;
	nrfx_timer_config_t timer_cfg = {
		.frequency = NRF_TIMER_FREQ_1MHz,
		.mode      = NRF_TIMER_MODE_TIMER,
		.bit_width = NRF_TIMER_BIT_WIDTH_24,
		.p_context = (void *) config,
	};

	err = nrfx_timer_init(&timer, &timer_cfg, timer_handler);
	if (err != NRFX_SUCCESS) {
		printk("nrfx_timer_init failed with: %d\n", err);
	}
}

void radio_handler(const void *context)
{
	const struct radio_test_config *config =
		(const struct radio_test_config *) context;

	if (nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_CRCOK)) {
		nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_CRCOK);
		rx_packet_cnt++;
	}

	if (nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_END)) {
		nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_END);

		tx_packet_cnt++;
		if (tx_packet_cnt == config->params.modulated_tx.packets_num) {
			radio_disable();
			config->params.modulated_tx.cb();
		}
	}
}

int radio_test_init(struct radio_test_config *config)
{
	nrf_rng_task_trigger(NRF_RNG, NRF_RNG_TASK_START);

#if CONFIG_NRF21540_FEM
	int err = nrf21540_init();

	if (err) {
		return err;
	}
#endif /* CONFIG_NRF21540_FEM */

	timer_init(config);
	IRQ_CONNECT(TIMER0_IRQn, NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY,
		nrfx_timer_0_irq_handler, NULL, 0);

	irq_connect_dynamic(RADIO_IRQn, 7, radio_handler, config, 0);
	irq_enable(RADIO_IRQn);

	return 0;
}
