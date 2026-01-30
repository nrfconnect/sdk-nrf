/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file fmac_event.h
 *
 * @brief Header containing event specific declarations in the
 * Offloaded raw TX mode for the FMAC IF Layer of the Wi-Fi driver.
 */
#ifndef __FMAC_EVENT_OFF_RAW_TX_EVENT_H__
#define __FMAC_EVENT_OFF_RAW_TX_EVENT_H__

/**
 * @brief RPU event classifier and handler.
 *
 * This callback classifies and processes an event. This classification of the
 * event is based on whether it contains data or control messages
 * and invokes further handlers based on that.
 *
 * @param mac_dev_ctx Pointer to the device driver context.
 * @param rpu_event_data Pointer to event data.
 * @param rpu_event_len Length of event data pointed to by @p rpu_event_data.
 *
 * @return Status
 *         - Pass: NRF_WIFI_STATUS_SUCCESS
 *         - Fail: NRF_WIFI_STATUS_FAIL
 */
enum nrf_wifi_status nrf_wifi_off_raw_tx_fmac_event_callback(void *mac_dev_ctx,
							     void *rpu_event_data,
							     unsigned int rpu_event_len);

#endif /* __FMAC_EVENT_OFF_RAW_TX_EVENT_H__ */
