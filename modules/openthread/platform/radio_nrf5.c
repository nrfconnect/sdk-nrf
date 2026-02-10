/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 *   This file implements the OpenThread platform abstraction
 *   for radio communication utilizing nRF IEEE802.15.4 radio driver.
 *
 */

#include "radio_nrf5.h"

#include <openthread/error.h>
#define LOG_MODULE_NAME otPlat_nrf5_radio

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_OPENTHREAD_PLATFORM_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>

#include <openthread/ip6.h>
#include <openthread-system.h>
#include <openthread/instance.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/time.h>
#include <openthread/message.h>
#if defined(CONFIG_OPENTHREAD_NAT64_TRANSLATOR)
#include <openthread/nat64.h>
#endif

#include "nrf_802154.h"
#include "nrf_802154_const.h"

#if defined(CONFIG_NRF_802154_SER_HOST)
#include "nrf_802154_serialization_error.h"
#endif

#if defined(CONFIG_TRUSTED_EXECUTION_NONSECURE) && defined(NRF_FICR_S)
#include <soc_secure.h>
#else
#include <hal/nrf_ficr.h>
#endif

#if defined(CONFIG_OPENTHREAD_THREAD_VERSION_1_1)
#define ACK_PKT_LENGTH 5
#else
#define ACK_PKT_LENGTH MAX_PACKET_SIZE
#endif

#if defined(CONFIG_NRF5_UICR_EUI64_ENABLE)
#if defined(CONFIG_SOC_NRF5340_CPUAPP) || defined(CONFIG_SOC_SERIES_NRF54L)
#if defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
#error "NRF_UICR->OTP is not supported to read from non-secure"
#else
#define EUI64_ADDR (NRF_UICR->OTP)
#endif /* CONFIG_TRUSTED_EXECUTION_NONSECURE */
#else
#define EUI64_ADDR (NRF_UICR->CUSTOMER)
#endif /* CONFIG_SOC_NRF5340_CPUAPP || CONFIG_SOC_SERIES_NRF54L */
#endif /* CONFIG_NRF5_UICR_EUI64_ENABLE */

#if defined(CONFIG_NRF5_UICR_EUI64_ENABLE)
#define EUI64_ADDR_HIGH CONFIG_NRF5_UICR_EUI64_REG
#define EUI64_ADDR_LOW	(CONFIG_NRF5_UICR_EUI64_REG + 1)
#else
#define EUI64_ADDR_HIGH 0
#define EUI64_ADDR_LOW	1
#endif /* CONFIG_NRF5_UICR_EUI64_ENABLE */

#if defined(CONFIG_NRF5_VENDOR_OUI_ENABLE)
#define NRF5_VENDOR_OUI CONFIG_NRF5_VENDOR_OUI
#else
#define NRF5_VENDOR_OUI (uint32_t)0xF4CE36
#endif

#define CHANNEL_COUNT		       (OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MAX - \
					OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN + 1)
#define DRX_SLOT_RX		       0 /* Delayed reception window ID */
#define PHR_DURATION_US		       32U
#define NSEC_PER_TEN_SYMBOLS	       ((uint64_t)PHY_US_PER_SYMBOL * 1000 * 10)
#define NRF5_BROADCAST_ADDRESS	       0xffff
#define NRF5_NO_SHORT_ADDRESS_ASSIGNED 0xfffe

/* IE constants */
#define IE_VENDOR_SIZE_MIN 3 /* Minimum vendor OUI size in bytes */

#if defined(CONFIG_COOP_ENABLED)
#define OT_WORKER_PRIORITY K_PRIO_COOP(CONFIG_OPENTHREAD_THREAD_PRIORITY)
#else
#define OT_WORKER_PRIORITY K_PRIO_PREEMPT(CONFIG_OPENTHREAD_THREAD_PRIORITY)
#endif

#define MIN_CHANNEL_NUMBER (11)
#define MAX_CHANNEL_NUMBER (26)

#define PSDU_LENGTH(psdu) ((psdu)[0])
#define PSDU_DATA(psdu)	  ((psdu) + 1)

#define CSL_IE_SIZE (6) /* Buffer for CSL IE: 2 bytes header + 4 bytes content */
#define LM_IE_SIZE (32) /* Buffer for LM IE: 2 bytes header + 30 bytes content */

enum nrf5_pending_events {
	PENDING_EVENT_FRAME_RECEIVED,	  /* Radio has received new frame */
	PENDING_EVENT_RX_FAILED,	  /* The RX failed */
	PENDING_EVENT_TX_DONE,		  /* Radio transmission finished */
	PENDING_EVENT_DETECT_ENERGY,	  /* Requested to start Energy Detection procedure */
	PENDING_EVENT_DETECT_ENERGY_DONE, /* Energy Detection finished */
	PENDING_EVENT_SLEEP,		  /* Sleep if idle */
	PENDING_EVENT_COUNT		  /* Keep last */
};

enum nrf5_ie_type {
	NRF5_IE_TYPE_HEADER = 0x0,
	NRF5_IE_TYPE_PAYLOAD,
};

enum nrf5_header_ie_element_id {
	NRF5_HEADER_IE_ELEMENT_ID_VENDOR_SPECIFIC_IE = 0x00,
	NRF5_HEADER_IE_ELEMENT_ID_CSL_IE = 0x1a,
};

struct nrf5_header_ie_csl_reduced {
	uint16_t csl_phase;
	uint16_t csl_period;
} __packed;

struct nrf5_header_ie_link_metrics {
	uint8_t vendor_oui[IE_VENDOR_SIZE_MIN];
	uint8_t subtype; /* Vendor-specific subtype/probing ID */
	uint8_t lqi_token;
	uint8_t link_margin_token;
	uint8_t rssi_token;
} __packed;

struct nrf5_header_ie {
#if CONFIG_LITTLE_ENDIAN
	uint16_t length: 7;
	uint16_t element_id_low: 1;
	uint16_t element_id_high: 7;
	uint16_t type: 1;
#else
	uint16_t element_id_low;
	uint16_t length: 7;
	uint16_t type: 1;
	uint16_t element_id_high: 7;
#endif
	union {
		struct nrf5_header_ie_link_metrics link_metrics;
		struct nrf5_header_ie_csl_reduced csl_reduced;
	} content;
} __packed;

struct nrf5_rx_frame {
	void *fifo_reserved; /* 1st word reserved for use by fifo. */
	uint8_t *psdu;	     /* Pointer to a received frame. The first byte is PHR (length)*/
	uint64_t time;	     /* RX timestamp. */
	uint8_t lqi;	     /* Last received frame LQI value. */
	int8_t rssi;	     /* Last received frame RSSI value. */
	bool ack_fpb;	     /* FPB value in ACK sent for the received frame. */
	bool ack_seb;	     /* SEB value in ACK sent for the received frame. */
};

/** Energy detection callback */
typedef void (*nrf5_energy_detection_done_cb_t)(int16_t max_ed);

struct nrf5_data {
	ATOMIC_DEFINE(pending_events, PENDING_EVENT_COUNT);

	/* Radio state */
	otRadioState state;

	/* TX power */
	int8_t tx_power;

	/* Max TX power for channel */
	int8_t max_tx_power_table[CHANNEL_COUNT];

	/* Enable/disable RxOnWhenIdle MAC PIB attribute (Table 8-94). */
	bool rx_on_when_idle;

	/* Radio channel */
	uint8_t channel;

	/* Promiscuous mode */
	bool promiscuous;

	/* 802.15.4 HW address. */
	uint8_t mac[EXTENDED_ADDRESS_SIZE];

	/* Radio capabilities */
	otRadioCaps capabilities;

	struct {
		/* Buffers for passing received frame pointers and data to the
		 * RX thread via rx fifo object.
		 */
		struct nrf5_rx_frame frames[CONFIG_NRF_802154_RX_BUFFERS];

		/* RX fifo queue. */
		struct k_fifo fifo;

		/* Frame pending bit value in ACK sent for the last received frame. */
		bool last_frame_ack_fpb;

		/* Security Enabled bit value in ACK sent for the last received frame. */
		bool last_frame_ack_seb;

		/* RX result, updated in radio transmit callbacks. */
		otError result;
	} rx;

	struct {
		/* TX frame */
		otRadioFrame frame;

#if defined(CONFIG_OPENTHREAD_TIME_SYNC)
		otRadioIeInfo ie_info;
#endif

		/* TX buffer. First byte is PHR (length), remaining bytes are
		 * MPDU data.
		 */
		uint8_t psdu[PHR_SIZE + MAX_PACKET_SIZE];

		/* TX result, updated in radio transmit callbacks. */
		otError result;
	} tx;

	struct {
		/* A descriptor for the received ACK frame. psdu pointer be NULL if no
		 * ACK was requested/received.
		 */
		struct nrf5_rx_frame desc;

		/* ACK frame */
		otRadioFrame frame;

		/* ACK PSDU buffer */
		uint8_t psdu[ACK_PKT_LENGTH];

	} ack;

	struct {
		/* Energy detection callback */
		nrf5_energy_detection_done_cb_t cb;

		/* Duration of energy detection procedure */
		uint16_t time;

		/* Energy detection channel */
		uint8_t channel;

		/* Maximum energy detected value in dBm*/
		int16_t value;
	} energy_detection;

	/* Get RSSI complete semaphore. Unlocked when energy detect is complete. */
	struct k_sem rssi_wait;

#if defined(CONFIG_NRF_802154_SER_HOST) && defined(CONFIG_OPENTHREAD_CSL_RECEIVER)
	struct {
		/* The last configured value of CSL period in units of 10 symbols. */
		uint32_t period;
		/* The last configured value of CSL phase time in nanoseconds. */
		int64_t rx_time;
	} csl;
#endif /* CONFIG_NRF_802154_SER_HOST && CONFIG_OPENTHREAD_CSL_RECEIVER */
};

static struct nrf5_data nrf5_data;

static inline bool is_pending_event_set(enum nrf5_pending_events event)
{
	return atomic_test_bit(nrf5_data.pending_events, event);
}

static void set_pending_event(enum nrf5_pending_events event)
{
	atomic_set_bit(nrf5_data.pending_events, event);
	otSysEventSignalPending();
}

static void reset_pending_event(enum nrf5_pending_events event)
{
	atomic_clear_bit(nrf5_data.pending_events, event);
}

static int nrf5_set_channel(uint16_t channel)
{
	if (channel < MIN_CHANNEL_NUMBER || channel > MAX_CHANNEL_NUMBER) {
		return channel < MIN_CHANNEL_NUMBER ? -ENOTSUP : -EINVAL;
	}

	nrf_802154_channel_set(channel);
	nrf5_data.channel = channel;

	LOG_DBG("set channel %u", channel);

	return 0;
}

static int nrf5_energy_detection_start(uint16_t duration, nrf5_energy_detection_done_cb_t done_cb)
{
	int err = 0;

	if (nrf5_data.energy_detection.cb == NULL) {
		nrf5_data.energy_detection.cb = done_cb;

		if (nrf_802154_energy_detection((uint32_t)duration * NSEC_PER_USEC) == false) {
			nrf5_data.energy_detection.cb = NULL;
			err = -EBUSY;
		}
	} else {
		err = -EALREADY;
	}

	return err;
}

static int8_t get_transmit_power_for_channel(uint8_t aChannel)
{
	int8_t channel_max_power = OT_RADIO_POWER_INVALID;
	int8_t power = 0; /* 0 dbm as default value */

	if (aChannel >= OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN &&
	    aChannel <= OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MAX) {
		channel_max_power =
			nrf5_data.max_tx_power_table[aChannel - OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN];
	}

	if (nrf5_data.tx_power != OT_RADIO_POWER_INVALID) {
		power = (channel_max_power < nrf5_data.tx_power) ? channel_max_power
								 : nrf5_data.tx_power;
	} else if (channel_max_power != OT_RADIO_POWER_INVALID) {
		power = channel_max_power;
	}

	return power;
}

static int nrf5_set_tx_power(uint16_t channel)
{
	int8_t tx_power = get_transmit_power_for_channel(channel);

	nrf5_data.tx_power = tx_power;
	nrf_802154_tx_power_set(tx_power);

	LOG_DBG("set tx_power %d", tx_power);

	return 0;
}

#if defined(CONFIG_OPENTHREAD_CSL_RECEIVER) || defined(CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT)
/**
 * @brief Set ACK data for both short and extended addresses
 *
 * This function handles the address conversion and calls the nRF driver
 * to set ACK data for both address types.
 *
 * @param short_addr Short address
 * @param ext_addr Extended address
 * @param header_ie_buf Buffer containing the IE data
 * @param ie_length Length of the IE data in bytes
 * @return 0 on success, negative error code on failure
 */
static int nrf5_ack_data_set(uint16_t short_addr, const otExtAddress *ext_addr,
			     const uint8_t *header_ie_buf, size_t ie_length)
{
	uint8_t ext_addr_le[EXTENDED_ADDRESS_SIZE];
	uint8_t short_addr_le[SHORT_ADDRESS_SIZE];

	if (short_addr == NRF5_BROADCAST_ADDRESS || ext_addr == NULL) {
		return -ENOTSUP;
	}

	sys_put_le16(short_addr, short_addr_le);
	sys_memcpy_swap(ext_addr_le, ext_addr->m8, EXTENDED_ADDRESS_SIZE);

	if (short_addr != NRF5_NO_SHORT_ADDRESS_ASSIGNED) {
		nrf_802154_ack_data_set(short_addr_le, false, header_ie_buf, ie_length,
					NRF_802154_ACK_DATA_IE);
	}
	nrf_802154_ack_data_set(ext_addr_le, true, header_ie_buf, ie_length,
				NRF_802154_ACK_DATA_IE);

	return 0;
}

static int nrf5_ack_data_clear(uint16_t short_addr, const otExtAddress *ext_addr)
{
	uint8_t ext_addr_le[EXTENDED_ADDRESS_SIZE];
	uint8_t short_addr_le[SHORT_ADDRESS_SIZE];

	if (short_addr == NRF5_BROADCAST_ADDRESS || ext_addr == NULL) {
		return -ENOTSUP;
	}

	sys_put_le16(short_addr, short_addr_le);
	sys_memcpy_swap(ext_addr_le, ext_addr->m8, EXTENDED_ADDRESS_SIZE);

	if (short_addr != NRF5_NO_SHORT_ADDRESS_ASSIGNED) {
		nrf_802154_ack_data_clear(short_addr_le, false, NRF_802154_ACK_DATA_IE);
	}
	nrf_802154_ack_data_clear(ext_addr_le, true, NRF_802154_ACK_DATA_IE);

	return 0;
}
#endif

#if defined(CONFIG_OPENTHREAD_CSL_RECEIVER)
/**
 * @brief Create CSL IE using structured format with bit fields
 *
 * Creates a CSL Information Element using the defined structs with proper IEEE 802.15.4 format
 *
 * @param csl_period CSL period value
 * @param ie_buffer Buffer to store the IE (must be at least sizeof(struct nrf5_header_ie) bytes)
 * @return Length of the created IE in bytes
 */
static size_t create_csl_ie(uint32_t csl_period, uint8_t *ie_buffer)
{
	if (ie_buffer == NULL) {
		return 0;
	}

	struct nrf5_header_ie csl_ie;
	const uint8_t csl_element_id = NRF5_HEADER_IE_ELEMENT_ID_CSL_IE;

	memset(&csl_ie, 0, sizeof(csl_ie));

	csl_ie.length = sizeof(struct nrf5_header_ie_csl_reduced);
	csl_ie.element_id_low = csl_element_id & 0x01;
	csl_ie.element_id_high = (csl_element_id >> 1) & 0x7f;
	csl_ie.type = NRF5_IE_TYPE_HEADER;
	csl_ie.content.csl_reduced.csl_phase = sys_cpu_to_le16(0);
	csl_ie.content.csl_reduced.csl_period = sys_cpu_to_le16(csl_period);

	size_t csl_ie_size = sizeof(uint16_t) + sizeof(struct nrf5_header_ie_csl_reduced);

	memcpy(ie_buffer, &csl_ie, csl_ie_size);

	return csl_ie_size;
}
#endif

static void nrf5_get_eui64(uint8_t *mac)
{
	__ASSERT(mac != NULL, "nrf5_get_eui64: mac is NULL");

	uint64_t factoryAddress;
	uint32_t index = 0;

#if !defined(CONFIG_NRF5_UICR_EUI64_ENABLE)
	uint32_t deviceid[2];

	/* Set the MAC Address Block Larger (MA-L) formerly called OUI. */
	mac[index++] = (NRF5_VENDOR_OUI >> 16) & 0xff;
	mac[index++] = (NRF5_VENDOR_OUI >> 8) & 0xff;
	mac[index++] = NRF5_VENDOR_OUI & 0xff;

#if defined(NRF54H_SERIES)
	/* Can't access SICR with device id on a radio core. Use BLE.ADDR. */
	deviceid[0] = NRF_FICR->BLE.ADDR[0];
	deviceid[1] = NRF_FICR->BLE.ADDR[1];
#elif defined(CONFIG_TRUSTED_EXECUTION_NONSECURE) && defined(NRF_FICR_S)
	soc_secure_read_deviceid(deviceid);
#else
	deviceid[0] = nrf_ficr_deviceid_get(NRF_FICR, 0);
	deviceid[1] = nrf_ficr_deviceid_get(NRF_FICR, 1);
#endif

	factoryAddress = (uint64_t)deviceid[EUI64_ADDR_HIGH] << 32;
	factoryAddress |= deviceid[EUI64_ADDR_LOW];
#else
	/* Use device identifier assigned during the production. */
	factoryAddress = (uint64_t)EUI64_ADDR[EUI64_ADDR_HIGH] << 32;
	factoryAddress |= EUI64_ADDR[EUI64_ADDR_LOW];
#endif
	memcpy(mac + index, &factoryAddress, sizeof(factoryAddress) - index);
}

static otRadioCaps nrf5_get_caps(void)
{
	otRadioCaps caps = OT_RADIO_CAPS_NONE;

	nrf_802154_capabilities_t radio_caps = nrf_802154_capabilities_get();

	caps |= OT_RADIO_CAPS_ENERGY_SCAN | OT_RADIO_CAPS_ACK_TIMEOUT | OT_RADIO_CAPS_SLEEP_TO_TX |
		OT_RADIO_CAPS_RX_ON_WHEN_IDLE;

	if (radio_caps & NRF_802154_CAPABILITY_CSMA) {
		caps |= OT_RADIO_CAPS_CSMA_BACKOFF;
	}

#if !defined(CONFIG_OPENTHREAD_THREAD_VERSION_1_1)
	if (radio_caps & NRF_802154_CAPABILITY_SECURITY) {
		caps |= OT_RADIO_CAPS_TRANSMIT_SEC;
	}
#endif

	if (radio_caps & NRF_802154_CAPABILITY_DELAYED_TX) {
		caps |= OT_RADIO_CAPS_TRANSMIT_TIMING;
	}

	if (radio_caps & NRF_802154_CAPABILITY_DELAYED_RX) {
		caps |= OT_RADIO_CAPS_RECEIVE_TIMING;
	}

	return caps;
}

/**
 * @brief Convert 32-bit (potentially wrapped) OpenThread microsecond timestamps
 * to 64-bit Zephyr network subsystem nanosecond timestamps.
 *
 * This is a workaround until OpenThread is able to schedule 64-bit RX/TX time.
 *
 * @param target_time_ns_wrapped time in nanoseconds referred to the radio clock
 * modulo UINT32_MAX.
 *
 * @return 64-bit nanosecond timestamp
 */
static int64_t convert_32bit_us_wrapped_to_64bit_ns(uint32_t target_time_us_wrapped)
{
	/**
	 * OpenThread provides target time as a (potentially wrapped) 32-bit
	 * integer defining a moment in time in the microsecond domain.
	 *
	 * The target time can point to a moment in the future, but can be
	 * overdue as well. In order to determine what's the case and correctly
	 * set the absolute (non-wrapped) target time, it's necessary to compare
	 * the least significant 32 bits of the current 64-bit network subsystem
	 * time with the provided 32-bit target time. Let's assume that half of
	 * the 32-bit range can be used for specifying target times in the
	 * future, and the other half - in the past.
	 */
	uint64_t now_us = otPlatTimeGet();
	uint32_t now_us_wrapped = (uint32_t)now_us;
	uint32_t time_diff = target_time_us_wrapped - now_us_wrapped;
	uint64_t result = UINT64_C(0);

	if (time_diff < 0x80000000) {
		/**
		 * Target time is assumed to be in the future. Check if a 32-bit overflow
		 * occurs between the current time and the target time.
		 */
		if (now_us_wrapped > target_time_us_wrapped) {
			/**
			 * Add a 32-bit overflow and replace the least significant 32 bits
			 * with the provided target time.
			 */
			result = now_us + UINT32_MAX + 1;
			result &= ~(uint64_t)UINT32_MAX;
			result |= target_time_us_wrapped;
		} else {
			/**
			 * Leave the most significant 32 bits and replace the least significant
			 * 32 bits with the provided target time.
			 */
			result = (now_us & (~(uint64_t)UINT32_MAX)) | target_time_us_wrapped;
		}
	} else {
		/**
		 * Target time is assumed to be in the past. Check if a 32-bit overflow
		 * occurs between the target time and the current time.
		 */
		if (now_us_wrapped > target_time_us_wrapped) {
			/**
			 * Leave the most significant 32 bits and replace the least significant
			 * 32 bits with the provided target time.
			 */
			result = (now_us & (~(uint64_t)UINT32_MAX)) | target_time_us_wrapped;
		} else {
			/**
			 * Subtract a 32-bit overflow and replace the least significant
			 * 32 bits with the provided target time.
			 */
			result = now_us - UINT32_MAX - 1;
			result &= ~(uint64_t)UINT32_MAX;
			result |= target_time_us_wrapped;
		}
	}

	__ASSERT_NO_MSG(result <= INT64_MAX / NSEC_PER_USEC);
	return (int64_t)result * NSEC_PER_USEC;
}

void platformRadioInit(void)
{
	nrf5_data.state = OT_RADIO_STATE_DISABLED;

	/* Get the default tx output power from Kconfig */
	nrf5_data.tx_power = CONFIG_OPENTHREAD_DEFAULT_TX_POWER;

	for (size_t i = 0; i < CHANNEL_COUNT; i++) {
		nrf5_data.max_tx_power_table[i] = OT_RADIO_POWER_INVALID;
	}

	nrf5_data.rx_on_when_idle = true;

	nrf5_get_eui64(nrf5_data.mac);

	k_fifo_init(&nrf5_data.rx.fifo);

	nrf5_data.tx.frame.mPsdu = PSDU_DATA(nrf5_data.tx.psdu);
#if defined(CONFIG_OPENTHREAD_TIME_SYNC)
	nrf5_data.tx.frame.mInfo.mTxInfo.mIeInfo = &nrf5_data.tx.ie_info;
#endif

	k_sem_init(&nrf5_data.rssi_wait, 0, 1);

	nrf_802154_init();

	nrf5_data.capabilities = nrf5_get_caps();
}

static void openthread_handle_received_frame(otInstance *instance, struct nrf5_rx_frame *rx_frame)
{
	otRadioFrame recv_frame;
	uint8_t *psdu;

	ARG_UNUSED(instance);

	__ASSERT_NO_MSG(rx_frame->psdu != NULL);

	memset(&recv_frame, 0, sizeof(otRadioFrame));

	recv_frame.mPsdu = PSDU_DATA(rx_frame->psdu);
	/* Length inc. CRC. */
	recv_frame.mLength = PSDU_LENGTH(rx_frame->psdu);
	recv_frame.mChannel = nrf5_data.channel;
	recv_frame.mInfo.mRxInfo.mLqi = rx_frame->lqi;
	recv_frame.mInfo.mRxInfo.mRssi = rx_frame->rssi;
	recv_frame.mInfo.mRxInfo.mAckedWithFramePending = rx_frame->ack_fpb;
	recv_frame.mInfo.mRxInfo.mTimestamp = rx_frame->time;
	recv_frame.mInfo.mRxInfo.mAckedWithSecEnhAck = rx_frame->ack_seb;

	LOG_DBG("RX %p len: %u, ch: %u, rssi: %d", (void *)recv_frame.mPsdu, recv_frame.mLength,
		recv_frame.mChannel, recv_frame.mInfo.mRxInfo.mRssi);

	if (IS_ENABLED(CONFIG_OPENTHREAD_DIAG) && otPlatDiagModeGet()) {
		otPlatDiagRadioReceiveDone(instance, &recv_frame, OT_ERROR_NONE);
	} else {
		otPlatRadioReceiveDone(instance, &recv_frame, OT_ERROR_NONE);
	}

	psdu = rx_frame->psdu;
	rx_frame->psdu = NULL;
	nrf_802154_buffer_free_raw(psdu);
}

static void energy_detected(int16_t max_ed)
{
	nrf5_data.energy_detection.value = max_ed;
	set_pending_event(PENDING_EVENT_DETECT_ENERGY_DONE);
}

static bool nrf5_tx(const otRadioFrame *frame, uint8_t *payload, bool cca)
{
	if (payload == NULL) {
		return false;
	}

	nrf_802154_transmit_metadata_t metadata = {
		.frame_props = {
			.is_secured = frame->mInfo.mTxInfo.mIsSecurityProcessed,
			.dynamic_data_is_set = frame->mInfo.mTxInfo.mIsHeaderUpdated,
		},
		.cca = cca,
		.tx_power = {
			.use_metadata_value = true,
			.power = get_transmit_power_for_channel(frame->mChannel),
		},
	};

	nrf_802154_tx_error_t result = nrf_802154_transmit_raw(payload, &metadata);
	__ASSERT(result != NRF_802154_TX_ERROR_INVALID_REQUEST, "Invalid transmit request");

	return result == NRF_802154_TX_ERROR_NONE;
}

#if NRF_802154_CSMA_CA_ENABLED
static bool nrf5_tx_csma_ca(otRadioFrame *frame, uint8_t *payload)
{
	if (payload == NULL) {
		return false;
	}

	nrf_802154_transmit_csma_ca_metadata_t metadata = {
		.frame_props = {
			.is_secured = frame->mInfo.mTxInfo.mIsSecurityProcessed,
			.dynamic_data_is_set = frame->mInfo.mTxInfo.mIsHeaderUpdated,
		},
		.tx_power = {
			.use_metadata_value = true,
			.power = get_transmit_power_for_channel(frame->mChannel),
		},
	};

	nrf_802154_csma_ca_max_backoffs_set(frame->mInfo.mTxInfo.mMaxCsmaBackoffs);

	nrf_802154_tx_error_t result = nrf_802154_transmit_csma_ca_raw(payload, &metadata);
	__ASSERT(result != NRF_802154_TX_ERROR_INVALID_REQUEST, "Invalid transmit request");

	return result == NRF_802154_TX_ERROR_NONE;
}
#endif

static bool nrf5_tx_at(otRadioFrame *frame, uint8_t *payload)
{
	if (payload == NULL) {
		return false;
	}

	nrf_802154_transmit_at_metadata_t metadata = {
		.frame_props = {
			.is_secured = frame->mInfo.mTxInfo.mIsSecurityProcessed,
			.dynamic_data_is_set = frame->mInfo.mTxInfo.mIsHeaderUpdated,
		},
		.cca = true,
#if defined(CONFIG_NRF5_SELECTIVE_TXCHANNEL)
		.channel = frame->mChannel,
#else
		.channel = nrf_802154_channel_get(),
#endif
		.tx_power = {
			.use_metadata_value = true,
			.power = get_transmit_power_for_channel(frame->mChannel),
		},
	};

	/* The timestamp points to the start of PHR but `nrf_802154_transmit_raw_at`
	 * expects a timestamp pointing to start of SHR.
	 */
	uint64_t tx_at = nrf_802154_timestamp_phr_to_shr_convert(
		convert_32bit_us_wrapped_to_64bit_ns(
			nrf5_data.tx.frame.mInfo.mTxInfo.mTxDelayBaseTime +
			nrf5_data.tx.frame.mInfo.mTxInfo.mTxDelay) /
		NSEC_PER_USEC);

	nrf_802154_tx_error_t result = nrf_802154_transmit_raw_at(payload, tx_at, &metadata);
	__ASSERT(result != NRF_802154_TX_ERROR_INVALID_REQUEST, "Invalid transmit request");

	return result == NRF_802154_TX_ERROR_NONE;
}

static void handle_frame_received(otInstance *aInstance)
{
	struct nrf5_rx_frame *rx_frame;

	while ((rx_frame = (struct nrf5_rx_frame *)k_fifo_get(&nrf5_data.rx.fifo, K_NO_WAIT)) !=
	       NULL) {
		openthread_handle_received_frame(aInstance, rx_frame);
	}
}

static void handle_rx_failed(otInstance *aInstance)
{
	if (IS_ENABLED(CONFIG_OPENTHREAD_DIAG) && otPlatDiagModeGet()) {
		otPlatDiagRadioReceiveDone(aInstance, NULL, nrf5_data.rx.result);
	} else {
		otPlatRadioReceiveDone(aInstance, NULL, nrf5_data.rx.result);
	}
}

static otError transmit_frame(otInstance *aInstance)
{
	bool result = true;

	ARG_UNUSED(aInstance);

	if (nrf5_data.tx.frame.mLength > MAX_PACKET_SIZE) {
		LOG_ERR("Payload (with FCS) too large: %d", nrf5_data.tx.frame.mLength);
		return OT_ERROR_INVALID_ARGS;
	}

	LOG_DBG("TX %p len: %u", (void *)nrf5_data.tx.frame.mPsdu, nrf5_data.tx.frame.mLength);

	PSDU_LENGTH(nrf5_data.tx.psdu) = nrf5_data.tx.frame.mLength;

#if defined(CONFIG_OPENTHREAD_TIME_SYNC)
	if (nrf5_data.tx.frame.mInfo.mTxInfo.mIeInfo->mTimeIeOffset != 0) {
		uint8_t *time_ie = nrf5_data.tx.frame.mPsdu +
				   nrf5_data.tx.frame.mInfo.mTxInfo.mIeInfo->mTimeIeOffset;
		uint64_t offset_plat_time =
			otPlatTimeGet() +
			nrf5_data.tx.frame.mInfo.mTxInfo.mIeInfo->mNetworkTimeOffset;

		*(time_ie++) = nrf5_data.tx.frame.mInfo.mTxInfo.mIeInfo->mTimeSyncSeq;
		sys_put_le64(offset_plat_time, time_ie);
	}
#endif

	if ((nrf5_data.capabilities & OT_RADIO_CAPS_TRANSMIT_TIMING) &&
	    (nrf5_data.tx.frame.mInfo.mTxInfo.mTxDelay != 0)) {

		if (!IS_ENABLED(CONFIG_NRF5_SELECTIVE_TXCHANNEL)) {
			nrf5_set_channel(nrf5_data.tx.frame.mChannel);
		}

		if (!nrf5_tx_at(&nrf5_data.tx.frame, nrf5_data.tx.psdu)) {
			LOG_WRN("TX AT failed");
			nrf5_data.tx.result = OT_ERROR_ABORT;
			set_pending_event(PENDING_EVENT_TX_DONE);
			return OT_ERROR_NONE;
		}
	} else if (nrf5_data.tx.frame.mInfo.mTxInfo.mCsmaCaEnabled) {
		nrf5_set_channel(nrf5_data.tx.frame.mChannel);
		if (nrf5_data.capabilities & OT_RADIO_CAPS_CSMA_BACKOFF) {
			result = nrf5_tx_csma_ca(&nrf5_data.tx.frame, nrf5_data.tx.psdu);
		} else {
			result = nrf5_tx(&nrf5_data.tx.frame, nrf5_data.tx.psdu, true);
		}
	} else {
		nrf5_set_channel(nrf5_data.tx.frame.mChannel);
		result = nrf5_tx(&nrf5_data.tx.frame, nrf5_data.tx.psdu, false);
	}

	otPlatRadioTxStarted(aInstance, &nrf5_data.tx.frame);

	if (!result) {
		LOG_ERR("TX failed");
		nrf5_data.tx.result = OT_ERROR_CHANNEL_ACCESS_FAILURE;
		set_pending_event(PENDING_EVENT_TX_DONE);
	}

	return OT_ERROR_NONE;
}

static otError handle_ack(void)
{
	uint8_t ack_len;
	uint8_t frame_type;
	otError err = OT_ERROR_NONE;

	if (nrf5_data.ack.desc.time == NRF_802154_NO_TIMESTAMP) {
		/* Ack timestamp is invalid and cannot be used by the upper layer.
		 * Report the transmission as failed as if the Ack was not received at all.
		 */
		LOG_WRN("Invalid ACK timestamp.");
		err = OT_ERROR_NO_ACK;
		goto free_nrf_ack;
	}

	ack_len = PSDU_LENGTH(nrf5_data.ack.desc.psdu);
	if (ack_len > ACK_PKT_LENGTH) {
		LOG_ERR("Invalid ACK length %u", ack_len);
		err = OT_ERROR_NO_ACK;
		goto free_nrf_ack;
	}

	frame_type = *(PSDU_DATA(nrf5_data.ack.desc.psdu)) & FRAME_TYPE_MASK;
	if (frame_type != FRAME_TYPE_ACK) {
		LOG_ERR("Invalid frame type %u", frame_type);
		err = OT_ERROR_NO_ACK;
		goto free_nrf_ack;
	}

	if (nrf5_data.ack.frame.mLength != 0) {
		LOG_ERR("Overwriting unhandled ACK frame.");
	}

	/* Upper layers expect the frame to start at the MAC header, skip the
	 * PHY header (1 byte).
	 */
	memcpy(nrf5_data.ack.psdu, PSDU_DATA(nrf5_data.ack.desc.psdu), ack_len);

	nrf5_data.ack.frame.mPsdu = nrf5_data.ack.psdu;
	nrf5_data.ack.frame.mLength = ack_len;
	nrf5_data.ack.frame.mInfo.mRxInfo.mLqi = nrf5_data.ack.desc.lqi;
	nrf5_data.ack.frame.mInfo.mRxInfo.mRssi = nrf5_data.ack.desc.rssi;
	nrf5_data.ack.frame.mInfo.mRxInfo.mTimestamp = nrf5_data.ack.desc.time;

free_nrf_ack:
	nrf_802154_buffer_free_raw(nrf5_data.ack.desc.psdu);
	nrf5_data.ack.desc.psdu = NULL;

	return err;
}

static void handle_tx_done(otInstance *aInstance)
{
	if (nrf5_data.state == OT_RADIO_STATE_TRANSMIT) {
		nrf5_data.state = OT_RADIO_STATE_RECEIVE;

		if (nrf5_data.tx.result == OT_ERROR_NONE) {
			if (nrf5_data.ack.desc.psdu == NULL) {
				/* No ACK was requested. */
				nrf5_data.tx.result = OT_ERROR_NONE;
			} else {

				/* Handle ACK packet. */
				nrf5_data.tx.result = handle_ack();
			}
		}

		if (IS_ENABLED(CONFIG_OPENTHREAD_DIAG) && otPlatDiagModeGet()) {
			otPlatDiagRadioTransmitDone(aInstance, &nrf5_data.tx.frame,
						    nrf5_data.tx.result);
		} else {
			otPlatRadioTxDone(aInstance, &nrf5_data.tx.frame,
					  nrf5_data.ack.frame.mLength ? &nrf5_data.ack.frame : NULL,
					  nrf5_data.tx.result);
			nrf5_data.ack.frame.mLength = 0;
		}
	}
}

static void handle_sleep(otInstance *aInstance)
{
	ARG_UNUSED(otPlatRadioSleep(aInstance));
}

static bool handle_detect_energy(otInstance *aInstance)
{
	nrf5_set_channel(nrf5_data.energy_detection.channel);

	return nrf5_energy_detection_start(nrf5_data.energy_detection.time, energy_detected);
}

static void handle_detect_energy_done(otInstance *aInstance)
{
	otPlatRadioEnergyScanDone(aInstance, (int8_t)nrf5_data.energy_detection.value);
}

static void get_rssi_energy_detected(int16_t max_ed)
{
	nrf5_data.energy_detection.value = max_ed;
	k_sem_give(&nrf5_data.rssi_wait);
}

void platformRadioProcess(otInstance *aInstance)
{
	bool event_pending = false;

	if (is_pending_event_set(PENDING_EVENT_FRAME_RECEIVED)) {
		reset_pending_event(PENDING_EVENT_FRAME_RECEIVED);
		handle_frame_received(aInstance);
	}

	if (is_pending_event_set(PENDING_EVENT_RX_FAILED)) {
		reset_pending_event(PENDING_EVENT_RX_FAILED);
		handle_rx_failed(aInstance);
	}

	if (is_pending_event_set(PENDING_EVENT_TX_DONE)) {
		reset_pending_event(PENDING_EVENT_TX_DONE);
		handle_tx_done(aInstance);
	}

	if (is_pending_event_set(PENDING_EVENT_SLEEP)) {
		reset_pending_event(PENDING_EVENT_SLEEP);
		handle_sleep(aInstance);
	}

	/* handle events that can't run during transmission */
	if (nrf5_data.state != OT_RADIO_STATE_TRANSMIT) {
		if (is_pending_event_set(PENDING_EVENT_DETECT_ENERGY)) {
			if (handle_detect_energy(aInstance)) {
				reset_pending_event(PENDING_EVENT_DETECT_ENERGY);
			} else {
				event_pending = true;
			}
		}

		if (is_pending_event_set(PENDING_EVENT_DETECT_ENERGY_DONE)) {
			handle_detect_energy_done(aInstance);
			reset_pending_event(PENDING_EVENT_DETECT_ENERGY_DONE);
		}
	}

	if (event_pending) {
		otSysEventSignalPending();
	}
}

uint16_t platformRadioChannelGet(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	return nrf5_data.channel;
}

#if defined(CONFIG_OPENTHREAD_DIAG)
void platformRadioChannelSet(uint8_t aChannel)
{
	nrf5_data.channel = aChannel;
}
#endif

/* Radio configuration */

otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	return nrf5_data.capabilities;
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	return CONFIG_OPENTHREAD_DEFAULT_RX_SENSITIVITY;
}

void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
	ARG_UNUSED(aInstance);

	__ASSERT(aIeeeEui64 != NULL, "aIeeeEui64 is NULL");

	memcpy(aIeeeEui64, nrf5_data.mac, EXTENDED_ADDRESS_SIZE);
}

void otPlatRadioSetPanId(otInstance *aInstance, otPanId aPanId)
{
	uint8_t pan_id_le[2];

	ARG_UNUSED(aInstance);

	LOG_DBG("PanId: 0x%x", aPanId);

	sys_put_le16(aPanId, pan_id_le);
	nrf_802154_pan_id_set(pan_id_le);
}

void otPlatRadioSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtAddress)
{
	const uint8_t *ieee_addr = aExtAddress->m8;

	ARG_UNUSED(aInstance);

	LOG_DBG("IEEE address %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", ieee_addr[7], ieee_addr[6],
		ieee_addr[5], ieee_addr[4], ieee_addr[3], ieee_addr[2], ieee_addr[1], ieee_addr[0]);

	nrf_802154_extended_address_set(ieee_addr);
}

void otPlatRadioSetShortAddress(otInstance *aInstance, otShortAddress aShortAddress)
{
	uint8_t short_addr_le[2];

	ARG_UNUSED(aInstance);

	LOG_DBG("Short Address: 0x%x", aShortAddress);

	sys_put_le16(aShortAddress, short_addr_le);
	nrf_802154_short_address_set(short_addr_le);
}

otError otPlatRadioGetTransmitPower(otInstance *aInstance, int8_t *aPower)
{
	ARG_UNUSED(aInstance);

	if (aPower == NULL) {
		return OT_ERROR_INVALID_ARGS;
	}

	*aPower = nrf5_data.tx_power;

	return OT_ERROR_NONE;
}

otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower)
{
	ARG_UNUSED(aInstance);

	nrf5_data.tx_power = aPower;

	return OT_ERROR_NONE;
}

otError otPlatRadioGetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t *aThreshold)
{
	OT_UNUSED_VARIABLE(aInstance);
	OT_UNUSED_VARIABLE(aThreshold);

	return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatRadioSetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t aThreshold)
{
	OT_UNUSED_VARIABLE(aInstance);
	OT_UNUSED_VARIABLE(aThreshold);

	return OT_ERROR_NOT_IMPLEMENTED;
}

bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	LOG_DBG("PromiscuousMode=%d", nrf5_data.promiscuous ? 1 : 0);

	return nrf5_data.promiscuous;
}

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
	ARG_UNUSED(aInstance);

	LOG_DBG("PromiscuousMode=%d", aEnable ? 1 : 0);

	nrf5_data.promiscuous = aEnable;

	nrf_802154_promiscuous_set(nrf5_data.promiscuous);
}

void otPlatRadioSetRxOnWhenIdle(otInstance *aInstance, bool aEnable)
{
	ARG_UNUSED(aInstance);

	LOG_DBG("RxOnWhenIdle=%d", aEnable ? 1 : 0);

	nrf5_data.rx_on_when_idle = aEnable;
	nrf_802154_rx_on_when_idle_set(nrf5_data.rx_on_when_idle);

	if (!nrf5_data.rx_on_when_idle) {
		(void)nrf_802154_sleep_if_idle();
	}
}

#if !defined(CONFIG_OPENTHREAD_THREAD_VERSION_1_1)
static void nrf5_key_store(uint8_t *key_value, nrf_802154_key_id_mode_t key_id_mode,
			   uint8_t *key_id)
{
	nrf_802154_key_t key = {
		.value.p_cleartext_key = key_value,
		.id.mode = key_id_mode,
		.id.p_key_id = key_id,
		.type = NRF_802154_KEY_CLEARTEXT,
		.frame_counter = 0,
		.use_global_frame_counter = true,
	};

	__ASSERT_EVAL((void)nrf_802154_security_key_store(&key),
		nrf_802154_security_error_t err = nrf_802154_security_key_store(&key),
		err == NRF_802154_SECURITY_ERROR_NONE ||
		err == NRF_802154_SECURITY_ERROR_ALREADY_PRESENT,
		"Storing key failed, err: %d", err);
}

void otPlatRadioSetMacKey(otInstance *aInstance, uint8_t aKeyIdMode, uint8_t aKeyId,
			  const otMacKeyMaterial *aPrevKey, const otMacKeyMaterial *aCurrKey,
			  const otMacKeyMaterial *aNextKey, otRadioKeyType aKeyType)
{
	ARG_UNUSED(aInstance);
	__ASSERT_NO_MSG(aPrevKey != NULL && aCurrKey != NULL && aNextKey != NULL);

#if defined(CONFIG_OPENTHREAD_PLATFORM_KEYS_EXPORTABLE_ENABLE)
	__ASSERT_NO_MSG(aKeyType == OT_KEY_TYPE_KEY_REF);
	size_t keyLen;
	otError error;

	error = otPlatCryptoExportKey(aPrevKey->mKeyMaterial.mKeyRef,
				      (uint8_t *)aPrevKey->mKeyMaterial.mKey.m8, OT_MAC_KEY_SIZE,
				      &keyLen);
	__ASSERT_NO_MSG(error == OT_ERROR_NONE);
	error = otPlatCryptoExportKey(aCurrKey->mKeyMaterial.mKeyRef,
				      (uint8_t *)aCurrKey->mKeyMaterial.mKey.m8, OT_MAC_KEY_SIZE,
				      &keyLen);
	__ASSERT_NO_MSG(error == OT_ERROR_NONE);
	error = otPlatCryptoExportKey(aNextKey->mKeyMaterial.mKeyRef,
				      (uint8_t *)aNextKey->mKeyMaterial.mKey.m8, OT_MAC_KEY_SIZE,
				      &keyLen);
	__ASSERT_NO_MSG(error == OT_ERROR_NONE);
#else
	__ASSERT_NO_MSG(aKeyType == OT_KEY_TYPE_LITERAL_KEY);
#endif

	uint8_t key_id_mode = aKeyIdMode >> 3;

	if (key_id_mode == 1) {
		__ASSERT_NO_MSG(NRF_802154_SECURITY_KEY_STORAGE_SIZE >= 3);

		/* aKeyId in range: (1, 0x80) means valid keys */
		uint8_t prev_key_id = aKeyId == 1 ? 0x80 : aKeyId - 1;
		uint8_t next_key_id = aKeyId == 0x80 ? 1 : aKeyId + 1;

		nrf_802154_security_key_remove_all();

		nrf5_key_store((uint8_t *)aPrevKey->mKeyMaterial.mKey.m8, key_id_mode,
			       &prev_key_id);
		nrf5_key_store((uint8_t *)aCurrKey->mKeyMaterial.mKey.m8, key_id_mode, &aKeyId);
		nrf5_key_store((uint8_t *)aNextKey->mKeyMaterial.mKey.m8, key_id_mode,
			       &next_key_id);

	} else {
		/* aKeyId == 0 is used only to clear keys for stack reset in RCP */
		__ASSERT_NO_MSG((key_id_mode == 0) && (aKeyId == 0));

		nrf_802154_security_key_remove_all();
	}
}

void otPlatRadioSetMacFrameCounter(otInstance *aInstance, uint32_t aMacFrameCounter)
{
	ARG_UNUSED(aInstance);

	nrf_802154_security_global_frame_counter_set(aMacFrameCounter);
}

void otPlatRadioSetMacFrameCounterIfLarger(otInstance *aInstance, uint32_t aMacFrameCounter)
{
	ARG_UNUSED(aInstance);

	nrf_802154_security_global_frame_counter_set_if_larger(aMacFrameCounter);
}
#endif

/* Radio operations */

uint64_t otPlatTimeGet(void)
{
	return nrf_802154_time_get();
}

uint64_t otPlatRadioGetNow(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	return otPlatTimeGet();
}

otRadioState otPlatRadioGetState(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	return nrf5_data.state;
}

otError otPlatRadioEnable(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	if (nrf5_data.state != OT_RADIO_STATE_DISABLED && nrf5_data.state != OT_RADIO_STATE_SLEEP) {
		return OT_ERROR_INVALID_STATE;
	}

	nrf5_data.state = OT_RADIO_STATE_SLEEP;
	return OT_ERROR_NONE;
}

otError otPlatRadioDisable(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	if (nrf5_data.state != OT_RADIO_STATE_DISABLED && nrf5_data.state != OT_RADIO_STATE_SLEEP) {
		return OT_ERROR_INVALID_STATE;
	}

	nrf5_data.state = OT_RADIO_STATE_DISABLED;
	return OT_ERROR_NONE;
}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	return (nrf5_data.state != OT_RADIO_STATE_DISABLED) ? true : false;
}

otError otPlatRadioSleep(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	if (nrf5_data.state != OT_RADIO_STATE_SLEEP && nrf5_data.state != OT_RADIO_STATE_RECEIVE) {
		return OT_ERROR_INVALID_STATE;
	}

#if defined(CONFIG_OPENTHREAD_CSL_RECEIVER)
	if (nrf_802154_sleep_if_idle() != NRF_802154_SLEEP_ERROR_NONE) {
		set_pending_event(PENDING_EVENT_SLEEP);
		Z_SPIN_DELAY(1);
	}
#else
	if (!nrf_802154_sleep()) {
		LOG_ERR("Error while stopping radio");
		return OT_ERROR_FAILED;
	}
#endif

	LOG_DBG("nRF5 OT radio stopped");

	nrf5_data.state = OT_RADIO_STATE_SLEEP;

	return OT_ERROR_NONE;
}

otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
	ARG_UNUSED(aInstance);

	if (nrf5_data.state == OT_RADIO_STATE_DISABLED) {
		return OT_ERROR_INVALID_STATE;
	}

	nrf5_data.channel = aChannel;

	nrf5_set_channel(nrf5_data.channel);
	nrf5_set_tx_power(nrf5_data.channel);

	if (!nrf_802154_receive()) {
		LOG_ERR("Failed to enter receive state");
		return OT_ERROR_FAILED;
	}

	nrf5_data.state = OT_RADIO_STATE_RECEIVE;

	LOG_DBG("nRF5 OT radio RX started (channel: %d)", nrf5_data.channel);

	return OT_ERROR_NONE;
}

#if defined(CONFIG_OPENTHREAD_CSL_RECEIVER) || defined(CONFIG_OPENTHREAD_WAKEUP_END_DEVICE)
otError otPlatRadioReceiveAt(otInstance *aInstance, uint8_t aChannel, uint32_t aStart,

			     uint32_t aDuration)
{
	bool result;

	ARG_UNUSED(aInstance);

	/* Note that even if the nrf_802154_receive_at function is not called in time
	 * (for example due to the call being blocked by higher priority threads) and
	 * the delayed reception window is not scheduled, the CSL phase will still be
	 * calculated as if the following reception windows were at times
	 * anchor_time + n * csl_period. The previously set
	 * anchor_time will be used for calculations.
	 */
	result = nrf_802154_receive_at(convert_32bit_us_wrapped_to_64bit_ns(aStart) / NSEC_PER_USEC,
				       aDuration, aChannel, DRX_SLOT_RX);

	LOG_DBG("nRF5 OT radio RX AT started (channel: %d, aStart: %u, aDuration: %u)", aChannel,
		aStart, aDuration);

	return result ? OT_ERROR_NONE : OT_ERROR_FAILED;
}
#endif

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	return &nrf5_data.tx.frame;
}

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aFrame)
{
	otError error = OT_ERROR_INVALID_STATE;

	ARG_UNUSED(aInstance);
	ARG_UNUSED(aFrame);

	__ASSERT_NO_MSG(aFrame == &nrf5_data.tx.frame);

	/* OT_RADIO_STATE_SLEEP allowed assuming the nrf5 radio has HW_SLEEP_TO_TX capability */
	if (nrf5_data.state == OT_RADIO_STATE_RECEIVE || nrf5_data.state == OT_RADIO_STATE_SLEEP) {
		nrf5_data.state = OT_RADIO_STATE_TRANSMIT;
		error = transmit_frame(aInstance);
	}

	return error;
}

int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
	int8_t ret_rssi = INT8_MAX;
	int error = 0;
	const uint16_t detection_time = 1;

	ARG_UNUSED(aInstance);

	/*
	 * Blocking implementation of get RSSI
	 * using no-blocking nrf5_energy_detection_start
	 */
	error = nrf5_energy_detection_start(detection_time, get_rssi_energy_detected);

	if (error == 0) {
		k_sem_take(&nrf5_data.rssi_wait, K_FOREVER);
		ret_rssi = (int8_t)nrf5_data.energy_detection.value;
	}

	return ret_rssi;
}

otError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration)
{
	int error = 0;

	nrf5_data.energy_detection.time = aScanDuration;
	nrf5_data.energy_detection.channel = aScanChannel;

	reset_pending_event(PENDING_EVENT_DETECT_ENERGY);
	reset_pending_event(PENDING_EVENT_DETECT_ENERGY_DONE);

	nrf5_set_channel(nrf5_data.energy_detection.channel);

	error = nrf5_energy_detection_start(nrf5_data.energy_detection.time, energy_detected);

	if (error != 0) {
		/*
		 * OpenThread API does not accept failure of this function,
		 * it can return 'No Error' or 'Not Implemented' error only.
		 * If ed_scan start failed event is set to schedule the scan at
		 * later time.
		 */
		LOG_ERR("Failed do start energy scan, scheduling for later");
		set_pending_event(PENDING_EVENT_DETECT_ENERGY);
	}

	return OT_ERROR_NONE;
}

void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
	ARG_UNUSED(aInstance);

	if (aEnable) {
		nrf_802154_src_addr_matching_method_set(NRF_802154_SRC_ADDR_MATCH_THREAD);
	}

	nrf_802154_auto_pending_bit_set(aEnable);
}

otError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, otShortAddress aShortAddress)
{
	ARG_UNUSED(aInstance);

	uint8_t short_address[SHORT_ADDRESS_SIZE];

	sys_put_le16(aShortAddress, short_address);

	if (!nrf_802154_pending_bit_for_addr_set(short_address, false)) {
		return OT_ERROR_NO_BUFS;
	}

	return OT_ERROR_NONE;
}

otError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
	ARG_UNUSED(aInstance);

	if (!nrf_802154_pending_bit_for_addr_set((uint8_t *)aExtAddress->m8, true)) {
		return OT_ERROR_NO_BUFS;
	}

	return OT_ERROR_NONE;
}

otError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, otShortAddress aShortAddress)
{
	ARG_UNUSED(aInstance);

	uint8_t short_address[SHORT_ADDRESS_SIZE];

	sys_put_le16(aShortAddress, short_address);

	if (!nrf_802154_pending_bit_for_addr_clear(short_address, false)) {
		return OT_ERROR_NO_ADDRESS;
	}

	return OT_ERROR_NONE;
}

otError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
	ARG_UNUSED(aInstance);

	if (!nrf_802154_pending_bit_for_addr_clear((uint8_t *)aExtAddress->m8, true)) {
		return OT_ERROR_NO_ADDRESS;
	}

	return OT_ERROR_NONE;
}

void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	nrf_802154_pending_bit_for_addr_reset(false);
}

void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	nrf_802154_pending_bit_for_addr_reset(true);
}

#if defined(CONFIG_OPENTHREAD_CSL_RECEIVER)
otError otPlatRadioEnableCsl(otInstance *aInstance, uint32_t aCslPeriod, otShortAddress aShortAddr,
			     const otExtAddress *aExtAddr)
{
	int result;

	ARG_UNUSED(aInstance);

	nrf_802154_csl_writer_period_set(aCslPeriod);
#if defined(CONFIG_NRF_802154_SER_HOST)
	nrf5_data.csl.period = aCslPeriod;
#endif

	if (aCslPeriod == 0) {
		result = nrf5_ack_data_clear(aShortAddr, aExtAddr);
	} else {
		uint8_t csl_ie_buf[CSL_IE_SIZE];
		size_t ie_length = create_csl_ie(aCslPeriod, csl_ie_buf);

		result = nrf5_ack_data_set(aShortAddr, aExtAddr, csl_ie_buf, ie_length);
	}

	return result ? OT_ERROR_FAILED : OT_ERROR_NONE;
}

otError otPlatRadioResetCsl(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	nrf_802154_csl_writer_period_set(0);
#if defined(CONFIG_NRF_802154_SER_HOST)
	nrf5_data.csl.period = 0;
#endif

	nrf_802154_ack_data_remove_all(false, NRF_802154_ACK_DATA_IE);
	nrf_802154_ack_data_remove_all(true, NRF_802154_ACK_DATA_IE);

	return OT_ERROR_NONE;
}

void otPlatRadioUpdateCslSampleTime(otInstance *aInstance, uint32_t aCslSampleTime)
{
	ARG_UNUSED(aInstance);

	/* CSL sample time points to "start of MAC" while the expected RX time
	 * refers to "end of SFD".
	 */
	int64_t expected_rx_time =
		convert_32bit_us_wrapped_to_64bit_ns(aCslSampleTime - PHR_DURATION_US);

#if defined(CONFIG_NRF_802154_SER_HOST)
	int64_t period_ns = (int64_t)nrf5_data.csl.period * NSEC_PER_TEN_SYMBOLS;
	bool changed = (expected_rx_time - nrf5_data.csl.rx_time) % period_ns;

	nrf5_data.csl.rx_time = expected_rx_time;

	if (changed) {
#endif /* CONFIG_NRF_802154_SER_HOST */
		nrf_802154_csl_writer_anchor_time_set(
			nrf_802154_timestamp_phr_to_mhr_convert(expected_rx_time / NSEC_PER_USEC));
#if defined(CONFIG_NRF_802154_SER_HOST)
	}
#endif /* CONFIG_NRF_802154_SER_HOST */
}
#endif

uint8_t otPlatRadioGetCslAccuracy(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	return CONFIG_NRF5_DELAY_TRX_ACC;
}

#if defined(CONFIG_OPENTHREAD_PLATFORM_CSL_UNCERT)
uint8_t otPlatRadioGetCslUncertainty(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	return CONFIG_OPENTHREAD_PLATFORM_CSL_UNCERT;
}
#endif

otError otPlatRadioSetChannelMaxTransmitPower(otInstance *aInstance, uint8_t aChannel,
					      int8_t aMaxPower)
{
	ARG_UNUSED(aInstance);

	if (aChannel < OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN ||
	    aChannel > OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MAX) {
		return OT_ERROR_INVALID_ARGS;
	}

	nrf5_data.max_tx_power_table[aChannel - OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN] = aMaxPower;

	if (nrf5_data.channel == aChannel) {
		nrf5_data.tx_power = get_transmit_power_for_channel(nrf5_data.channel);
	}

	return OT_ERROR_NONE;
}

#if defined(CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT)

/**
 * @brief Set vendor IE header for link metrics using structured format
 *
 * This function creates a properly formatted IEEE 802.15.4 vendor-specific header IE
 * for OpenThread link metrics probing
 *
 * +---------------------------------+----------------------+
 * | Length    | Element ID | Type=0 |      Vendor OUI      |
 * +-----------+------------+--------+----------------------+
 * |           Bytes: 0-1            |          2-4         |
 * +-----------+---------------------+----------------------+
 * | Bits: 0-6 |    7-14    |   15   | IE_VENDOR_THREAD_OUI |
 * +-----------+------------+--------+----------------------|
 *
 * Thread v1.2.1 Spec., 4.11.3.4.4.6
 * +---------------------------------+-------------------+------------------+
 * |                  Vendor Specific Information                           |
 * +---------------------------------+-------------------+------------------+
 * |                5                |         6         |   7 (optional)   |
 * +---------------------------------+-------------------+------------------+
 * | IE_VENDOR_THREAD_ACK_PROBING_ID | LINK_METRIC_TOKEN | LINK_METRIC_TOKEN|
 * |---------------------------------|-------------------|------------------|
 *
 * @param lqi Include LQI metric
 * @param link_margin Include link margin metric
 * @param rssi Include RSSI metric
 * @param ie_header Buffer to store the IE header
 */
static void set_vendor_ie_header_lm(bool lqi, bool link_margin, bool rssi, uint8_t *ie_header)
{
	/* OpenThread vendor-specific constants */
	const uint8_t ie_vendor_id = NRF5_HEADER_IE_ELEMENT_ID_VENDOR_SPECIFIC_IE;
	const uint8_t ie_vendor_thread_ack_probing_id = 0x00;
	const uint32_t ie_vendor_thread_oui = 0xeab89b;
	const uint8_t ie_vendor_thread_rssi_token = 0x01;
	const uint8_t ie_vendor_thread_margin_token = 0x02;
	const uint8_t ie_vendor_thread_lqi_token = 0x03;

	struct nrf5_header_ie vendor_ie;
	uint8_t link_metrics_data_len = (uint8_t)lqi + (uint8_t)link_margin + (uint8_t)rssi;
	uint8_t token_offset;

	__ASSERT(link_metrics_data_len <= 2, "Thread limits to 2 metrics at most");
	__ASSERT(ie_header, "Invalid argument");

	if (link_metrics_data_len == 0) {
		ie_header[0] = 0;
		return;
	}

	/* Clear the structure */
	memset(&vendor_ie, 0, sizeof(vendor_ie));

	vendor_ie.length = 4 + link_metrics_data_len;
	vendor_ie.element_id_low = ie_vendor_id & 0x01;
	vendor_ie.element_id_high = (ie_vendor_id >> 1) & 0x7f;
	vendor_ie.type = NRF5_IE_TYPE_HEADER;
	vendor_ie.content.link_metrics.vendor_oui[0] = (ie_vendor_thread_oui) & 0xff;
	vendor_ie.content.link_metrics.vendor_oui[1] = (ie_vendor_thread_oui >> 8) & 0xff;
	vendor_ie.content.link_metrics.vendor_oui[2] = (ie_vendor_thread_oui >> 16) & 0xff;
	vendor_ie.content.link_metrics.subtype = ie_vendor_thread_ack_probing_id;

	memcpy(ie_header, &vendor_ie, 2);

	uint8_t *content_ptr = ie_header + 2;

	content_ptr[0] = (ie_vendor_thread_oui) & 0xff;
	content_ptr[1] = (ie_vendor_thread_oui >> 8) & 0xff;
	content_ptr[2] = (ie_vendor_thread_oui >> 16) & 0xff;
	content_ptr[3] = ie_vendor_thread_ack_probing_id;

	token_offset = 4;

	if (lqi) {
		content_ptr[token_offset++] = ie_vendor_thread_lqi_token;
	}

	if (link_margin) {
		content_ptr[token_offset++] = ie_vendor_thread_margin_token;
	}

	if (rssi) {
		content_ptr[token_offset++] = ie_vendor_thread_rssi_token;
	}
}

otError otPlatRadioConfigureEnhAckProbing(otInstance *aInstance, otLinkMetrics aLinkMetrics,
					  const otShortAddress aShortAddress,
					  const otExtAddress *aExtAddress)
{
	ARG_UNUSED(aInstance);

	/* Use the proper IEEE 802.15.4 driver configure interface */
	uint8_t header_ie_buf[LM_IE_SIZE];

	/* Validate addresses */
	if (aShortAddress == NRF5_BROADCAST_ADDRESS || aExtAddress == NULL) {
		return OT_ERROR_INVALID_ARGS;
	}

	/* Create the vendor-specific IE header */
	set_vendor_ie_header_lm(aLinkMetrics.mLqi, aLinkMetrics.mLinkMargin, aLinkMetrics.mRssi,
				header_ie_buf);

	/* If no metrics requested, clear the ACK data using the helper function */
	if (header_ie_buf[0] == 0) {
		return nrf5_ack_data_clear(aShortAddress, aExtAddress) ? OT_ERROR_FAILED
								       : OT_ERROR_NONE;
	}

	/* Calculate IE length: first byte contains length field */
	uint8_t ie_length = (header_ie_buf[0] & 0x7f) + 2; /* +2 for the header itself */

	return nrf5_ack_data_set(aShortAddress, aExtAddress, header_ie_buf, ie_length)
		       ? OT_ERROR_FAILED
		       : OT_ERROR_NONE;
}
#endif

/* Platform related */

#if defined(CONFIG_NRF5_CARRIER_FUNCTIONS)
otError platformRadioTransmitCarrier(otInstance *aInstance, bool aEnable)
{
	ARG_UNUSED(aInstance);

	if ((aEnable) && (nrf5_data.state == OT_RADIO_STATE_RECEIVE)) {
		nrf_802154_tx_power_set(get_transmit_power_for_channel(nrf5_data.channel));

		if (!nrf_802154_continuous_carrier()) {
			LOG_ERR("Failed to enter continuous carrier state");
			return OT_ERROR_FAILED;
		}

		LOG_DBG("Continuous carrier wave transmission started (channel: %d)",
			nrf5_data.channel);

		nrf5_data.state = OT_RADIO_STATE_TRANSMIT;

		return OT_ERROR_NONE;
	} else if ((!aEnable) && (nrf5_data.state == OT_RADIO_STATE_TRANSMIT)) {
		return otPlatRadioReceive(aInstance, nrf5_data.channel);
	} else {
		return OT_ERROR_INVALID_STATE;
	}
}

otError platformRadioTransmitModulatedCarrier(otInstance *aInstance, bool aEnable,
					      const uint8_t *aData)
{
	ARG_UNUSED(aInstance);

	if (aEnable && nrf5_data.state == OT_RADIO_STATE_RECEIVE) {
		if (aData == NULL) {
			return OT_ERROR_INVALID_ARGS;
		}

		nrf_802154_tx_power_set(get_transmit_power_for_channel(nrf5_data.channel));

		if (!nrf_802154_modulated_carrier(aData)) {
			LOG_ERR("Failed to enter modulated carrier state");
			return OT_ERROR_FAILED;
		}

		LOG_DBG("Modulated carrier wave transmission started (channel: %d)",
			nrf5_data.channel);

		nrf5_data.state = OT_RADIO_STATE_TRANSMIT;

		return OT_ERROR_NONE;
	} else if ((!aEnable) && nrf5_data.state == OT_RADIO_STATE_TRANSMIT) {
		return otPlatRadioReceive(aInstance, nrf5_data.channel);
	} else {
		return OT_ERROR_INVALID_STATE;
	}
}

#endif /* CONFIG_NRF5_CARRIER_FUNCTIONS */

/* nRF5 radio driver callbacks */

void nrf_802154_received_timestamp_raw(uint8_t *data, int8_t power, uint8_t lqi, uint64_t time)
{
	for (uint32_t i = 0; i < ARRAY_SIZE(nrf5_data.rx.frames); i++) {
		if (nrf5_data.rx.frames[i].psdu != NULL) {
			continue;
		}

		nrf5_data.rx.frames[i].psdu = data;
		nrf5_data.rx.frames[i].rssi = power;
		nrf5_data.rx.frames[i].lqi = lqi;

		nrf5_data.rx.frames[i].time =
			nrf_802154_timestamp_end_to_phr_convert(time, data[0]);

		nrf5_data.rx.frames[i].ack_fpb = nrf5_data.rx.last_frame_ack_fpb;
		nrf5_data.rx.frames[i].ack_seb = nrf5_data.rx.last_frame_ack_seb;
		nrf5_data.rx.last_frame_ack_fpb = false;
		nrf5_data.rx.last_frame_ack_seb = false;

		k_fifo_put(&nrf5_data.rx.fifo, &nrf5_data.rx.frames[i]);
		set_pending_event(PENDING_EVENT_FRAME_RECEIVED);

		return;
	}

	__ASSERT(false, "Not enough rx frames allocated for nrf5 radio");
}

void nrf_802154_receive_failed(nrf_802154_rx_error_t error, uint32_t id)
{
#if defined(CONFIG_OPENTHREAD_CSL_RECEIVER)
	if (id == DRX_SLOT_RX && error == NRF_802154_RX_ERROR_DELAYED_TIMEOUT) {
		if (!nrf5_data.rx_on_when_idle) {
			/* Transition to RxOff done automatically by the driver */
			return;
		}
		set_pending_event(PENDING_EVENT_SLEEP);
	}
#else
	ARG_UNUSED(id);
#endif

	if (IS_ENABLED(CONFIG_NRF5_LOG_RX_FAILURES)) {
		LOG_INF("Rx failed, error = %d", error);
	}

	nrf5_data.rx.last_frame_ack_fpb = false;
	nrf5_data.rx.last_frame_ack_seb = false;

	if (nrf5_data.state == OT_RADIO_STATE_RECEIVE) {
		switch (error) {
		case NRF_802154_RX_ERROR_INVALID_FRAME:
		case NRF_802154_RX_ERROR_DELAYED_TIMEOUT:
			nrf5_data.rx.result = OT_ERROR_NO_FRAME_RECEIVED;
			break;

		case NRF_802154_RX_ERROR_INVALID_FCS:
			nrf5_data.rx.result = OT_ERROR_FCS;
			break;

		case NRF_802154_RX_ERROR_INVALID_DEST_ADDR:
			nrf5_data.rx.result = OT_ERROR_DESTINATION_ADDRESS_FILTERED;
			break;

		case NRF_802154_RX_ERROR_ABORTED:
		case NRF_802154_RX_ERROR_DELAYED_ABORTED:
			nrf5_data.rx.result = OT_ERROR_ABORT;
			break;

		case NRF_802154_RX_ERROR_NO_BUFFER:
			nrf5_data.rx.result = OT_ERROR_NO_BUFS;
			break;

		default:
			nrf5_data.rx.result = OT_ERROR_FAILED;
			break;
		}
		set_pending_event(PENDING_EVENT_RX_FAILED);
	}
}

void nrf_802154_tx_ack_started(const uint8_t *data)
{
	nrf5_data.rx.last_frame_ack_fpb = data[FRAME_PENDING_OFFSET] & FRAME_PENDING_BIT;
	nrf5_data.rx.last_frame_ack_seb = data[SECURITY_ENABLED_OFFSET] & SECURITY_ENABLED_BIT;
}

static void update_tx_frame_info(otRadioFrame *frame,
				 const nrf_802154_transmit_done_metadata_t *metadata)
{
	frame->mInfo.mTxInfo.mIsSecurityProcessed = metadata->frame_props.is_secured;
	frame->mInfo.mTxInfo.mIsHeaderUpdated = metadata->frame_props.dynamic_data_is_set;
}

void nrf_802154_transmitted_raw(uint8_t *frame, const nrf_802154_transmit_done_metadata_t *metadata)
{
	ARG_UNUSED(frame);

	nrf5_data.tx.result = OT_ERROR_NONE;
	nrf5_data.ack.desc.psdu = metadata->data.transmitted.p_ack;

	if (nrf5_data.ack.desc.psdu) {
		nrf5_data.ack.desc.rssi = metadata->data.transmitted.power;
		nrf5_data.ack.desc.lqi = metadata->data.transmitted.lqi;

		if (metadata->data.transmitted.time == NRF_802154_NO_TIMESTAMP) {
			/* Ack timestamp is invalid. Keep this value to detect it when handling Ack
			 */
			nrf5_data.ack.desc.time = NRF_802154_NO_TIMESTAMP;
		} else {
			nrf5_data.ack.desc.time = nrf_802154_timestamp_end_to_phr_convert(
				metadata->data.transmitted.time, nrf5_data.ack.desc.psdu[0]);
		}
	}

	update_tx_frame_info(&nrf5_data.tx.frame, metadata);

	set_pending_event(PENDING_EVENT_TX_DONE);
}

static otError nrf5_tx_error_to_ot_error(nrf_802154_tx_error_t error)
{
	switch (error) {
	case NRF_802154_TX_ERROR_BUSY_CHANNEL:
	case NRF_802154_TX_ERROR_TIMESLOT_ENDED:
	case NRF_802154_TX_ERROR_TIMESLOT_DENIED:
		return OT_ERROR_CHANNEL_ACCESS_FAILURE;
	case NRF_802154_TX_ERROR_NO_MEM:
		return OT_ERROR_NO_BUFS;
	case NRF_802154_TX_ERROR_INVALID_ACK:
	case NRF_802154_TX_ERROR_NO_ACK:
		return OT_ERROR_NO_ACK;
	case NRF_802154_TX_ERROR_ABORTED:
	default:
		return OT_ERROR_ABORT;
	}
}

void nrf_802154_transmit_failed(uint8_t *frame, nrf_802154_tx_error_t error,
				const nrf_802154_transmit_done_metadata_t *metadata)
{
	ARG_UNUSED(frame);

	nrf5_data.tx.result = nrf5_tx_error_to_ot_error(error);

	LOG_WRN("nrf_802154_transmit_failed: %u, tx result: %u", error, nrf5_data.tx.result);
	update_tx_frame_info(&nrf5_data.tx.frame, metadata);

	set_pending_event(PENDING_EVENT_TX_DONE);
}

void nrf_802154_energy_detected(const nrf_802154_energy_detected_t *result)
{
	if (nrf5_data.energy_detection.cb != NULL) {
		nrf5_energy_detection_done_cb_t callback = nrf5_data.energy_detection.cb;

		nrf5_data.energy_detection.cb = NULL;
		callback(result->ed_dbm);
	}
}

void nrf_802154_energy_detection_failed(nrf_802154_ed_error_t error)
{
	if (nrf5_data.energy_detection.cb != NULL) {
		nrf5_energy_detection_done_cb_t callback = nrf5_data.energy_detection.cb;

		nrf5_data.energy_detection.cb = NULL;
		callback(SHRT_MAX);
	}
}

#if defined(CONFIG_NRF_802154_SER_HOST)
void nrf_802154_serialization_error(const nrf_802154_ser_err_data_t *err)
{
	__ASSERT(false, "802.15.4 serialization error: %d", err->reason);
	k_oops();
}
#endif

void openthread_platform_radio_set_eui64(uint8_t eui64[EXTENDED_ADDRESS_SIZE])
{
	memcpy(nrf5_data.mac, eui64, EXTENDED_ADDRESS_SIZE);
}
