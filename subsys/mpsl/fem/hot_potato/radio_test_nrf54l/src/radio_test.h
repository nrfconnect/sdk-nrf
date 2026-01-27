/*
 * Copyright (c) 2020-2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef RADIO_TEST_H_
#define RADIO_TEST_H_

#include <zephyr/types.h>
#include <hal/nrf_radio.h>
#include <zephyr/drivers/gpio.h>

/** Maximum radio RX or TX payload. */
#define RADIO_MAX_PAYLOAD_LEN	255
/** IEEE 802.15.4 maximum payload length. */
#define IEEE_MAX_PAYLOAD_LEN	127
/** IEEE 802.15.4 minimum channel. */
#define IEEE_MIN_CHANNEL	11
/** IEEE 802.15.4 maximum channel. */
#define IEEE_MAX_CHANNEL	26

#define MODULATED_TX_INTERVAL_MAX_MS 10000

/** Maximum number of RSSI samples to store for received packets */
#define MAX_RSSISAMPLE_STORED	1500

/**@brief Radio transmit and address pattern. */
enum transmit_pattern {
	/** Random pattern. */
	TRANSMIT_PATTERN_RANDOM,

	/** Pattern 11110000(F0). */
	TRANSMIT_PATTERN_11110000,

	/** Pattern 11001100(CC). */
	TRANSMIT_PATTERN_11001100,
};

/**@brief Radio test mode. */
enum radio_test_mode {
	/** TX carrier. */
	UNMODULATED_TX,

	/** Modulated TX carrier. */
	MODULATED_TX,

	/** RX carrier. */
	RX,

	/** TX carrier sweep. */
	TX_SWEEP,

	/** RX carrier sweep. */
	RX_SWEEP,

	/** Duty-cycled modulated TX carrier. */
	MODULATED_TX_DUTY_CYCLE,

    /** Modulated TX carrier with interval between packets. */
	MODULATED_TX_INTERVAL,
};

/**@brief Radio test configuration. */
struct radio_test_config {
	/** Radio test type. */
	enum radio_test_mode type;

	/** Radio mode. Data rate and modulation. */
	nrf_radio_mode_t mode;

	bool crc_enabled;

	union {
		struct {
			/** Radio output power. */
			int8_t txpower;

			/** Radio channel. */
			uint8_t channel;
		} unmodulated_tx;

		struct {
			/** Radio output power. */
			int8_t txpower;

			/** Radio transmission pattern. */
			enum transmit_pattern pattern;

			/** Radio channel. */
			uint8_t channel;

			/**
			 * Number of pacets to transmit.
			 * Set to zero for continuous TX.
			 */
			uint32_t packets_num;

            /**
			 * Number of bytes in TX packet payload.
			 */
			uint32_t payload_size;

			/** Callback to indicate that TX is finished. */
			void (*cb)(void);
		} modulated_tx;

		struct {
			/** Radio transmission pattern. */
			enum transmit_pattern pattern;

			/** Radio channel. */
			uint8_t channel;
		} rx;

		struct {
			/** Radio output power. */
			int8_t txpower;

			/** Radio start channel (frequency). */
			uint8_t channel_start;

			/** Radio end channel (frequency). */
			uint8_t channel_end;

			/** Delay time in milliseconds. */
			uint32_t delay_ms;
		} tx_sweep;

		struct {
			/** Radio start channel (frequency). */
			uint8_t channel_start;

			/** Radio end channel (frequency). */
			uint8_t channel_end;

			/** Delay time in milliseconds. */
			uint32_t delay_ms;
		} rx_sweep;

		struct {
			/** Radio output power. */
			int8_t txpower;

			/** Radio transmission pattern. */
			enum transmit_pattern pattern;

			/** Radio channel. */
			uint8_t channel;

			/** Duty cycle. */
			uint32_t duty_cycle;
		} modulated_tx_duty_cycle;

		struct {
			/** Radio output power. */
			int8_t txpower;

			/** Radio transmission pattern. */
			enum transmit_pattern pattern;

			/** Radio channel. */
			uint8_t channel;

			/** Interval. */
			uint32_t interval;

            /**
			 * Number of bytes in TX packet payload.
			 */
			uint32_t payload_size;
		} modulated_tx_interval;

	} params;
};

/**@brief Radio RX statistics. */
struct radio_rx_stats {
	/** Content of the last packet. */
	struct {
		/** Content of the last packet. */
		uint8_t *buf;

		/** Length of the last packet. */
		size_t len;
	} last_packet;

	/** Number of received packets with valid CRC. */
	uint32_t packet_cnt;
};

/**
 * @brief Function for initializing the Radio Test module.
 *
 * @param[in] config  Radio test configuration.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int radio_test_init(struct radio_test_config *config);

/**
 * @brief Function for starting radio test.
 *
 * @param[in] config  Radio test configuration.
 */
void radio_test_start(const struct radio_test_config *config);

/**
 * @brief Function for stopping ongoing test (Radio and Timer operations).
 */
void radio_test_cancel(void);

/**
 * @brief Function for get RX statistics.
 *
 * @param[out] rx_stats RX statistics.
 */
void radio_rx_stats_get(struct radio_rx_stats *rx_stats);

/**
 * @brief Function for toggling the DC/DC converter state.
 *
 * @param[in] dcdc_state  DC/DC converter state.
 */
void toggle_dcdc_state(uint8_t dcdc_state);

#if defined(CONFIG_MPSL_FEM_HOT_POTATO_REGULATOR_CONTROL_MANUAL)
enum radio_test_regulator_mode {
	RADIO_TEST_REGULATOR_MODE_LDO = 0,
	RADIO_TEST_REGULATOR_MODE_DCDC = 1,
};

/**
 * @brief Function for forcing regulator mode (LDO or DCDC).
 *
 * @param[in] mode  Regulator mode.
 */
void radio_test_regulator_mode_set(enum radio_test_regulator_mode mode);
#endif /* CONFIG_MPSL_FEM_HOT_POTATO_REGULATOR_CONTROL_MANUAL */

/**
 * @brief Function for getting averaged RSSI samples of latest received packets.
 *
 * @param[in] num_rssi_samples  Number of last RSSI samples to average.
 *
 * @retval Averaged RSSI sample.
 */
uint8_t rssi_sample_averaged_packets_get(uint32_t num_rssi_samples);

#endif /* RADIO_TEST_H_ */
