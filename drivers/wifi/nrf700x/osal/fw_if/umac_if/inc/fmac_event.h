/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing event specific declarations for the FMAC IF Layer
 * of the Wi-Fi driver.
 */

#ifndef __FMAC_EVENT_H__
#define __FMAC_EVENT_H__

/**
 * wifi_nrf_fmac_event_callback() - RPU event classifier and handler.
 * @data: Pointer to the device driver context.
 * @event_data: Pointer to event data.
 * @len: Length of event data pointed to by @event_data.
 *
 * This callback classifies and processes an event. This classification of the
 * event is based on whether it contains data or control messages
 * and invokes further handlers based on that.
 *
 * Return: Status
 *		Pass: WIFI_NRF_STATUS_SUCCESS
 *		Fail: WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_event_callback(void *data,
						  void *event_data,
						  unsigned int len);
#endif /* __FMAC_EVENT_H__ */
