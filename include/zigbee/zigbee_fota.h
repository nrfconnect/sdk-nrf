/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file zigbee_fota.h
 *
 * @defgroup zigbee_fota Zigbee firmware over-the-air download library
 * @{
 * @brief  Library for downloading a firmware update through Zigbee network.
 *
 * @details Discovers Zigbee OTA server, queries it for new images and
 * downloads the matching file to the secondary partition of MCUboot.
 * After the file has been downloaded, the secondary slot is tagged as having
 * valid firmware inside it.
 */

#ifndef ZIGBEE_FOTA_H_
#define ZIGBEE_FOTA_H_

#include <zephyr/kernel.h>
#include <zboss_api.h>

#define ZIGBEE_FOTA_EVT_DL_COMPLETE_VAL 100

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Zigbee FOTA download event IDs.
 */
enum zigbee_fota_evt_id {
	/** Zigbee FOTA download progress report. */
	ZIGBEE_FOTA_EVT_PROGRESS,
	/** Zigbee FOTA download finished. */
	ZIGBEE_FOTA_EVT_FINISHED,
	/** Zigbee FOTA download error. */
	ZIGBEE_FOTA_EVT_ERROR,
};

/**
 * @brief Zigbee FOTA download progress event data.
 */
struct zigbee_fota_event_dl {
	int progress; /* Download progress percent, 0-100 */
};

/**
 * @brief Zigbee FOTA download event data.
 */
struct zigbee_fota_evt {
	enum zigbee_fota_evt_id id;
	union {
		struct zigbee_fota_event_dl dl;
	};
};

/**
 * @brief Zigbee FOTA download asynchronous callback function.
 *
 * @param evt Event.
 *
 */
typedef void (*zigbee_fota_callback_t)(const struct zigbee_fota_evt *evt);

/**
 * @brief Initialize the Zigbee firmware over-the-air download library.
 *
 * @param client_callback Callback for the generated events.
 *
 * @retval 0 If successfully initialized.
 *           Otherwise, a negative value is returned.
 */
int zigbee_fota_init(zigbee_fota_callback_t client_callback);

/**
 * @brief Abort all pending updates performed via Zigbee network.
 */
void zigbee_fota_abort(void);

/**
 * @brief Function for passing Zigbee stack signals to the Zigbee FOTA logic.
 *
 * @param[in] bufid  Reference to the Zigbee stack buffer used to pass signal.
 */
void zigbee_fota_signal_handler(zb_bufid_t bufid);

/**
 * @brief Function for passing ZCL callback events to the Zigbee FOTA logic.
 *
 * @param[in] bufid  Reference to the Zigbee stack buffer used to pass event.
 */
void zigbee_fota_zcl_cb(zb_bufid_t bufid);

#ifdef __cplusplus
}
#endif

#endif /* ZIGBEE_FOTA_H_ */

/**@} */
