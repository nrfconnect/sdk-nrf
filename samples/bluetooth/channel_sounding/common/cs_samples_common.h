/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CS_SAMPLES_COMMON_H_
#define CS_SAMPLES_COMMON_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/conn.h>
#ifdef CONFIG_BT_SCAN
#include <bluetooth/scan.h>
#endif /* CONFIG_BT_SCAN */

#ifdef __cplusplus
extern "C" {
#endif

/* Shared connection state for Channel Sounding samples.
 *
 * These are defined in `cs_samples_common.c` and reused across the samples.
 */
extern struct bt_conn *connection;
extern struct bt_conn_le_cs_config cs_config;
extern struct k_sem sem_connected;
extern struct k_sem sem_security;
extern struct k_sem sem_cs_security_enabled;
extern struct k_sem sem_remote_capabilities_obtained;
extern struct k_sem sem_config_created;

/** @brief Median of @a values (sorted in place); returns NAN if @a count is 0. */
float common_median_inplace(int count, float *values);

#ifdef CONFIG_BT_SCAN
/** @brief Scan callback: filter matched (@ref BT_SCAN_CB_INIT). */
void common_scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match, bool connectable);

/** @brief Scan callback: connection failed; restart passive scan (@ref BT_SCAN_CB_INIT). */
void common_scan_connecting_error(struct bt_scan_device_info *device_info);

/** @brief Scan callback: connecting (@ref BT_SCAN_CB_INIT). */
void common_scan_connecting(struct bt_scan_device_info *device_info, struct bt_conn *conn);
#endif /* CONFIG_BT_SCAN */

/** @brief @c connected handler for Channel Sounding samples. */
void common_connected_cb(struct bt_conn *conn, uint8_t err);

/** @brief @c disconnected handler for Channel Sounding samples. */
void common_disconnected_cb(struct bt_conn *conn, uint8_t reason);

/** @brief @c security_changed handler for Channel Sounding samples. */
void common_security_changed_cb(struct bt_conn *conn, bt_security_t level,
				enum bt_security_err err);

/** @brief @c le_cs_security_enable_complete handler. */
void common_cs_security_enable_cb(struct bt_conn *conn, uint8_t status);

/** @brief @c le_cs_read_remote_capabilities_complete handler. */
void common_remote_capabilities_cb(struct bt_conn *conn, uint8_t status,
				   struct bt_conn_le_cs_capabilities *params);

/** @brief @c le_cs_config_complete handler. */
void common_config_create_cb(struct bt_conn *conn, uint8_t status,
			     struct bt_conn_le_cs_config *config);

/** @brief @c le_cs_procedure_enable_complete handler. */
void common_procedure_enable_cb(struct bt_conn *conn, uint8_t status,
				struct bt_conn_le_cs_procedure_enable_complete *params);

#ifdef __cplusplus
}
#endif

#endif /* CS_SAMPLES_COMMON_H_ */
