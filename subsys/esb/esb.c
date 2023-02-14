/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <errno.h>
#include <nrf.h>
#include <esb.h>
#include <stddef.h>
#include <string.h>
#include <nrf_erratas.h>

#include <hal/nrf_radio.h>
#include <hal/nrf_timer.h>
#include <nrfx_timer.h>

#ifdef NRF53_SERIES
#include "hal/nrf_vreqctrl.h"
#endif /* NRF53_SERIES */

#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <mpsl_fem_protocol_api.h>

#include "esb_peripherals.h"
#include "esb_ppi_api.h"

LOG_MODULE_REGISTER(esb, CONFIG_ESB_LOG_LEVEL);

/* Constants */

/* 2 Mb RX wait for acknowledgment time-out value.
 * Smallest reliable value: 160.
 */
#define RX_ACK_TIMEOUT_US_2MBPS 160
/* 1 Mb RX wait for acknowledgment time-out value. */
#define RX_ACK_TIMEOUT_US_1MBPS 300
/* 250 Kb RX wait for acknowledgment time-out value. */
#define RX_ACK_TIMEOUT_US_250KBPS 300
/* 1 Mb RX wait for acknowledgment time-out (combined with BLE). */
#define RX_ACK_TIMEOUT_US_1MBPS_BLE 300

/* Minimum retransmit time */
#define RETRANSMIT_DELAY_MIN 435

/* Radio Tx ramp-up time in microseconds. */
#define TX_RAMP_UP_TIME_US 129

/* Radio Rx ramp-up time in microseconds. */
#define RX_RAMP_UP_TIME_US 124

/* Interrupt flags */
/* Interrupt mask value for TX success. */
#define INT_TX_SUCCESS_MSK BIT(0)
/* Interrupt mask value for TX failure. */
#define INT_TX_FAILED_MSK BIT(1)
/* Interrupt mask value for RX_DR. */
#define INT_RX_DATA_RECEIVED_MSK BIT(2)

/* Mask value to signal updating BASE0 radio address. */
#define ADDR_UPDATE_MASK_BASE0  BIT(0)
/* Mask value to signal updating BASE1 radio address. */
#define ADDR_UPDATE_MASK_BASE1  BIT(1)
/* Mask value to signal updating radio prefixes. */
#define ADDR_UPDATE_MASK_PREFIX BIT(2)

/* Radio address event latency in microseconds. */
#define ADDR_EVENT_LATENCY_US (13)

 /* The maximum value for PID. */
#define PID_MAX 3

#define BIT_MASK_UINT_8(x) (0xFF >> (8 - (x)))

/* Radio base frequency. */
#define RADIO_BASE_FREQUENCY 2400UL

/* NRF5340 Radio high voltage gain. */
#define NRF5340_HIGH_VOLTAGE_GAIN 3

#define RADIO_SHORTS_COMMON                                                              \
	(NRF_RADIO_SHORT_READY_START_MASK | NRF_RADIO_SHORT_END_DISABLE_MASK |           \
	NRF_RADIO_SHORT_ADDRESS_RSSISTART_MASK | NRF_RADIO_SHORT_DISABLED_RSSISTOP_MASK)

/* Internal Enhanced ShockBurst module state. */
enum esb_state {
	ESB_STATE_IDLE,		/* Idle. */
	ESB_STATE_PTX_TX,       /* Transmitting without acknowledgment. */
	ESB_STATE_PTX_TX_ACK,   /* Transmitting with acknowledgment. */
	ESB_STATE_PTX_RX_ACK,   /* Transmitting with acknowledgment and
				 * reception of payload with the
				 * acknowledgment response.
				 */
	ESB_STATE_PRX,		/* Receiving packets without ACK. */
	ESB_STATE_PRX_SEND_ACK, /* Transmitting ACK in RX mode. */
};

/* Pipe info PID and CRC and acknowledgment payload. */
struct pipe_info {
	uint16_t crc;	  /* CRC of the last received packet.
			   * Used to detect retransmits.
			   */
	uint8_t pid;	  /* Packet ID of the last received packet
			   * Used to detect retransmits.
			   */
	bool ack_payload; /* State of the transmission of ACK payloads. */
};

/* Structure used by the PRX to organize ACK payloads for multiple pipes. */
struct payload_wrap {
	/* Pointer to the ACK payload. */
	struct esb_payload  *p_payload;
	/* Value used to determine if the current payload pointer is used. */
	bool in_use;
	/* Pointer to the next ACK payload queued on the same pipe. */
	struct payload_wrap *p_next;
};

/* First-in, first-out queue of payloads to be transmitted. */
struct payload_tx_fifo {
	 /* Payload queue */
	struct esb_payload *payload[CONFIG_ESB_TX_FIFO_SIZE];

	uint32_t back;	/* Back of the queue (last in). */
	uint32_t front;	/* Front of queue (first out). */
	uint32_t count;	/* Number of elements in the queue. */
};

/* First-in, first-out queue of received payloads. */
struct payload_rx_fifo {
	 /* Payload queue */
	struct esb_payload *payload[CONFIG_ESB_RX_FIFO_SIZE];

	uint32_t back;	/* Back of the queue (last in). */
	uint32_t front;	/* Front of queue (first out). */
	uint32_t count;	/* Number of elements in the queue. */
};

/* Fixed radio PDU header definition. */
struct esb_radio_fixed_pdu {
	/* Packet ID of the last received packet. Used to detect retransmits. */
	uint8_t pid:2;
	uint8_t rfu:6;
	uint8_t rfu1;
} __packed;

/* Dynamic length radio PDU header definition. */
struct esb_radio_dynamic_pdu {
	/* Payload length. */
	uint8_t length:6;
	uint8_t rfu0:2;

	/* Disable acknowledge. */
	uint8_t no_ack:1;

	/* Packet ID of the last received packet. Used to detect retransmits. */
	uint8_t pid:2;
	uint8_t rfu1:5;
} __packed;

/* Radio PDU header definition. */
union esb_radio_pdu_type {
	/* Fixed PDU header. */
	struct esb_radio_fixed_pdu fixed_pdu;

	/* Dynamic PDU header. */
	struct esb_radio_dynamic_pdu dpl_pdu;
} __packed;

/* Radio PDU definition. */
struct esb_radio_pdu {
	/* PDU header. */
	union esb_radio_pdu_type type;

	/* PDU data. */
	uint8_t data[];
} __packed;

/* Enhanced ShockBurst address.
 *
 * Enhanced ShockBurst addresses consist of a base address and a prefix
 * that is unique for each pipe. See @ref esb_addressing in the ESB user
 * guide for more information.
 */
struct esb_address {
	uint8_t base_addr_p0[4];	/* Base address for pipe 0, in big endian. */
	uint8_t base_addr_p1[4];   /* Base address for pipe 1-7, in big endian. */
	uint8_t pipe_prefixes[8];	/* Address prefix for pipe 0 to 7. */
	uint8_t num_pipes;		/* Number of pipes available. */
	uint8_t addr_length;	/* Length of the address plus the prefix. */
	uint8_t rx_pipes_enabled;	/* Bitfield for enabled pipes. */
	uint8_t rf_channel;        /* Channel to use (between 0 and 100). */
};

static nrfx_timer_t esb_timer = ESB_TIMER_INSTANCE;

static bool esb_initialized;
static struct esb_config esb_cfg;
static volatile enum esb_state esb_state = ESB_STATE_IDLE;

/* Default address configuration for ESB.
 * Roughly equal to the nRF24Lxx defaults, except for the number of pipes,
 * because more pipes are supported.
 */
__ALIGN(4)
static struct esb_address esb_addr = {
	.base_addr_p0 = {0xE7, 0xE7, 0xE7, 0xE7},
	.base_addr_p1 = {0xC2, 0xC2, 0xC2, 0xC2},
	.pipe_prefixes = {0xE7, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8},
	.addr_length = 5,
	.num_pipes = CONFIG_ESB_PIPE_COUNT,
	.rf_channel = 2,
	.rx_pipes_enabled = 0xFF
};

static esb_event_handler event_handler;
static struct esb_payload *current_payload;

/* FIFOs and buffers */
static struct payload_tx_fifo tx_fifo;
static struct payload_rx_fifo rx_fifo;

static uint8_t tx_payload_buffer[CONFIG_ESB_MAX_PAYLOAD_LENGTH +
				 sizeof(struct esb_radio_pdu)];
static uint8_t rx_payload_buffer[CONFIG_ESB_MAX_PAYLOAD_LENGTH +
				 sizeof(struct esb_radio_pdu)];

/* Random access buffer variables for ACK payload handling */
struct payload_wrap ack_pl_wrap[CONFIG_ESB_TX_FIFO_SIZE];
struct payload_wrap *ack_pl_wrap_pipe[CONFIG_ESB_PIPE_COUNT];

/* Run time variables */
static uint8_t pids[CONFIG_ESB_PIPE_COUNT];
static struct pipe_info rx_pipe_info[CONFIG_ESB_PIPE_COUNT];
static volatile uint32_t interrupt_flags;
static volatile uint32_t retransmits_remaining;
static volatile uint32_t last_tx_attempts;
static volatile uint32_t wait_for_ack_timeout_us;

static uint32_t radio_shorts_common = RADIO_SHORTS_COMMON;

static const mpsl_fem_event_t rx_event = {
	.type = MPSL_FEM_EVENT_TYPE_TIMER,
	.event.timer = {
		.p_timer_instance = ESB_NRF_TIMER_INSTANCE,
		.compare_channel_mask = (BIT(NRF_TIMER_CC_CHANNEL2) | BIT(NRF_TIMER_CC_CHANNEL3)),
		.counter_period = {
			.end = RX_RAMP_UP_TIME_US,
		},
	},
};

static const mpsl_fem_event_t tx_event = {
	.type = MPSL_FEM_EVENT_TYPE_TIMER,
	.event.timer = {
		.p_timer_instance = ESB_NRF_TIMER_INSTANCE,
		.compare_channel_mask = (BIT(NRF_TIMER_CC_CHANNEL2) | BIT(NRF_TIMER_CC_CHANNEL3)),
		.counter_period = {
			.end = TX_RAMP_UP_TIME_US,
		},
	},
};

static mpsl_fem_event_t tx_time_shifted = {
	.type = MPSL_FEM_EVENT_TYPE_TIMER,
	.event.timer = {
		.p_timer_instance = ESB_NRF_TIMER_INSTANCE,
		.compare_channel_mask = (BIT(NRF_TIMER_CC_CHANNEL2) | BIT(NRF_TIMER_CC_CHANNEL3)),
	},
};

static mpsl_fem_event_t disable_event = {
	.type = MPSL_FEM_EVENT_TYPE_GENERIC,
};

/* These function pointers are changed dynamically, depending on protocol
 * configuration and state. Note that they will be 0 initialized.
 */
static void (*on_radio_disabled)(void);
static void (*update_rf_payload_format)(uint32_t payload_length);

/*  The following functions are assigned to the function pointers above. */
static void on_radio_disabled_tx_noack(void);
static void on_radio_disabled_tx(void);
static void on_radio_disabled_tx_wait_for_ack(void);
static void on_radio_disabled_rx(void);
static void on_radio_disabled_rx_ack(void);

/*  Function to do bytewise bit-swap on an unsigned 32-bit value */
static uint32_t bytewise_bit_swap(const uint8_t *input)
{
#if __CORTEX_M == (0x04U)
	uint32_t inp = (*(uint32_t *)input);

	return sys_cpu_to_be32((uint32_t)__RBIT(inp));
#else
	uint32_t inp = sys_cpu_to_le32(*(uint32_t *)input);

	inp = (inp & 0xF0F0F0F0) >> 4 | (inp & 0x0F0F0F0F) << 4;
	inp = (inp & 0xCCCCCCCC) >> 2 | (inp & 0x33333333) << 2;
	inp = (inp & 0xAAAAAAAA) >> 1 | (inp & 0x55555555) << 1;
	return inp;
#endif
}

/* Convert a base address from nRF24L format to nRF5 format */
static uint32_t addr_conv(const uint8_t *addr)
{
	return __REV(bytewise_bit_swap(addr));
}

static inline void apply_errata143_workaround(void)
{
	/* Workaround for Errata 143
	 * Check if the most significant bytes of address 0 (including
	 * prefix) match those of another address. It's recommended to
	 * use a unique address 0 since this will avoid the 3dBm penalty
	 * incurred from the workaround.
	 */
	uint32_t base_address_mask =
		esb_addr.addr_length == 5 ? 0xFFFF0000 : 0xFF000000;

	/* Load the two addresses before comparing them to ensure
	 * defined ordering of volatile accesses.
	 */
	uint32_t addr0 = nrf_radio_base0_get(NRF_RADIO) & base_address_mask;
	uint32_t addr1 = nrf_radio_base1_get(NRF_RADIO) & base_address_mask;

	if (addr0 == addr1) {
		uint32_t radio_prefix0 = nrf_radio_prefix0_get(NRF_RADIO);
		uint32_t radio_prefix1 = nrf_radio_prefix1_get(NRF_RADIO);

		uint8_t prefix0 = radio_prefix0 & RADIO_PREFIX0_AP0_Msk;
		uint8_t prefix1 = (radio_prefix0 & RADIO_PREFIX0_AP1_Msk) >> RADIO_PREFIX0_AP1_Pos;
		uint8_t prefix2 = (radio_prefix0 & RADIO_PREFIX0_AP2_Msk) >> RADIO_PREFIX0_AP2_Pos;
		uint8_t prefix3 = (radio_prefix0 & RADIO_PREFIX0_AP3_Msk) >> RADIO_PREFIX0_AP3_Pos;
		uint8_t prefix4 = radio_prefix1 & RADIO_PREFIX1_AP4_Msk;
		uint8_t prefix5 = (radio_prefix1 & RADIO_PREFIX1_AP5_Msk) >> RADIO_PREFIX1_AP5_Pos;
		uint8_t prefix6 = (radio_prefix1 & RADIO_PREFIX1_AP6_Msk) >> RADIO_PREFIX1_AP6_Pos;
		uint8_t prefix7 = (radio_prefix1 & RADIO_PREFIX1_AP7_Msk) >> RADIO_PREFIX1_AP7_Pos;

		if ((prefix0 == prefix1) || (prefix0 == prefix2) ||
		    (prefix0 == prefix3) || (prefix0 == prefix4) ||
		    (prefix0 == prefix5) || (prefix0 == prefix6) ||
		    (prefix0 == prefix7)) {
			/* This will cause a 3dBm sensitivity loss,
			 * avoid using such address combinations if possible.
			 */
			*(volatile uint32_t *)0x40001774 =
				((*(volatile uint32_t *)0x40001774) & 0xfffffffe) | 0x01000000;
		}
	}
}

static void esb_fem_for_tx_set(bool ack)
{
	uint32_t timer_shorts;

	timer_shorts = NRF_TIMER_SHORT_COMPARE2_STOP_MASK;

	if (ack) {
		/* In case of expecting ACK reception, clear the timer to start counting from 0
		 * on the RADIO_DISABLED event. This is needed to start external front-end module
		 * with valid timeout because radio uses DISABLED_RXEN short.
		 */
		timer_shorts |= NRF_TIMER_SHORT_COMPARE2_CLEAR_MASK;
	}

	if (mpsl_fem_pa_configuration_set(&tx_event, &disable_event) == 0) {
		esb_ppi_for_fem_set();
	} else {
		/* We want to start counting ACK timeout and potential packet retransmission from
		 * RADIO_DISABLED event so timer starts through EGU together with radio ramp-up,
		 * we want to stop and clear it before RADIO_DISABLED event to start it again
		 * when this event occurs. The timer value must be big enough to give us possibility
		 * to reconfigure timer shorts before timer will be cleared by them.
		 */
		nrf_timer_cc_set(esb_timer.p_reg, NRF_TIMER_CC_CHANNEL2, TX_RAMP_UP_TIME_US);
	}

	nrf_timer_shorts_set(esb_timer.p_reg, timer_shorts);
}

static void esb_fem_for_rx_set(void)
{
	if (mpsl_fem_lna_configuration_set(&rx_event, &disable_event) == 0) {
		esb_ppi_for_fem_set();
		nrf_timer_shorts_set(esb_timer.p_reg,
			(NRF_TIMER_SHORT_COMPARE2_CLEAR_MASK | NRF_TIMER_SHORT_COMPARE2_STOP_MASK));
	}
}

static void esb_fem_for_ack_rx(void)
{
	/* Timer is running and timer's shorts and PPI connections have been configured. */
	mpsl_fem_pa_configuration_clear();
	mpsl_fem_lna_configuration_set(&rx_event, &disable_event);
}

static void esb_fem_for_tx_ack(void)
{
	/* Timer is running and timer's shorts and PPI connections have been configured. */
	mpsl_fem_lna_configuration_clear();
	mpsl_fem_pa_configuration_set(&tx_event, &disable_event);
}

static void esb_fem_reset(void)
{
	nrf_timer_task_trigger(esb_timer.p_reg, NRF_TIMER_TASK_SHUTDOWN);

	mpsl_fem_lna_configuration_clear();
	mpsl_fem_pa_configuration_clear();

	esb_ppi_for_fem_clear();

	mpsl_fem_deactivate_now(MPSL_FEM_ALL);
	mpsl_fem_disable();
}

static void esb_fem_lna_reset(void)
{
	nrf_timer_task_trigger(esb_timer.p_reg, NRF_TIMER_TASK_SHUTDOWN);

	esb_ppi_for_fem_clear();

	mpsl_fem_lna_configuration_clear();
	mpsl_fem_disable();
}

static void esb_fem_pa_reset(void)
{
	mpsl_fem_pa_configuration_clear();

	nrf_timer_task_trigger(esb_timer.p_reg, NRF_TIMER_TASK_SHUTDOWN);
	esb_ppi_for_fem_clear();

	mpsl_fem_disable();
}

void esb_fem_for_rx_ack(void)
{
	mpsl_fem_pa_configuration_clear();
	mpsl_fem_lna_configuration_set(&rx_event, &disable_event);

	/* The timer is running, shorts must be reconfigured because we do not want to
	 * stop timer after front-end module was triggered. Timer needs to count ACK reception
	 * timeout and potential retransmission delay. Timer cannot be stopped here because
	 * these timeout are counted since RADIO_DISABLED event.
	 */
	nrf_timer_shorts_disable(esb_timer.p_reg,
			(NRF_TIMER_SHORT_COMPARE2_CLEAR_MASK | NRF_TIMER_SHORT_COMPARE2_STOP_MASK));
}

void esb_fem_for_tx_retry(void)
{
	/* The radio is ramped-up with delay set in the timer compare channel 1.
	 * Calculate the ramp-up time for external front-end module.
	 */
	tx_time_shifted.event.timer.counter_period.end =
		nrf_timer_cc_get(esb_timer.p_reg, NRF_TIMER_CC_CHANNEL1) + TX_RAMP_UP_TIME_US;

	/* This starts the ESB TIMER on the radio disabled event. This connection is needed even
	 * when front-end module is not used.
	 */
	esb_ppi_for_fem_set();

	if (mpsl_fem_pa_configuration_set(&tx_time_shifted, &disable_event) == 0) {
		/* In case of retransmission timer is configured to trigger RADIO_TXEN task after
		 * retransmission timeout, but with external front-end module, it must also
		 * schedule front-end module ramp-up which occurs later. Shorts need to be
		 * reconfigured here to stop and clear the timer after front-end module will be
		 * ramped-up.
		 */
		nrf_timer_shorts_set(esb_timer.p_reg,
			(NRF_TIMER_SHORT_COMPARE2_CLEAR_MASK | NRF_TIMER_SHORT_COMPARE2_STOP_MASK));
	}
}

void esb_fem_for_tx_retry_clear(void)
{
	esb_ppi_for_fem_clear();

	mpsl_fem_pa_configuration_clear();
	mpsl_fem_deactivate_now(MPSL_FEM_ALL);

	nrf_timer_shorts_disable(esb_timer.p_reg,
			(NRF_TIMER_SHORT_COMPARE2_CLEAR_MASK | NRF_TIMER_SHORT_COMPARE2_STOP_MASK));
}

static void radio_start(void)
{
	/* Event generator unit is used to start radio and protocol timer if needed. */
	nrf_egu_task_trigger(ESB_EGU, ESB_EGU_TASK);
}

static void update_rf_payload_format_esb_dpl(uint32_t payload_length)
{
	nrf_radio_packet_conf_t packet_config = { 0 };

	packet_config.s0len = 0;
	packet_config.s1len = 3;

	/* Using 6 bits or 8 bits for length */
	packet_config.lflen = (CONFIG_ESB_MAX_PAYLOAD_LENGTH <= 32) ? 6 : 8;
	packet_config.whiteen = false;
	packet_config.big_endian = true;
	packet_config.balen = (esb_addr.addr_length - 1);
	packet_config.statlen = 0;
	packet_config.maxlen = CONFIG_ESB_MAX_PAYLOAD_LENGTH;

	nrf_radio_packet_configure(NRF_RADIO, &packet_config);
}

static void update_rf_payload_format_esb(uint32_t payload_length)
{
	const nrf_radio_packet_conf_t packet_config = {
		.s0len = 1,
		.lflen = 0,
		.s1len = 1,
		.whiteen = false,
		.big_endian = true,
		.balen = (esb_addr.addr_length - 1),
		.statlen = payload_length,
		.maxlen = payload_length
	};

	nrf_radio_packet_configure(NRF_RADIO, &packet_config);
}

static void update_radio_addresses(uint8_t update_mask)
{
	if ((update_mask & ADDR_UPDATE_MASK_BASE0) != 0) {
		nrf_radio_base0_set(NRF_RADIO, addr_conv(esb_addr.base_addr_p0));
	}

	if ((update_mask & ADDR_UPDATE_MASK_BASE1) != 0) {
		nrf_radio_base1_set(NRF_RADIO, addr_conv(esb_addr.base_addr_p1));
	}

	if ((update_mask & ADDR_UPDATE_MASK_PREFIX) != 0) {
		nrf_radio_prefix0_set(NRF_RADIO, bytewise_bit_swap(&esb_addr.pipe_prefixes[0]));
		nrf_radio_prefix1_set(NRF_RADIO, bytewise_bit_swap(&esb_addr.pipe_prefixes[4]));
	}

	/* Workaround for Errata 143 */
#if NRF52_ERRATA_143_ENABLE_WORKAROUND
	if (nrf52_errata_143()) {
		apply_errata143_workaround();
	}
#endif
}

static void update_radio_tx_power(void)
{
	int32_t err;
	mpsl_tx_power_split_t tx_power;

	(void)mpsl_fem_tx_power_split(esb_cfg.tx_output_power, &tx_power,
				      (RADIO_BASE_FREQUENCY + esb_addr.rf_channel), false);

	err = mpsl_fem_pa_gain_set(&tx_power.fem);
	if (err) {
		/* Should not happen. */
		__ASSERT_NO_MSG(false);
	}

#ifdef NRF53_SERIES
	bool high_voltage = false;

	if (tx_power.radio_tx_power > 0) {
		high_voltage = true;
		tx_power.radio_tx_power -= NRF5340_HIGH_VOLTAGE_GAIN;
	}

	nrf_vreqctrl_radio_high_voltage_set(NRF_VREQCTRL_NS, high_voltage);
#endif /* NRF53_SERIES */

	nrf_radio_txpower_set(NRF_RADIO, tx_power.radio_tx_power);
}

static bool update_radio_bitrate(void)
{
	nrf_radio_mode_set(NRF_RADIO, esb_cfg.bitrate);

	switch (esb_cfg.bitrate) {
	case ESB_BITRATE_2MBPS:

#if defined(RADIO_MODE_MODE_Ble_2Mbit)
	case ESB_BITRATE_2MBPS_BLE:
#endif /* defined(RADIO_MODE_MODE_Ble_2Mbit) */

		wait_for_ack_timeout_us = RX_ACK_TIMEOUT_US_2MBPS;
		break;

	case ESB_BITRATE_1MBPS:
		wait_for_ack_timeout_us = RX_ACK_TIMEOUT_US_1MBPS;
		break;

#if defined(RADIO_MODE_MODE_Nrf_250Kbit)
	case ESB_BITRATE_250KBPS:
		wait_for_ack_timeout_us = RX_ACK_TIMEOUT_US_250KBPS;
		break;
#endif /* defined(RADIO_MODE_MODE_Nrf_250Kbit) */

	case ESB_BITRATE_1MBPS_BLE:
		wait_for_ack_timeout_us = RX_ACK_TIMEOUT_US_1MBPS_BLE;
		break;

	default:
		/* Should not be reached */
		return false;
	}

	return true;
}

static bool update_radio_protocol(void)
{
	switch (esb_cfg.protocol) {
	case ESB_PROTOCOL_ESB_DPL:
		update_rf_payload_format = update_rf_payload_format_esb_dpl;
		break;

	case ESB_PROTOCOL_ESB:
		update_rf_payload_format = update_rf_payload_format_esb;
		break;

	default:
		/* Should not be reached */
		return false;
	}
	return true;
}

static bool update_radio_crc(void)
{
	switch (esb_cfg.crc) {
	case ESB_CRC_16BIT:
		nrf_radio_crcinit_set(NRF_RADIO, 0xFFFFUL); /* Initial value */
		nrf_radio_crc_configure(NRF_RADIO, ESB_CRC_16BIT, NRF_RADIO_CRC_ADDR_INCLUDE,
					0x11021UL); /* CRC poly: x^16+x^12^x^5+1 */
		break;

	case ESB_CRC_8BIT:
		nrf_radio_crcinit_set(NRF_RADIO, 0xFFUL); /* Initial value */
		nrf_radio_crc_configure(NRF_RADIO, ESB_CRC_8BIT, NRF_RADIO_CRC_ADDR_INCLUDE,
					0x107UL); /* CRC poly: x^8+x^2^x^1+1 */
		break;

	case ESB_CRC_OFF:
		nrf_radio_crcinit_set(NRF_RADIO, 0x00UL);
		nrf_radio_crc_configure(NRF_RADIO, ESB_CRC_OFF, NRF_RADIO_CRC_ADDR_INCLUDE, 0x00UL);

		break;

	default:
		return false;
	}

	return true;
}

static bool update_radio_parameters(void)
{
	bool params_valid = true;

	params_valid &= update_radio_bitrate();
	params_valid &= update_radio_protocol();
	params_valid &= update_radio_crc();
	update_rf_payload_format(esb_cfg.payload_length);
	params_valid &=
	    (esb_cfg.retransmit_delay >= RETRANSMIT_DELAY_MIN);

	return params_valid;
}

static void reset_fifos(void)
{
	tx_fifo.back = 0;
	tx_fifo.front = 0;
	tx_fifo.count = 0;

	rx_fifo.back = 0;
	rx_fifo.front = 0;
	rx_fifo.count = 0;
}

static void initialize_fifos(void)
{
	static struct esb_payload rx_payload[CONFIG_ESB_RX_FIFO_SIZE];
	static struct esb_payload tx_payload[CONFIG_ESB_TX_FIFO_SIZE];

	reset_fifos();

	for (size_t i = 0; i < CONFIG_ESB_TX_FIFO_SIZE; i++) {
		tx_fifo.payload[i] = &tx_payload[i];
	}

	for (size_t i = 0; i < CONFIG_ESB_RX_FIFO_SIZE; i++) {
		rx_fifo.payload[i] = &rx_payload[i];
	}

	for (size_t i = 0; i < CONFIG_ESB_TX_FIFO_SIZE; i++) {
		ack_pl_wrap[i].p_payload = &tx_payload[i];
		ack_pl_wrap[i].in_use = false;
		ack_pl_wrap[i].p_next = 0;
	}

	for (size_t i = 0; i < CONFIG_ESB_PIPE_COUNT; i++) {
		ack_pl_wrap_pipe[i] = 0;
	}
}

static void tx_fifo_remove_last(void)
{
	if (tx_fifo.count == 0) {
		return;
	}

	unsigned int key = irq_lock();

	tx_fifo.count--;
	if (++tx_fifo.front >= CONFIG_ESB_TX_FIFO_SIZE) {
		tx_fifo.front = 0;
	}

	irq_unlock(key);
}

/*  Function to push the content of the rx_buffer to the RX FIFO.
 *
 *  The module will point the register NRF_RADIO->PACKETPTR to a buffer for
 *  receiving packets. After receiving a packet the module will call this
 *  function to copy the received data to the RX FIFO.
 *
 *  @param  pipe Pipe number to set for the packet.
 *  @param  pid  Packet ID.
 *
 *  @retval true   Operation successful.
 *  @retval false  Operation failed.
 */
static bool rx_fifo_push_rfbuf(uint8_t pipe, uint8_t pid)
{
	struct esb_radio_pdu *rx_pdu = (struct esb_radio_pdu *)rx_payload_buffer;

	if (rx_fifo.count >= CONFIG_ESB_RX_FIFO_SIZE) {
		return false;
	}

	if (esb_cfg.protocol == ESB_PROTOCOL_ESB_DPL) {
		if (rx_pdu->type.dpl_pdu.length > CONFIG_ESB_MAX_PAYLOAD_LENGTH) {
			return false;
		}

		rx_fifo.payload[rx_fifo.back]->length = rx_pdu->type.dpl_pdu.length;
	} else if (esb_cfg.mode == ESB_MODE_PTX) {
		/* Received packet is an acknowledgment */
		rx_fifo.payload[rx_fifo.back]->length = 0;
	} else {
		rx_fifo.payload[rx_fifo.back]->length = esb_cfg.payload_length;
	}

	memcpy(rx_fifo.payload[rx_fifo.back]->data, rx_pdu->data,
	       rx_fifo.payload[rx_fifo.back]->length);

	rx_fifo.payload[rx_fifo.back]->pipe = pipe;
	rx_fifo.payload[rx_fifo.back]->rssi = nrf_radio_rssi_sample_get(NRF_RADIO);
	rx_fifo.payload[rx_fifo.back]->pid = pid;
	rx_fifo.payload[rx_fifo.back]->noack = !rx_pdu->type.dpl_pdu.no_ack;

	if (++rx_fifo.back >= CONFIG_ESB_RX_FIFO_SIZE) {
		rx_fifo.back = 0;
	}
	rx_fifo.count++;

	return true;
}

static void esb_timer_handler(nrf_timer_event_t event_type, void *context)
{

}

static int sys_timer_init(void)
{
	nrfx_err_t nrfx_err;
	const nrfx_timer_config_t config = {
		.frequency = NRF_TIMER_FREQ_1MHz,
		.mode = NRF_TIMER_MODE_TIMER,
		.bit_width = NRF_TIMER_BIT_WIDTH_16,
	};

	nrfx_err = nrfx_timer_init(&esb_timer, &config, esb_timer_handler);
	if (nrfx_err != NRFX_SUCCESS) {
		LOG_ERR("Failed to initialize nrfx timer (err %d)", nrfx_err);
		return -EFAULT;
	}

	return 0;
}

static void sys_timer_deinit(void)
{
	nrfx_timer_uninit(&esb_timer);
}

static void start_tx_transaction(void)
{
	bool ack = true;
	struct esb_radio_pdu *pdu = (struct esb_radio_pdu *)tx_payload_buffer;
	last_tx_attempts = 1;
	/* Prepare the payload */
	current_payload = tx_fifo.payload[tx_fifo.front];

	switch (esb_cfg.protocol) {
	case ESB_PROTOCOL_ESB:
		memset(&pdu->type.fixed_pdu, 0, sizeof(pdu->type.fixed_pdu));
		update_rf_payload_format(current_payload->length);

		pdu->type.fixed_pdu.pid = current_payload->pid;

		memcpy(pdu->data, current_payload->data, current_payload->length);

		nrf_radio_shorts_set(NRF_RADIO,
				     (radio_shorts_common | NRF_RADIO_SHORT_DISABLED_RXEN_MASK));
		nrf_radio_int_enable(NRF_RADIO, NRF_RADIO_INT_DISABLED_MASK);

		/* Configure the retransmit counter */
		retransmits_remaining = esb_cfg.retransmit_count;
		on_radio_disabled = on_radio_disabled_tx;
		esb_state = ESB_STATE_PTX_TX_ACK;
		break;

	case ESB_PROTOCOL_ESB_DPL:
		memset(&pdu->type.dpl_pdu, 0, sizeof(pdu->type.dpl_pdu));
		ack = !current_payload->noack || !esb_cfg.selective_auto_ack;

		pdu->type.dpl_pdu.length = current_payload->length;
		pdu->type.dpl_pdu.pid = current_payload->pid;
		pdu->type.dpl_pdu.no_ack = current_payload->noack ? 0x00 : 0x01;

		memcpy(pdu->data, current_payload->data, current_payload->length);

		/* Handling ack if noack is set to false or if
		 * selective auto ack is turned off
		 */
		if (ack) {
			nrf_radio_shorts_set(NRF_RADIO,
				(radio_shorts_common | NRF_RADIO_SHORT_DISABLED_RXEN_MASK));

			/* Configure the retransmit counter */
			retransmits_remaining = esb_cfg.retransmit_count;
			on_radio_disabled = on_radio_disabled_tx;
			esb_state = ESB_STATE_PTX_TX_ACK;
		} else {
			nrf_radio_shorts_set(NRF_RADIO, radio_shorts_common);

			on_radio_disabled = on_radio_disabled_tx_noack;
			esb_state = ESB_STATE_PTX_TX;
		}

		nrf_radio_int_enable(NRF_RADIO, NRF_RADIO_INT_DISABLED_MASK);

		break;

	default:
		/* Should not be reached */
		break;
	}

	nrf_radio_txaddress_set(NRF_RADIO, current_payload->pipe);
	nrf_radio_rxaddresses_set(NRF_RADIO, BIT(current_payload->pipe));
	nrf_radio_frequency_set(NRF_RADIO, (RADIO_BASE_FREQUENCY + esb_addr.rf_channel));

	update_radio_tx_power();

	nrf_radio_packetptr_set(NRF_RADIO, pdu);

	NVIC_ClearPendingIRQ(RADIO_IRQn);
	irq_enable(RADIO_IRQn);

	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_ADDRESS);
	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_PAYLOAD);
	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_DISABLED);

	esb_ppi_for_txrx_set(false, ack);
	esb_fem_for_tx_set(ack);

	radio_start();
}

static void on_radio_disabled_tx_noack(void)
{
	esb_fem_pa_reset();
	esb_ppi_for_txrx_clear(false, false);

	interrupt_flags |= INT_TX_SUCCESS_MSK;
	tx_fifo_remove_last();

	if (tx_fifo.count == 0) {
		esb_state = ESB_STATE_IDLE;
		NVIC_SetPendingIRQ(ESB_EVT_IRQ);
	} else {
		NVIC_SetPendingIRQ(ESB_EVT_IRQ);
		start_tx_transaction();
	}
}

static void on_radio_disabled_tx(void)
{
	esb_ppi_for_txrx_clear(false, true);
	/* The timer was triggered on radio disabled event so we can clear PPI connections here. */
	esb_ppi_for_fem_clear();
	esb_fem_for_rx_ack();

	/* Remove the DISABLED -> RXEN shortcut, to make sure the radio stays
	 * disabled after the RX window
	 */
	nrf_radio_shorts_set(NRF_RADIO, radio_shorts_common);

	/* Make sure the timer is started the next time the radio is ready,
	 * and that it will disable the radio automatically if no packet is
	 * received by the time defined in wait_for_ack_timeout_us
	 */

	nrfx_timer_compare(&esb_timer, NRF_TIMER_CC_CHANNEL0,
			   (wait_for_ack_timeout_us + ADDR_EVENT_LATENCY_US), false);
	nrfx_timer_compare(&esb_timer, NRF_TIMER_CC_CHANNEL1,
			   (esb_cfg.retransmit_delay - TX_RAMP_UP_TIME_US), false);

	nrf_timer_shorts_set(esb_timer.p_reg,
		(NRF_TIMER_SHORT_COMPARE1_STOP_MASK | NRF_TIMER_SHORT_COMPARE1_CLEAR_MASK));

	nrf_timer_event_clear(esb_timer.p_reg, NRF_TIMER_EVENT_COMPARE0);
	nrf_timer_event_clear(esb_timer.p_reg, NRF_TIMER_EVENT_COMPARE1);

	esb_ppi_for_wait_for_ack_set();
	esb_ppi_for_retransmission_clear();

	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_END);

	if (esb_cfg.protocol == ESB_PROTOCOL_ESB) {
		update_rf_payload_format(0);
	}

	nrf_radio_packetptr_set(NRF_RADIO, rx_payload_buffer);
	on_radio_disabled = on_radio_disabled_tx_wait_for_ack;
	esb_state = ESB_STATE_PTX_RX_ACK;
}

static void on_radio_disabled_tx_wait_for_ack(void)
{
	struct esb_radio_pdu *rx_pdu = (struct esb_radio_pdu *)rx_payload_buffer;
	/* This marks the completion of a TX_RX sequence (TX with ACK) */

	/* Make sure the timer will not deactivate the radio if a packet is
	 * received.
	 */
	esb_ppi_for_wait_for_ack_clear();

	/* Just clear LNA configuration and disable front-end module. */
	mpsl_fem_lna_configuration_clear();
	mpsl_fem_disable();

	/* If the radio has received a packet and the CRC status is OK */
	if (nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_END) &&
	    nrf_radio_crc_status_check(NRF_RADIO)) {
		interrupt_flags |= INT_TX_SUCCESS_MSK;
		last_tx_attempts = esb_cfg.retransmit_count - retransmits_remaining + 1;

		tx_fifo_remove_last();

		if ((esb_cfg.protocol != ESB_PROTOCOL_ESB) && (rx_pdu->type.dpl_pdu.length > 0)) {
			if (rx_fifo_push_rfbuf(
				nrf_radio_txaddress_get(NRF_RADIO), rx_pdu->type.dpl_pdu.pid)) {
				interrupt_flags |= INT_RX_DATA_RECEIVED_MSK;
			}
		}

		if ((tx_fifo.count == 0) || (esb_cfg.tx_mode == ESB_TXMODE_MANUAL)) {
			esb_state = ESB_STATE_IDLE;
			NVIC_SetPendingIRQ(ESB_EVT_IRQ);
		} else {
			NVIC_SetPendingIRQ(ESB_EVT_IRQ);
			start_tx_transaction();
		}
	} else {
		if (retransmits_remaining-- == 0) {
			nrf_timer_task_trigger(esb_timer.p_reg, NRF_TIMER_TASK_SHUTDOWN);

			/* All retransmits are expended, and the TX operation is
			 * suspended
			 */
			last_tx_attempts = esb_cfg.retransmit_count + 1;
			interrupt_flags |= INT_TX_FAILED_MSK;

			esb_state = ESB_STATE_IDLE;
			NVIC_SetPendingIRQ(ESB_EVT_IRQ);
		} else {
			bool radio_started = true;

			nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_READY);

			/* There are still more retransmits left, TX mode should
			 * be entered again as soon as the system timer reaches
			 * CC[1].
			 */
			nrf_radio_shorts_set(NRF_RADIO,
				(radio_shorts_common | NRF_RADIO_SHORT_DISABLED_RXEN_MASK));
			update_rf_payload_format(current_payload->length);

			nrf_radio_packetptr_set(NRF_RADIO, tx_payload_buffer);

			on_radio_disabled = on_radio_disabled_tx;
			esb_state = ESB_STATE_PTX_TX_ACK;

			update_radio_tx_power();

			/* Transmission is armed on TIMER's CC1. */
			esb_fem_for_tx_retry();
			esb_ppi_for_retransmission_set();

			/* Check if PPI worked. If not we are to late with retransmission but it is
			 * ok to start retransmission here.
			 */
			if (nrf_timer_event_check(esb_timer.p_reg, NRF_TIMER_EVENT_COMPARE1)) {
				radio_started =
					(nrf_radio_state_get(NRF_RADIO) == NRF_RADIO_STATE_TXRU) ||
					(nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_READY));
			} else {
				/* Resume timer in case of CRC errors. */
				nrf_timer_task_trigger(esb_timer.p_reg, NRF_TIMER_TASK_START);
			}

			if (!radio_started) {
				/* Clear retransmission configuration. */
				esb_fem_for_tx_retry_clear();
				esb_ppi_for_retransmission_clear();

				/* Start radio here. */
				esb_ppi_for_txrx_set(false, true);
				esb_fem_for_tx_set(true);

				radio_start();
			}
		}
	}
}

static void clear_events_restart_rx(void)
{
	esb_fem_lna_reset();
	esb_ppi_for_txrx_clear(true, false);

	nrf_radio_shorts_set(NRF_RADIO, radio_shorts_common);

	update_rf_payload_format(esb_cfg.payload_length);

	nrf_radio_packetptr_set(NRF_RADIO, rx_payload_buffer);

	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_DISABLED);
	nrf_radio_task_trigger(NRF_RADIO, NRF_RADIO_TASK_DISABLE);

	while (!nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_DISABLED)) {
		/* wait for register to settle */
	}

	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_DISABLED);

	nrf_radio_shorts_set(NRF_RADIO, (radio_shorts_common | NRF_RADIO_SHORT_DISABLED_TXEN_MASK));

	esb_ppi_for_txrx_set(true, false);
	esb_fem_for_rx_set();

	radio_start();
}

static void on_radio_disabled_rx_dpl(bool retransmit_payload,
				     struct pipe_info *pipe_info)
{
	struct esb_radio_pdu *tx_pdu = (struct esb_radio_pdu *)tx_payload_buffer;
	struct esb_radio_pdu *rx_pdu = (struct esb_radio_pdu *)rx_payload_buffer;

	uint32_t pipe = nrf_radio_rxmatch_get(NRF_RADIO);

	if (tx_fifo.count > 0 && ack_pl_wrap_pipe[pipe] != 0) {
		current_payload = ack_pl_wrap_pipe[pipe]->p_payload;

		/* Pipe stays in ACK with payload until TX FIFO is empty */
		/* Do not report TX success on first ack payload or retransmit */
		if (pipe_info->ack_payload == true && !retransmit_payload) {
			ack_pl_wrap_pipe[pipe]->in_use = false;
			ack_pl_wrap_pipe[pipe] = ack_pl_wrap_pipe[pipe]->p_next;
			tx_fifo.count--;
			if (tx_fifo.count > 0 && ack_pl_wrap_pipe[pipe] != 0) {
				current_payload = ack_pl_wrap_pipe[pipe]->p_payload;
			} else {
				current_payload = 0;
			}

			/* ACK payloads also require TX_DS */
			/* (page 40 of the 'nRF24LE1_Product_Specification_rev1_6.pdf') */
			interrupt_flags |= INT_TX_SUCCESS_MSK;
		}

		if (current_payload != 0) {
			pipe_info->ack_payload = true;
			update_rf_payload_format(current_payload->length);

			tx_pdu->type.dpl_pdu.length = current_payload->length;
			memcpy(tx_pdu->data, current_payload->data, current_payload->length);
		} else {
			pipe_info->ack_payload = false;
			update_rf_payload_format(0);
			tx_pdu->type.dpl_pdu.length = 0;
		}
	} else {
		pipe_info->ack_payload = false;
		update_rf_payload_format(0);
		tx_pdu->type.dpl_pdu.length = 0;
	}

	tx_pdu->type.dpl_pdu.pid = rx_pdu->type.dpl_pdu.pid;
	tx_pdu->type.dpl_pdu.no_ack = rx_pdu->type.dpl_pdu.no_ack;
}

static void on_radio_disabled_rx(void)
{
	bool retransmit_payload = false;
	bool send_rx_event = true;
	struct pipe_info *pipe_info;
	struct esb_radio_pdu *rx_pdu = (struct esb_radio_pdu *)rx_payload_buffer;
	struct esb_radio_pdu *tx_pdu = (struct esb_radio_pdu *)tx_payload_buffer;

	if (!nrf_radio_crc_status_check(NRF_RADIO)) {
		clear_events_restart_rx();
		return;
	}

	if (rx_fifo.count >= CONFIG_ESB_RX_FIFO_SIZE) {
		clear_events_restart_rx();
		return;
	}

	pipe_info = &rx_pipe_info[nrf_radio_rxmatch_get(NRF_RADIO)];

	if ((nrf_radio_rxcrc_get(NRF_RADIO) == pipe_info->crc) &&
	    (rx_pdu->type.dpl_pdu.pid) == pipe_info->pid) {
		retransmit_payload = true;
		send_rx_event = false;
	}

	pipe_info->pid = rx_pdu->type.dpl_pdu.pid;
	pipe_info->crc = nrf_radio_rxcrc_get(NRF_RADIO);

	/* Check if an ack should be sent */
	if ((esb_cfg.selective_auto_ack == false) || rx_pdu->type.dpl_pdu.no_ack) {
		esb_fem_for_tx_ack();

		nrf_radio_shorts_set(NRF_RADIO,
				     (radio_shorts_common | NRF_RADIO_SHORT_DISABLED_RXEN_MASK));

		switch (esb_cfg.protocol) {
		case ESB_PROTOCOL_ESB_DPL:
			on_radio_disabled_rx_dpl(retransmit_payload, pipe_info);
			break;

		case ESB_PROTOCOL_ESB:
			update_rf_payload_format(0);

			tx_pdu->type.fixed_pdu.pid = rx_pdu->type.fixed_pdu.pid;
			tx_pdu->type.fixed_pdu.rfu1 = 0;

			break;
		}

		esb_state = ESB_STATE_PRX_SEND_ACK;

		update_radio_tx_power();

		nrf_radio_txaddress_set(NRF_RADIO, nrf_radio_rxmatch_get(NRF_RADIO));
		nrf_radio_packetptr_set(NRF_RADIO, tx_pdu);

		on_radio_disabled = on_radio_disabled_rx_ack;
	} else {
		clear_events_restart_rx();
	}

	if (send_rx_event) {
		/* Push the new packet to the RX buffer and trigger a received
		 * event if the operation was
		 * successful.
		 */
		if (rx_fifo_push_rfbuf(nrf_radio_rxmatch_get(NRF_RADIO), pipe_info->pid)) {
			interrupt_flags |= INT_RX_DATA_RECEIVED_MSK;
			NVIC_SetPendingIRQ(ESB_EVT_IRQ);
		}
	}
}

static void on_radio_disabled_rx_ack(void)
{
	esb_fem_for_ack_rx();

	nrf_radio_shorts_set(NRF_RADIO, (radio_shorts_common | NRF_RADIO_SHORT_DISABLED_TXEN_MASK));
	update_rf_payload_format(esb_cfg.payload_length);

	nrf_radio_packetptr_set(NRF_RADIO, rx_payload_buffer);
	on_radio_disabled = on_radio_disabled_rx;

	esb_state = ESB_STATE_PRX;
}

/* Retrieve interrupt flags and reset them.
 *
 * @param[out] interrupts	Interrupt flags.
 */
static void get_and_clear_irqs(uint32_t *interrupts)
{
	__ASSERT_NO_MSG(interrupts != NULL);

	unsigned int key = irq_lock();

	*interrupts = interrupt_flags;
	interrupt_flags = 0;

	irq_unlock(key);
}

static void radio_irq_handler(void)
{
	if (nrf_radio_int_enable_check(NRF_RADIO, NRF_RADIO_INT_DISABLED_MASK) &&
	    nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_DISABLED)) {
		nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_DISABLED);
		/* Call the correct on_radio_disable function, depending on the
		 * current protocol state.
		 */
		if (on_radio_disabled) {
			on_radio_disabled();
		}
	}
}

static void esb_evt_irq_handler(void)
{
	uint32_t interrupts;
	struct esb_evt event;

	event.tx_attempts = last_tx_attempts;

	get_and_clear_irqs(&interrupts);
	if (event_handler != NULL) {
		if (interrupts & INT_TX_SUCCESS_MSK) {
			event.evt_id = ESB_EVENT_TX_SUCCESS;
			event_handler(&event);
		}
		if (interrupts & INT_TX_FAILED_MSK) {
			event.evt_id = ESB_EVENT_TX_FAILED;
			event_handler(&event);
		}
		if (interrupts & INT_RX_DATA_RECEIVED_MSK) {
			event.evt_id = ESB_EVENT_RX_RECEIVED;
			event_handler(&event);
		}
	}
}

#if IS_ENABLED(CONFIG_ESB_DYNAMIC_INTERRUPTS)

void RADIO_IRQHandler(const void *args)
{
	ARG_UNUSED(args);
	radio_irq_handler();
	ISR_DIRECT_PM();
}

void ESB_EVT_IRQHandler(const void *args)
{
	ARG_UNUSED(args);
	esb_evt_irq_handler();
	ISR_DIRECT_PM();
}

void ESB_TIMER_IRQHandler(const void *args)
{
	ARG_UNUSED(args);
	ESB_TIMER_IRQ_HANDLER();
	ISR_DIRECT_PM();
}

#else /* !IS_ENABLED(CONFIG_ESB_DYNAMIC_INTERRUPTS) */

ISR_DIRECT_DECLARE(RADIO_IRQHandler)
{
	radio_irq_handler();

	ISR_DIRECT_PM();

	return 1;
}


ISR_DIRECT_DECLARE(ESB_EVT_IRQHandler)
{
	esb_evt_irq_handler();

	ISR_DIRECT_PM();

	return 1;
}

ISR_DIRECT_DECLARE(ESB_SYS_TIMER_IRQHandler)
{
	ISR_DIRECT_PM();

	return 1;
}

#endif /* IS_ENABLED(CONFIG_ESB_DYNAMIC_INTERRUPTS) */

static void esb_irq_disable(void)
{
	irq_disable(RADIO_IRQn);
	irq_disable(ESB_EVT_IRQ);
	irq_disable(ESB_TIMER_IRQ);
}

int esb_init(const struct esb_config *config)
{
	int err;

	if (!config) {
		return -EINVAL;
	}

	if (esb_initialized) {
		esb_disable();
	}

	event_handler = config->event_handler;

	memcpy(&esb_cfg, config, sizeof(esb_cfg));

	interrupt_flags = 0;

	memset(rx_pipe_info, 0, sizeof(rx_pipe_info));
	memset(pids, 0, sizeof(pids));

	update_radio_parameters();

	/* Configure radio address registers according to ESB default values */
	nrf_radio_base0_set(NRF_RADIO, 0xE7E7E7E7);
	nrf_radio_base1_set(NRF_RADIO, 0x43434343);
	nrf_radio_prefix0_set(NRF_RADIO, 0x23C343E7);
	nrf_radio_prefix1_set(NRF_RADIO, 0x13E363A3);

	initialize_fifos();

	err = sys_timer_init();
	if (err) {
		LOG_ERR("Failed to initialize ESB system timer");
		return err;
	}

	err = esb_ppi_init();
	if (err) {
		LOG_ERR("Failed to initialize PPI");
		return err;
	}

	disable_event.event.generic.event = esb_ppi_radio_disabled_get();

#if IS_ENABLED(CONFIG_ESB_DYNAMIC_INTERRUPTS)

	/* Ensure IRQs are disabled before attaching. */
	esb_irq_disable();

	ARM_IRQ_DIRECT_DYNAMIC_CONNECT(RADIO_IRQn, CONFIG_ESB_RADIO_IRQ_PRIORITY,
				       0, reschedule);
	ARM_IRQ_DIRECT_DYNAMIC_CONNECT(ESB_EVT_IRQ, CONFIG_ESB_EVENT_IRQ_PRIORITY,
				       0, reschedule);
	ARM_IRQ_DIRECT_DYNAMIC_CONNECT(ESB_TIMER_IRQ, CONFIG_ESB_EVENT_IRQ_PRIORITY,
				       0, reschedule);

	irq_connect_dynamic(RADIO_IRQn, CONFIG_ESB_RADIO_IRQ_PRIORITY,
			    RADIO_IRQHandler, NULL, 0);
	irq_connect_dynamic(ESB_EVT_IRQ, CONFIG_ESB_EVENT_IRQ_PRIORITY,
			    ESB_EVT_IRQHandler,  NULL, 0);
	irq_connect_dynamic(ESB_TIMER_IRQ, CONFIG_ESB_EVENT_IRQ_PRIORITY,
			    ESB_TIMER_IRQHandler, NULL, 0);

#else /* !IS_ENABLED(CONFIG_ESB_DYNAMIC_INTERRUPTS) */

	IRQ_DIRECT_CONNECT(RADIO_IRQn, CONFIG_ESB_RADIO_IRQ_PRIORITY,
			   RADIO_IRQHandler, 0);
	IRQ_DIRECT_CONNECT(ESB_EVT_IRQ, CONFIG_ESB_EVENT_IRQ_PRIORITY,
			   ESB_EVT_IRQHandler, 0);
	IRQ_DIRECT_CONNECT(ESB_TIMER_IRQ, CONFIG_ESB_EVENT_IRQ_PRIORITY,
			   ESB_TIMER_IRQ_HANDLER, 0);

#endif /* IS_ENABLED(CONFIG_ESB_DYNAMIC_INTERRUPTS) */

	irq_enable(RADIO_IRQn);
	irq_enable(ESB_EVT_IRQ);
	irq_enable(ESB_TIMER_IRQ);

	esb_state = ESB_STATE_IDLE;
	esb_initialized = true;

	if (nrf52_errata_182()) {
		/* Check if the device is an nRF52832 Rev. 2. */
		/* Workaround for nRF52832 rev 2 errata 182 */
		*(volatile uint32_t *)0x4000173C |= (1 << 10);
	}

	return 0;
}

int esb_suspend(void)
{
	if (esb_state != ESB_STATE_IDLE) {
		return -EBUSY;
	}

	/*  Clear PPI */
	esb_ppi_disable_all();

	esb_state = ESB_STATE_IDLE;

	return 0;
}

void esb_disable(void)
{
	esb_ppi_disable_all();
	esb_fem_reset();

	sys_timer_deinit();
	esb_ppi_deinit();

	esb_state = ESB_STATE_IDLE;
	esb_initialized = false;

	reset_fifos();

	memset(rx_pipe_info, 0, sizeof(rx_pipe_info));
	memset(pids, 0, sizeof(pids));

	esb_irq_disable();
}

bool esb_is_idle(void)
{
	return (esb_state == ESB_STATE_IDLE);
}

static struct payload_wrap *find_free_payload_cont(void)
{
	for (int i = 0; i < CONFIG_ESB_TX_FIFO_SIZE; i++) {
		if (!ack_pl_wrap[i].in_use) {
			return &ack_pl_wrap[i];
		}
	}

	return 0;
}

int esb_write_payload(const struct esb_payload *payload)
{
	if (!esb_initialized) {
		return -EACCES;
	}

	if (payload == NULL) {
		return -EINVAL;
	}

	if ((payload->length == 0) || (payload->length > CONFIG_ESB_MAX_PAYLOAD_LENGTH) ||
	    ((esb_cfg.protocol == ESB_PROTOCOL_ESB) &&
	     (payload->length > esb_cfg.payload_length))) {
		return -EMSGSIZE;
	}

	if (tx_fifo.count >= CONFIG_ESB_TX_FIFO_SIZE) {
		return -ENOMEM;
	}

	if (payload->pipe >= CONFIG_ESB_PIPE_COUNT) {
		return -EINVAL;
	}

	unsigned int key = irq_lock();

	if (esb_cfg.mode == ESB_MODE_PTX) {
		memcpy(tx_fifo.payload[tx_fifo.back], payload, sizeof(struct esb_payload));

		pids[payload->pipe] = (pids[payload->pipe] + 1) % (PID_MAX + 1);
		tx_fifo.payload[tx_fifo.back]->pid = pids[payload->pipe];

		if (++tx_fifo.back >= CONFIG_ESB_TX_FIFO_SIZE) {
			tx_fifo.back = 0;
		}

		tx_fifo.count++;
	} else {
		struct payload_wrap *new_ack_payload = find_free_payload_cont();

		if (new_ack_payload != 0) {
			new_ack_payload->in_use = true;
			new_ack_payload->p_next = 0;
			memcpy(new_ack_payload->p_payload, payload, sizeof(struct esb_payload));

			pids[payload->pipe] = (pids[payload->pipe] + 1) % (PID_MAX + 1);
			new_ack_payload->p_payload->pid = pids[payload->pipe];

			if (ack_pl_wrap_pipe[payload->pipe] == 0) {
				ack_pl_wrap_pipe[payload->pipe] = new_ack_payload;
			} else {
				struct payload_wrap *pl = ack_pl_wrap_pipe[payload->pipe];

				while (pl->p_next != 0) {
					pl = (struct payload_wrap *)pl->p_next;
				}
				pl->p_next = (struct payload_wrap *)new_ack_payload;
			}
			tx_fifo.count++;
		}
	}

	irq_unlock(key);

	if (esb_cfg.mode == ESB_MODE_PTX &&
	    esb_cfg.tx_mode == ESB_TXMODE_AUTO &&
	    esb_state == ESB_STATE_IDLE) {
		start_tx_transaction();
	}

	return 0;
}

int esb_read_rx_payload(struct esb_payload *payload)
{
	if (!esb_initialized) {
		return -EACCES;
	}
	if (payload == NULL) {
		return -EINVAL;
	}

	if (rx_fifo.count == 0) {
		return -ENODATA;
	}

	unsigned int key = irq_lock();

	payload->length = rx_fifo.payload[rx_fifo.front]->length;
	payload->pipe = rx_fifo.payload[rx_fifo.front]->pipe;
	payload->rssi = rx_fifo.payload[rx_fifo.front]->rssi;
	payload->pid = rx_fifo.payload[rx_fifo.front]->pid;
	payload->noack = rx_fifo.payload[rx_fifo.front]->noack;
	memcpy(payload->data, rx_fifo.payload[rx_fifo.front]->data,
	       payload->length);

	if (++rx_fifo.front >= CONFIG_ESB_RX_FIFO_SIZE) {
		rx_fifo.front = 0;
	}

	rx_fifo.count--;

	irq_unlock(key);

	return 0;
}

int esb_start_tx(void)
{
	if (esb_state != ESB_STATE_IDLE) {
		return -EBUSY;
	}

	if (tx_fifo.count == 0) {
		return -ENODATA;
	}

	start_tx_transaction();

	return 0;
}

int esb_start_rx(void)
{
	if (esb_state != ESB_STATE_IDLE) {
		return -EBUSY;
	}

	nrf_radio_int_disable(NRF_RADIO, 0xFFFFFFFF);
	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_DISABLED);

	on_radio_disabled = on_radio_disabled_rx;

	nrf_radio_shorts_set(NRF_RADIO, (radio_shorts_common | NRF_RADIO_SHORT_DISABLED_TXEN_MASK));
	nrf_radio_int_enable(NRF_RADIO, NRF_RADIO_INT_DISABLED_MASK);

	esb_state = ESB_STATE_PRX;

	nrf_radio_rxaddresses_set(NRF_RADIO, esb_addr.rx_pipes_enabled);
	nrf_radio_frequency_set(NRF_RADIO, (RADIO_BASE_FREQUENCY + esb_addr.rf_channel));
	nrf_radio_packetptr_set(NRF_RADIO, rx_payload_buffer);

	NVIC_ClearPendingIRQ(RADIO_IRQn);
	irq_enable(RADIO_IRQn);

	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_ADDRESS);
	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_PAYLOAD);
	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_DISABLED);

	esb_ppi_for_txrx_set(true, false);
	esb_fem_for_rx_set();

	radio_start();

	return 0;
}

int esb_stop_rx(void)
{
	if ((esb_state != ESB_STATE_PRX) && (esb_state != ESB_STATE_PRX_SEND_ACK)) {
		return -EINVAL;
	}

	esb_ppi_for_txrx_clear(true, false);
	esb_fem_reset();

	nrf_radio_shorts_disable(NRF_RADIO, 0xFFFFFFFF);
	nrf_radio_int_disable(NRF_RADIO, 0xFFFFFFFF);

	on_radio_disabled = NULL;

	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_DISABLED);
	nrf_radio_task_trigger(NRF_RADIO, NRF_RADIO_TASK_DISABLE);

	while (!nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_DISABLED)) {
		/* wait for register to settle */
	}

	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_DISABLED);

	esb_state = ESB_STATE_IDLE;

	return 0;
}

int esb_flush_tx(void)
{
	if (!esb_initialized) {
		return -EACCES;
	}

	unsigned int key = irq_lock();

	tx_fifo.count = 0;
	tx_fifo.back = 0;
	tx_fifo.front = 0;

	irq_unlock(key);

	return 0;
}

int esb_pop_tx(void)
{
	if (!esb_initialized) {
		return -EACCES;
	}
	if (tx_fifo.count == 0) {
		return -ENODATA;
	}

	unsigned int key = irq_lock();

	if (++tx_fifo.back >= CONFIG_ESB_TX_FIFO_SIZE) {
		tx_fifo.back = 0;
	}
	tx_fifo.count--;

	irq_unlock(key);

	return 0;
}

int esb_flush_rx(void)
{
	if (!esb_initialized) {
		return -EACCES;
	}

	unsigned int key = irq_lock();

	rx_fifo.count = 0;
	rx_fifo.back = 0;
	rx_fifo.front = 0;

	memset(rx_pipe_info, 0, sizeof(rx_pipe_info));

	irq_unlock(key);

	return 0;
}

int esb_set_address_length(uint8_t length)
{
	if (esb_state != ESB_STATE_IDLE) {
		return -EBUSY;
	}
	if (!((length > 2) && (length < 6))) {
		return -EINVAL;
	}

	esb_addr.addr_length = length;

	update_rf_payload_format(esb_cfg.payload_length);

	return 0;
}

int esb_set_base_address_0(const uint8_t *addr)
{
	if (esb_state != ESB_STATE_IDLE) {
		return -EBUSY;
	}
	if (addr == NULL) {
		return -EINVAL;
	}

	memcpy(esb_addr.base_addr_p0, addr, sizeof(esb_addr.base_addr_p0));

	update_radio_addresses(ADDR_UPDATE_MASK_BASE0);

	return 0;
}

int esb_set_base_address_1(const uint8_t *addr)
{
	if (esb_state != ESB_STATE_IDLE) {
		return -EBUSY;
	}
	if (addr == NULL) {
		return -EINVAL;
	}

	memcpy(esb_addr.base_addr_p1, addr, sizeof(esb_addr.base_addr_p1));

	update_radio_addresses(ADDR_UPDATE_MASK_BASE1);

	return 0;
}

int esb_set_prefixes(const uint8_t *prefixes, uint8_t num_pipes)
{
	if (esb_state != ESB_STATE_IDLE) {
		return -EBUSY;
	}
	if (prefixes == NULL) {
		return -EINVAL;
	}
	if (!(num_pipes <= CONFIG_ESB_PIPE_COUNT)) {
		return -EINVAL;
	}

	memcpy(esb_addr.pipe_prefixes, prefixes, num_pipes);

	esb_addr.num_pipes = num_pipes;
	esb_addr.rx_pipes_enabled = BIT_MASK_UINT_8(num_pipes);

	update_radio_addresses(ADDR_UPDATE_MASK_PREFIX);

	return 0;
}

int esb_update_prefix(uint8_t pipe, uint8_t prefix)
{
	if (esb_state != ESB_STATE_IDLE) {
		return -EBUSY;
	}
	if (pipe >= CONFIG_ESB_PIPE_COUNT) {
		return -EINVAL;
	}

	esb_addr.pipe_prefixes[pipe] = prefix;

	update_radio_addresses(ADDR_UPDATE_MASK_PREFIX);

	return 0;
}

int esb_enable_pipes(uint8_t enable_mask)
{
	if (esb_state != ESB_STATE_IDLE) {
		return -EBUSY;
	}
	if ((enable_mask | BIT_MASK_UINT_8(CONFIG_ESB_PIPE_COUNT)) !=
	    BIT_MASK_UINT_8(CONFIG_ESB_PIPE_COUNT)) {
		return -EINVAL;
	}

	esb_addr.rx_pipes_enabled = enable_mask;

	return 0;
}

int esb_set_rf_channel(uint32_t channel)
{
	if (esb_state != ESB_STATE_IDLE) {
		return -EBUSY;
	}
	if (channel > 100) {
		return -EINVAL;
	}

	esb_addr.rf_channel = channel;

	return 0;
}

int esb_get_rf_channel(uint32_t *channel)
{
	if (channel == NULL) {
		return -EINVAL;
	}

	*channel = esb_addr.rf_channel;

	return 0;
}

int esb_set_tx_power(int8_t tx_output_power)
{
	if (esb_state != ESB_STATE_IDLE) {
		return -EBUSY;
	}

	esb_cfg.tx_output_power = tx_output_power;

	return 0;
}

int esb_set_retransmit_delay(uint16_t delay)
{
	if (esb_state != ESB_STATE_IDLE) {
		return -EBUSY;
	}
	if (delay < RETRANSMIT_DELAY_MIN) {
		return -EINVAL;
	}

	esb_cfg.retransmit_delay = delay;

	return 0;
}

int esb_set_retransmit_count(uint16_t count)
{
	if (esb_state != ESB_STATE_IDLE) {
		return -EBUSY;
	}

	esb_cfg.retransmit_count = count;

	return 0;
}

int esb_set_bitrate(enum esb_bitrate bitrate)
{
	if (esb_state != ESB_STATE_IDLE) {
		return -EBUSY;
	}

	esb_cfg.bitrate = bitrate;

	return update_radio_bitrate() ? 0 : -EINVAL;
}

int esb_reuse_pid(uint8_t pipe)
{
	if (esb_state != ESB_STATE_IDLE) {
		return -EBUSY;
	}
	if (!(pipe < CONFIG_ESB_PIPE_COUNT)) {
		return -EINVAL;
	}

	pids[pipe] = (pids[pipe] + PID_MAX) % (PID_MAX + 1);

	return 0;
}
