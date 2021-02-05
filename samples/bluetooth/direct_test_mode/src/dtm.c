/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <stdlib.h>

#include "dtm.h"
#include "dtm_hw.h"
#include "dtm_hw_config.h"

#if CONFIG_NRF21540_FEM
#include "nrf21540.h"
#endif

#include <drivers/clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>

#include <hal/nrf_nvmc.h>
#include <hal/nrf_radio.h>
#include <helpers/nrfx_gppi.h>
#include <nrfx_timer.h>

#if defined(CONFIG_HAS_HW_NRF_PPI)
#include <nrfx_ppi.h>
#define gppi_channel_t nrf_ppi_channel_t
#define gppi_channel_alloc nrfx_ppi_channel_alloc
#elif defined(CONFIG_HAS_HW_NRF_DPPIC)
#include <nrfx_dppi.h>
#define gppi_channel_t uint8_t
#define gppi_channel_alloc nrfx_dppi_channel_alloc
#else
#error "No PPI or DPPI"
#endif

/* DT definition for clock required in DT_INST_LABEL macro */
#define DT_DRV_COMPAT nordic_nrf_clock

/* Default timer used for timing. */
#define DEFAULT_TIMER_INSTANCE     0
#define DEFAULT_TIMER_IRQ          NRFX_CONCAT_3(TIMER,			 \
						 DEFAULT_TIMER_INSTANCE, \
						 _IRQn)
#define DEFAULT_TIMER_IRQ_HANDLER  NRFX_CONCAT_3(nrfx_timer_,		 \
						 DEFAULT_TIMER_INSTANCE, \
						 _irq_handler)
/* Timer used for the workaround for errata 172 on affected nRF5 devices. */
#define ANOMALY_172_TIMER_INSTANCE     1
#define ANOMALY_172_TIMER_IRQ          NRFX_CONCAT_3(TIMER,		    \
						ANOMALY_172_TIMER_INSTANCE, \
						_IRQn)
#define ANOMALY_172_TIMER_IRQ_HANDLER  NRFX_CONCAT_3(nrfx_timer_,	    \
						ANOMALY_172_TIMER_INSTANCE, \
						_irq_handler)


/* Helper macro for labeling timer instances. */
#define NRFX_TIMER_CONFIG_LABEL(_num) NRFX_CONCAT_3(CONFIG_, NRFX_TIMER, _num)

BUILD_ASSERT(NRFX_TIMER_CONFIG_LABEL(DEFAULT_TIMER_INSTANCE) == 1,
	     "Core DTM timer needs additional KConfig configuration");
BUILD_ASSERT(NRFX_TIMER_CONFIG_LABEL(ANOMALY_172_TIMER_INSTANCE) == 1,
	     "Anomaly DTM timer needs additional KConfig configuration");


/* Values that for now are "constants" - they could be configured by a function
 * setting them, but most of these are set by the BLE DTM standard, so changing
 * them is not relevant.
 * RF-PHY test packet patterns, for the repeated octet packets.
 */
#define RFPHY_TEST_0X0F_REF_PATTERN  0x0f
#define RFPHY_TEST_0X55_REF_PATTERN  0x55
#define RFPHY_TEST_0XFF_REF_PATTERN  0xFF

/* Event status response bits for Read Supported variant of LE Test Setup
 * command.
 */
#define LE_TEST_SETUP_DLE_SUPPORTED         BIT(1)
#define LE_TEST_SETUP_2M_PHY_SUPPORTED      BIT(2)
#define LE_TEST_STABLE_MODULATION_SUPPORTED BIT(3)
#define LE_TEST_CODED_PHY_SUPPORTED         BIT(4)
#define LE_TEST_CTE_SUPPORTED               BIT(5)
#define DTM_LE_ANTENNA_SWITCH               BIT(6)
#define DTM_LE_AOD_1US_TANSMISSION          BIT(7)
#define DTM_LE_AOD_1US_RECEPTION            BIT(8)
#define DTM_LE_AOA_1US_RECEPTION            BIT(9)

/* Time between start of TX packets (in us). */
#define TX_INTERVAL 625
/* The RSSI threshold at which to toggle strict mode. */
#define BLOCKER_FIX_RSSI_THRESHOLD 95
/* Timeout of Anomaly timer (in ms). */
#define BLOCKER_FIX_WAIT_DEFAULT 10
/* Timeout of Anomaly timer (in us) */
#define BLOCKER_FIX_WAIT_END 500
/* Threshold used to determine necessary strict mode status changes. */
#define BLOCKER_FIX_CNTDETECTTHR 15
/* Threshold used to determine necessary strict mode status changes. */
#define BLOCKER_FIX_CNTADDRTHR 2

/* DTM Radio address. */
#define DTM_RADIO_ADDRESS 0x71764129

/* Index where the header of the pdu is located. */
#define DTM_HEADER_OFFSET 0
/* Size of PDU header. */
#define DTM_HEADER_SIZE 2
/* Size of PDU header with CTEInfo field. */
#define DTM_HEADER_WITH_CTE_SIZE 3
/* CTEInfo field offset in payload. */
#define DTM_HEADER_CTEINFO_OFFSET 2
/* CTE Reference period sample count. */
#define DTM_CTE_REF_SAMPLE_CNT 8
/* CTEInfo Preset bit. Indicates whether
 * the CTEInfo field is present in the packet.
 */
#define DTM_PKT_CP_BIT 0x20
/* Maximum payload size allowed during DTM execution. */
#define DTM_PAYLOAD_MAX_SIZE     255
/* Index where the length of the payload is encoded. */
#define DTM_LENGTH_OFFSET        (DTM_HEADER_OFFSET + 1)
/* Maximum PDU size allowed during DTM execution. */
#define DTM_PDU_MAX_MEMORY_SIZE \
	(DTM_HEADER_WITH_CTE_SIZE + DTM_PAYLOAD_MAX_SIZE)
/* Size of the packet on air without the payload
 * (preamble + sync word + type + RFU + length + CRC).
 */
#define DTM_ON_AIR_OVERHEAD_SIZE  10
/**< CRC polynomial. */
#define CRC_POLY                  (0x0000065B)
/* Initial value for CRC calculation. */
#define CRC_INIT                  (0x00555555)
/* Length of S0 field in packet Header (in bytes). */
#define PACKET_HEADER_S0_LEN      1
/* Length of S1 field in packet Header (in bits). */
#define PACKET_HEADER_S1_LEN      0
/* Length of length field in packet Header (in bits). */
#define PACKET_HEADER_LF_LEN      8
/* Number of bytes sent in addition to the variable payload length. */
#define PACKET_STATIC_LEN         0
/* Base address length in bytes. */
#define PACKET_BA_LEN             3
/* CTE IQ sample data size. */
#define DTM_CTE_SAMPLE_DATA_SIZE  128
/* Vendor specific packet type for internal use. */
#define DTM_PKT_TYPE_VENDORSPECIFIC  0xFE
/* 1111111 bit pattern packet type for internal use. */
#define DTM_PKT_TYPE_0xFF            0xFF
/* Response event data shift. */
#define DTM_RESPONSE_EVENT_SHIFT 0x01
/* Maximum number of payload octets that the local Controller supports for
 * transmission of a single Link Layer Data Physical Channel PDU.
 */
#define NRF_MAX_PAYLOAD_OCTETS   0x00FF

/* Maximum length of the Constant Tone Extension that the local Controller
 * supports for transmission in a Link Layer packet, in 8 us units.
 */
#define NRF_CTE_MAX_LENGTH 0x14
/* CTE time unit in us. CTE length is expressed in 8us unit. */
#define NRF_CTE_TIME_IN_US 0x08

/* Constant defining RX mode for radio during DTM test. */
#define RX_MODE  true
/* Constant defining TX mode for radio during DTM test. */
#define TX_MODE  false

/* Maximum number of valid channels in BLE. */
#define PHYS_CH_MAX 39

#define DTM_MAX_ANTENNA_COUNT DT_PROP(DT_PATH(zephyr_user), dtm_antenna_count)

/* States used for the DTM test implementation */
enum dtm_state {
	/* DTM is uninitialized */
	STATE_UNINITIALIZED,

	/* DTM initialized, or current test has completed */
	STATE_IDLE,

	/* DTM Transmission test is running */
	STATE_TRANSMITTER_TEST,

	/* DTM Carrier test is running (Vendor specific test) */
	STATE_CARRIER_TEST,

	/* DTM Receive test is running */
	STATE_RECEIVER_TEST,
};

/* Constant Tone Extension mode. */
enum dtm_cte_mode {
	/* Do not use the Constant Tone Extension. */
	DTM_CTE_MODE_OFF = 0x00,

	/* Constant Tone Extension: Use Angle-of-Departure. */
	DTM_CTE_MODE_AOD = 0x02,

	/* Constant Tone Extension: Use Angle-of-Arrival. */
	DTM_CTE_MODE_AOA = 0x03
};

/* Constatnt Tone Extension slot. */
enum dtm_cte_slot {
	/* Constant Tone Extension: Sample with 1 us slot. */
	DTM_CTE_SLOT_2US = 0x01,

	/* Constant Tone Extension: Sample with 2 us slot. */
	DTM_CTE_SLOT_1US = 0x02,
};

/* Constatnt Tone Extension antenna switch pattern. */
enum dtm_antenna_pattern {
	/* Constant Tone Extension: Antenna switch pattern 1, 2, 3 ...N. */
	DTM_ANTENNA_PATTERN_123N123N = 0x00,

	/* Constant Tone Extension: Antenna switch pattern
	 * 1, 2, 3 ...N, N - 1, N - 2, ..., 1, ...
	 */
	DTM_ANTENNA_PATTERN_123N2123 = 0x01
};

/* The PDU payload type for each bit pattern. Identical to the PKT value
 * except pattern 0xFF which is 0x04.
 */
enum dtm_pdu_type {
	/* PRBS9 bit pattern */
	DTM_PDU_TYPE_PRBS9 = 0x00,

	/* 11110000 bit pattern  (LSB is the leftmost bit). */
	DTM_PDU_TYPE_0X0F = 0x01,

	/* 10101010 bit pattern (LSB is the leftmost bit). */
	DTM_PDU_TYPE_0X55 = 0x02,

	/* 11111111 bit pattern (Used only for coded PHY). */
	DTM_PDU_TYPE_0XFF = 0x04,
};

/* Structure holding the PDU used for transmitting/receiving a PDU. */
struct dtm_pdu {
	/* PDU packet content. */
	uint8_t content[DTM_PDU_MAX_MEMORY_SIZE];
};

struct dtm_cte_info {
	/* Constant Tone Extension mode. */
	enum dtm_cte_mode mode;

	/* Constant Tone Extension sample slot */
	enum dtm_cte_slot slot;

	/* Antenna switch pattern. */
	enum dtm_antenna_pattern antenna_pattern;

	/* Received CTE IQ sample data. */
	uint32_t data[DTM_CTE_SAMPLE_DATA_SIZE];

	/* Constant Tone Extension length in 8us unit. */
	uint8_t time;

	/* Number of antenna in the antenna array. */
	uint8_t antenna_number;

	/* CTEInfo. */
	uint8_t info;
};

struct nrf21540_parameters {
	/* nRF21540 activation delay. */
	uint32_t active_delay;

	/* nRF21540 Vendor activation delay. */
	uint32_t vendor_active_delay;
};

/* DTM instance definition */
static struct dtm_instance {
	/* Current machine state. */
	enum dtm_state state;

	/* Current command status - initially "ok", may be set if error
	 * detected, or to packet count.
	 */
	uint16_t event;

	/* Command has been processed - number of not yet reported event
	 * bytes.
	 */
	bool new_event;

	/* Number of valid packets received. */
	uint16_t rx_pkt_count;

	/**< RX/TX PDU. */
	struct dtm_pdu pdu;

	/* Payload length of TX PDU, bits 2:7 of 16-bit dtm command. */
	uint32_t packet_len;

	/* Bits 0..1 of 16-bit transmit command, or 0xFFFFFFFF. */
	uint32_t packet_type;

	/* 0..39 physical channel number (base 2402 MHz, Interval 2 MHz),
	 * bits 8:13 of 16-bit dtm command.
	 */
	uint32_t phys_ch;

	/* Length of the preamble. */
	nrf_radio_preamble_length_t packet_hdr_plen;

	/* Address. */
	uint32_t address;

	/* Counter for interrupts from timer to ensure that the 2 bytes forming
	 * a DTM command are received within the time window.
	 */
	uint32_t current_time;

	/* Timer to be used for scheduling TX packets. */
	const nrfx_timer_t timer;

	/* Timer to be used to handle Anomaly 172. */
	const nrfx_timer_t anomaly_timer;

	/* Enable or disable the workaround for Errata 172. */
	bool anomaly_172_wa_enabled;

	/* Enable or disable strict mode to workaround Errata 172. */
	bool strict_mode;

	/* Radio PHY mode. */
	nrf_radio_mode_t radio_mode;

	/* Radio output power. */
	nrf_radio_txpower_t txpower;

	/* Constant Tone Extension configuration. */
	struct dtm_cte_info cte_info;

	/* nRF21540 FEM parameters. */
	struct nrf21540_parameters nrf21540;

	/* Radio TX Enable PPI channel. */
	gppi_channel_t ppi_radio_txen;
} dtm_inst = {
	.state = STATE_UNINITIALIZED,
	.packet_hdr_plen = NRF_RADIO_PREAMBLE_LENGTH_8BIT,
	.address = DTM_RADIO_ADDRESS,
	.timer = NRFX_TIMER_INSTANCE(DEFAULT_TIMER_INSTANCE),
	.anomaly_timer = NRFX_TIMER_INSTANCE(ANOMALY_172_TIMER_INSTANCE),
	.radio_mode = NRF_RADIO_MODE_BLE_1MBIT,
	.txpower = NRF_RADIO_TXPOWER_0DBM,
};

/* The PRBS9 sequence used as packet payload.
 * The bytes in the sequence is in the right order, but the bits of each byte
 * in the array is reverse of that found by running the PRBS9 algorithm.
 * This is because of the endianness of the nRF5 radio.
 */
static uint8_t const dtm_prbs_content[] = {
	0xFF, 0xC1, 0xFB, 0xE8, 0x4C, 0x90, 0x72, 0x8B,
	0xE7, 0xB3, 0x51, 0x89, 0x63, 0xAB, 0x23, 0x23,
	0x02, 0x84, 0x18, 0x72, 0xAA, 0x61, 0x2F, 0x3B,
	0x51, 0xA8, 0xE5, 0x37, 0x49, 0xFB, 0xC9, 0xCA,
	0x0C, 0x18, 0x53, 0x2C, 0xFD, 0x45, 0xE3, 0x9A,
	0xE6, 0xF1, 0x5D, 0xB0, 0xB6, 0x1B, 0xB4, 0xBE,
	0x2A, 0x50, 0xEA, 0xE9, 0x0E, 0x9C, 0x4B, 0x5E,
	0x57, 0x24, 0xCC, 0xA1, 0xB7, 0x59, 0xB8, 0x87,
	0xFF, 0xE0, 0x7D, 0x74, 0x26, 0x48, 0xB9, 0xC5,
	0xF3, 0xD9, 0xA8, 0xC4, 0xB1, 0xD5, 0x91, 0x11,
	0x01, 0x42, 0x0C, 0x39, 0xD5, 0xB0, 0x97, 0x9D,
	0x28, 0xD4, 0xF2, 0x9B, 0xA4, 0xFD, 0x64, 0x65,
	0x06, 0x8C, 0x29, 0x96, 0xFE, 0xA2, 0x71, 0x4D,
	0xF3, 0xF8, 0x2E, 0x58, 0xDB, 0x0D, 0x5A, 0x5F,
	0x15, 0x28, 0xF5, 0x74, 0x07, 0xCE, 0x25, 0xAF,
	0x2B, 0x12, 0xE6, 0xD0, 0xDB, 0x2C, 0xDC, 0xC3,
	0x7F, 0xF0, 0x3E, 0x3A, 0x13, 0xA4, 0xDC, 0xE2,
	0xF9, 0x6C, 0x54, 0xE2, 0xD8, 0xEA, 0xC8, 0x88,
	0x00, 0x21, 0x86, 0x9C, 0x6A, 0xD8, 0xCB, 0x4E,
	0x14, 0x6A, 0xF9, 0x4D, 0xD2, 0x7E, 0xB2, 0x32,
	0x03, 0xC6, 0x14, 0x4B, 0x7F, 0xD1, 0xB8, 0xA6,
	0x79, 0x7C, 0x17, 0xAC, 0xED, 0x06, 0xAD, 0xAF,
	0x0A, 0x94, 0x7A, 0xBA, 0x03, 0xE7, 0x92, 0xD7,
	0x15, 0x09, 0x73, 0xE8, 0x6D, 0x16, 0xEE, 0xE1,
	0x3F, 0x78, 0x1F, 0x9D, 0x09, 0x52, 0x6E, 0xF1,
	0x7C, 0x36, 0x2A, 0x71, 0x6C, 0x75, 0x64, 0x44,
	0x80, 0x10, 0x43, 0x4E, 0x35, 0xEC, 0x65, 0x27,
	0x0A, 0xB5, 0xFC, 0x26, 0x69, 0x3F, 0x59, 0x99,
	0x01, 0x63, 0x8A, 0xA5, 0xBF, 0x68, 0x5C, 0xD3,
	0x3C, 0xBE, 0x0B, 0xD6, 0x76, 0x83, 0xD6, 0x57,
	0x05, 0x4A, 0x3D, 0xDD, 0x81, 0x73, 0xC9, 0xEB,
	0x8A, 0x84, 0x39, 0xF4, 0x36, 0x0B, 0xF7
};

#if DIRECTION_FINDING_SUPPORTED

static void radio_gpio_pattern_clear(void)
{
	NRF_RADIO->CLEARPATTERN = 1;
}

static void antenna_radio_pin_config(void)
{
	const uint32_t *pin = dtm_hw_radion_antenna_pin_array_get();

	for (uint8_t i = 0; i < dtm_radio_antenna_pin_array_size_get(); i++) {
		NRF_RADIO->PSEL.DFEGPIO[i] = pin[i];
	}
}

static void switch_pattern_set(void)
{
	/* Set antenna for the guard period and for the reference period. */
	NRF_RADIO->SWITCHPATTERN = 1;
	NRF_RADIO->SWITCHPATTERN = 1;

	switch (dtm_inst.cte_info.antenna_pattern) {
	case DTM_ANTENNA_PATTERN_123N123N:
		for (uint16_t i = 1;
		     i <= dtm_inst.cte_info.antenna_number;
		     i++) {
			NRF_RADIO->SWITCHPATTERN = i;
		}

		break;

	case DTM_ANTENNA_PATTERN_123N2123:
		for (uint16_t i = 1;
		     i <= dtm_inst.cte_info.antenna_number;
		     i++) {
			NRF_RADIO->SWITCHPATTERN = i;
		}

		for (uint16_t i = dtm_inst.cte_info.antenna_number - 1;
		     i > 0;
		     i--) {
			NRF_RADIO->SWITCHPATTERN = i;
		}

		break;
	}
}

static void radio_cte_prepare(bool rx)
{
	if ((rx && (dtm_inst.cte_info.mode ==  DTM_CTE_MODE_AOA)) ||
	    ((!rx) && (dtm_inst.cte_info.mode == DTM_CTE_MODE_AOD))) {
		antenna_radio_pin_config();
		switch_pattern_set();

		/* Set antenna switch spacing. */
		NRF_RADIO->DFECTRL1 &= ~RADIO_DFECTRL1_TSWITCHSPACING_Msk;
		NRF_RADIO->DFECTRL1 |= (dtm_inst.cte_info.slot <<
					RADIO_DFECTRL1_TSWITCHSPACING_Pos);
	}

	NRF_RADIO->DFEMODE = dtm_inst.cte_info.mode;
	NRF_RADIO->PCNF0 |= (8 << RADIO_PCNF0_S1LEN_Pos);

	if (rx) {
		/* Enable parsing CTEInfo from received packet. */
		NRF_RADIO->CTEINLINECONF |=
				RADIO_CTEINLINECONF_CTEINLINECTRLEN_Enabled;
		NRF_RADIO->CTEINLINECONF |=
			(RADIO_CTEINLINECONF_CTEINFOINS1_InS1 <<
			 RADIO_CTEINLINECONF_CTEINFOINS1_Pos);

		/* Set S0 Mask and Configuration to check if CP bit is set
		 * in received PDU.
		 */
		NRF_RADIO->CTEINLINECONF |=
				(0x20 << RADIO_CTEINLINECONF_S0CONF_Pos) |
				(0x20 << RADIO_CTEINLINECONF_S0MASK_Pos);

		NRF_RADIO->DFEPACKET.PTR = (uint32_t)dtm_inst.cte_info.data;
		NRF_RADIO->DFEPACKET.MAXCNT =
				(uint16_t)sizeof(dtm_inst.cte_info.data);
	} else {
		/* Disable parsing CTEInfo from received packet. */
		NRF_RADIO->CTEINLINECONF &=
				~RADIO_CTEINLINECONF_CTEINLINECTRLEN_Enabled;

		NRF_RADIO->DFECTRL1 &= ~RADIO_DFECTRL1_NUMBEROF8US_Msk;
		NRF_RADIO->DFECTRL1 |= dtm_inst.cte_info.time;
	}
}
#endif /* DIRECTION_FINDING_SUPPORTED */

static int clock_init(void)
{
	int err;
	const struct device *clock;

	clock = device_get_binding(DT_INST_LABEL(0));
	if (!clock) {
		printk("Unable to find clock device binding\n");
		return -ENODEV;
	}

	err = clock_control_on(clock, CLOCK_CONTROL_NRF_SUBSYS_HF);
	if (err) {
		printk("Unable to turn on the clock: %d", err);
	}

	return err;
}

static void timer_handler(nrf_timer_event_t event_type, void *context)
{
	/* Do nothing, use timer in polling mode. */
}

static int timer_init(void)
{
	nrfx_err_t err;
	nrfx_timer_config_t timer_cfg = {
		.frequency = NRF_TIMER_FREQ_1MHz,
		.mode = NRF_TIMER_MODE_TIMER,
		.bit_width = NRF_TIMER_BIT_WIDTH_16,
	};

	err = nrfx_timer_init(&dtm_inst.timer, &timer_cfg, timer_handler);
	if (err != NRFX_SUCCESS) {
		printk("nrfx_timer_init failed with: %d\n", err);
		return -EAGAIN;
	}

	IRQ_CONNECT(DEFAULT_TIMER_IRQ, 6, DEFAULT_TIMER_IRQ_HANDLER, NULL, 0);

	/* Timer is polled, but enable the compare0 interrupt in order to
	 * wakeup from CPU sleep.
	 */
	nrfx_timer_extended_compare(&dtm_inst.timer,
			NRF_TIMER_CC_CHANNEL0,
			nrfx_timer_us_to_ticks(&dtm_inst.timer, TX_INTERVAL),
			NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK,
			false);

	nrfx_timer_compare(&dtm_inst.timer,
		NRF_TIMER_CC_CHANNEL1,
		nrfx_timer_us_to_ticks(&dtm_inst.timer, DTM_UART_POLL_CYCLE),
		false);

	dtm_inst.current_time = 0;
	nrfx_timer_enable(&dtm_inst.timer);

	return 0;
}

static int anomaly_timer_init(void)
{
	nrfx_err_t err;
	nrfx_timer_config_t timer_cfg = {
		.frequency = NRF_TIMER_FREQ_125kHz,
		.mode = NRF_TIMER_MODE_TIMER,
		.bit_width = NRF_TIMER_BIT_WIDTH_16,
	};

	err = nrfx_timer_init(&dtm_inst.anomaly_timer, &timer_cfg,
			      timer_handler);
	if (err != NRFX_SUCCESS) {
		printk("nrfx_timer_init failed with: %d\n", err);
		return -EAGAIN;
	}

	IRQ_CONNECT(ANOMALY_172_TIMER_IRQ, 6, ANOMALY_172_TIMER_IRQ_HANDLER,
		    NULL, 0);

	nrfx_timer_compare(&dtm_inst.anomaly_timer,
		NRF_TIMER_CC_CHANNEL0,
		nrfx_timer_ms_to_ticks(&dtm_inst.anomaly_timer,
				       BLOCKER_FIX_WAIT_DEFAULT),
		false);

	return 0;
}

static int gppi_init(void)
{
	nrfx_err_t err;

	err = gppi_channel_alloc(&dtm_inst.ppi_radio_txen);
	if (err != NRFX_SUCCESS) {
		printk("gppi_channel_alloc failed with: %d\n", err);
		return -EAGAIN;
	}

	return 0;
}

static void radio_reset(void)
{
	nrfx_gppi_channels_disable(BIT(dtm_inst.ppi_radio_txen));

	nrf_radio_shorts_set(NRF_RADIO, 0);
	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_DISABLED);

	nrf_radio_task_trigger(NRF_RADIO, NRF_RADIO_TASK_DISABLE);
	while (!nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_DISABLED)) {
		/* Do nothing */
	}
	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_DISABLED);

	dtm_inst.rx_pkt_count = 0;
}

static int radio_init(void)
{
	nrf_radio_packet_conf_t packet_conf;

	if (!dtm_hw_radio_validate(dtm_inst.txpower, dtm_inst.radio_mode)) {
		printk("Incorrect settings for radio mode and TX power\n");

		dtm_inst.event = LE_TEST_STATUS_EVENT_ERROR;
		return -EINVAL;
	}

	/* Turn off radio before configuring it */
	radio_reset();

	nrf_radio_txpower_set(NRF_RADIO, dtm_inst.txpower);
	nrf_radio_mode_set(NRF_RADIO, dtm_inst.radio_mode);

	/* Set the access address, address0/prefix0 used for both Rx and Tx
	 * address.
	 */
	nrf_radio_prefix0_set(NRF_RADIO, dtm_inst.address >> 24);
	nrf_radio_base0_set(NRF_RADIO, dtm_inst.address << 8);
	nrf_radio_rxaddresses_set(NRF_RADIO, RADIO_RXADDRESSES_ADDR0_Enabled);
	nrf_radio_txaddress_set(NRF_RADIO, 0x00);

	/* Configure CRC calculation. */
	nrf_radio_crcinit_set(NRF_RADIO, CRC_INIT);
	nrf_radio_crc_configure(NRF_RADIO, RADIO_CRCCNF_LEN_Three,
				NRF_RADIO_CRC_ADDR_SKIP, CRC_POLY);

	memset(&packet_conf, 0, sizeof(packet_conf));
	packet_conf.s0len = PACKET_HEADER_S0_LEN;
	packet_conf.s1len = PACKET_HEADER_S1_LEN;
	packet_conf.lflen = PACKET_HEADER_LF_LEN;
	packet_conf.plen = dtm_inst.packet_hdr_plen;
	packet_conf.whiteen = false;
	packet_conf.big_endian = false;
	packet_conf.balen = PACKET_BA_LEN;
	packet_conf.statlen = PACKET_STATIC_LEN;
	packet_conf.maxlen = DTM_PAYLOAD_MAX_SIZE;

	if (dtm_inst.radio_mode != NRF_RADIO_MODE_BLE_1MBIT &&
	    dtm_inst.radio_mode != NRF_RADIO_MODE_BLE_2MBIT) {
		/* Coded PHY (Long range) */
#if defined(RADIO_PCNF0_TERMLEN_Msk)
		packet_conf.termlen = 3;
#endif /* defined(RADIO_PCNF0_TERMLEN_Msk) */

#if defined(RADIO_PCNF0_CILEN_Msk)
		packet_conf.cilen = 2;
#endif /* defined(RADIO_PCNF0_CILEN_Msk) */
	}

	nrf_radio_packet_configure(NRF_RADIO, &packet_conf);

	return 0;
}

int dtm_init(void)
{
	int err;

	err = clock_init();
	if (err) {
		return err;
	}

	err = timer_init();
	if (err) {
		return err;
	}

	/* Enable the timer used by nRF52840 anomaly 172 if running on an
	 * affected device.
	 */
	err = anomaly_timer_init();
	if (err) {
		return err;
	}

	err = gppi_init();
	if (err) {
		return err;
	}

	err = radio_init();
	if (err) {
		return err;
	}
#if CONFIG_NRF21540_FEM
	err = nrf21540_init();
	if (err) {
		return err;
	}
#endif /* CONFIG_NRF21540_FEM */

	dtm_inst.state = STATE_IDLE;
	dtm_inst.new_event = false;
	dtm_inst.packet_len = 0;

	return 0;
}

/* Function for verifying that a received PDU has the expected structure and
 * content.
 */
static bool check_pdu(void)
{
	/* Repeating octet value in payload */
	uint8_t pattern;
	/* PDU packet type is a 4-bit field in HCI, but 2 bits in BLE DTM */
	uint32_t pdu_packet_type;
	uint32_t length = 0;
	uint8_t header_len;

	pdu_packet_type = (uint32_t)
			  (dtm_inst.pdu.content[DTM_HEADER_OFFSET] & 0x0F);
	length = dtm_inst.pdu.content[DTM_LENGTH_OFFSET];

	header_len = (dtm_inst.cte_info.mode != DTM_CTE_MODE_OFF) ?
		     DTM_HEADER_WITH_CTE_SIZE : DTM_HEADER_SIZE;

	/* Check that the length is valid. */
	if (length > DTM_PAYLOAD_MAX_SIZE) {
		return false;
	}

	/* If the 1Mbit or 2Mbit radio mode is active, check that one of the
	 * three valid uncoded DTM packet types are selected.
	 */
	if ((dtm_inst.radio_mode == NRF_RADIO_MODE_BLE_1MBIT ||
	     dtm_inst.radio_mode == NRF_RADIO_MODE_BLE_2MBIT) &&
	    (pdu_packet_type > (uint32_t) DTM_PDU_TYPE_0X55)) {
		return false;
	}

	/* If a long range radio mode is active, check that one of the four
	 * valid coded DTM packet types are selected.
	 */
	if (dtm_hw_radio_lr_check(dtm_inst.radio_mode) &&
	    (pdu_packet_type > (uint32_t) DTM_PDU_TYPE_0XFF)) {
		return false;
	}

	if (pdu_packet_type == DTM_PDU_TYPE_PRBS9) {
		/* Payload does not consist of one repeated octet; must
		 * compare it with entire block.
		 */
		uint8_t *payload = dtm_inst.pdu.content + header_len;

		return (memcmp(payload, dtm_prbs_content, length) == 0);
	}

	switch (pdu_packet_type) {
	case DTM_PDU_TYPE_0X0F:
		pattern = RFPHY_TEST_0X0F_REF_PATTERN;
		break;
	case DTM_PDU_TYPE_0X55:
		pattern = RFPHY_TEST_0X55_REF_PATTERN;
		break;
	case DTM_PDU_TYPE_0XFF:
		pattern = RFPHY_TEST_0XFF_REF_PATTERN;
		break;
	default:
		/* No valid packet type set. */
		return false;
	}

	for (uint8_t k = 0; k < length; k++) {
		/* Check repeated pattern filling the PDU payload */
		if (dtm_inst.pdu.content[k + 2] != pattern) {
			return false;
		}
	}

#if DIRECTION_FINDING_SUPPORTED
	/* Check CTEInfo and IQ sample cnt */
	if (dtm_inst.cte_info.mode != DTM_CTE_MODE_OFF) {
		uint8_t cte_info;
		uint8_t cte_sample_cnt;
		uint8_t expected_sample_cnt;

		cte_info = dtm_inst.pdu.content[DTM_HEADER_CTEINFO_OFFSET];

		expected_sample_cnt =
			DTM_CTE_REF_SAMPLE_CNT +
			((dtm_inst.cte_info.time * 8)) /
			((dtm_inst.cte_info.slot == DTM_CTE_SLOT_1US) ? 2 : 4);
		cte_sample_cnt = NRF_RADIO->DFEPACKET.AMOUNT;

		memset(dtm_inst.cte_info.data, 0,
		       sizeof(dtm_inst.cte_info.data));

		if ((cte_info != dtm_inst.cte_info.mode) ||
		    (expected_sample_cnt != cte_sample_cnt)) {
			return false;
		}
	}
#endif /* DIRECTION_FINDING_SUPPORTED */

	return true;
}

/* Radio configuration used as a workaround for nRF52840 anomaly 172 */
static void anomaly_172_radio_operation(void)
{
	*(volatile uint32_t *) 0x40001040 = 1;
	*(volatile uint32_t *) 0x40001038 = 1;
}

/* Function to gather RSSI data and set strict mode accordingly.
 * Used as part of the workaround for nRF52840 anomaly 172
 */
static uint8_t anomaly_172_rssi_check(void)
{
	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_RSSIEND);

	nrf_radio_task_trigger(NRF_RADIO, NRF_RADIO_TASK_RSSISTART);
	while (!nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_RSSIEND)) {
	}

	return nrf_radio_rssi_sample_get(NRF_RADIO);
}

/* Strict mode setting will be used only by devices affected by nRF52840
 * anomaly 172
 */
static void anomaly_172_strict_mode_set(bool enable)
{
	uint8_t dbcCorrTh;
	uint8_t dsssMinPeakCount;

	if (enable == true) {
		dbcCorrTh = 0x7d;
		dsssMinPeakCount = 6;

		*(volatile uint32_t *) 0x4000173c =
			((*((volatile uint32_t *) 0x4000173c)) & 0x7FFFFF00) |
			0x80000000 |
			(((uint32_t)(dbcCorrTh)) << 0);
		*(volatile uint32_t *) 0x4000177c =
			((*((volatile uint32_t *) 0x4000177c)) & 0x7FFFFF8F) |
			0x80000000 |
			((((uint32_t)dsssMinPeakCount) & 0x00000007) << 4);
	} else {
		*(volatile uint32_t *) 0x4000173c = 0x40003034;
		/* Unset override of dsssMinPeakCount */
		*(volatile uint32_t *) 0x4000177c =
			((*((volatile uint32_t *) 0x4000177c)) & 0x7FFFFFFF);
	}

	dtm_inst.strict_mode = enable;
}

static bool dtm_wait_internal(void)
{
	if (dtm_inst.state == STATE_RECEIVER_TEST &&
	    nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_ADDRESS)) {
		nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_ADDRESS);
		if (dtm_inst.anomaly_172_wa_enabled) {
			nrfx_timer_disable(&dtm_inst.anomaly_timer);
		}
	}

	/* Event may be the reception of a packet -
	 * handle radio first, to give it highest priority.
	 */
	if (nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_END)) {
		nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_END);

		NVIC_ClearPendingIRQ(RADIO_IRQn);

		if (dtm_inst.state == STATE_RECEIVER_TEST) {
#if CONFIG_NRF21540_FEM
			nrf21540_txrx_configuration_clear();
			nrf21540_txrx_stop();

			(void) nrf21540_rx_configure(NRF21540_EXECUTE_NOW,
					nrf_radio_event_address_get(
						NRF_RADIO,
						NRF_RADIO_EVENT_DISABLED),
					dtm_inst.nrf21540.active_delay);
#else
			nrf_radio_task_trigger(NRF_RADIO, NRF_RADIO_TASK_RXEN);
#endif /* CONFIG_NRF21540_FEM */
			if (dtm_inst.anomaly_172_wa_enabled) {
				nrfx_timer_compare(&dtm_inst.anomaly_timer,
					NRF_TIMER_CC_CHANNEL0,
					nrfx_timer_ms_to_ticks(
						&dtm_inst.anomaly_timer,
						BLOCKER_FIX_WAIT_DEFAULT),
					false);
				nrfx_timer_compare(&dtm_inst.anomaly_timer,
					NRF_TIMER_CC_CHANNEL1,
					nrfx_timer_us_to_ticks(
						&dtm_inst.anomaly_timer,
						BLOCKER_FIX_WAIT_END),
					false);
				nrfx_timer_clear(&dtm_inst.anomaly_timer);
				nrfx_timer_enable(&dtm_inst.anomaly_timer);
			}

			if (nrf_radio_crc_status_check(NRF_RADIO) &&
			    check_pdu()) {
				/* Count the number of successfully received
				 * packets.
				 */
				dtm_inst.rx_pkt_count++;
			}

			/* Note that failing packets are simply ignored (CRC or
			 * contents error).
			 */

			/* Zero fill all pdu fields to avoid stray data */
			memset(&dtm_inst.pdu, 0, DTM_PDU_MAX_MEMORY_SIZE);
		}
		/* If no RECEIVER_TEST is running, ignore incoming packets
		 * (but do clear IRQ!)
		 */
	}

	if (dtm_inst.state == STATE_RECEIVER_TEST &&
	    nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_READY)) {
		nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_READY);

		if (dtm_inst.anomaly_172_wa_enabled) {
			nrfx_timer_clear(&dtm_inst.anomaly_timer);
			if (!nrfx_timer_is_enabled(&dtm_inst.anomaly_timer)) {
				nrfx_timer_enable(&dtm_inst.anomaly_timer);
			}
		}
	}

	/* Check for timeouts from core timer */
	if (nrf_timer_event_check(dtm_inst.timer.p_reg,
				  NRF_TIMER_EVENT_COMPARE0)) {
		nrf_timer_event_clear(dtm_inst.timer.p_reg,
				      NRF_TIMER_EVENT_COMPARE0);
	} else if (nrf_timer_event_check(dtm_inst.timer.p_reg,
					 NRF_TIMER_EVENT_COMPARE1)) {
		/* Reset timeout event flag for next iteration. */
		nrf_timer_event_clear(dtm_inst.timer.p_reg,
				      NRF_TIMER_EVENT_COMPARE1);
		NRFX_IRQ_PENDING_CLEAR(
			nrfx_get_irq_number(dtm_inst.timer.p_reg));

		++dtm_inst.current_time;

		return false;
	}

	/* Check for timeouts from anomaly timer */
	if (nrf_timer_event_check(dtm_inst.anomaly_timer.p_reg,
				  NRF_TIMER_EVENT_COMPARE0)) {
		uint8_t rssi = anomaly_172_rssi_check();

		if (dtm_inst.strict_mode) {
			if (rssi > BLOCKER_FIX_RSSI_THRESHOLD) {
				anomaly_172_strict_mode_set(false);
			}
		} else {
			bool too_many_detects = false;
			uint32_t packetcnt2 = *(volatile uint32_t *) 0x40001574;
			uint32_t detect_cnt = packetcnt2 & 0xffff;
			uint32_t addr_cnt = (packetcnt2 >> 16) & 0xffff;

			if ((detect_cnt > BLOCKER_FIX_CNTDETECTTHR) &&
			    (addr_cnt < BLOCKER_FIX_CNTADDRTHR)) {
				too_many_detects = true;
			}

			if ((rssi < BLOCKER_FIX_RSSI_THRESHOLD) ||
			    too_many_detects) {
				anomaly_172_strict_mode_set(true);
			}
		}

		anomaly_172_radio_operation();

		nrfx_timer_disable(&dtm_inst.anomaly_timer);

		nrfx_timer_compare(&dtm_inst.anomaly_timer,
			NRF_TIMER_CC_CHANNEL0,
			nrfx_timer_ms_to_ticks(&dtm_inst.anomaly_timer,
					       BLOCKER_FIX_WAIT_DEFAULT),
			false);

		nrfx_timer_clear(&dtm_inst.anomaly_timer);
		nrf_timer_event_clear(dtm_inst.anomaly_timer.p_reg,
				      NRF_TIMER_EVENT_COMPARE0);
		nrfx_timer_enable(&dtm_inst.anomaly_timer);

		NRFX_IRQ_PENDING_CLEAR(
			nrfx_get_irq_number(dtm_inst.anomaly_timer.p_reg));
	}

	if (nrf_timer_event_check(dtm_inst.anomaly_timer.p_reg,
				  NRF_TIMER_EVENT_COMPARE1)) {
		uint8_t rssi = anomaly_172_rssi_check();

		if (rssi >= BLOCKER_FIX_RSSI_THRESHOLD) {
			anomaly_172_strict_mode_set(false);
		} else {
			anomaly_172_strict_mode_set(true);
		}

		anomaly_172_radio_operation();

		/* Disable this event. */
		nrfx_timer_compare(&dtm_inst.anomaly_timer,
				   NRF_TIMER_CC_CHANNEL1,
				   0,
				   false);

		NRFX_IRQ_PENDING_CLEAR(
			nrfx_get_irq_number(dtm_inst.anomaly_timer.p_reg));
	}

	/* Other events: No processing */
	return true;
}

uint32_t dtm_wait(void)
{
	for (;;) {
		if (!dtm_wait_internal()) {
			break;
		}
	}

	return dtm_inst.current_time;
}

static void dtm_test_done(void)
{
	nrfx_gppi_channels_disable(BIT(dtm_inst.ppi_radio_txen));
	/* Break connection from timer to radio to stop transmit loop */
	nrfx_gppi_event_endpoint_clear(dtm_inst.ppi_radio_txen,
			(uint32_t) nrf_timer_event_address_get(
			       dtm_inst.timer.p_reg,
			       NRF_TIMER_EVENT_COMPARE0));
	nrfx_gppi_task_endpoint_clear(dtm_inst.ppi_radio_txen,
			nrf_radio_task_address_get(NRF_RADIO,
				NRF_RADIO_TASK_TXEN));

	nrfx_timer_disable(&dtm_inst.anomaly_timer);

	radio_reset();
#if CONFIG_NRF21540_FEM
	nrf21540_txrx_configuration_clear();
	nrf21540_txrx_stop();
#endif /* CONFIG_NRF21540_FEM */
	dtm_inst.state = STATE_IDLE;
}

static void radio_prepare(bool rx)
{
#if DIRECTION_FINDING_SUPPORTED
	if (dtm_inst.cte_info.mode != DTM_CTE_MODE_OFF) {
		radio_cte_prepare(rx);
	}
#endif /* DIRECTION_FINDING_SUPPORTED */

	/* Actual frequency (MHz): 2402 + 2N */
	nrf_radio_frequency_set(NRF_RADIO, (dtm_inst.phys_ch << 1) + 2402);

	/* Setting packet pointer will start the radio */
	nrf_radio_packetptr_set(NRF_RADIO, &dtm_inst.pdu);
	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_READY);

	/* Set shortcuts:
	 * between READY event and START task and
	 * between END event and DISABLE task
	 */
#if DIRECTION_FINDING_SUPPORTED
	nrf_radio_shorts_set(NRF_RADIO,
		NRF_RADIO_SHORT_READY_START_MASK |
		(dtm_inst.cte_info.mode == DTM_CTE_MODE_OFF ?
		 NRF_RADIO_SHORT_END_DISABLE_MASK :
		 NRF_RADIO_SHORT_PHYEND_DISABLE_MASK));
#else
	nrf_radio_shorts_set(NRF_RADIO,
			     NRF_RADIO_SHORT_READY_START_MASK |
			     NRF_RADIO_SHORT_END_DISABLE_MASK);
#endif /* DIRECTION_FINDING_SUPPORTED */

#if CONFIG_NRF21540_FEM
	if (dtm_inst.nrf21540.vendor_active_delay == 0) {
		dtm_inst.nrf21540.active_delay =
			nrf21540_default_active_delay_calculate(rx,
							dtm_inst.radio_mode);
	} else {
		dtm_inst.nrf21540.active_delay =
				dtm_inst.nrf21540.vendor_active_delay;
	}
#endif
	if (rx) {
		/* Enable strict mode for anomaly 172 */
		if (dtm_inst.anomaly_172_wa_enabled) {
			anomaly_172_strict_mode_set(true);
		}

		nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_END);

#if CONFIG_NRF21540_FEM
		(void) nrf21540_rx_configure(NRF21540_EXECUTE_NOW,
			nrf_radio_event_address_get(
						NRF_RADIO,
						NRF_RADIO_EVENT_DISABLED),
			dtm_inst.nrf21540.active_delay);
#else
		/* Shorts will start radio in RX mode when it is ready */
		nrf_radio_task_trigger(NRF_RADIO, NRF_RADIO_TASK_RXEN);
#endif /* CONFIG_NRF21540_FEM */
	} else { /* tx */
		nrf_radio_txpower_set(NRF_RADIO, dtm_inst.txpower);

		/* Stop the timer used by anomaly 172 */
		if (dtm_inst.anomaly_172_wa_enabled) {
			nrfx_timer_disable(&dtm_inst.anomaly_timer);
			nrfx_timer_clear(&dtm_inst.anomaly_timer);

			nrf_timer_event_clear(dtm_inst.anomaly_timer.p_reg,
					      NRF_TIMER_EVENT_COMPARE0);
			nrf_timer_event_clear(dtm_inst.anomaly_timer.p_reg,
					      NRF_TIMER_EVENT_COMPARE1);
		}
	}
}

static bool dtm_set_txpower(uint32_t new_tx_power)
{
	/* radio->TXPOWER register is 32 bits, low octet a tx power value,
	 * upper 24 bits zeroed.
	 */
	uint8_t new_power8 = (uint8_t) (new_tx_power & 0xFF);

	/* The two most significant bits are not sent in the 6 bit field of
	 * the DTM command. These two bits are 1's if and only if the tx_power
	 * is a negative number. All valid negative values have a non zero bit
	 * in among the two most significant of the 6-bit value. By checking
	 * these bits, the two most significant bits can be determined.
	 */
	new_power8 = (new_power8 & 0x30) != 0 ?
		     (new_power8 | 0xC0) : new_power8;

	if (dtm_inst.state > STATE_IDLE) {
		/* Radio must be idle to change the TX power */
		return false;
	}

	if (!dtm_hw_radio_validate(new_power8, dtm_inst.radio_mode)) {
		return false;
	}

	dtm_inst.txpower = new_power8;

	return true;
}

static enum dtm_err_code  dtm_vendor_specific_pkt(uint32_t vendor_cmd,
						  uint32_t vendor_option)
{
	switch (vendor_cmd) {
	/* nRFgo Studio uses CARRIER_TEST_STUDIO to indicate a continuous
	 * carrier without a modulated signal.
	 */
	case CARRIER_TEST:
	case CARRIER_TEST_STUDIO:
		/* Not a packet type, but used to indicate that a continuous
		 * carrier signal should be transmitted by the radio.
		 */
		radio_prepare(TX_MODE);

		nrf_radio_modecnf0_set(NRF_RADIO, false,
				       RADIO_MODECNF0_DTX_Center);

		/* Shortcut between READY event and START task */
		nrf_radio_shorts_set(NRF_RADIO,
				     NRF_RADIO_SHORT_READY_START_MASK);

#if CONFIG_NRF21540_FEM
		(void) nrf21540_tx_configure(NRF21540_EXECUTE_NOW,
			nrf_radio_event_address_get(NRF_RADIO,
						    NRF_RADIO_EVENT_DISABLED),
			dtm_inst.nrf21540.active_delay);
#else
		/* Shortcut will start radio in Tx mode when it is ready */
		nrf_radio_task_trigger(NRF_RADIO, NRF_RADIO_TASK_TXEN);
#endif /* CONFIG_NRF21540_FEM */
		dtm_inst.state = STATE_CARRIER_TEST;
		break;

	case SET_TX_POWER:
		if (!dtm_set_txpower(vendor_option)) {
			dtm_inst.event = LE_TEST_STATUS_EVENT_ERROR;
			return DTM_ERROR_ILLEGAL_CONFIGURATION;
		}
		break;

#if CONFIG_NRF21540_FEM
	case NRF21540_ANTENNA_SELECT:
		if (nrf21540_antenna_select(vendor_option) != 0) {
			dtm_inst.event = LE_TEST_STATUS_EVENT_ERROR;
			return DTM_ERROR_ILLEGAL_CONFIGURATION;
		}

		break;

	case NRF21540_GAIN_SET:
		if (nrf21540_tx_gain_set(vendor_option) != 0) {
			dtm_inst.event = LE_TEST_STATUS_EVENT_ERROR;
			return DTM_ERROR_ILLEGAL_CONFIGURATION;
		}
		break;

	case NRF21540_ACTIVE_DELAY_SET:
		dtm_inst.nrf21540.vendor_active_delay = vendor_option;

		break;
#endif /* CONFIG_NRF21540_FEM */
	}

	/* Event code is unchanged, successful */
	return DTM_SUCCESS;
}

static uint32_t dtm_packet_interval_calculate(uint32_t test_payload_length,
					      nrf_radio_mode_t mode)
{
	/* [us] NOTE: bits are us at 1Mbit */
	uint32_t test_packet_length = 0;
	/* us */
	uint32_t packet_interval = 0;
	/* bits */
	uint32_t overhead_bits = 0;

	/* Packet overhead
	 * see BLE [Vol 6, Part F] page 213
	 * 4.1 LE TEST PACKET FORMAT
	 */
	if (mode == NRF_RADIO_MODE_BLE_2MBIT) {
		/* 16 preamble
		 * 32 sync word
		 *  8 PDU header, actually packetHeaderS0len * 8
		 *  8 PDU length, actually packetHeaderLFlen
		 * 24 CRC
		 */
		overhead_bits = 88; /* 11 bytes */
	} else if (mode == NRF_RADIO_MODE_NRF_1MBIT) {
		/*  8 preamble
		 * 32 sync word
		 *  8 PDU header, actually packetHeaderS0len * 8
		 *  8 PDU length, actually packetHeaderLFlen
		 * 24 CRC
		 */
		overhead_bits = 80; /* 10 bytes */
#if defined(RADIO_MODE_MODE_Ble_LR125Kbit)
	} else if (mode == NRF_RADIO_MODE_BLE_LR125KBIT) {
		/* 80     preamble
		 * 32 * 8 sync word coding=8
		 *  2 * 8 Coding indicator, coding=8
		 *  3 * 8 TERM1 coding=8
		 *  8 * 8 PDU header, actually packetHeaderS0len * 8 coding=8
		 *  8 * 8 PDU length, actually packetHeaderLFlen coding=8
		 * 24 * 8 CRC coding=8
		 *  3 * 8 TERM2 coding=8
		 */
		overhead_bits = 720; /* 90 bytes */
	} else if (mode == NRF_RADIO_MODE_BLE_LR500KBIT) {
		/* 80     preamble
		 * 32 * 8 sync word coding=8
		 *  2 * 8 Coding indicator, coding=8
		 *  3 * 8 TERM 1 coding=8
		 *  8 * 2 PDU header, actually packetHeaderS0len * 8 coding=2
		 *  8 * 2 PDU length, actually packetHeaderLFlen coding=2
		 * 24 * 2 CRC coding=2
		 *  3 * 2 TERM2 coding=2
		 * NOTE: this makes us clock out 46 bits for CI + TERM1 + TERM2
		 *       assumption the radio will handle this
		 */
		overhead_bits = 462; /* 57.75 bytes */
#endif /* defined(RADIO_MODE_MODE_Ble_LR125Kbit) */
	}

	/* Add PDU payload test_payload length */
	test_packet_length = (test_payload_length * 8); /* in bits */

	/* Account for the encoding of PDU */
#if defined(RADIO_MODE_MODE_Ble_LR125Kbit)
	if (mode == NRF_RADIO_MODE_BLE_LR125KBIT) {
		test_packet_length *= 8; /* 1 to 8 encoding */
	}
#endif /* defined(RADIO_MODE_MODE_Ble_LR125Kbit) */
#if defined(RADIO_MODE_MODE_Ble_LR500Kbit)
	if (mode == NRF_RADIO_MODE_BLE_LR500KBIT) {
		test_packet_length *= 2; /* 1 to 2 encoding */
	}
#endif /* defined(RADIO_MODE_MODE_Ble_LR500Kbit) */

	/* Add overhead calculated above */
	test_packet_length += overhead_bits;
	/* remember this bits are us in 1Mbit */
	if (mode == NRF_RADIO_MODE_BLE_2MBIT) {
		test_packet_length /= 2; /* double speed */
	}

	if (dtm_inst.cte_info.mode != DTM_CTE_MODE_OFF) {
		/* Add 8 - bit S1 field with CTEInfo. */
		((test_packet_length += mode) == RADIO_MODE_MODE_Ble_1Mbit) ?
						 8 : 4;

		/* Add CTE length in us to test packet length. */
		test_packet_length +=
				dtm_inst.cte_info.time * NRF_CTE_TIME_IN_US;
	}

	/* Packet_interval = ceil((test_packet_length + 249) / 625) * 625
	 * NOTE: To avoid floating point an equivalent calculation is used.
	 */
	uint32_t i = 0;
	uint32_t timeout = 0;

	do {
		i++;
		timeout = i * 625;
	} while (test_packet_length + 249 > timeout);

	packet_interval = i * 625;

	return packet_interval;
}

static enum dtm_err_code phy_set(uint8_t phy)
{
	if ((phy >= LE_PHY_1M_MIN_RANGE) &&
	    (phy <= LE_PHY_1M_MAX_RANGE)) {
		dtm_inst.radio_mode = NRF_RADIO_MODE_BLE_1MBIT;
		dtm_inst.packet_hdr_plen =
			NRF_RADIO_PREAMBLE_LENGTH_8BIT;
#ifdef NRF52840_XXAA
		/* Workaround for Errata 191 "Radio: High packet error rate in BLE Long Range mode"
		 * found at the Errata document for your device located at https://infocenter.nordicsemi.com/index.jsp
		 */
		*(volatile uint32_t *) 0x40001740 = ((*((volatile uint32_t *) 0x40001740)) & 0x7FFFFFFF);

		/* Disable the workaround anomaly 172 */
		anomaly_172_strict_mode_set(false);
		nrfx_timer_disable(&dtm_inst.anomaly_timer);
		dtm_inst.anomaly_172_wa_enabled = false;
#endif
		return radio_init();
	} else if ((phy >= LE_PHY_2M_MIN_RANGE) &&
		   (phy <= LE_PHY_2M_MAX_RANGE)) {
		dtm_inst.radio_mode = NRF_RADIO_MODE_BLE_2MBIT;
		dtm_inst.packet_hdr_plen =
			NRF_RADIO_PREAMBLE_LENGTH_16BIT;

#ifdef NRF52840_XXAA
		/* Workaround for Errata 191 "Radio: High packet error rate in BLE Long Range mode"
		 * found at the Errata document for your device located at https://infocenter.nordicsemi.com/index.jsp
		 */
		*(volatile uint32_t *) 0x40001740 = ((*((volatile uint32_t *) 0x40001740)) & 0x7FFFFFFF);

		/* Disable the workaround anomaly 172 */
		anomaly_172_strict_mode_set(false);
		nrfx_timer_disable(&dtm_inst.anomaly_timer);
		dtm_inst.anomaly_172_wa_enabled = false;
#endif
		return radio_init();
	} else if ((phy >= LE_PHY_LE_CODED_S8_MIN_RANGE) &&
		   (phy <= LE_PHY_LE_CODED_S8_MAX_RANGE)) {
#if defined(RADIO_MODE_MODE_Ble_LR125Kbit)
		dtm_inst.radio_mode =
			NRF_RADIO_MODE_BLE_LR125KBIT;
		dtm_inst.packet_hdr_plen =
			NRF_RADIO_PREAMBLE_LENGTH_LONG_RANGE;
#ifdef NRF52840_XXAA
		/* Workaround for Errata 191 "Radio: High packet error rate in BLE Long Range mode"
		 * found at the Errata document for your device located at https://infocenter.nordicsemi.com/index.jsp
		 */
		*(volatile uint32_t *) 0x40001740 = ((*((volatile uint32_t *) 0x40001740)) & 0x7FFF00FF) |
		0x80000000 | (((uint32_t) (196)) << 8);

		/* Enable the workaround for anomaly 172. */
		if ((*(volatile uint32_t *) 0x40001788) == 0) {
			dtm_inst.anomaly_172_wa_enabled = true;
		}
#endif /* NRF52840_XXAA */
		return radio_init();
#else
		dtm_inst.event = LE_TEST_STATUS_EVENT_ERROR;
		return DTM_ERROR_ILLEGAL_CONFIGURATION;
#endif /* defined(RADIO_MODE_MODE_Ble_LR125Kbit) */
	} else if ((phy >= LE_PHY_LE_CODED_S2_MIN_RANGE) &&
		   (phy <= LE_PHY_LE_CODED_S2_MAX_RANGE)) {
#if defined(RADIO_MODE_MODE_Ble_LR500Kbit)
		dtm_inst.radio_mode =
			NRF_RADIO_MODE_BLE_LR500KBIT;
		dtm_inst.packet_hdr_plen =
			NRF_RADIO_PREAMBLE_LENGTH_LONG_RANGE;

#ifdef NRF52840_XXAA
		/* Workaround for Errata 191 "Radio: High packet error rate in BLE Long Range mode"
		 * found at the Errata document for your device located at https://infocenter.nordicsemi.com/index.jsp
		 */
		*(volatile uint32_t *) 0x40001740 = ((*((volatile uint32_t *) 0x40001740)) & 0x7FFF00FF) |
		0x80000000 | (((uint32_t) (196)) << 8);

		/* Enable the workaround for anomaly 172. */
		if ((*(volatile uint32_t *) 0x40001788) == 0) {
			dtm_inst.anomaly_172_wa_enabled = true;
		}
#endif /* NRF52840_XXAA */

		return radio_init();
#else
		dtm_inst.event = LE_TEST_STATUS_EVENT_ERROR;
		return DTM_ERROR_ILLEGAL_CONFIGURATION;
#endif /* defined(RADIO_MODE_MODE_Ble_LR500Kbit) */
	}

	dtm_inst.event = LE_TEST_STATUS_EVENT_ERROR;
	return DTM_ERROR_ILLEGAL_CONFIGURATION;
}

static enum dtm_err_code modulation_set(uint8_t modulation)
{
	/* Only standard modulation is supported. */
	if (modulation > LE_MODULATION_INDEX_STANDARD_MAX_RANGE) {
		dtm_inst.event = LE_TEST_STATUS_EVENT_ERROR;
		return DTM_ERROR_ILLEGAL_CONFIGURATION;
	}

	return DTM_SUCCESS;
}

static enum dtm_err_code feature_read(uint8_t cmd)
{
	if (cmd > LE_TEST_FEATURE_READ_MAX_RANGE) {
		dtm_inst.event = LE_TEST_STATUS_EVENT_ERROR;
		return DTM_ERROR_ILLEGAL_CONFIGURATION;
	}

	/* 0XXXXXXXXXXX0110 indicate that 2Mbit and DLE is
	 * supported and stable modulation is not supported
	 * (No nRF5 device supports this).
	 */
	dtm_inst.event =
		LE_TEST_STATUS_EVENT_SUCCESS |
#if defined(RADIO_MODE_MODE_Ble_LR125Kbit) || defined(RADIO_MODE_MODE_Ble_LR500Kbit)
		LE_TEST_CODED_PHY_SUPPORTED |
#endif /* defined(RADIO_MODE_MODE_Ble_LR125Kbit) || defined(RADIO_MODE_MODE_Ble_LR500Kbit) */
#if DIRECTION_FINDING_SUPPORTED
		LE_TEST_CTE_SUPPORTED |
		DTM_LE_ANTENNA_SWITCH |
		DTM_LE_AOD_1US_TANSMISSION |
		DTM_LE_AOD_1US_RECEPTION |
		DTM_LE_AOA_1US_RECEPTION |
#endif /* DIRECTION_FINDING_SUPPORTED */
		LE_TEST_SETUP_DLE_SUPPORTED |
		LE_TEST_SETUP_2M_PHY_SUPPORTED;

	return DTM_SUCCESS;
}

static enum dtm_err_code maximum_supported_value_read(uint8_t parameter)
{
	/* Read supportedMaxTxOctets */
	if (parameter <= LE_TEST_SUPPORTED_TX_OCTETS_MAX_RANGE) {
		dtm_inst.event =
			NRF_MAX_PAYLOAD_OCTETS << DTM_RESPONSE_EVENT_SHIFT;
	}
	/* Read supportedMaxTxTime */
	else if ((parameter >= LE_TEST_SUPPORTED_TX_TIME_MIN_RANGE) &&
		 (parameter <= LE_TEST_SUPPORTED_TX_TIME_MAX_RANGE)) {
		dtm_inst.event = NRF_MAX_RX_TX_TIME << DTM_RESPONSE_EVENT_SHIFT;
	}
	/* Read supportedMaxRxOctets */
	else if ((parameter >= LE_TEST_SUPPORTED_RX_OCTETS_MIN_RANGE) &&
		 (parameter <= LE_TEST_SUPPORTED_RX_OCTETS_MAX_RANGE)) {
		dtm_inst.event =
			NRF_MAX_PAYLOAD_OCTETS << DTM_RESPONSE_EVENT_SHIFT;
	}
	/* Read supportedMaxRxTime */
	else if ((parameter >= LE_TEST_SUPPORTED_RX_TIME_MIN_RANGE) &&
		 (parameter <= LE_TEST_SUPPORTED_RX_TIME_MAX_RANGE)) {
		dtm_inst.event = NRF_MAX_RX_TX_TIME << DTM_RESPONSE_EVENT_SHIFT;
	}
#if DIRECTION_FINDING_SUPPORTED
	/* Read maximum length of Constant Tone Extension */
	else if (parameter == LE_TEST_SUPPORTED_CTE_LENGTH) {
		dtm_inst.event = NRF_CTE_MAX_LENGTH << DTM_RESPONSE_EVENT_SHIFT;
	}
#endif /* DIRECTION_FINDING_SUPPORTED */
	else {
		dtm_inst.event = LE_TEST_STATUS_EVENT_ERROR;
		return DTM_ERROR_ILLEGAL_CONFIGURATION;
	}

	return DTM_SUCCESS;
}

#if DIRECTION_FINDING_SUPPORTED
static uint32_t constant_tone_setup(uint8_t cte_info)
{
	uint8_t type = (cte_info >> LE_CTE_TYPE_POS) & LE_CTE_TYPE_MASK;

	dtm_inst.cte_info.time = cte_info & LE_CTE_CTETIME_MASK;
	dtm_inst.cte_info.info = cte_info;


	if ((dtm_inst.cte_info.time < LE_CTE_LENGTH_MIN) ||
	    (dtm_inst.cte_info.time > LE_CTE_LENGTH_MAX)) {
		dtm_inst.event = LE_TEST_STATUS_EVENT_ERROR;
		return DTM_ERROR_ILLEGAL_CONFIGURATION;
	}

	switch (type) {
	case LE_CTE_TYPE_AOA:
		dtm_inst.cte_info.mode = DTM_CTE_MODE_AOA;

		break;

	case LE_CTE_TYPE_AOD_1US:
		dtm_inst.cte_info.mode = DTM_CTE_MODE_AOD;
		dtm_inst.cte_info.slot = DTM_CTE_SLOT_1US;

		break;

	case LE_CTE_TYPE_AOD_2US:
		dtm_inst.cte_info.mode = DTM_CTE_MODE_AOD;
		dtm_inst.cte_info.slot = DTM_CTE_SLOT_2US;

		break;

	default:
		dtm_inst.event = LE_TEST_STATUS_EVENT_ERROR;
		return DTM_ERROR_ILLEGAL_CONFIGURATION;
	}

	return DTM_SUCCESS;
}

static uint32_t constant_tone_slot_set(uint8_t cte_slot)
{
	switch (cte_slot) {
	case LE_CTE_TYPE_AOD_1US:
		dtm_inst.cte_info.slot = DTM_CTE_SLOT_1US;
		break;

	case LE_CTE_TYPE_AOD_2US:
		dtm_inst.cte_info.slot = DTM_CTE_SLOT_2US;

	default:
		dtm_inst.event = LE_TEST_STATUS_EVENT_ERROR;
		return DTM_ERROR_ILLEGAL_CONFIGURATION;
	}

	return DTM_SUCCESS;
}

static uint32_t antenna_set(uint8_t antenna)
{
	dtm_inst.cte_info.antenna_number = antenna & LE_ANTENNA_NUMBER_MASK;
	dtm_inst.cte_info.antenna_pattern =
		(enum dtm_antenna_pattern)(antenna &
					   LE_ANTENNA_SWITCH_PATTERN_MASK);

	if ((dtm_inst.cte_info.antenna_number < LE_TEST_ANTENNA_NUMBER_MIN) ||
	    (dtm_inst.cte_info.antenna_number > LE_TEST_ANTENNA_NUMBER_MAX) ||
	    (dtm_inst.cte_info.antenna_number > DTM_MAX_ANTENNA_COUNT)) {
		dtm_inst.event = LE_TEST_STATUS_EVENT_ERROR;
		return DTM_ERROR_ILLEGAL_CONFIGURATION;
	}

	return DTM_SUCCESS;
}
#endif /* DIRECTION_FINDING_SUPPORTED */

static uint32_t transmit_power_set(int8_t parameter)
{
	size_t size = dtm_hw_radio_power_array_size_get();
	const uint32_t *power = dtm_hw_radio_power_array_get();

	if (parameter == LE_TRANSMIT_POWER_LVL_SET_MIN) {
		dtm_inst.txpower = dtm_hw_radio_min_power_get();
		dtm_inst.event =
			((dtm_inst.txpower <<
			  LE_TRANSMIT_POWER_RESPONSE_LVL_POS) &
			 LE_TRANSMIT_POWER_RESPONSE_LVL_MASK) |
			LE_TRANSMIT_POWER_MIN_LVL_BIT;

		return DTM_SUCCESS;
	}

	if (parameter == LE_TRANSMIT_POWER_LVL_SET_MAX) {
		dtm_inst.txpower = dtm_hw_radio_max_power_get();
		dtm_inst.event =
			((dtm_inst.txpower <<
			  LE_TRANSMIT_POWER_RESPONSE_LVL_POS) &
			 LE_TRANSMIT_POWER_RESPONSE_LVL_MASK) |
			LE_TRANSMIT_POWER_MAX_LVL_BIT;

		return DTM_SUCCESS;
	}

	if ((parameter < LE_TRANSMIT_POWER_LVL_MIN) ||
	    (parameter > LE_TRANSMIT_POWER_LVL_MAX)) {
		dtm_inst.event =
			((dtm_inst.txpower <<
			  LE_TRANSMIT_POWER_RESPONSE_LVL_POS) &
			 LE_TRANSMIT_POWER_RESPONSE_LVL_MASK) |
			LE_TEST_STATUS_EVENT_ERROR;

		if (dtm_inst.txpower == dtm_hw_radio_min_power_get()) {
			dtm_inst.event |= LE_TRANSMIT_POWER_MIN_LVL_BIT;
		} else if (dtm_inst.txpower == dtm_hw_radio_max_power_get()) {
			dtm_inst.event |= LE_TRANSMIT_POWER_MAX_LVL_BIT;
		} else {
			/* Do nothing. */
		}

		return DTM_ERROR_ILLEGAL_CONFIGURATION;
	}

	if (parameter <= ((int8_t) dtm_hw_radio_min_power_get())) {
		dtm_inst.txpower = dtm_hw_radio_min_power_get();
		dtm_inst.event =
			((dtm_inst.txpower <<
			  LE_TRANSMIT_POWER_RESPONSE_LVL_POS) &
			 LE_TRANSMIT_POWER_RESPONSE_LVL_MASK) |
			LE_TRANSMIT_POWER_MIN_LVL_BIT;

		return DTM_SUCCESS;
	}

	if (parameter >= ((int8_t) dtm_hw_radio_max_power_get())) {
		dtm_inst.txpower = dtm_hw_radio_max_power_get();
		dtm_inst.event =
			((dtm_inst.txpower <<
			  LE_TRANSMIT_POWER_RESPONSE_LVL_POS) &
			 LE_TRANSMIT_POWER_RESPONSE_LVL_MASK) |
			LE_TRANSMIT_POWER_MAX_LVL_BIT;

		return DTM_SUCCESS;
	}

	/* Look for the nearest tansmit power level and set it. */
	for (size_t i = 1; i < size; i++) {
		if (((int8_t) power[i]) > parameter) {
			int8_t diff = abs((int8_t) power[i] - parameter);

			if (diff <  abs((int8_t) power[i - 1] - parameter)) {
				dtm_inst.txpower = power[i];
			} else {
				dtm_inst.txpower = power[i - 1];
			}

			break;
		}
	}

	dtm_inst.event =
		(dtm_inst.txpower << LE_TRANSMIT_POWER_RESPONSE_LVL_POS) &
		LE_TRANSMIT_POWER_RESPONSE_LVL_MASK;

	return DTM_SUCCESS;
}

static enum dtm_err_code on_test_setup_cmd(enum dtm_ctrl_code control,
					   uint8_t parameter)
{
	/* Note that timer will continue running after a reset */
	dtm_test_done();

	switch (control) {
	case LE_TEST_SETUP_RESET:
		if (parameter > LE_RESET_MAX_RANGE) {
			dtm_inst.event = LE_TEST_STATUS_EVENT_ERROR;
			return DTM_ERROR_ILLEGAL_CONFIGURATION;
		}

		/* Reset the packet length upper bits. */
		dtm_inst.packet_len = 0;

		/* Reset the selected PHY to 1Mbit */
		dtm_inst.radio_mode = NRF_RADIO_MODE_BLE_1MBIT;
		dtm_inst.packet_hdr_plen =
			NRF_RADIO_PREAMBLE_LENGTH_8BIT;
		dtm_inst.nrf21540.vendor_active_delay = 0;

#if DIRECTION_FINDING_SUPPORTED
		memset(&dtm_inst.cte_info, 0, sizeof(dtm_inst.cte_info));
		radio_gpio_pattern_clear();
#endif /* DIRECTION_FINDING_SUPPORTED */

#ifdef NRF52840_XXAA
		/* Workaround for Errata 191 "Radio: High packet error rate in BLE Long Range mode"
		 * found at the Errata document for your device located at https://infocenter.nordicsemi.com/index.jsp
		 */
		*(volatile uint32_t *) 0x40001740 = ((*((volatile uint32_t *) 0x40001740)) & 0x7FFFFFFF);

		/* Disable the workaround anomaly 172 */
		anomaly_172_strict_mode_set(false);
		nrfx_timer_disable(&dtm_inst.anomaly_timer);
		dtm_inst.anomaly_172_wa_enabled = false;
#endif
		radio_init();

		break;

	case LE_TEST_SETUP_SET_UPPER:
		if (parameter > LE_SET_UPPER_BITS_MAX_RANGE) {
			dtm_inst.event = LE_TEST_STATUS_EVENT_ERROR;
			return DTM_ERROR_ILLEGAL_CONFIGURATION;
		}

		dtm_inst.packet_len =
			(parameter & LE_UPPER_BITS_MASK) << LE_UPPER_BITS_POS;

		break;

	case LE_TEST_SETUP_SET_PHY:
		return phy_set(parameter);

	case LE_TEST_SETUP_SELECT_MODULATION:
		return modulation_set(parameter);

	case LE_TEST_SETUP_READ_SUPPORTED:
		return feature_read(parameter);

	case LE_TEST_SETUP_READ_MAX:
		return maximum_supported_value_read(parameter);

	case LE_TEST_SETUP_TRANSMIT_POWER:
		return transmit_power_set(parameter);

#if DIRECTION_FINDING_SUPPORTED
	case LE_TEST_SETUP_CONSTANT_TONE_EXTENSION:
		return constant_tone_setup(parameter);

	case LE_TEST_SETUP_CONSTANT_TONE_EXTENSION_SLOT:
		return constant_tone_slot_set(parameter);

	case LE_TEST_SETUP_ANTENNA_ARRAY:
		return antenna_set(parameter);
#endif /* DIRECTION_FINDING_SUPPORTED */

	default:
		dtm_inst.event = LE_TEST_STATUS_EVENT_ERROR;
		return DTM_ERROR_ILLEGAL_CONFIGURATION;
	}

	return DTM_SUCCESS;
}

static enum dtm_err_code on_test_end_cmd(void)
{
	if (dtm_inst.state == STATE_IDLE) {
		/* Sequencing error, only rx or tx test may be ended */
		dtm_inst.event = LE_TEST_STATUS_EVENT_ERROR;
		return DTM_ERROR_INVALID_STATE;
	}

	dtm_inst.event = LE_PACKET_REPORTING_EVENT |
			 dtm_inst.rx_pkt_count;
	dtm_test_done();

	return DTM_SUCCESS;
}

static enum dtm_err_code on_test_receive_cmd(void)
{
	/* Zero fill all pdu fields to avoid stray data from earlier
	 * test run.
	 */
	memset(&dtm_inst.pdu, 0, DTM_PDU_MAX_MEMORY_SIZE);

	/* Reinitialize "everything"; RF interrupts OFF */
	radio_prepare(RX_MODE);

	dtm_inst.state = STATE_RECEIVER_TEST;
	return DTM_SUCCESS;
}

static enum dtm_err_code on_test_transmit_cmd(uint32_t length, uint32_t freq)
{
	uint8_t header_len;

	/* Check for illegal values of packet_len. Skip the check
	 * if the packet is vendor spesific.
	 */
	if (dtm_inst.packet_type != DTM_PKT_TYPE_VENDORSPECIFIC &&
	    dtm_inst.packet_len > DTM_PAYLOAD_MAX_SIZE) {
		/* Parameter error */
		dtm_inst.event = LE_TEST_STATUS_EVENT_ERROR;
		return DTM_ERROR_ILLEGAL_LENGTH;
	}

	header_len = (dtm_inst.cte_info.mode != DTM_CTE_MODE_OFF) ?
		     DTM_HEADER_WITH_CTE_SIZE : DTM_HEADER_SIZE;

	dtm_inst.pdu.content[DTM_LENGTH_OFFSET] = dtm_inst.packet_len;
	/* Note that PDU uses 4 bits even though BLE DTM uses only 2
	 * (the HCI SDU uses all 4)
	 */
	switch (dtm_inst.packet_type) {
	case DTM_PKT_PRBS9:
		dtm_inst.pdu.content[DTM_HEADER_OFFSET] =
			DTM_PDU_TYPE_PRBS9;
		/* Non-repeated, must copy entire pattern to PDU */
		memcpy(dtm_inst.pdu.content + header_len,
		       dtm_prbs_content, dtm_inst.packet_len);
		break;

	case DTM_PKT_0X0F:
		dtm_inst.pdu.content[DTM_HEADER_OFFSET] =
			DTM_PDU_TYPE_0X0F;
		/* Bit pattern 00001111 repeated */
		memset(dtm_inst.pdu.content + header_len,
		       RFPHY_TEST_0X0F_REF_PATTERN,
		       dtm_inst.packet_len);
		break;

	case DTM_PKT_0X55:
		dtm_inst.pdu.content[DTM_HEADER_OFFSET] =
			DTM_PDU_TYPE_0X55;
		/* Bit pattern 01010101 repeated */
		memset(dtm_inst.pdu.content + header_len,
		       RFPHY_TEST_0X55_REF_PATTERN,
		       dtm_inst.packet_len);
		break;

	case DTM_PKT_TYPE_0xFF:
		dtm_inst.pdu.content[DTM_HEADER_OFFSET] =
			DTM_PDU_TYPE_0XFF;
		/* Bit pattern 11111111 repeated. Only available in
		 * coded PHY (Long range).
		 */
		memset(dtm_inst.pdu.content + header_len,
		       RFPHY_TEST_0XFF_REF_PATTERN,
		       dtm_inst.packet_len);
		break;

	case DTM_PKT_TYPE_VENDORSPECIFIC:
		/* The length field is for indicating the vendor
		 * specific command to execute. The frequency field
		 * is used for vendor specific options to the command.
		 */
		return dtm_vendor_specific_pkt(length, freq);

	default:
		/* Parameter error */
		dtm_inst.event = LE_TEST_STATUS_EVENT_ERROR;
		return DTM_ERROR_ILLEGAL_CONFIGURATION;
	}

	if (dtm_inst.cte_info.mode != DTM_CTE_MODE_OFF) {
		dtm_inst.pdu.content[DTM_HEADER_OFFSET] |= DTM_PKT_CP_BIT;
		dtm_inst.pdu.content[DTM_HEADER_CTEINFO_OFFSET] =
							dtm_inst.cte_info.info;
	}

	/* Initialize CRC value, set channel */
	radio_prepare(TX_MODE);

	/* Set the timer to the correct period. The delay between each
	 * packet is described in the Bluetooth Core Spsification
	 * version 4.2 Vol. 6 Part F Section 4.1.6.
	 */
	nrfx_timer_compare(&dtm_inst.timer,
			   NRF_TIMER_CC_CHANNEL0,
			   dtm_packet_interval_calculate(
				   dtm_inst.packet_len,
				   dtm_inst.radio_mode),
			   false);

#if CONFIG_NRF21540_FEM
	(void) nrf21540_tx_configure(nrf_timer_event_address_get(
						dtm_inst.timer.p_reg,
						NRF_TIMER_EVENT_COMPARE0),
				     nrf_radio_event_address_get(
						NRF_RADIO,
						NRF_RADIO_EVENT_DISABLED),
				     dtm_inst.nrf21540.active_delay);
#else
	/* Configure PPI so that timer will activate radio every
	 * 625 us
	 */
	nrfx_gppi_channel_endpoints_setup(
			dtm_inst.ppi_radio_txen,
			(uint32_t) nrf_timer_event_address_get(
						dtm_inst.timer.p_reg,
						NRF_TIMER_EVENT_COMPARE0),
			nrf_radio_task_address_get(NRF_RADIO,
						   NRF_RADIO_TASK_TXEN));
	nrfx_gppi_channels_enable(BIT(dtm_inst.ppi_radio_txen));
#endif /* CONFIG_NRF21540_FEM */

	dtm_inst.state = STATE_TRANSMITTER_TEST;

	return DTM_SUCCESS;
}

enum dtm_err_code dtm_cmd_put(uint16_t cmd)
{
	enum dtm_cmd_code cmd_code = (cmd >> 14) & 0x03;
	uint32_t freq = (cmd >> 8) & 0x3F;
	uint32_t length = (cmd >> 2) & 0x3F;
	enum dtm_pkt_type payload = cmd & 0x03;

	/* Clean out any non-retrieved event that might linger from an earlier
	 * test.
	 */
	dtm_inst.new_event = true;

	/* Set default event; any error will set it to
	 * LE_TEST_STATUS_EVENT_ERROR
	 */
	dtm_inst.event = LE_TEST_STATUS_EVENT_SUCCESS;

	if (dtm_inst.state == STATE_UNINITIALIZED) {
		/* Application has not explicitly initialized DTM. */
		return DTM_ERROR_UNINITIALIZED;
	}

	if (cmd_code == LE_TEST_SETUP) {
		enum dtm_ctrl_code control = (cmd >> 8) & 0x3F;
		uint8_t parameter = cmd;

		return on_test_setup_cmd(control, parameter);
	}

	if (cmd_code == LE_TEST_END) {
		return on_test_end_cmd();
	}

	if (dtm_inst.state != STATE_IDLE) {
		/* Sequencing error - only TEST_END/RESET are legal while
		 * test is running. Note: State is unchanged;
		 * ongoing test not affected.
		 */
		dtm_inst.event = LE_TEST_STATUS_EVENT_ERROR;
		return DTM_ERROR_INVALID_STATE;
	}

	/* Save specified packet in static variable for tx/rx functions to use.
	 * Note that BLE conformance testers always use full length packets.
	 */
	dtm_inst.packet_len = (dtm_inst.packet_len & 0xC0) |
			      ((uint8_t) length & 0x3F);
	dtm_inst.packet_type = payload;
	dtm_inst.phys_ch = freq;


	/* If 1 Mbit or 2 Mbit radio mode is in use check for Vendor Specific
	 * payload.
	 */
	if (payload == DTM_PKT_0XFF_OR_VS) {
		/* Note that in a HCI adaption layer, as well as in the DTM PDU
		 * format, the value 0x03 is a distinct bit pattern (PRBS15).
		 * Even though BLE does not support PRBS15, this implementation
		 * re-maps 0x03 to DTM_PKT_TYPE_VENDORSPECIFIC, to avoid the
		 * risk of confusion, should the code be extended to greater
		 * coverage.
		 */
		if ((dtm_inst.radio_mode == NRF_RADIO_MODE_BLE_1MBIT ||
		     dtm_inst.radio_mode == NRF_RADIO_MODE_BLE_2MBIT)) {
			dtm_inst.packet_type = DTM_PKT_TYPE_VENDORSPECIFIC;
		} else {
			dtm_inst.packet_type = DTM_PKT_TYPE_0xFF;
		}
	}


	/* Check for illegal values of m_phys_ch. Skip the check if the
	 * packet is vendor spesific.
	 */
	if (payload != DTM_PKT_0XFF_OR_VS &&
	    dtm_inst.phys_ch > PHYS_CH_MAX) {
		/* Parameter error */
		/* Note: State is unchanged; ongoing test not affected */
		dtm_inst.event = LE_TEST_STATUS_EVENT_ERROR;
		return DTM_ERROR_ILLEGAL_CHANNEL;
	}

	dtm_inst.rx_pkt_count = 0;

	if (cmd_code == LE_RECEIVER_TEST) {
		return on_test_receive_cmd();
	}

	if (cmd_code == LE_TRANSMITTER_TEST) {
		return on_test_transmit_cmd(length, freq);
	}

	return DTM_SUCCESS;
}

bool dtm_event_get(uint16_t *dtm_event)
{
	bool was_new = dtm_inst.new_event;

	/* mark the current event as retrieved */
	dtm_inst.new_event = false;
	*dtm_event = dtm_inst.event;

	/* return value indicates whether this value was already retrieved. */
	return was_new;
}
