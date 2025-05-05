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

#include <openthread/error.h>
#define LOG_MODULE_NAME net_otPlat_radio

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

// TODO: AG: config
#define CONFIG_NRF5_RX_STACK_SIZE 1024

#if defined(CONFIG_NRF_802154_SER_HOST)
#include "nrf_802154_serialization_error.h"
#endif

#if defined(CONFIG_TRUSTED_EXECUTION_NONSECURE) && defined(NRF_FICR_S)
#include <soc_secure.h>
#else
#include <hal/nrf_ficr.h>
#endif

// TODO: AG: configs
#if defined(CONFIG_NRF5_UICR_EUI64_ENABLE)
#if defined(CONFIG_SOC_NRF5340_CPUAPP)
#if defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
#error "NRF_UICR->OTP is not supported to read from non-secure"
#else
#define EUI64_ADDR (NRF_UICR->OTP)
#endif /* CONFIG_TRUSTED_EXECUTION_NONSECURE */
#else
#define EUI64_ADDR (NRF_UICR->CUSTOMER)
#endif /* CONFIG_SOC_NRF5340_CPUAPP */
#endif /* CONFIG_NRF5_UICR_EUI64_ENABLE */

#if defined(CONFIG_NRF5_UICR_EUI64_ENABLE)
#define EUI64_ADDR_HIGH CONFIG_NRF5_UICR_EUI64_REG
#define EUI64_ADDR_LOW	(CONFIG_NRF5_UICR_EUI64_REG + 1)
#else
#define EUI64_ADDR_HIGH 0
#define EUI64_ADDR_LOW	1
#endif /* CONFIG_NRF5_UICR_EUI64_ENABLE */

#if CONFIG_NDOR_OUI_ENABLE
#define NRF5_VENDOR_OUI CONFIG_NDOR_OUI
#else
#define NRF5_VENDOR_OUI (uint32_t)0xF4CE36
#endif

#define CHANNEL_COUNT		       OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MAX - OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN + 1
#define DRX_SLOT_RX		       0 /* Delayed reception window ID */
#define PHR_DURATION_US		       32U
#define NSEC_PER_TEN_SYMBOLS	       ((uint64_t)PHY_US_PER_SYMBOL * 1000 * 10)
#define NRF5_BROADCAST_ADDRESS	       0xffff
#define NRF5_NO_SHORT_ADDRESS_ASSIGNED 0xfffe

enum nrf5_pending_events {
	PENDING_EVENT_FRAME_TO_SEND,	  /* There is a tx frame to send  */
	PENDING_EVENT_FRAME_RECEIVED,	  /* Radio has received new frame */
	PENDING_EVENT_RX_FAILED,	  /* The RX failed */
	PENDING_EVENT_TX_STARTED,	  /* Radio has started transmitting */
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

// TODO: AG: replace with otRadioFrame?
struct nrf5_rx_frame {
	// TODO: AG: remove?
	void *fifo_reserved; /* 1st word reserved for use by fifo. */
	uint8_t *psdu;	     /* Pointer to a received frame. */
	// TODO: AG: remove?
	uint64_t time; /* RX timestamp. */
	uint8_t lqi;   /* Last received frame LQI value. */
	int8_t rssi;   /* Last received frame RSSI value. */
	bool ack_fpb;  /* FPB value in ACK sent for the received frame. */
	// TODO: AG: remove?
	bool ack_seb; /* SEB value in ACK sent for the received frame. */
};

/** Energy scan callback */
typedef void (*energy_scan_done_cb_t)(int16_t max_ed);

struct nrf5_data {
	/* 802.15.4 HW address. */
	uint8_t mac[EXTENDED_ADDRESS_SIZE];

	/* RX fifo queue. */
	struct k_fifo rx_fifo;

	/* Buffers for passing received frame pointers and data to the
	 * RX thread via rx_fifo object.
	 */
	struct nrf5_rx_frame rx_frames[CONFIG_NRF_802154_RX_BUFFERS];

	/* Frame pending bit value in ACK sent for the last received frame. */
	bool last_frame_ack_fpb;

	/* Security Enabled bit value in ACK sent for the last received frame. */
	bool last_frame_ack_seb;

	/* Enable/disable RxOnWhenIdle MAC PIB attribute (Table 8-94). */
	bool rx_on_when_idle;

	/* Radio capabilities */
	otRadioCaps capabilities;

	otError rx_result;

	ATOMIC_DEFINE(pending_events, PENDING_EVENT_COUNT);
};

static struct nrf5_data nrf5_data;

K_SEM_DEFINE(radio_sem, 0, 1);

static otRadioState sState = OT_RADIO_STATE_DISABLED;
static otRadioFrame sTransmitFrame;

#if defined(CONFIG_OPENTHREAD_TIME_SYNC)
static otRadioIeInfo tx_ie_info;
#endif

/* Get the default tx output power from Kconfig */
static int8_t tx_power = CONFIG_OPENTHREAD_DEFAULT_TX_POWER;

static int8_t max_tx_power_table[CHANNEL_COUNT];

static uint8_t channel;

static bool promiscuous;

static energy_scan_done_cb_t energy_scan_done_cb;
static int16_t energy_detected_value;
static uint16_t energy_detection_time;
static uint8_t energy_detection_channel;

#if defined(CONFIG_NRF_802154_SER_HOST) && defined(CONFIG_OPENTHREAD_CSL_RECEIVER)
/* The last configured value of CSL period in units of 10 symbols. */
static uint32_t csl_period;
/* The last configured value of CSL phase time in nanoseconds. */
uint64_t csl_rx_time;
#endif /* CONFIG_NRF_802154_SER_HOST && CONFIG_OPENTHREAD_CSL_RECEIVER */

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

static inline void clear_pending_events(void)
{
	atomic_clear(nrf5_data.pending_events);
}

static int nrf5_set_channel(uint16_t channel)
{
	LOG_DBG("set channel %u", channel);

	if (channel < 11 || channel > 26) {
		return channel < 11 ? -ENOTSUP : -EINVAL;
	}

	nrf_802154_channel_set(channel);

	return 0;
}

static int8_t get_transmit_power_for_channel(uint8_t aChannel)
{
	int8_t channel_max_power = OT_RADIO_POWER_INVALID;
	int8_t power = 0; /* 0 dbm as default value */

	if (aChannel >= OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN &&
	    aChannel <= OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MAX) {
		channel_max_power =
			max_tx_power_table[aChannel - OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN];
	}

	if (tx_power != OT_RADIO_POWER_INVALID) {
		power = (channel_max_power < tx_power) ? channel_max_power : tx_power;
	} else if (channel_max_power != OT_RADIO_POWER_INVALID) {
		power = channel_max_power;
	}

	return power;
}

static int nrf5_ack_data_set(uint16_t short_addr, const otExtAddress *ext_addr,
			     const struct nrf5_header_ie *header_ie)

{
	uint8_t ext_addr_le[EXTENDED_ADDRESS_SIZE];
	uint8_t short_addr_le[SHORT_ADDRESS_SIZE];

	if (short_addr == NRF5_BROADCAST_ADDRESS || ext_addr == NULL) {
		return -ENOTSUP;
	}

	sys_put_le16(short_addr, short_addr_le);
	sys_memcpy_swap(ext_addr_le, ext_addr->m8, EXTENDED_ADDRESS_SIZE);

	if (short_addr != NRF5_NO_SHORT_ADDRESS_ASSIGNED) {
		nrf_802154_ack_data_set(short_addr_le, false, header_ie,
					header_ie->length + IE_HEADER_SIZE, NRF_802154_ACK_DATA_IE);
	}
	nrf_802154_ack_data_set(ext_addr_le, true, header_ie, header_ie->length + IE_HEADER_SIZE,
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

static void nrf5_get_eui64(uint8_t *mac)
{
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

#if defined(CONFIG_NET_PKT_TXTIME)
	if (radio_caps & NRF_802154_CAPABILITY_DELAYED_TX) {
		caps |= OT_RADIO_CAPS_TRANSMIT_TIMING;
	}
#endif

	if (radio_caps & NRF_802154_CAPABILITY_DELAYED_RX) {
		caps |= OT_RADIO_CAPS_RECEIVE_TIMING;
	}

	return caps;
}

static void dataInit(void)
{
	sTransmitFrame.mPsdu = malloc(OT_RADIO_FRAME_MAX_SIZE);
	__ASSERT_NO_MSG(sTransmitFrame.mPsdu != NULL);

	for (size_t i = 0; i < CHANNEL_COUNT; i++) {
		max_tx_power_table[i] = OT_RADIO_POWER_INVALID;
	}

#if defined(CONFIG_OPENTHREAD_TIME_SYNC)
	sTransmitFrame.mInfo.mTxInfo.mIeInfo = &tx_ie_info;
#endif
}

// TODO: AG: config
#if !defined(CONFIG_NRF5_EXT_IRQ_MGMT)
static void nrf5_radio_irq(const void *arg)
{
	ARG_UNUSED(arg);

	nrf_802154_radio_irq_handler();
}
#endif

static void nrf5_irq_config(void)
{
#if !defined(CONFIG_NRF5_EXT_IRQ_MGMT)
	IRQ_CONNECT(DT_IRQN(DT_NODELABEL(radio)), NRF_802154_IRQ_PRIORITY, nrf5_radio_irq, NULL, 0);
	irq_enable(DT_IRQN(DT_NODELABEL(radio)));
#endif
}

void platformRadioInit(void)
{
	dataInit();

	nrf5_get_eui64(nrf5_data.mac);
	nrf5_data.capabilities = nrf5_get_caps();

	k_fifo_init(&nrf5_data.rx_fifo);

	nrf5_data.rx_on_when_idle = true;
	nrf5_irq_config();

	// TODO: work queue?

	nrf_802154_init();
}

static void openthread_handle_received_frame(otInstance *instance, struct nrf5_rx_frame *rx_frame)
{
	otRadioFrame recv_frame;
	uint8_t *psdu;

	ARG_UNUSED(instance);

	memset(&recv_frame, 0, sizeof(otRadioFrame));

	recv_frame.mPsdu = &rx_frame->psdu[1];
	/* Length inc. CRC. */
	recv_frame.mLength = rx_frame->psdu[0];
	recv_frame.mChannel = channel;
	recv_frame.mInfo.mRxInfo.mLqi = rx_frame->lqi;
	recv_frame.mInfo.mRxInfo.mRssi = rx_frame->rssi;
	recv_frame.mInfo.mRxInfo.mAckedWithFramePending = rx_frame->ack_fpb;

#define CONFIG_NET_PKT_TIMESTAMP
#if defined(CONFIG_NET_PKT_TIMESTAMP)
	recv_frame.mInfo.mRxInfo.mTimestamp = rx_frame->time;
#endif

	recv_frame.mInfo.mRxInfo.mAckedWithSecEnhAck = rx_frame->ack_seb;
	// TODO: AG: ???
	//  recv_frame.mInfo.mRxInfo.mAckFrameCounter = net_pkt_ieee802154_ack_fc(pkt);
	//  recv_frame.mInfo.mRxInfo.mAckKeyId = net_pkt_ieee802154_ack_keyid(pkt);

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
	energy_detected_value = max_ed;
	set_pending_event(PENDING_EVENT_DETECT_ENERGY_DONE);
}

void platformRadioProcess(otInstance *aInstance)
{
	bool event_pending = false;

	if (is_pending_event_set(PENDING_EVENT_FRAME_TO_SEND)) {
		// TODO: AG: tx
		// struct net_pkt *evt_pkt;

		reset_pending_event(PENDING_EVENT_FRAME_TO_SEND);
		// TODO: AG: tx
		// while ((evt_pkt = (struct net_pkt *)k_fifo_get(&tx_pkt_fifo, K_NO_WAIT)) != NULL)
		// { 	if (IS_ENABLED(CONFIG_OPENTHREAD_COPROCESSOR_RCP)) {
		// net_pkt_unref(evt_pkt); 	} else {
		// openthread_handle_frame_to_send(aInstance, evt_pkt);
		// 	}
		// }
	}

	if (is_pending_event_set(PENDING_EVENT_FRAME_RECEIVED)) {
		struct nrf5_rx_frame *rx_frame;

		reset_pending_event(PENDING_EVENT_FRAME_RECEIVED);
		// TODO: AG: rx
		while ((rx_frame = (struct nrf5_rx_frame *)k_fifo_get(&nrf5_data.rx_fifo,
								      K_NO_WAIT)) != NULL) {
			openthread_handle_received_frame(aInstance, rx_frame);
		}
	}

	if (is_pending_event_set(PENDING_EVENT_RX_FAILED)) {
		reset_pending_event(PENDING_EVENT_RX_FAILED);
		if (IS_ENABLED(CONFIG_OPENTHREAD_DIAG) && otPlatDiagModeGet()) {
			otPlatDiagRadioReceiveDone(aInstance, NULL, nrf5_data.rx_result);
		} else {
			otPlatRadioReceiveDone(aInstance, NULL, nrf5_data.rx_result);
		}
	}

	if (is_pending_event_set(PENDING_EVENT_TX_STARTED)) {
		reset_pending_event(PENDING_EVENT_TX_STARTED);
		otPlatRadioTxStarted(aInstance, &sTransmitFrame);
	}

	if (is_pending_event_set(PENDING_EVENT_TX_DONE)) {
		reset_pending_event(PENDING_EVENT_TX_DONE);

		if (sState == OT_RADIO_STATE_TRANSMIT) {
			sState = OT_RADIO_STATE_RECEIVE;
			// TODO: AG: tx
			//  handle_tx_done(aInstance);
		}
	}

	if (is_pending_event_set(PENDING_EVENT_SLEEP)) {
		reset_pending_event(PENDING_EVENT_SLEEP);
		ARG_UNUSED(otPlatRadioSleep(aInstance));
	}

	/* handle events that can't run during transmission */
	if (sState != OT_RADIO_STATE_TRANSMIT) {
		if (is_pending_event_set(PENDING_EVENT_DETECT_ENERGY)) {
			nrf5_set_channel(energy_detection_channel);

			int error = 0;
			if (energy_scan_done_cb == NULL) {
				energy_scan_done_cb = energy_detected;

				if (nrf_802154_energy_detection((uint32_t)energy_detection_time *
								1000) == false) {
					energy_scan_done_cb = NULL;
					error = -EBUSY;
				}
			} else {
				error = -EALREADY;
			}

			if (error == 0) {
				reset_pending_event(PENDING_EVENT_DETECT_ENERGY);
			} else {
				event_pending = true;
			}
		}

		if (is_pending_event_set(PENDING_EVENT_DETECT_ENERGY_DONE)) {
			otPlatRadioEnergyScanDone(aInstance, (int8_t)energy_detected_value);
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

	return channel;
}

#if defined(CONFIG_OPENTHREAD_DIAG)
void platformRadioChannelSet(uint8_t aChannel)
{
	channel = aChannel;
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

	*aPower = tx_power;

	return OT_ERROR_NONE;
}

otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower)
{
	ARG_UNUSED(aInstance);

	tx_power = aPower;

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

	LOG_DBG("PromiscuousMode=%d", promiscuous ? 1 : 0);

	return promiscuous;
}

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
	ARG_UNUSED(aInstance);

	LOG_DBG("PromiscuousMode=%d", aEnable ? 1 : 0);

	promiscuous = aEnable;
	nrf_802154_promiscuous_set(promiscuous);
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
		__ASSERT_NO_MSG(NRF_802154_SECURITY_KEY_STORAGE_SIZE >= 3)

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
	return nrf_802154_time_get() * NSEC_PER_USEC;
}

uint64_t otPlatRadioGetNow(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	return otPlatTimeGet();
}

otRadioState otPlatRadioGetState(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	return sState;
}

otError otPlatRadioEnable(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	if (sState != OT_RADIO_STATE_DISABLED && sState != OT_RADIO_STATE_SLEEP) {
		return OT_ERROR_INVALID_STATE;
	}

	sState = OT_RADIO_STATE_SLEEP;
	return OT_ERROR_NONE;
}

otError otPlatRadioDisable(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	if (sState != OT_RADIO_STATE_DISABLED && sState != OT_RADIO_STATE_SLEEP) {
		return OT_ERROR_INVALID_STATE;
	}

	sState = OT_RADIO_STATE_DISABLED;
	return OT_ERROR_NONE;
}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	return (sState != OT_RADIO_STATE_DISABLED) ? true : false;
}

// TODO: remove events
// static void handle_radio_event(const struct device *dev, enum ieee802154_event evt,
// 			       void *event_params)
// {
// 	ARG_UNUSED(event_params);
//
// 	switch (evt) {
// 	case IEEE802154_EVENT_TX_STARTED:
// 		if (sState == OT_RADIO_STATE_TRANSMIT) {
// 			set_pending_event(PENDING_EVENT_TX_STARTED);
// 		}
// 		break;
// 	default:
// 		/* do nothing - ignore event */
// 		break;
// 	}
// }

otError otPlatRadioSleep(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	if (sState != OT_RADIO_STATE_SLEEP && sState != OT_RADIO_STATE_RECEIVE) {
		return OT_ERROR_INVALID_STATE;
	}

#if defined(CONFIG_OPENTHREAD_CSL_RECEIVER)
	if (nrf_802154_sleep_if_idle() != NRF_802154_SLEEP_ERROR_NONE) {
		set_pending_event(PENDING_EVENT_SLEEP);
		// TODO: AG: needed?
		Z_SPIN_DELAY(1);
	}
#else
	ARG_UNUSED(dev);

	if (!nrf_802154_sleep()) {
		LOG_ERR("Error while stopping radio");
	}
#endif

	LOG_DBG("nRF5 802154 radio stopped");

	sState = OT_RADIO_STATE_SLEEP;

	return OT_ERROR_NONE;
}

otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
	ARG_UNUSED(aInstance);

	if (sState == OT_RADIO_STATE_DISABLED) {
		return OT_ERROR_INVALID_STATE;
	}

	channel = aChannel;

	nrf5_set_channel(channel);
	nrf_802154_tx_power_set(get_transmit_power_for_channel(channel));

	if (!nrf_802154_receive()) {
		LOG_ERR("Failed to enter receive state");
		return OT_ERROR_FAILED;
	}

	LOG_DBG("nRF5 802154 radio started (channel: %d)", nrf_802154_channel_get());

	sState = OT_RADIO_STATE_RECEIVE;

	return OT_ERROR_NONE;
}

#if defined(CONFIG_OPENTHREAD_CSL_RECEIVER) || defined(CONFIG_OPENTHREAD_WAKEUP_END_DEVICE)
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
static uint64_t convert_32bit_us_wrapped_to_64bit_ns(uint32_t target_time_us_wrapped)
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
	return result * NSEC_PER_USEC;
}

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
				       aDuration / NSEC_PER_USEC, aChannel, DRX_SLOT_RX);

	return result ? OT_ERROR_FAILED : OT_ERROR_NONE;
}
#endif

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	return &sTransmitFrame;
}

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aFrame)
{
	otError error = OT_ERROR_INVALID_STATE;

	ARG_UNUSED(aInstance);
	ARG_UNUSED(aFrame);

	__ASSERT_NO_MSG(aFrame == &sTransmitFrame);

	if (sState == OT_RADIO_STATE_RECEIVE || sState == OT_RADIO_STATE_SLEEP) {
		// TODO: AG:
		//  if (run_tx_task(aInstance) == 0) {
		error = OT_ERROR_NONE;
		//  }
	}

	return error;
}

static void get_rssi_energy_detected(int16_t max_ed)
{
	energy_detected_value = max_ed;
	k_sem_give(&radio_sem);
}

int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
	int8_t ret_rssi = INT8_MAX;
	int error = 0;
	const uint16_t detection_time = 1;

	ARG_UNUSED(aInstance);

	/*
	 * Blocking implementation of get RSSI
	 * using no-blocking nrf_802154_energy_detection
	 */
	if (energy_scan_done_cb == NULL) {
		energy_scan_done_cb = get_rssi_energy_detected;

		if (nrf_802154_energy_detection((uint32_t)detection_time * 1000) == false) {
			energy_scan_done_cb = NULL;
			error = -EBUSY;
		}
	} else {
		error = -EALREADY;
	}

	if (error == 0) {
		k_sem_take(&radio_sem, K_FOREVER);

		ret_rssi = (int8_t)energy_detected_value;
	}

	return ret_rssi;
}

otError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration)
{
	int error = 0;

	energy_detection_time = aScanDuration;
	energy_detection_channel = aScanChannel;

	reset_pending_event(PENDING_EVENT_DETECT_ENERGY);
	reset_pending_event(PENDING_EVENT_DETECT_ENERGY_DONE);

	nrf5_set_channel(energy_detection_channel);

	if (energy_scan_done_cb == NULL) {
		energy_scan_done_cb = energy_detected;

		if (nrf_802154_energy_detection((uint32_t)aScanDuration * 1000) == false) {
			energy_scan_done_cb = NULL;
			error = -EBUSY;
		}
	} else {
		error = -EALREADY;
	}

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

	const struct nrf5_header_ie header_ie = {
		.length = sizeof(struct nrf5_header_ie_csl_reduced),
		.element_id_high = (NRF5_HEADER_IE_ELEMENT_ID_CSL_IE) >> 1U,
		.element_id_low = (NRF5_HEADER_IE_ELEMENT_ID_CSL_IE) & 0x01,
		.type = NRF5_IE_TYPE_HEADER,
		.content.csl_reduced =
			{
				.csl_phase = 0,
				.csl_period = aCslPeriod,
			},
	};

	nrf_802154_csl_writer_period_set(aCslPeriod);
#if defined(CONFIG_NRF_802154_SER_HOST)
	csl_period = aCslPeriod;
#endif

	if (aCslPeriod == 0) {
		result = nrf5_ack_data_clear(aShortAddr, aExtAddr);
	} else {
		result = nrf5_ack_data_set(aShortAddr, aExtAddr, &header_ie);
	}

	return result ? OT_ERROR_FAILED : OT_ERROR_NONE;
}

otError otPlatRadioResetCsl(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	nrf_802154_csl_writer_period_set(0);
#if defined(CONFIG_NRF_802154_SER_HOST)
	csl_period = 0;
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
	uint64_t expected_rx_time =
		convert_32bit_us_wrapped_to_64bit_ns(aCslSampleTime - PHR_DURATION_US);

#if defined(CONFIG_NRF_802154_SER_HOST)
	uint64_t period_ns = (uint64_t)csl_period * NSEC_PER_TEN_SYMBOLS;
	bool changed = (expected_rx_time - csl_rx_time) % period_ns;

	csl_rx_time = expected_rx_time;

	if (changed)
#endif /* CONFIG_NRF_802154_SER_HOST */
	{
		nrf_802154_csl_writer_anchor_time_set(
			nrf_802154_timestamp_phr_to_mhr_convert(expected_rx_time / NSEC_PER_USEC));
	}
}
#endif

uint8_t otPlatRadioGetCslAccuracy(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	return CONFIG_CLOCK_CONTROL_NRF_ACCURACY;
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

	max_tx_power_table[aChannel - OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN] = aMaxPower;

	if (aChannel == channel) {
		tx_power = get_transmit_power_for_channel(aChannel);
	}

	return OT_ERROR_NONE;
}

#if defined(CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT)
otError otPlatRadioConfigureEnhAckProbing(otInstance *aInstance, otLinkMetrics aLinkMetrics,
					  otShortAddress aShortAddress,
					  const otExtAddress *aExtAddress)
{
	int result;

	ARG_UNUSED(aInstance);

	const struct nrf5_header_ie header_ie = {
		.length = sizeof(struct nrf5_header_ie_link_metrics),
		.element_id_high = (NRF5_HEADER_IE_ELEMENT_ID_VENDOR_SPECIFIC_IE) >> 1U,
		.element_id_low = (NRF5_HEADER_IE_ELEMENT_ID_VENDOR_SPECIFIC_IE) & 0x01,
		.type = NRF5_IE_TYPE_HEADER,
		.content.link_metrics =
			{
				.vendor_oui[0] = (IE_VENDOR_THREAD_OUI >> 0 & 0xff),
				.vendor_oui[1] = (IE_VENDOR_THREAD_OUI >> 8 & 0xff),
				.vendor_oui[2] = (IE_VENDOR_THREAD_OUI >> 16 & 0xff),
				.lqi_token = aLinkMetrics.mLqi ? IE_VENDOR_THREAD_LQI_TOKEN : 0,
				.link_margin_token = aLinkMetrics.mLinkMargin
							     ? IE_VENDOR_THREAD_MARGIN_TOKEN
							     : 0,
				.rssi_token = aLinkMetrics.mRssi ? IE_VENDOR_THREAD_RSSI_TOKEN : 0,
			},
	};

	result = nrf5_ack_data_set(aShortAddress, aExtAddress, &header_ie);

	return result ? OT_ERROR_FAILED : OT_ERROR_NONE;
}
#endif

/* Platform related */

// TODO: AG: nrf5 carrier configs
#if defined(CONFIG_IEEE802154_CARRIER_FUNCTIONS)
otError platformRadioTransmitCarrier(otInstance *aInstance, bool aEnable)
{
	ARG_UNUSED(aInstance);

	if ((aEnable) && (sState == OT_RADIO_STATE_RECEIVE)) {
		nrf_802154_tx_power_set(get_transmit_power_for_channel(channel));

		if (!nrf_802154_continuous_carrier()) {
			LOG_ERR("Failed to enter continuous carrier state");
			return OT_ERROR_FAILED;
		}

		LOG_DBG("Continuous carrier wave transmission started (channel: %d)",
			nrf_802154_channel_get());

		sState = OT_RADIO_STATE_TRANSMIT;

		return OT_ERROR_NONE;
	} else if ((!aEnable) && (sState == OT_RADIO_STATE_TRANSMIT)) {
		return otPlatRadioReceive(aInstance, channel);
	} else {
		return OT_ERROR_INVALID_STATE;
	}
}

otError platformRadioTransmitModulatedCarrier(otInstance *aInstance, bool aEnable,
					      const uint8_t *aData)
{
	ARG_UNUSED(aInstance);

	if (aEnable && sState == OT_RADIO_STATE_RECEIVE) {
		if (aData == NULL) {
			return OT_ERROR_INVALID_ARGS;
		}

		nrf_802154_tx_power_set(get_transmit_power_for_channel(channel));

		if (!nrf_802154_modulated_carrier(aData)) {
			LOG_ERR("Failed to enter modulated carrier state");
			return OT_ERROR_FAILED;
		}

		LOG_DBG("Modulated carrier wave transmission started (channel: %d)",
			nrf_802154_channel_get());

		sState = OT_RADIO_STATE_TRANSMIT;

		return OT_ERROR_NONE;
	} else if ((!aEnable) && sState == OT_RADIO_STATE_TRANSMIT) {
		return otPlatRadioReceive(aInstance, channel);
	} else {
		return OT_ERROR_INVALID_STATE;
	}
}

#endif /* CONFIG_IEEE802154_CARRIER_FUNCTIONS */

/* nRF5 radio driver callbacks */

void nrf_802154_received_timestamp_raw(uint8_t *data, int8_t power, uint8_t lqi, uint64_t time)
{
	for (uint32_t i = 0; i < ARRAY_SIZE(nrf5_data.rx_frames); i++) {
		if (nrf5_data.rx_frames[i].psdu != NULL) {
			continue;
		}

		nrf5_data.rx_frames[i].psdu = data;
		nrf5_data.rx_frames[i].rssi = power;
		nrf5_data.rx_frames[i].lqi = lqi;

// TODO: AG: remove
#if defined(CONFIG_NET_PKT_TIMESTAMP)
		nrf5_data.rx_frames[i].time =
			nrf_802154_timestamp_end_to_phr_convert(time, data[0]);
#endif

		nrf5_data.rx_frames[i].ack_fpb = nrf5_data.last_frame_ack_fpb;
		nrf5_data.rx_frames[i].ack_seb = nrf5_data.last_frame_ack_seb;
		nrf5_data.last_frame_ack_fpb = false;
		nrf5_data.last_frame_ack_seb = false;

		k_fifo_put(&nrf5_data.rx_fifo, &nrf5_data.rx_frames[i]);
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

	// TODO: AG: config
	if (IS_ENABLED(CONFIG_NRF5_LOG_RX_FAILURES)) {
		LOG_INF("Rx failed, error = %d", error);
	}

	nrf5_data.last_frame_ack_fpb = false;
	nrf5_data.last_frame_ack_seb = false;

	if (sState == OT_RADIO_STATE_RECEIVE) {
		// TODO: AG: check API
		switch (error) {
		case NRF_802154_RX_ERROR_INVALID_FRAME:
		case NRF_802154_RX_ERROR_DELAYED_TIMEOUT:
			nrf5_data.rx_result = OT_ERROR_NO_FRAME_RECEIVED;
			break;

		case NRF_802154_RX_ERROR_INVALID_FCS:
			nrf5_data.rx_result = OT_ERROR_FCS;
			break;

		case NRF_802154_RX_ERROR_INVALID_DEST_ADDR:
			nrf5_data.rx_result = OT_ERROR_DESTINATION_ADDRESS_FILTERED;
			break;

		case NRF_802154_RX_ERROR_ABORTED:
		case NRF_802154_RX_ERROR_DELAYED_ABORTED:
			nrf5_data.rx_result = OT_ERROR_DESTINATION_ADDRESS_FILTERED;
			break;

		case NRF_802154_RX_ERROR_NO_BUFFER:
			nrf5_data.rx_result = OT_ERROR_NO_BUFS;
			break;

		default:
			nrf5_data.rx_result = OT_ERROR_FAILED;
			break;
		}
		set_pending_event(PENDING_EVENT_RX_FAILED);
	}
}

// void nrf_802154_tx_ack_started(const uint8_t *data)
// {
// 	nrf5_data.last_frame_ack_fpb = data[FRAME_PENDING_OFFSET] & FRAME_PENDING_BIT;
// 	nrf5_data.last_frame_ack_seb = data[SECURITY_ENABLED_OFFSET] & SECURITY_ENABLED_BIT;
// }
//
// void nrf_802154_transmitted_raw(uint8_t *frame, const nrf_802154_transmit_done_metadata_t
// *metadata)
// {
// 	ARG_UNUSED(frame);
//
// 	nrf5_data.tx_result = NRF_802154_TX_ERROR_NONE;
// 	nrf5_data.tx_frame_is_secured = metadata->frame_props.is_secured;
// 	nrf5_data.tx_frame_mac_hdr_rdy = metadata->frame_props.dynamic_data_is_set;
// 	nrf5_data.ack_frame.psdu = metadata->data.transmitted.p_ack;
//
// 	if (nrf5_data.ack_frame.psdu) {
// 		nrf5_data.ack_frame.rssi = metadata->data.transmitted.power;
// 		nrf5_data.ack_frame.lqi = metadata->data.transmitted.lqi;
//
// #if defined(CONFIG_NET_PKT_TIMESTAMP)
// 		if (metadata->data.transmitted.time == NRF_802154_NO_TIMESTAMP) {
// 			/* Ack timestamp is invalid. Keep this value to detect it when handling Ack
// 			 */
// 			nrf5_data.ack_frame.time = NRF_802154_NO_TIMESTAMP;
// 		} else {
// 			nrf5_data.ack_frame.time = nrf_802154_timestamp_end_to_phr_convert(
// 				metadata->data.transmitted.time, nrf5_data.ack_frame.psdu[0]);
// 		}
// #endif
// 	}
//
// 	k_sem_give(&nrf5_data.tx_wait);
// }
//
// void nrf_802154_transmit_failed(uint8_t *frame, nrf_802154_tx_error_t error,
// 				const nrf_802154_transmit_done_metadata_t *metadata)
// {
// 	ARG_UNUSED(frame);
//
// 	nrf5_data.tx_result = error;
// 	nrf5_data.tx_frame_is_secured = metadata->frame_props.is_secured;
// 	nrf5_data.tx_frame_mac_hdr_rdy = metadata->frame_props.dynamic_data_is_set;
//
// 	k_sem_give(&nrf5_data.tx_wait);
// }
//
// void nrf_802154_cca_done(bool channel_free)
// {
// 	nrf5_data.channel_free = channel_free;
//
// 	k_sem_give(&nrf5_data.cca_wait);
// }
//
// void nrf_802154_cca_failed(nrf_802154_cca_error_t error)
// {
// 	ARG_UNUSED(error);
//
// 	nrf5_data.channel_free = false;
//
// 	k_sem_give(&nrf5_data.cca_wait);
// }

void nrf_802154_energy_detected(const nrf_802154_energy_detected_t *result)
{
	if (energy_scan_done_cb != NULL) {
		energy_scan_done_cb_t callback = energy_scan_done_cb;

		energy_scan_done_cb = NULL;
		callback(result->ed_dbm);
	}
}

void nrf_802154_energy_detection_failed(nrf_802154_ed_error_t error)
{
	if (energy_scan_done_cb != NULL) {
		energy_scan_done_cb_t callback = energy_scan_done_cb;

		energy_scan_done_cb = NULL;
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
