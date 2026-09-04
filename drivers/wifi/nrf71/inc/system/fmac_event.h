/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file fmac_event.h
 *
 * @brief Header containing event specific declarations in the system mode
 * for the FMAC IF Layer of the Wi-Fi driver.
 */

#ifndef __FMAC_EVENT_EVENT_H__
#define __FMAC_EVENT_EVENT_H__

enum nrf_wifi_status nrf_wifi_sys_fmac_event_callback(void *mac_dev_ctx,
						      void *rpu_event_data,
						      unsigned int rpu_event_len);

#endif /* __FMAC_EVENT_EVENT_H__ */
