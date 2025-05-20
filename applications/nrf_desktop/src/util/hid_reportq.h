/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief HID report queue header.
 */

#ifndef _HID_REPORTQ_H_
#define _HID_REPORTQ_H_

/**
 * @defgroup hid_reportq HID report queue
 * @brief Utility that simplifies queuing HID reports from HID peripherals on a HID dongle.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque type representing HID report queue object. */
struct hid_reportq;

/**
 * @brief Allocate a HID report queue object instance.
 *
 * The function allocates HID report queue object instance from internal pool.
 *
 * @param[in] sub_id		ID of related HID subscriber.
 * @param[in] report_max	Maximum number of reports with different ID, which can be processed
 *                              by related HID subscriber.
 *
 * @return Pointer to the allocated HID report queue object instance or NULL in case of too small
 *         object pool.
 */
struct hid_reportq *hid_reportq_alloc(const void *sub_id, uint8_t report_max);

/**
 * @brief Free HID report queue object instance.
 *
 * @param[in] q		Pointer to the queue to be freed.
 */
void hid_reportq_free(struct hid_reportq *q);

/**
 * @brief Get ID of HID subscriber related to a HID report queue object instance.
 *
 * @param[in] q		Pointer to the queue instance.
 *
 * @return ID of related HID subscriber.
 */
const void *hid_reportq_get_sub_id(struct hid_reportq *q);

/**
 * @brief Add a HID report to the queue.
 *
 * The function returns an error if HID report subscription is disabled for the added HID report.
 *
 * If number of enqueued reports with a given report ID exceeds limit defined by the configuration
 * (@kconfig{CONFIG_DESKTOP_HID_REPORTQ_MAX_ENQUEUED_REPORTS}), the oldest enqueued HID report with
 * the ID is dropped.
 *
 * @param[in] q		Pointer to the queue instance.
 * @param[in] src_id    ID of HID report source.
 * @param[in] rep_id	HID report ID.
 * @param[in] data	HID report data.
 * @param[in] size	Size of the HID report data.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int hid_reportq_report_add(struct hid_reportq *q, const void *src_id, uint8_t rep_id,
			   const uint8_t *data, size_t size);

/**
 * @brief Notify HID report queue that HID report was sent.
 *
 * @param[in] q		Pointer to the queue instance.
 * @param[in] rep_id	HID report ID.
 * @param[in] err	Error occurred on send.
 */
void hid_reportq_report_sent(struct hid_reportq *q, uint8_t rep_id, bool err);

/**
 * @brief Check if HID report queue is subscribed for HID report with given ID.
 *
 * @param[in] q		Pointer to the queue instance.
 * @param[in] rep_id	HID report ID.
 *
 * @return true if subscribed, false otherwise.
 */
bool hid_reportq_is_subscribed(struct hid_reportq *q, uint8_t rep_id);

/**
 * @brief Subscribe for HID report with given ID.
 *
 * @param[in] q		Pointer to the queue instance.
 * @param[in] rep_id	HID report ID.
 */
void hid_reportq_subscribe(struct hid_reportq *q, uint8_t rep_id);

/**
 * @brief Unsubscribe from HID report with given ID.
 *
 * Disabling subscription for a given HID report ID, drops all of the enqueued HID reports related
 * to the ID.
 *
 * @param[in] q		Pointer to the queue instance.
 * @param[in] rep_id	HID report ID.
 */
void hid_reportq_unsubscribe(struct hid_reportq *q, uint8_t rep_id);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /*_HID_REPORTQ_H_ */
