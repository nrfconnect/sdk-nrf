/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**@file
 * @brief Azure Firmware Over The Air (FOTA) library.
 */

#ifndef AZURE_FOTA_H__
#define AZURE_FOTA_H__

/**
 * @defgroup azure_fota Azure FOTA library
 * @{
 * @brief Library for performing Firmware Over The Air updates using
 *	  firmware information in JSON format.
 */

#ifdef __cplusplus
extern "C" {
#endif

enum azure_fota_evt_type {
	/** Azure FOTA state report is created and ready to be sent */
	AZURE_FOTA_EVT_REPORT,
	/** Azure FOTA download has started */
	AZURE_FOTA_EVT_START,
	/** Azure FOTA complete and status reported to job document */
	AZURE_FOTA_EVT_DONE,
	/** Azure FOTA error */
	AZURE_FOTA_EVT_ERROR,
	/** Azure FOTA erase pending */
	AZURE_FOTA_EVT_ERASE_PENDING,
	/** Azure FOTA erase done */
	AZURE_FOTA_EVT_ERASE_DONE,
};

/** @brief Azure FOTA event. */
struct azure_fota_event {
	/** Event type, @ref azure_fota_evt_type. */
	enum azure_fota_evt_type type;
	/** Pointer to the report associated with the event. A report is a JSON
	 *  string with information of the state of the FOTA process intended
	 *  to be sent to Azure IoT Hub device twin's reported property.
	 *  A valid report is provided if the pointer is not NULL.
	 */
	char *report;
	/* Firmware version associated with the event. */
	char *version;
};

/**
 * @typedef azure_fota_callback_t
 * @brief Callback to receive Azure FOTA events.
 *
 * @param evt Pointer to event structure.
 */
typedef void (*azure_fota_callback_t)(struct azure_fota_event *evt);

/** @brief Initialize the Azure Firmware Over the Air library.
 *
 *  @param evt_handler Callback function to receive events.
 *
 *  @retval 0 If successfully initialized.
 *	    Otherwise, a negative value is returned.
 */
int azure_fota_init(azure_fota_callback_t evt_handler);

/** @brief Process an Azure IoT Hub device twin JSON object, check for
 *	   FOTA information and act accordingly if found.
 *
 *  @param buf Pointer to buffer.
 *  @param size Size of buffer.
 *
 *  @retval 0 If no FOTA information found.
 *  @retval 1 If FOTA information was processed.
 *	    If an error occurs, a negative value is returned.
 */
int azure_fota_msg_process(const char *const buf, size_t size);

#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /* AZURE_FOTA_H__ */
