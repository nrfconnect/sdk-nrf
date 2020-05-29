/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <kernel.h>
#include <logging/log.h>
#include <nrf_802154.h>
#include <nrf_802154_const.h>
#include <zboss_api.h>
#include <zb_macll.h>
#include <zb_transceiver.h>
#include "zb_nrf_platform.h"

#if !defined NRF_802154_FRAME_TIMESTAMP_ENABLED ||                             \
	!NRF_802154_FRAME_TIMESTAMP_ENABLED
#warning Must define NRF_802154_FRAME_TIMESTAMP_ENABLED!
#endif

LOG_MODULE_DECLARE(zboss_osif, CONFIG_ZBOSS_OSIF_LOG_LEVEL);


/* Definition of FIFO queue entry for the received frame */
typedef struct nrf_802154_rx_frame {
	void	*fifo_reserved; /* 1st word reserved for the kernel's use. */
	u8_t	*data; /* Pointer to a received frame. */
	s8_t	power; /* Last received frame RSSI value. */
	u8_t	lqi; /* Last received frame LQI value. */
	u32_t	time; /* Timestamp in microseconds. */
	bool	pending_bit;
} rx_frame_t;

/* RX fifo queue. */
static struct k_fifo rx_fifo;

/* Array of pointers to received frames and their metadata.
 *
 * Keep one additional element in case a new frame is received after
 * nrf_802154_buffer_free_raw(..) is called and before the rx_frame->data
 * is set back to NULL.
 */
static rx_frame_t rx_frames[NRF_802154_RX_BUFFERS + 1];

static struct {
	/* Semaphore for waiting for end of energy detection procedure. */
	struct k_sem sem;
	volatile bool failed;   /* Energy detection procedure failed. */
	volatile u32_t time_us; /* Duration of energy detection procedure. */
	volatile u8_t rssi_val; /* Detected energy level. */
} energy_detect;

static volatile bool acked_with_pending_bit;

#if !CONFIG_MPSL
static void nrf5_radio_irq(void *arg)
{
	ARG_UNUSED(arg);

	nrf_802154_radio_irq_handler();
}
#endif

static void nrf5_irq_config(void)
{
#if !CONFIG_MPSL
	IRQ_CONNECT(RADIO_IRQn, NRF_802154_IRQ_PRIORITY, nrf5_radio_irq, NULL,
		    0);
	irq_enable(RADIO_IRQn);
#endif
}

/* Initializes the transceiver. */
void zb_trans_hw_init(void)
{
	nrf_802154_init();
	nrf_802154_auto_ack_set(ZB_TRUE);
	nrf_802154_src_addr_matching_method_set(
		NRF_802154_SRC_ADDR_MATCH_ZIGBEE);

	/* WORKAROUND: Send continuous carrier to leave the sleep state
	 *             and request the HFCLK.
	 *             Manually wait for HFCLK to synchronize by sleeping
	 *             for 1 ms afterwards.
	 *
	 * The bug causes the Radio Driver to start transmitting
	 * frames while the HFCLK is still starting.
	 * As a result, the Radio Driver, as well as the ZBOSS,
	 * is notified that the frame was sent, but it was not, because
	 * the analog part of the radio was not working at that time.
	 * The carrier will not be transmitted for the same reason.
	 * It is placed here just to trigger the proper set of events
	 * in Radio Driver.
	 *
	 * Since the coordinator/router/non-sleepy end device does
	 * not reenter sleep state, it is enough to apply this
	 * procedure here. For SED it has to be repeated every time
	 * the Radio leaves sleep state.
	 *
	 * More info: https://github.com/zephyrproject-rtos/zephyr/issues/20712
	 * JIRA issue: KRKNWK-5533
	 */
	/*(void)nrf_802154_sleep(); */
	(void)nrf_802154_continuous_carrier();
	k_sleep(K_MSEC(1));
	(void)nrf_802154_receive();

	k_fifo_init(&rx_fifo);
	k_sem_init(&energy_detect.sem, 1, 1);

	nrf5_irq_config();

	LOG_DBG("The 802.15.4 radio initialized.");
}

/* Sets the PAN ID used by the device. */
void zb_trans_set_pan_id(zb_uint16_t pan_id)
{
	LOG_DBG("Function: %s, PAN ID: 0x%x", __func__, pan_id);
	nrf_802154_pan_id_set((zb_uint8_t *)(&pan_id));
}

/* Sets the long address in the radio driver. */
void zb_trans_set_long_addr(zb_ieee_addr_t long_addr)
{
	LOG_DBG("Function: %s, long addr: 0x%llx", __func__, (u64_t)*long_addr);
	nrf_802154_extended_address_set(long_addr);
}

/* Sets the short address of the device. */
void zb_trans_set_short_addr(zb_uint16_t addr)
{
	LOG_DBG("Function: %s, 0x%x", __func__, addr);
	nrf_802154_short_address_set((u8_t *)(&addr));
}

/* Start the energy detection procedure */
void zb_trans_start_get_rssi(zb_uint8_t scan_duration_bi)
{
	energy_detect.failed = false;
	energy_detect.time_us = ZB_TIME_BEACON_INTERVAL_TO_USEC(
		scan_duration_bi);

	LOG_DBG("Function: %s, scan duration: %d", __func__,
		energy_detect.time_us);
	k_sem_take(&energy_detect.sem, K_FOREVER);
	while (!nrf_802154_energy_detection(energy_detect.time_us)) {
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
		if (energy_detect.failed == false) {
			*rssi_value_p = energy_detect.rssi_val;
			LOG_DBG("Energy detected: %d", *rssi_value_p);
			break;
		}
		/* Try again */
		LOG_DBG("Energy detect failed, tries again");
		energy_detect.failed = false;
		while (!nrf_802154_energy_detection(energy_detect.time_us)) {
			k_usleep(500);
		}
	}
	k_sem_give(&energy_detect.sem);
}

/* Set channel and go to the normal (not ed scan) mode */
void zb_trans_set_channel(zb_uint8_t channel_number)
{
	LOG_DBG("Function: %s, channel number: %d", __func__, channel_number);
	nrf_802154_channel_set(channel_number);
}

/* Sets the transmit power. */
void zb_trans_set_tx_power(zb_int8_t power)
{
	LOG_DBG("Function: %s, power: %d", __func__, power);
	nrf_802154_tx_power_set(power);
}

/* Gets the currently set transmit power. */
void zb_trans_get_tx_power(zb_int8_t *power)
{
	*power = (zb_int8_t)nrf_802154_tx_power_get();
	LOG_DBG("Function: %s, power: %d", __func__, *power);
}

/* Configures the device as the PAN coordinator. */
void zb_trans_set_pan_coord(zb_bool_t enabled)
{
	LOG_DBG("Function: %s, enabled: %d", __func__, enabled);
	nrf_802154_pan_coord_set((bool)enabled);
}

/* Enables or disables the automatic acknowledgments (auto ACK) */
void zb_trans_set_auto_ack(zb_bool_t enabled)
{
	LOG_DBG("Function: %s, enabled: %d", __func__, enabled);
	nrf_802154_auto_ack_set((bool)enabled);
}

/* Enables or disables the promiscuous radio mode. */
void zb_trans_set_promiscuous_mode(zb_bool_t enabled)
{
	LOG_DBG("Function: %s, enabled: %d", __func__, enabled);
	nrf_802154_promiscuous_set((bool)enabled);
}

/* Changes the radio state to receive. */
void zb_trans_enter_receive(void)
{
	LOG_DBG("Function: %s", __func__);
	(void)nrf_802154_receive();
}

/* Changes the radio state to sleep. */
void zb_trans_enter_sleep(void)
{
	LOG_DBG("Function: %s", __func__);
	(void)nrf_802154_sleep_if_idle();
}

/* Returns ZB_TRUE if radio is in receive state, otherwise ZB_FALSE */
zb_bool_t zb_trans_is_receiving(void)
{
	zb_bool_t is_receiv =
		(nrf_802154_state_get() == NRF_802154_STATE_RECEIVE) ?
			ZB_TRUE : ZB_FALSE;

	LOG_DBG("Function: %s, is receiv: %d", __func__, is_receiv);
	return is_receiv;
}

/* Returns ZB_TRUE if radio is ON or ZB_FALSE if is in sleep state. */
zb_bool_t zb_trans_is_active(void)
{
	zb_bool_t is_active =
		(nrf_802154_state_get() != NRF_802154_STATE_SLEEP) ?
			ZB_TRUE : ZB_FALSE;

	LOG_DBG("Function: %s, is active: %d", __func__, is_active);
	return is_active;
}

zb_bool_t zb_trans_transmit(zb_uint8_t wait_type, zb_time_t tx_at,
			    zb_uint8_t *tx_buf, zb_uint8_t current_channel)
{
	LOG_DBG("Function: %s, channel: %d", __func__, current_channel);
	zb_bool_t transmit_status = ZB_FALSE;

#ifndef ZB_ENABLE_ZGP_DIRECT
	ARG_UNUSED(tx_at);
	ARG_UNUSED(current_channel);
#endif

	switch (wait_type) {
	case ZB_MAC_TX_WAIT_CSMACA:
#if !NRF_802154_CSMA_CA_ENABLED
		while (!transmit_status) {
			transmit_status = (zb_bool_t)nrf_802154_transmit_raw(
				tx_buf, ZB_FALSE);
		}
		break;
#else
		transmit_status = ZB_TRUE;
		nrf_802154_transmit_csma_ca_raw(tx_buf);
#endif
		break;

#ifdef ZB_ENABLE_ZGP_DIRECT
	case ZB_MAC_TX_WAIT_ZGP:
		LOG_ERR("ZB_MAC_TX_WAIT_ZGP - not implemented in osif, TBD");
		break;
#endif
	case ZB_MAC_TX_WAIT_NONE:
		/* First transmit attempt without CCA. */
		transmit_status = (zb_bool_t)nrf_802154_transmit_raw(
			tx_buf, ZB_FALSE);
		break;
	default:
		LOG_DBG("Illegal wait_type parameter: %d", wait_type);
		ZB_ASSERT(0);
		break;
	}
	return transmit_status;
}

/* Notifies the driver that the buffer containing the received frame
 * is not used anymore
 */
void zb_trans_buffer_free(zb_uint8_t *buf)
{
	LOG_DBG("Function: %s", __func__);
	nrf_802154_buffer_free_raw(buf);
}

zb_bool_t zb_trans_set_pending_bit(zb_uint8_t *addr, zb_bool_t value,
				   zb_bool_t extended)
{
	LOG_DBG("Function: %s, value: %d", __func__, value);
	if (!value) {
		return (zb_bool_t)nrf_802154_pending_bit_for_addr_set(
			(const u8_t *)addr, (bool)extended);
	} else {
		return (zb_bool_t)nrf_802154_pending_bit_for_addr_clear(
			(const u8_t *)addr, (bool)extended);
	}
}

void zb_trans_src_match_tbl_drop(void)
{
	LOG_DBG("Function: %s", __func__);

	/* reset for short addresses */
	nrf_802154_pending_bit_for_addr_reset(ZB_FALSE);

	/* reset for long addresses */
	nrf_802154_pending_bit_for_addr_reset(ZB_TRUE);
}

#ifdef CONFIG_RADIO_STATISTICS
static zb_osif_radio_stats_t nrf_radio_statistics;

zb_osif_radio_stats_t *zb_osif_get_radio_stats(void)
{
	return &nrf_radio_statistics;
}
#endif /* defined CONFIG_RADIO_STATISTICS */

zb_time_t osif_sub_trans_timer(zb_time_t t2, zb_time_t t1)
{
	return ZB_TIME_SUBTRACT(t2, t1);
}

zb_uint8_t zb_trans_get_next_packet(zb_bufid_t buf)
{
	LOG_DBG("Function: %s", __func__);
	zb_uint8_t *data_ptr;
	zb_uint8_t length = 0;

	if (!buf) {
		return 0;
	}

	/*Packed received with correct CRC, PANID and address*/
	rx_frame_t *rx_frame = k_fifo_get(&rx_fifo, K_NO_WAIT);

	if (!rx_frame) {
		return 0;
	}

	length = rx_frame->data[0];

	data_ptr = zb_buf_initial_alloc(buf, length);

	/*Copy received data*/
	ZB_MEMCPY(data_ptr, (void const *)(rx_frame->data + 1), length);

	/*Put LQI, RSSI*/
	zb_macll_metadata_t *metadata = ZB_MACLL_GET_METADATA(buf);

	metadata->lqi = rx_frame->lqi;
	metadata->power = rx_frame->power;
	/* Put timestamp (usec) into the packet tail */
	*ZB_BUF_GET_PARAM(buf, zb_time_t) = rx_frame->time;
	/* Additional buffer status for Data Request command */
	zb_macll_set_received_data_status(buf, rx_frame->pending_bit);

	nrf_802154_buffer_free_raw(rx_frame->data);
	rx_frame->data = NULL;

	return 1;
}

/************ 802.15.4 radio driver callbacks **************/

/**
 * @brief Notify that frame was transmitted.
 *
 * @note If ACK was requested for transmitted frame this function
 *       is called after proper ACK is received.
 *       If ACK was not requested this function is called just
 *       after transmission is ended.
 * @note Buffer pointed by the @p ack pointer is not modified
 *       by the radio driver (and can't be used to receive a frame)
 *       until @sa nrf_802154_buffer_free_raw() function is called.
 * @note Buffer pointed by the @p ack pointer may be modified by
 *       the function handler (and other modules) until
 *       @sa nrf_802154_buffer_free_raw() function is called.
 * @note The next higher layer should handle @sa nrf_802154_transmitted_raw()
 *       or @sa nrf_802154_transmitted() function. It should not handle both.
 *
 * @param[in]  ack    Pointer to received ACK buffer. Fist byte
 *                    in the buffer is length of the frame (PHR) and
 *                    following bytes are the ACK frame itself (PSDU).
 *                    Length byte (PHR) includes FCS. FCS is already
 *                    verified by the hardware and may be modified by
 *                    the hardware. If ACK was not requested @p ack
 *                    is set to NULL.
 * @param[in]  power  RSSI of received frame or 0 if ACK was not requested.
 * @param[in]  lqi    LQI of received frame or 0 if ACK was not requested.
 */
void nrf_802154_transmitted_raw(const u8_t *frame, u8_t *ack,
				s8_t power, u8_t lqi)
{
	ARG_UNUSED(frame);
	ARG_UNUSED(power);
	ARG_UNUSED(lqi);

#ifdef CONFIG_RADIO_STATISTICS
	zb_osif_get_radio_stats()->tx_successful++;
#endif /* defined CONFIG_RADIO_STATISTICS */

	zb_macll_transmitted_raw(ack);

	/* Raise signal to indicate radio event */
	zigbee_event_notify(ZIGBEE_EVENT_TX_DONE);
}

/**
 * @brief Notify that frame was not transmitted due to busy channel.
 *
 * This function is called if transmission procedure fails.
 *
 * @param[in]  error  Reason of the failure.
 */

void nrf_802154_transmit_failed(u8_t const *frame,
				nrf_802154_tx_error_t error)
{
	ARG_UNUSED(frame);
#ifdef CONFIG_RADIO_STATISTICS
	switch (error) {
	case NRF_802154_TX_ERROR_NONE:
		zb_osif_get_radio_stats()->tx_err_none++;
		break;
	case NRF_802154_TX_ERROR_BUSY_CHANNEL:
		zb_osif_get_radio_stats()->tx_err_busy_channel++;
		break;
	case NRF_802154_TX_ERROR_INVALID_ACK:
		zb_osif_get_radio_stats()->tx_err_invalid_ack++;
		break;
	case NRF_802154_TX_ERROR_NO_MEM:
		zb_osif_get_radio_stats()->tx_err_no_mem++;
		break;
	case NRF_802154_TX_ERROR_TIMESLOT_ENDED:
		zb_osif_get_radio_stats()->tx_err_timeslot_ended++;
		break;
	case NRF_802154_TX_ERROR_NO_ACK:
		zb_osif_get_radio_stats()->tx_err_no_ack++;
		break;
	case NRF_802154_TX_ERROR_ABORTED:
		zb_osif_get_radio_stats()->tx_err_aborted++;
		break;
	case NRF_802154_TX_ERROR_TIMESLOT_DENIED:
		zb_osif_get_radio_stats()->tx_err_timeslot_denied++;
		break;
	}
#endif /* defined CONFIG_RADIO_STATISTICS */

	switch (error) {
	case NRF_802154_TX_ERROR_NO_MEM:
	case NRF_802154_TX_ERROR_ABORTED:
	case NRF_802154_TX_ERROR_TIMESLOT_DENIED:
	case NRF_802154_TX_ERROR_TIMESLOT_ENDED:
	case NRF_802154_TX_ERROR_BUSY_CHANNEL:
		zb_macll_transmit_failed(ZB_TRANS_CHANNEL_BUSY_ERROR);
		break;

	case NRF_802154_TX_ERROR_INVALID_ACK:
	case NRF_802154_TX_ERROR_NO_ACK:
		zb_macll_transmit_failed(ZB_TRANS_NO_ACK);
		break;
	}

	/* Raise signal to indicate radio event */
	zigbee_event_notify(ZIGBEE_EVENT_TX_FAILED);
}

/* Callback notifies about the start of the ACK frame transmission. */
void nrf_802154_tx_ack_started(const u8_t *data)
{
    /* Check if the frame pending bit is set in ACK frame. */
	acked_with_pending_bit = data[FRAME_PENDING_OFFSET] & FRAME_PENDING_BIT;
}

/**
 * @brief Notify that frame was received.
 *
 * @note Buffer pointed by the data pointer is not modified
 *       by the radio driver (and can't be used to receive a frame)
 *       until nrf_802154_buffer_free_raw() function is called.
 * @note Buffer pointed by the data pointer may be modified by the function
 *       handler (and other modules) until @sa nrf_802154_buffer_free_raw()
 *       function is called.
 * @note The next higher layer should handle @sa nrf_802154_received_raw()
 *       or @sa nrf_802154_received() function. It should not handle both.
 *
 * data
 * v
 * +-----+---------------------------------------------------+------------+
 * | PHR | MAC Header and payload                            | FCS        |
 * +-----+---------------------------------------------------+------------+
 *       |                                                                |
 *       | <---------------------------- PHR ---------------------------> |
 *
 * @param[in]  data    Pointer to the buffer containing received data
 *                     (PHR + PSDU). First byte in the buffer is length
 *                     of the frame (PHR) and following bytes is the frame
 *                     itself (PSDU). Length byte (PHR) includes FCS.
 *                     FCS is already verified by the hardware and may be
 *                     modified by the hardware.
 * @param[in]  power   RSSI of received frame.
 * @param[in]  lqi     LQI of received frame.
 */
void nrf_802154_received_timestamp_raw(u8_t *data, s8_t power,
				       u8_t lqi, u32_t time)
{
#ifdef CONFIG_RADIO_STATISTICS
	zb_osif_get_radio_stats()->rx_successful++;
#endif /* defined CONFIG_RADIO_STATISTICS */

	rx_frame_t *rx_frame_free_slot = NULL;

	/* Find free slot for frame */
	for (u32_t i = 0; i < ARRAY_SIZE(rx_frames); i++) {
		if (rx_frames[i].data == NULL) {
			rx_frame_free_slot = &rx_frames[i];
			break;
		}
	}

	if (rx_frame_free_slot == NULL) {
		__ASSERT(false,
			 "Not enough rx frames allocated for 15.4 driver");
		return;
	}

	rx_frame_free_slot->data = data;
	rx_frame_free_slot->power = power;
	rx_frame_free_slot->lqi = lqi;
	rx_frame_free_slot->time = time;

	if (data[ACK_REQUEST_OFFSET] & ACK_REQUEST_BIT) {
		rx_frame_free_slot->pending_bit = acked_with_pending_bit;
	} else {
		rx_frame_free_slot->pending_bit = ZB_FALSE;
	}

	acked_with_pending_bit = ZB_FALSE;
	k_fifo_put(&rx_fifo, rx_frame_free_slot);

	zb_macll_set_rx_flag();
	zb_macll_set_trans_int();

	/* Raise signal to indicate radio event */
	zigbee_event_notify(ZIGBEE_EVENT_RX_DONE);
}

/* Callback notify that reception of a frame failed. */
void nrf_802154_receive_failed(nrf_802154_rx_error_t error)
{
	acked_with_pending_bit = ZB_FALSE;

#ifdef CONFIG_RADIO_STATISTICS
	switch (error) {
	case NRF_802154_RX_ERROR_NONE:
		zb_osif_get_radio_stats()->rx_err_none++;
		break;
	case NRF_802154_RX_ERROR_INVALID_FRAME:
		zb_osif_get_radio_stats()->rx_err_invalid_frame++;
		break;
	case NRF_802154_RX_ERROR_INVALID_FCS:
		zb_osif_get_radio_stats()->rx_err_invalid_fcs++;
		break;
	case NRF_802154_RX_ERROR_INVALID_DEST_ADDR:
		zb_osif_get_radio_stats()->rx_err_invalid_dest_addr++;
		break;
	case NRF_802154_RX_ERROR_RUNTIME:
		zb_osif_get_radio_stats()->rx_err_runtime++;
		break;
	case NRF_802154_RX_ERROR_TIMESLOT_ENDED:
		zb_osif_get_radio_stats()->rx_err_timeslot_ended++;
		break;
	case NRF_802154_RX_ERROR_ABORTED:
		zb_osif_get_radio_stats()->rx_err_aborted++;
		break;
	}
#endif /* defined CONFIG_RADIO_STATISTICS */
}

/* Callback notify that Energy Detection procedure finished. */
void nrf_802154_energy_detected(u8_t result)
{
	energy_detect.rssi_val = result;
	k_sem_give(&energy_detect.sem);
}

/* Callback notifies that the energy detection procedure failed. */
void nrf_802154_energy_detection_failed(nrf_802154_ed_error_t error)
{
	ARG_UNUSED(error);
	energy_detect.failed = true;
	k_sem_give(&energy_detect.sem);
}
