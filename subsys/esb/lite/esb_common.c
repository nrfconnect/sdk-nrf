/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <hal/nrf_timer.h>
#include <hal/nrf_egu.h>
#include <nrfx_dppi.h>
#include <esb.h>
#include "esb_common.h"
#include "esb_graph.h"

LOG_MODULE_REGISTER(esb, CONFIG_ESB_LOG_LEVEL);

enum esb_mode esb_active_mode;
struct esb_config esb_config;
struct esb_address_info esb_address;
bool esb_running;
void (*volatile esb_active_radio_irq)(void);
void (*volatile esb_active_timer_irq)(void);

/** Flag set by the API functions that changes ESB configuration or address.
 *  When set, the radio will be reconfigured on the next transmission/reception.
 */
static bool esb_reconfigure_radio;

/** ESB is initialized (either PRX or PTX).
 */
static bool esb_initialized;

/** Default address values used on initialization.
 */
static struct esb_address_info default_address_values = {
	.base = { 0xE7E7E7E7, 0xC2C2C2C2 },
	.prefixes = { 0xE7, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8 },
	.base_length = 4,
	.frequency = 2400 + CONFIG_ESB_DEFAULT_RF_CHANNEL,
	.pipes_enabled = BIT_MASK(CONFIG_ESB_PIPE_COUNT),
};

/** Contains CRC radio registers values. */
struct crc_reg_values {
	uint32_t polynomial;
	uint32_t initial_value;
};

/** Array of all supported CRC configurations with their register values.
 *  Array index corresponds to number of CRC bytes minus one.
 */
static const struct crc_reg_values crc_reg_values[3] = {
	{
		/* CRC-8 CCITT */
		/* CRC poly: x^8+x^2+x^1+1 */
		.polynomial = 0x107,
		.initial_value = 0xFF,
	},
	{
		/* CRC-16 CCITT */
		/* CRC poly: x^16+x^12+x^5+1 */
		.polynomial = 0x11021,
		.initial_value = 0xFFFF,
	},
	{
		/* CRC-24 RFC-9580 */
		/* CRC poly: x^24+x^23+x^18+x^17+x^14+x^11+x^10+x^7+x^6+x^5+x^4+x^3+x^1+1 */
		.polynomial = 0x1864CFB,
		.initial_value = 0xB704CE,
	}
};


static uint32_t dbm_to_tx_power(int dbm, int *actual_dbm);


ISR_DIRECT_DECLARE(esb_radio_irq_direct_handler)
{
	esb_active_radio_irq();
	return 0;
}


ISR_DIRECT_DECLARE(esb_timer_irq_direct_handler)
{
	esb_active_timer_irq();
	return 0;
}


ISR_DIRECT_DECLARE(esb_egu_irq_direct_handler)
{
	if (ESB_ACTIVE_MODE == ESB_MODE_PTX) {
		esb_ptx_egu_irq();
	} else {
		esb_prx_egu_irq();
	}
	return 1;
}


int esb_init(const struct esb_config *config)
{
	int ret = 0;

	if (esb_initialized) {
		if (esb_active_mode == ESB_MODE_PTX && IS_ENABLED(CONFIG_ESB_PTX)) {
			esb_ptx_disable();
		} else if (IS_ENABLED(CONFIG_ESB_PRX)) {
			esb_prx_disable();
		} else {
			return -EINVAL;
		}
	}

	IRQ_DIRECT_CONNECT(ESB_RADIO_IRQ_NUMBER, CONFIG_ESB_RADIO_IRQ_PRIORITY,
			   esb_radio_irq_direct_handler, 0);
	IRQ_DIRECT_CONNECT(ESB_TIMER_IRQ_NUMBER, CONFIG_ESB_RADIO_IRQ_PRIORITY,
			   esb_timer_irq_direct_handler, 0);
	IRQ_DIRECT_CONNECT(ESB_EGU_IRQ_NUMBER, CONFIG_ESB_EVENT_IRQ_PRIORITY,
			   esb_egu_irq_direct_handler, 0);

	esb_active_mode = config->mode;

	memcpy(&esb_config, config, sizeof(esb_config));
	memcpy(&esb_address, &default_address_values, sizeof(esb_address));

	if (config->protocol != ESB_PROTOCOL_ESB_DPL) {
		ret = -EINVAL;
	} else if (esb_active_mode == ESB_MODE_PTX && IS_ENABLED(CONFIG_ESB_PTX)) {
		ret = esb_ptx_init(config);
	} else if (IS_ENABLED(CONFIG_ESB_PRX)) {
		ret = esb_prx_init(config);
	} else {
		ret = -EINVAL;
	}

	if (ret) {
		return ret;
	}

	esb_initialized = true;

	esb_configure_peripherals(true);

	return 0;
}


void esb_configure_peripherals(bool force_reconfig)
{
	bool reconfig = force_reconfig || esb_reconfigure_radio;

	esb_reconfigure_radio = false;

	if (reconfig) {
		nrf_radio_packet_conf_t conf = {
			.lflen = (ESB_MAX_PAYLOAD_LENGTH <= 32) ? 6 : 8,
			.s0len = 0,
			.s1len = 3,
#if defined(RADIO_PCNF0_PLEN_Msk)
			.plen = (esb_config.bitrate == ESB_BITRATE_2MBPS
				|| esb_config.bitrate == ESB_BITRATE_2MBPS_BLE
#if defined(RADIO_MODE_MODE_Nrf_4Mbit_0BT6)
				|| esb_config.bitrate == ESB_BITRATE_4MBPS
#endif
			) ? NRF_RADIO_PREAMBLE_LENGTH_16BIT : NRF_RADIO_PREAMBLE_LENGTH_8BIT,
#endif
			.maxlen = ESB_MAX_PAYLOAD_LENGTH,
			.statlen = 0,
			.balen = esb_address.base_length,
			.big_endian = true,
			.whiteen = false,
		};

		nrf_radio_packet_configure(NRF_RADIO, &conf);
		nrf_radio_fast_ramp_up_enable_set(NRF_RADIO, true);
		if (esb_config.crc != ESB_CRC_OFF) {
			const struct crc_reg_values *crc = &crc_reg_values[esb_config.crc - 1];

			nrf_radio_crcinit_set(NRF_RADIO, crc->initial_value);
			nrf_radio_crc_configure(NRF_RADIO, esb_config.crc,
						NRF_RADIO_CRC_ADDR_INCLUDE,
						crc->polynomial);
		} else {
			nrf_radio_crc_configure(NRF_RADIO, 0, NRF_RADIO_CRC_ADDR_INCLUDE, 0);
		}
		nrf_egu_int_enable(ESB_EGU, NRF_EGU_INT_TRIGGERED0);
		nrf_radio_base0_set(NRF_RADIO, esb_address.base[0]);
		nrf_radio_base1_set(NRF_RADIO, esb_address.base[1]);
		nrf_radio_prefix0_set(NRF_RADIO, *(uint32_t *)&esb_address.prefixes[0]);
		nrf_radio_prefix1_set(NRF_RADIO, *(uint32_t *)&esb_address.prefixes[4]);
		nrf_radio_frequency_set(NRF_RADIO, esb_address.frequency);
		nrf_radio_txpower_set(NRF_RADIO, dbm_to_tx_power(esb_config.tx_output_power, NULL));
		nrf_radio_mode_set(NRF_RADIO, esb_config.bitrate);

		nrf_timer_mode_set(ESB_TIMER, NRF_TIMER_MODE_TIMER);
		nrf_timer_bit_width_set(ESB_TIMER, NRF_TIMER_BIT_WIDTH_32);
		nrf_timer_prescaler_set(ESB_TIMER, NRF_TIMER_PRESCALER_CALCULATE(
			NRF_TIMER_BASE_FREQUENCY_GET(ESB_TIMER), NRFX_MHZ_TO_HZ(1)));
	}
}


bool esb_is_idle(void)
{
	return !esb_running;
}


int esb_set_address_length(uint8_t length)
{
	if (esb_running) {
		return -EBUSY;
	} else if (length < 3 || length > 5) {
		return -EINVAL;
	}

	esb_address.base_length = length - 1;
	esb_reconfigure_radio = true;

	return 0;
}


static int set_base_address(const uint8_t *addr, uint32_t index)
{
	int i;
	uint32_t value = 0;

	if (esb_running) {
		return -EBUSY;
	} else if (addr == NULL) {
		return -EINVAL;
	}

	for (i = 24; i >= 8 * (4 - (int)esb_address.base_length); i -= 8) {
		value |= (uint32_t)(*addr) << i;
		addr++;
	}

	esb_address.base[index] = value;
	esb_reconfigure_radio = true;

	return 0;
}


int esb_set_base_address_0(const uint8_t *addr)
{
	return set_base_address(addr, 0);
}


int esb_set_base_address_1(const uint8_t *addr)
{
	return set_base_address(addr, 1);
}


int esb_set_prefixes(const uint8_t *prefixes, uint8_t num_pipes)
{
	if (esb_running) {
		return -EBUSY;
	} else if (prefixes == NULL || num_pipes > CONFIG_ESB_PIPE_COUNT) {
		return -EINVAL;
	}

	memcpy(esb_address.prefixes, prefixes, num_pipes);
	esb_address.pipes_enabled = BIT_MASK(num_pipes);
	esb_reconfigure_radio = true;

	return 0;
}


int esb_enable_pipes(uint8_t enable_mask)
{
	if (esb_running) {
		return -EBUSY;
	} else if (enable_mask > BIT_MASK(CONFIG_ESB_PIPE_COUNT)) {
		return -EINVAL;
	}

	esb_address.pipes_enabled = enable_mask;
	esb_reconfigure_radio = true;

	return 0;
}


int esb_update_prefix(uint8_t pipe, uint8_t prefix)
{
	if (esb_running) {
		return -EBUSY;
	} else if (pipe > CONFIG_ESB_PIPE_COUNT) {
		return -EINVAL;
	}

	esb_address.prefixes[pipe] = prefix;
	esb_reconfigure_radio = true;

	return 0;
}


int esb_set_rf_channel(uint32_t channel)
{
	if (esb_running) {
		return -EBUSY;
	} else if (channel > 100) {
		return -EINVAL;
	}

	esb_address.frequency = 2400 + channel;
	esb_reconfigure_radio = true;

	return 0;
}


int esb_get_rf_channel(uint32_t *channel)
{
	*channel = esb_address.frequency - 2400;
	return 0;
}


int esb_set_tx_power(int8_t tx_output_power)
{
	int actual_dbm;

	if (esb_running) {
		return -EBUSY;
	}

	esb_config.tx_output_power = tx_output_power;
	esb_reconfigure_radio = true;
	dbm_to_tx_power(tx_output_power, &actual_dbm);
	if (actual_dbm != tx_output_power) {
		LOG_WRN("TX power %d dBm not supported. Using %d dBm instead.",
			tx_output_power, actual_dbm);
	}

	return 0;
}


int esb_set_bitrate(enum esb_bitrate bitrate)
{
	if (esb_running) {
		return -EBUSY;
	}

	esb_config.bitrate = bitrate;
	esb_reconfigure_radio = true;

	return 0;
}


/** Convert power in dBm to radio TX power register value. If the exact
 *  value is not found, the next higher supported power level is used.
 *  @param dbm        Power in dBm.
 *  @param actual_dbm If not NULL, will be set to the actual dBm that can be
 *                    different than requested if a supported value was found.
 */
static uint32_t dbm_to_tx_power(int dbm, int *actual_dbm)
{
	static const struct {
		int16_t dbm;
		uint16_t value;
	} power_map[] = {
		#if defined(RADIO_TXPOWER_TXPOWER_Neg100dBm)
		{ .dbm = -100, .value = RADIO_TXPOWER_TXPOWER_Neg100dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_Neg70dBm)
		{ .dbm = -70, .value = RADIO_TXPOWER_TXPOWER_Neg70dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_Neg46dBm)
		{ .dbm = -46, .value = RADIO_TXPOWER_TXPOWER_Neg46dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_Neg40dBm)
		{ .dbm = -40, .value = RADIO_TXPOWER_TXPOWER_Neg40dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_Neg30dBm)
		{ .dbm = -30, .value = RADIO_TXPOWER_TXPOWER_Neg30dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_Neg28dBm)
		{ .dbm = -28, .value = RADIO_TXPOWER_TXPOWER_Neg28dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_Neg26dBm)
		{ .dbm = -26, .value = RADIO_TXPOWER_TXPOWER_Neg26dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_Neg22dBm)
		{ .dbm = -22, .value = RADIO_TXPOWER_TXPOWER_Neg22dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_Neg20dBm)
		{ .dbm = -20, .value = RADIO_TXPOWER_TXPOWER_Neg20dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_Neg18dBm)
		{ .dbm = -18, .value = RADIO_TXPOWER_TXPOWER_Neg18dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_Neg16dBm)
		{ .dbm = -16, .value = RADIO_TXPOWER_TXPOWER_Neg16dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_Neg14dBm)
		{ .dbm = -14, .value = RADIO_TXPOWER_TXPOWER_Neg14dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_Neg12dBm)
		{ .dbm = -12, .value = RADIO_TXPOWER_TXPOWER_Neg12dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_Neg10dBm)
		{ .dbm = -10, .value = RADIO_TXPOWER_TXPOWER_Neg10dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_Neg9dBm)
		{ .dbm = -9, .value = RADIO_TXPOWER_TXPOWER_Neg9dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_Neg8dBm)
		{ .dbm = -8, .value = RADIO_TXPOWER_TXPOWER_Neg8dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_Neg7dBm)
		{ .dbm = -7, .value = RADIO_TXPOWER_TXPOWER_Neg7dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_Neg6dBm)
		{ .dbm = -6, .value = RADIO_TXPOWER_TXPOWER_Neg6dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_Neg5dBm)
		{ .dbm = -5, .value = RADIO_TXPOWER_TXPOWER_Neg5dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_Neg4dBm)
		{ .dbm = -4, .value = RADIO_TXPOWER_TXPOWER_Neg4dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_Neg3dBm)
		{ .dbm = -3, .value = RADIO_TXPOWER_TXPOWER_Neg3dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_Neg2dBm)
		{ .dbm = -2, .value = RADIO_TXPOWER_TXPOWER_Neg2dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_Neg1dBm)
		{ .dbm = -1, .value = RADIO_TXPOWER_TXPOWER_Neg1dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_0dBm)
		{ .dbm = 0, .value = RADIO_TXPOWER_TXPOWER_0dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_Pos1dBm)
		{ .dbm = 1, .value = RADIO_TXPOWER_TXPOWER_Pos1dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_Pos2dBm)
		{ .dbm = 2, .value = RADIO_TXPOWER_TXPOWER_Pos2dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_Pos3dBm)
		{ .dbm = 3, .value = RADIO_TXPOWER_TXPOWER_Pos3dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_Pos4dBm)
		{ .dbm = 4, .value = RADIO_TXPOWER_TXPOWER_Pos4dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_Pos5dBm)
		{ .dbm = 5, .value = RADIO_TXPOWER_TXPOWER_Pos5dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_Pos6dBm)
		{ .dbm = 6, .value = RADIO_TXPOWER_TXPOWER_Pos6dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_Pos7dBm)
		{ .dbm = 7, .value = RADIO_TXPOWER_TXPOWER_Pos7dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_Pos8dBm)
		{ .dbm = 8, .value = RADIO_TXPOWER_TXPOWER_Pos8dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_Pos9dBm)
		{ .dbm = 9, .value = RADIO_TXPOWER_TXPOWER_Pos9dBm },
		#endif
		#if defined(RADIO_TXPOWER_TXPOWER_Pos10dBm)
		{ .dbm = 10, .value = RADIO_TXPOWER_TXPOWER_Pos10dBm },
		#endif
	};
	uint32_t i = 0;

	while (true) {
		if (power_map[i].dbm >= dbm || i == ARRAY_SIZE(power_map) - 1) {
			if (actual_dbm != NULL) {
				*actual_dbm = power_map[i].dbm;
			}
			return power_map[i].value;
		}
		i++;
	}
}
