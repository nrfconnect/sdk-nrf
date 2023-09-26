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
 * nrf_wifi_fmac_event_callback() - RPU event classifier and handler.
 * @data: Pointer to the device driver context.
 * @event_data: Pointer to event data.
 * @len: Length of event data pointed to by @event_data.
 *
 * This callback classifies and processes an event. This classification of the
 * event is based on whether it contains data or control messages
 * and invokes further handlers based on that.
 *
 * Return: Status
 *		Pass: NRF_WIFI_STATUS_SUCCESS
 *		Fail: NRF_WIFI_STATUS_FAIL
 */
enum nrf_wifi_status nrf_wifi_fmac_event_callback(void *data,
						  void *event_data,
						  unsigned int len);
#endif /* __FMAC_EVENT_H__ */
