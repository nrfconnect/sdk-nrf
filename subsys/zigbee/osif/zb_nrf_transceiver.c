/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <kernel.h>
#include <device.h>
#include <sys/byteorder.h>
#include <logging/log.h>
#include <net/ieee802154_radio.h>
#include <zboss_api.h>
#include <zb_macll.h>
#include <zb_transceiver.h>
#include "zb_nrf_platform.h"

#define PHR_LENGTH                1
#define FCS_LENGTH                2
#define ACK_PKT_LENGTH            5
#define FRAME_TYPE_MASK           0x07
#define FRAME_TYPE_ACK            0x02

#if defined(NRF52840_XXAA) || defined(NRF52811_XXAA) || \
defined(NRF5340_XXAA_NETWORK) || defined(NRF5340_XXAA_APPLICATION)
/* dBm value corresponding to value 0 in the EDSAMPLE register. */
#define ZBOSS_ED_MIN_DBM       (-92)
/* Factor needed to calculate the ED result based on the data from the RADIO peripheral. */
#define ZBOSS_ED_RESULT_FACTOR 4

#elif defined(NRF52833_XXAA) || defined(NRF52820_XXAA)
/* dBm value corresponding to value 0 in the EDSAMPLE register. */
#define ZBOSS_ED_MIN_DBM       (-93)
/* Factor needed to calculate the ED result based on the data from the RADIO peripheral. */
#define ZBOSS_ED_RESULT_FACTOR 5

#else
#error "Selected chip is not supported."
#endif


BUILD_ASSERT(IS_ENABLED(CONFIG_NET_PKT_TIMESTAMP), "Timestamp is required");
BUILD_ASSERT(!IS_ENABLED(CONFIG_IEEE802154_NET_IF_NO_AUTO_START),
	     "Option not supported");

/* Required by workaround for KRKNWK-12301. */
#define NO_ACK_DELAY_MS           23U

LOG_MODULE_DECLARE(zboss_osif, CONFIG_ZBOSS_OSIF_LOG_LEVEL);

enum ieee802154_radio_state {
	RADIO_802154_STATE_SLEEP,
	RADIO_802154_STATE_ACTIVE,
	RADIO_802154_STATE_RECEIVE,
	RADIO_802154_STATE_TRANSMIT,
};

struct ieee802154_state_cache {
	int8_t power;
	enum ieee802154_radio_state radio_state;
};

static struct ieee802154_state_cache state_cache = {
	.power = SCHAR_MIN,
	.radio_state = RADIO_802154_STATE_SLEEP,
};

/* RX fifo queue. */
static struct k_fifo rx_fifo;

static uint8_t ack_frame_buf[ACK_PKT_LENGTH + PHR_LENGTH];
static uint8_t *ack_frame;

static struct {
	/* Semaphore for waiting for end of energy detection procedure. */
	struct k_sem sem;
	volatile bool failed;      /* Energy detection procedure failed. */
	volatile uint32_t time_ms; /* Duration of energy detection procedure. */
	volatile uint8_t rssi_val; /* Detected energy level. */
} energy_detect;

static const struct device *radio_dev;
static struct ieee802154_radio_api *radio_api;
static struct net_if *net_iface;

void zb_trans_hw_init(void)
{
	/* Radio hardware is initialized in 802.15.4 driver */
}

/* Sets the PAN ID used by the device. */
void zb_trans_set_pan_id(zb_uint16_t pan_id)
{
	struct ieee802154_filter filter = { .pan_id = pan_id };

	LOG_DBG("Function: %s, PAN ID: 0x%x", __func__, pan_id);

	radio_api->filter(radio_dev, true, IEEE802154_FILTER_TYPE_PAN_ID, &filter);
}

/* Sets the long address in the radio driver. */
void zb_trans_set_long_addr(zb_ieee_addr_t long_addr)
{
	struct ieee802154_filter filter = { .ieee_addr = long_addr };

	LOG_DBG("Function: %s, long addr: 0x%llx", __func__, (uint64_t)*long_addr);

	radio_api->filter(radio_dev,
			  true,
			  IEEE802154_FILTER_TYPE_IEEE_ADDR,
			  &filter);
}

/* Sets the short address of the device. */
void zb_trans_set_short_addr(zb_uint16_t addr)
{
	struct ieee802154_filter filter = { .short_addr = addr };

	LOG_DBG("Function: %s, 0x%x", __func__, addr);

	radio_api->filter(radio_dev,
			  true,
			  IEEE802154_FILTER_TYPE_SHORT_ADDR,
			  &filter);
}

/* Energy detection callback */
static void energy_scan_done(const struct device *dev, int16_t max_ed)
{
	ARG_UNUSED(dev);

	if (max_ed == SHRT_MAX) {
		energy_detect.failed = true;
	} else {
		energy_detect.rssi_val =
			(max_ed - ZBOSS_ED_MIN_DBM) * ZBOSS_ED_RESULT_FACTOR;
	}
	k_sem_give(&energy_detect.sem);
}

/* Start the energy detection procedure */
void zb_trans_start_get_rssi(zb_uint8_t scan_duration_bi)
{
	energy_detect.failed = false;
	energy_detect.time_ms =
		ZB_TIME_BEACON_INTERVAL_TO_MSEC(scan_duration_bi);

	LOG_DBG("Function: %s, scan duration: %d ms", __func__,
		energy_detect.time_ms);

	k_sem_take(&energy_detect.sem, K_FOREVER);
	while (radio_api->ed_scan(radio_dev,
				  energy_detect.time_ms,
				  energy_scan_done)) {
		k_usleep(500);
	}
}

/* Waiting for the end of energy detection procedure and reads the RSSI */
void zb_trans_get_rssi(zb_uint8_t *rssi_value_p)
{
	LOG_DBG("Function: %s", __func__);

	/*Wait until the ED scan finishes.*/
	while (1) {
		k_sem_take(&energy_detect.sem, K_FOREVER);
		if (!energy_detect.failed) {
			*rssi_value_p = energy_detect.rssi_val;
			LOG_DBG("Energy detected: %d", *rssi_value_p);
			break;
		}

		/* Try again */
		LOG_DBG("Energy detect failed, tries again");
		energy_detect.failed = false;
		while (radio_api->ed_scan(radio_dev,
					  energy_detect.time_ms,
					  energy_scan_done)) {
			k_usleep(500);
		}
	}
	k_sem_give(&energy_detect.sem);
}

/* Set channel and go to the normal (not ed scan) mode */
void zb_trans_set_channel(zb_uint8_t channel_number)
{
	LOG_DBG("Function: %s, channel number: %d", __func__, channel_number);

	radio_api->set_channel(radio_dev, channel_number);
}

/* Sets the transmit power. */
void zb_trans_set_tx_power(zb_int8_t power)
{
	LOG_DBG("Function: %s, power: %d", __func__, power);

	radio_api->set_txpower(radio_dev, power);
	state_cache.power = power;
}

/* Gets the currently set transmit power. */
void zb_trans_get_tx_power(zb_int8_t *power)
{
	__ASSERT_NO_MSG(state_cache.power != SCHAR_MIN);

	*power = state_cache.power;

	LOG_DBG("Function: %s, power: %d", __func__, *power);
}

/* Configures the device as the PAN coordinator. */
void zb_trans_set_pan_coord(zb_bool_t enabled)
{
	struct ieee802154_config config = { .pan_coordinator = enabled };

	LOG_DBG("Function: %s, enabled: %d", __func__, enabled);

	radio_api->configure(radio_dev,
			     IEEE802154_CONFIG_PAN_COORDINATOR,
			     &config);
}

/* Enables or disables the automatic acknowledgments (auto ACK) */
void zb_trans_set_auto_ack(zb_bool_t enabled)
{
	struct ieee802154_config config = {
		.auto_ack_fpb = {
			.enabled = enabled,
			.mode = IEEE802154_FPB_ADDR_MATCH_ZIGBEE
		}
	};

	LOG_DBG("Function: %s, enabled: %d", __func__, enabled);

	radio_api->configure(radio_dev,
			     IEEE802154_CONFIG_AUTO_ACK_FPB,
			     &config);
}

/* Enables or disables the promiscuous radio mode. */
void zb_trans_set_promiscuous_mode(zb_bool_t enabled)
{
	struct ieee802154_config config = {
		.promiscuous = enabled
	};

	LOG_DBG("Function: %s, enabled: %d", __func__, enabled);

	radio_api->configure(radio_dev,
			     IEEE802154_CONFIG_PROMISCUOUS,
			     &config);
}

/* Changes the radio state to receive. */
void zb_trans_enter_receive(void)
{
	LOG_DBG("Function: %s", __func__);

	radio_api->start(radio_dev);
	state_cache.radio_state = RADIO_802154_STATE_RECEIVE;
}

/* Changes the radio state to sleep. */
void zb_trans_enter_sleep(void)
{
	LOG_DBG("Function: %s", __func__);

	(void)radio_api->stop(radio_dev);
	state_cache.radio_state = RADIO_802154_STATE_SLEEP;
}

/* Returns ZB_TRUE if radio is in receive state, otherwise ZB_FALSE */
zb_bool_t zb_trans_is_receiving(void)
{
	zb_bool_t is_receiv =
		(state_cache.radio_state == RADIO_802154_STATE_RECEIVE) ?
			ZB_TRUE : ZB_FALSE;

	LOG_DBG("Function: %s, is receiv: %d", __func__, is_receiv);
	return is_receiv;
}

/* Returns ZB_TRUE if radio is ON or ZB_FALSE if is in sleep state. */
zb_bool_t zb_trans_is_active(void)
{
	zb_bool_t is_active =
		(state_cache.radio_state != RADIO_802154_STATE_SLEEP) ?
			ZB_TRUE : ZB_FALSE;

	LOG_DBG("Function: %s, is active: %d", __func__, is_active);
	return is_active;
}

zb_bool_t zb_trans_transmit(zb_uint8_t wait_type, zb_time_t tx_at,
			    zb_uint8_t *tx_buf, zb_uint8_t current_channel)
{
	struct net_pkt *pkt = NULL;
	struct net_buf frag = {
		.frags = NULL,
		.b = {
			.data = &tx_buf[1],
			.len = tx_buf[0] - FCS_LENGTH,
			.size = tx_buf[0] - FCS_LENGTH,
			.__buf = &tx_buf[1]
		}
	};
	int err = 0;

	LOG_DBG("Function: %s, channel: %d", __func__, current_channel);

#ifndef ZB_ENABLE_ZGP_DIRECT
	ARG_UNUSED(tx_at);
	ARG_UNUSED(current_channel);
#endif

	pkt = net_pkt_alloc(K_NO_WAIT);
	if (!pkt) {
		ZB_ASSERT(0);
		return ZB_FALSE;
	}

	ack_frame = NULL;

	switch (wait_type) {
	case ZB_MAC_TX_WAIT_CSMACA: {
		state_cache.radio_state = RADIO_802154_STATE_TRANSMIT;
		enum ieee802154_tx_mode mode;
		if (radio_api->get_capabilities(radio_dev)
		    & IEEE802154_HW_CSMA) {
			mode = IEEE802154_TX_MODE_CSMA_CA;
		} else {
			mode = IEEE802154_TX_MODE_CCA;
		}

		err = radio_api->tx(radio_dev, mode, pkt, &frag);
		break;
	}

#ifdef ZB_ENABLE_ZGP_DIRECT
	case ZB_MAC_TX_WAIT_ZGP: {
		if (!(radio_api->get_capabilities(radio_dev)
		      & IEEE802154_HW_TXTIME)) {
			net_pkt_unref(pkt);
			return ZB_FALSE;
		}

		net_pkt_set_txtime(pkt, (uint64_t)tx_at * NSEC_PER_USEC);
		state_cache.radio_state = RADIO_802154_STATE_TRANSMIT;
		err = radio_api->tx(radio_dev,
				    IEEE802154_TX_MODE_TXTIME,
				    pkt,
				    &frag);
		break;
	}
#endif
	case ZB_MAC_TX_WAIT_NONE:
		/* First transmit attempt without CCA. */
		state_cache.radio_state = RADIO_802154_STATE_TRANSMIT;
		err = radio_api->tx(radio_dev,
				    IEEE802154_TX_MODE_DIRECT,
				    pkt,
				    &frag);
		break;
	default:
		LOG_DBG("Illegal wait_type parameter: %d", wait_type);
		ZB_ASSERT(0);
		net_pkt_unref(pkt);
		return ZB_FALSE;
	}

	net_pkt_unref(pkt);
	state_cache.radio_state = RADIO_802154_STATE_RECEIVE;

	switch (err) {
	case 0:
		/* ack_frame is overwritten if ack frame was received */
		zb_macll_transmitted_raw(ack_frame);

		/* Raise signal to indicate radio event */
		zigbee_event_notify(ZIGBEE_EVENT_TX_DONE);
		break;
	case -ENOMSG:
		zb_macll_transmit_failed(ZB_TRANS_NO_ACK);
		zigbee_event_notify(ZIGBEE_EVENT_TX_FAILED);

		/* Workaround for KRKNWK-12301. */
		k_sleep(K_MSEC(NO_ACK_DELAY_MS));
		/* End of workaround. */
		break;
	case -EBUSY:
	case -EIO:
	default:
		zb_macll_transmit_failed(ZB_TRANS_CHANNEL_BUSY_ERROR);
		zigbee_event_notify(ZIGBEE_EVENT_TX_FAILED);
		break;
	}

	return ZB_TRUE;
}

/* Notifies the driver that the buffer containing the received frame
 * is not used anymore
 */
void zb_trans_buffer_free(zb_uint8_t *buf)
{
	ARG_UNUSED(buf);
	LOG_DBG("Function: %s", __func__);

	/* The buffer containing the released frame is freed
	 * in 802.15.4 shim driver
	 */
}

zb_bool_t zb_trans_set_pending_bit(zb_uint8_t *addr, zb_bool_t value,
				   zb_bool_t extended)
{
	struct ieee802154_config config = {
		.ack_fpb = {
			.addr = addr,
			.extended = extended,
			.enabled = !value
		}
	};
	int ret;

	LOG_DBG("Function: %s, value: %d", __func__, value);

	ret = radio_api->configure(radio_dev,
				   IEEE802154_CONFIG_ACK_FPB,
				   &config);
	return !ret ? ZB_TRUE : ZB_FALSE;
}

void zb_trans_src_match_tbl_drop(void)
{
	struct ieee802154_config config = {
		.ack_fpb = {
			.addr = NULL,
			.enabled = false
		}
	};

	LOG_DBG("Function: %s", __func__);

	/* reset for short addresses */
	config.ack_fpb.extended = false;
	radio_api->configure(radio_dev, IEEE802154_CONFIG_ACK_FPB, &config);

	/* reset for long addresses */
	config.ack_fpb.extended = true;
	radio_api->configure(radio_dev, IEEE802154_CONFIG_ACK_FPB, &config);
}

zb_time_t osif_sub_trans_timer(zb_time_t t2, zb_time_t t1)
{
	return ZB_TIME_SUBTRACT(t2, t1);
}

zb_bool_t zb_trans_rx_pending(void)
{
	return k_fifo_is_empty(&rx_fifo) ? ZB_FALSE : ZB_TRUE;
}

zb_uint8_t zb_trans_get_next_packet(zb_bufid_t buf)
{
	zb_uint8_t *data_ptr;
	size_t length;

	LOG_DBG("Function: %s", __func__);

	if (!buf) {
		return 0;
	}

	/* Packet received with correct CRC, PANID and address */
	struct net_pkt *pkt = k_fifo_get(&rx_fifo, K_NO_WAIT);

	if (!pkt) {
		return 0;
	}

	length = net_pkt_get_len(pkt);
	data_ptr = zb_buf_initial_alloc(buf, length);

	/* Copy received data */
	net_pkt_cursor_init(pkt);
	net_pkt_read(pkt, data_ptr, length);

	/* Put LQI, RSSI */
	zb_macll_metadata_t *metadata = ZB_MACLL_GET_METADATA(buf);

	metadata->lqi = net_pkt_ieee802154_lqi(pkt);
	metadata->power = net_pkt_ieee802154_rssi(pkt);

	/* Put timestamp (usec) into the packet tail */
	*ZB_BUF_GET_PARAM(buf, zb_time_t) =
		net_pkt_timestamp(pkt)->second * USEC_PER_SEC +
		net_pkt_timestamp(pkt)->nanosecond / NSEC_PER_USEC;
	/* Additional buffer status for Data Request command */
	zb_macll_set_received_data_status(buf,
		net_pkt_ieee802154_ack_fpb(pkt));

	/* Release the packet */
	net_pkt_unref(pkt);

	return 1;
}

zb_ret_t zb_trans_cca(void)
{
	int cca_result = radio_api->cca(radio_dev);

	switch (cca_result) {
	case 0:
		return RET_OK;
	case -EBUSY:
		return RET_BUSY;
	default:
		return RET_ERROR;
	}
}

void zb_osif_get_ieee_eui64(zb_ieee_addr_t ieee_eui64)
{
	__ASSERT_NO_MSG(net_iface);
	__ASSERT_NO_MSG(net_if_get_link_addr(net_iface)->len ==
			sizeof(zb_ieee_addr_t));

	sys_memcpy_swap(ieee_eui64,
			net_if_get_link_addr(net_iface)->addr,
			net_if_get_link_addr(net_iface)->len);
}

void ieee802154_init(struct net_if *iface)
{
	__ASSERT_NO_MSG(iface);
	net_iface = iface;

	radio_dev = net_if_get_device(iface);
	__ASSERT_NO_MSG(radio_dev);

	radio_api = (struct ieee802154_radio_api *)radio_dev->api;
	__ASSERT_NO_MSG(radio_api);

	zb_trans_set_auto_ack(ZB_TRUE);

	zigbee_init();

	k_fifo_init(&rx_fifo);
	k_sem_init(&energy_detect.sem, 1, 1);

	radio_api->stop(radio_dev);
	net_if_up(iface);
	LOG_DBG("The 802.15.4 interface initialized.");
}

enum net_verdict ieee802154_radio_handle_ack(struct net_if *iface,
					     struct net_pkt *pkt)
{
	ARG_UNUSED(iface);

	size_t ack_len = net_pkt_get_len(pkt);

	if (ack_len != ACK_PKT_LENGTH) {
		LOG_ERR("%s: ACK length error", __func__);
		return NET_CONTINUE;
	}

	if ((*net_pkt_data(pkt) & FRAME_TYPE_MASK) != FRAME_TYPE_ACK) {
		LOG_ERR("%s: ACK frame was expected", __func__);
		return NET_CONTINUE;
	}

	if (ack_frame != NULL) {
		LOG_ERR("Overwriting unhandled ACK frame.");
	}

	ack_frame_buf[0] = ack_len;
	if (net_pkt_read(pkt, &ack_frame_buf[1], ack_len) < 0) {
		LOG_ERR("Failed to read ACK frame.");
		return NET_CONTINUE;
	}

	/* ack_frame != NULL informs that ACK frame has been received */
	ack_frame = ack_frame_buf;

	return NET_OK;
}

static enum net_verdict zigbee_l2_recv(struct net_if *iface,
					struct net_pkt *pkt)
{
	ARG_UNUSED(iface);

	k_fifo_put(&rx_fifo, pkt);

	zb_macll_set_rx_flag();
	zb_macll_set_trans_int();

	/* Raise signal to indicate rx event */
	zigbee_event_notify(ZIGBEE_EVENT_RX_DONE);

	return NET_OK;
}

static enum net_l2_flags zigbee_l2_flags(struct net_if *iface)
{
	ARG_UNUSED(iface);

	return 0;
}

NET_L2_INIT(ZIGBEE_L2, zigbee_l2_recv, NULL, NULL, zigbee_l2_flags);
