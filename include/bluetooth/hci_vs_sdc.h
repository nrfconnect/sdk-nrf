/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _HCI_VS_SDC_
#define _HCI_VS_SDC_

/**
 * @file
 * @defgroup hci_vs_sdc Bluetooth HCI vendor specific APIs for the SoftDevice controller
 * @{
 * @brief Bluetooth HCI vendor specific APIs for the SoftDevice controller
 *
 * This file defines APIs that will make the Bluetooth Stack issue
 * the corresponding vendor specific HCI commands to the controller.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <sdc_hci_vs.h>

/** @brief Zephyr Read Version Information.
 *
 * For the complete API description, see sdc_hci_cmd_vs_zephyr_read_version_info().
 *
 * @param[out] return_params Return parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_zephyr_read_version_info(
	sdc_hci_cmd_vs_zephyr_read_version_info_return_t *return_params);

/** @brief Zephyr Read Supported Commands.
 *
 * For the complete API description, see sdc_hci_cmd_vs_zephyr_read_supported_commands().
 *
 * @param[out] return_params Return parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_zephyr_read_supported_commands(
	sdc_hci_cmd_vs_zephyr_read_supported_commands_return_t *return_params);

/** @brief Zephyr Write BD ADDR.
 *
 * For the complete API description, see sdc_hci_cmd_vs_zephyr_write_bd_addr().
 *
 * @param[in]  params Input parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_zephyr_write_bd_addr(const sdc_hci_cmd_vs_zephyr_write_bd_addr_t *params);

/** @brief Zephyr Read Static Addresses.
 *
 * For the complete API description, see sdc_hci_cmd_vs_zephyr_read_static_addresses().
 *
 * @param[out] return_params Return parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_zephyr_read_static_addresses(
	sdc_hci_cmd_vs_zephyr_read_static_addresses_return_t *return_params);

/** @brief Zephyr Read KEY Hierarchy Roots.
 *
 * For the complete API description, see sdc_hci_cmd_vs_zephyr_read_key_hierarchy_roots().
 *
 * @param[out] return_params Return parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_zephyr_read_key_hierarchy_roots(
	sdc_hci_cmd_vs_zephyr_read_key_hierarchy_roots_return_t *return_params);

/** @brief Zephyr Read Chip Temperature.
 *
 * For the complete API description, see sdc_hci_cmd_vs_zephyr_read_chip_temp().
 *
 * @param[out] return_params Return parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_zephyr_read_chip_temp(sdc_hci_cmd_vs_zephyr_read_chip_temp_return_t *return_params);

/** @brief Zephyr Write Tx Power Level (per Role/Connection).
 *
 * For the complete API description, see sdc_hci_cmd_vs_zephyr_write_tx_power().
 *
 * @param[in]  params Input parameters.
 * @param[out] return_params Return parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_zephyr_write_tx_power(
	const sdc_hci_cmd_vs_zephyr_write_tx_power_t *params,
	sdc_hci_cmd_vs_zephyr_write_tx_power_return_t *return_params);

/** @brief Zephyr Read Tx Power Level (per Role/Connection) Command.
 *
 * For the complete API description, see sdc_hci_cmd_vs_zephyr_read_tx_power().
 *
 * @param[in]  params Input parameters.
 * @param[out] return_params Return parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_zephyr_read_tx_power(
	const sdc_hci_cmd_vs_zephyr_read_tx_power_t *params,
	sdc_hci_cmd_vs_zephyr_read_tx_power_return_t *return_params);

/** @brief Read Supported Vendor Specific Commands.
 *
 * For the complete API description, see sdc_hci_cmd_vs_read_supported_vs_commands().
 *
 * @param[out] return_params Return parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_read_supported_vs_commands(
	sdc_hci_cmd_vs_read_supported_vs_commands_return_t *return_params);

/** @brief Set Low Latency Packet Mode.
 *
 * For the complete API description, see sdc_hci_cmd_vs_llpm_mode_set().
 *
 * @param[in]  params Input parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_llpm_mode_set(const sdc_hci_cmd_vs_llpm_mode_set_t *params);

/** @brief Connection Update.
 *
 * For the complete API description, see sdc_hci_cmd_vs_conn_update().
 *
 * @param[in]  params Input parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_conn_update(const sdc_hci_cmd_vs_conn_update_t *params);

/** @brief Enable or Disable Extended Connection Events.
 *
 * For the complete API description, see sdc_hci_cmd_vs_conn_event_extend().
 *
 * @param[in]  params Input parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_conn_event_extend(const sdc_hci_cmd_vs_conn_event_extend_t *params);

/** @brief QoS Connection Event Reports enable.
 *
 * For the complete API description, see sdc_hci_cmd_vs_qos_conn_event_report_enable().
 *
 * @param[in]  params Input parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_qos_conn_event_report_enable(
	const sdc_hci_cmd_vs_qos_conn_event_report_enable_t *params);

/** @brief Set event length for ACL connections.
 *
 * For the complete API description, see sdc_hci_cmd_vs_event_length_set().
 *
 * @param[in]  params Input parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_event_length_set(const sdc_hci_cmd_vs_event_length_set_t *params);

/** @brief Set event length for periodic advertisers.
 *
 * For the complete API description, see sdc_hci_cmd_vs_periodic_adv_event_length_set().
 *
 * @param[in]  params Input parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_periodic_adv_event_length_set(
	const sdc_hci_cmd_vs_periodic_adv_event_length_set_t *params);

/** @brief Set peripheral latency mode.
 *
 * For the complete API description, see sdc_hci_cmd_vs_peripheral_latency_mode_set().
 *
 * @param[in]  params Input parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_peripheral_latency_mode_set(
	const sdc_hci_cmd_vs_peripheral_latency_mode_set_t *params);

/** @brief Write remote transmit power level.
 *
 * For the complete API description, see sdc_hci_cmd_vs_write_remote_tx_power().
 *
 * @param[in]  params Input parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_write_remote_tx_power(const sdc_hci_cmd_vs_write_remote_tx_power_t *params);

/** @brief Set advertising randomness.
 *
 * For the complete API description, see sdc_hci_cmd_vs_set_adv_randomness().
 *
 * @param[in]  params Input parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_set_adv_randomness(const sdc_hci_cmd_vs_set_adv_randomness_t *params);

/** @brief Set Compatibility mode for window offset.
 *
 * For the complete API description, see sdc_hci_cmd_vs_compat_mode_window_offset_set().
 *
 * @param[in]  params Input parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_compat_mode_window_offset_set(
	const sdc_hci_cmd_vs_compat_mode_window_offset_set_t *params);

/** @brief Enable the Quality of Service (QoS) channel survey module.
 *
 * For the complete API description, see sdc_hci_cmd_vs_qos_channel_survey_enable().
 *
 * @param[in]  params Input parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_qos_channel_survey_enable(const sdc_hci_cmd_vs_qos_channel_survey_enable_t *params);

/** @brief Set LE Power Control Request procedure parameters.
 *
 * For the complete API description, see sdc_hci_cmd_vs_set_power_control_request_params().
 *
 * @param[in]  params Input parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_set_power_control_request_params(
	const sdc_hci_cmd_vs_set_power_control_request_params_t *params);

/** @brief Read average RSSI.
 *
 * For the complete API description, see sdc_hci_cmd_vs_read_average_rssi().
 *
 * @param[in]  params Input parameters.
 * @param[out] return_params Return parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_read_average_rssi(const sdc_hci_cmd_vs_read_average_rssi_t *params,
					 sdc_hci_cmd_vs_read_average_rssi_return_t *return_params);

/** @brief Set Central ACL event spacing.
 *
 * For the complete API description, see sdc_hci_cmd_vs_central_acl_event_spacing_set().
 *
 * @param[in]  params Input parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_central_acl_event_spacing_set(
	const sdc_hci_cmd_vs_central_acl_event_spacing_set_t *params);

/** @brief Set Connection Event Trigger.
 *
 * For the complete API description, see sdc_hci_cmd_vs_set_conn_event_trigger().
 *
 * @param[in]  params Input parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_set_conn_event_trigger(const sdc_hci_cmd_vs_set_conn_event_trigger_t *params);

/** @brief Get Next Connection Event Counter.
 *
 * For the complete API description, see sdc_hci_cmd_vs_get_next_conn_event_counter().
 *
 * @param[in]  params Input parameters.
 * @param[out] return_params Return parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_get_next_conn_event_counter(
	const sdc_hci_cmd_vs_get_next_conn_event_counter_t *params,
	sdc_hci_cmd_vs_get_next_conn_event_counter_return_t *return_params);

/** @brief Allow Parallel Connection Establishment.
 *
 * For the complete API description, see sdc_hci_cmd_vs_allow_parallel_connection_establishments().
 *
 * @param[in]  params Input parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_allow_parallel_connection_establishments(
	const sdc_hci_cmd_vs_allow_parallel_connection_establishments_t *params);

/** @brief Set the minimum value that will be used as maximum Tx octets for ACL connections.
 *
 * For the complete API description, see sdc_hci_cmd_vs_min_val_of_max_acl_tx_payload_set().
 *
 * @param[in]  params Input parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_min_val_of_max_acl_tx_payload_set(
	const sdc_hci_cmd_vs_min_val_of_max_acl_tx_payload_set_t *params);

/** @brief Iso Read Tx Timestamp.
 *
 * For the complete API description, see sdc_hci_cmd_vs_iso_read_tx_timestamp().
 *
 * @param[in]  params Input parameters.
 * @param[out] return_params Return parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_iso_read_tx_timestamp(
	const sdc_hci_cmd_vs_iso_read_tx_timestamp_t *params,
	sdc_hci_cmd_vs_iso_read_tx_timestamp_return_t *return_params);

/** @brief Set the default BIG reserved time.
 *
 * For the complete API description, see sdc_hci_cmd_vs_big_reserved_time_set().
 *
 * @param[in]  params Input parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_big_reserved_time_set(const sdc_hci_cmd_vs_big_reserved_time_set_t *params);

/** @brief Set the default CIG reserved time.
 *
 * For the complete API description, see sdc_hci_cmd_vs_cig_reserved_time_set().
 *
 * @param[in]  params Input parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_cig_reserved_time_set(const sdc_hci_cmd_vs_cig_reserved_time_set_t *params);

/** @brief Set the CIS subevent length in microseconds.
 *
 * For the complete API description, see sdc_hci_cmd_vs_cis_subevent_length_set().
 *
 * @param[in]  params Input parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_cis_subevent_length_set(const sdc_hci_cmd_vs_cis_subevent_length_set_t *params);

/** @brief Set the channel map for scanning and initiating.
 *
 * For the complete API description, see sdc_hci_cmd_vs_scan_channel_map_set().
 *
 * @param[in]  params Input parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_scan_channel_map_set(const sdc_hci_cmd_vs_scan_channel_map_set_t *params);

/** @brief Scan accept extended advertising packets set.
 *
 * For the complete API description, see sdc_hci_cmd_vs_scan_accept_ext_adv_packets_set().
 *
 * @param[in]  params Input parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_scan_accept_ext_adv_packets_set(
	const sdc_hci_cmd_vs_scan_accept_ext_adv_packets_set_t *params);

/** @brief Set priority of a BT role.
 *
 * For the complete API description, see sdc_hci_cmd_vs_set_role_priority().
 *
 * @param[in]  params Input parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_set_role_priority(const sdc_hci_cmd_vs_set_role_priority_t *params);

/** @brief Set Event Start Task.
 *
 * For the complete API description, see sdc_hci_cmd_vs_set_event_start_task().
 *
 * @param[in]  params Input parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_set_event_start_task(const sdc_hci_cmd_vs_set_event_start_task_t *params);

/** @brief Connection Anchor_Point Update Event Reports enable.
 *
 * For the complete API description,
 * see sdc_hci_cmd_vs_conn_anchor_point_update_event_report_enable().
 *
 * @param[in]  params Input parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int hci_vs_sdc_conn_anchor_point_update_event_report_enable(
	const sdc_hci_cmd_vs_conn_anchor_point_update_event_report_enable_t *params);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _HCI_VS_SDC_ */
