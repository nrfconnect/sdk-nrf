/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_ESL_CLIENT_INTERNAL_H_
#define BT_ESL_CLIENT_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @file
 * @brief ESL service client internal file
 */

/** @brief Remove pair data after receiving response of OP unassociate.
 *
 * This function should be called after AP received unassociated ECP response OP from Tag.
 *
 * The description below is extracted from ESLS V1.0, Section 3.9.2.2.
 * The Unassociate from AP command allows an ESL to be disassociated from the AP.
 * When the Unassociate from AP command is received, and the response has been sent, the ESL shall
 * remove all bonding information with the AP, delete the value of the AP Sync Key Material in
 * internal storage, the ESL ID, and delete all stored commands.
 *
 * Which means AP should not remove pair data till received ECP response.
 *
 * @param[in] conn_idx Connection index.
 *
 * @retval 0 If the pair data has been removed successful.
 * @retval (-EINVAL) If the Pair data removing is failed.
 */
int bt_c_esl_post_unassociate(uint8_t conn_idx);

/** @brief Start Periodic Advertising with Responses to send ESL AP command.
 *
 * This function should be called automatically after AP boot-up.
 * Could be called by shell for debugging purpose.
 *
 * @retval 0 If starts PAwR successful.
 * @retval Otherwise, a negative error code is returned.
 */
int esl_start_pawr(void);

/** @brief Stop Periodic Advertising with Responses to send ESL AP command.
 *
 * This function could be called by shell for debugging purpose.
 *
 * @retval 0 If stops PAwR successful.
 * @retval Otherwise, a negative error code is returned.
 */
int esl_stop_pawr(void);

/** @brief Change AP current absolute time locally(AP side).
 *
 * This function could be called by shell when ESL backend wants to change current time of AP.
 *
 * @param[in] new_abs_time is a 32-bit reference time integer value (uint32)
 * with a resolution of 1 millisecond (ms).
 */
void esl_c_abs_timer_update(uint32_t new_abs_time);

/** @brief Change current absolute time of AP to connected Tag.
 *
 * This function should be called when AP wants to configure its absolute time to connected Tag.
 *
 * @param[in] conn_idx Connection index.
 */
void esl_c_tag_abs_timer_update(uint8_t conn_idx);

/** @brief Write AP key material to connected Tag.
 *
 * This function should be called when AP wants to configure AP key material to connected Tag.
 *
 * @param[in] conn_idx Connection index.
 */
void esl_c_ap_key_update(uint8_t conn_idx);

/** @brief Write Response key material to connected Tag.
 *
 * This function should be called when AP wants to configure response key material to connected Tag.
 * The response key material will be generate as random number, or read from Tag database inside AP.
 *
 * @param[in] conn_idx Connection index.
 */
void esl_c_rsp_key_update(uint8_t conn_idx);

/** @brief Add BLE address of ESL Tag to scanned list.
 *
 * This function should be called when new advertising ESL Tag scanned.
 *
 * @param[in] addr BLE Address of ESL Tag.
 *
 * @retval 1 If the operation was successful.
 * @retval 0 If the tag is in scanned list.
 * @retval (-ENOBUFS) Special error code used when there is no memory for list node.
 * @retval Otherwise, a negative error code is returned.
 */
int esl_c_scanned_tag_add(const bt_addr_le_t *addr);

/** @brief Print scanned Tag list.
 *
 * This function is called periodically to show list of advertising Tags.
 * This function could be called by shell when ESL backend wants to know advertising tag list.
 *
 * @param[in] dump_connected Print connected Tags or not.
 */
void esl_c_list_scanned(bool dump_connected);

/** @brief Print connected Tag list.
 *
 * This function could be called by shell when ESL backend wants to know connected tag list.
 *
 */
void esl_c_list_connection(void);

/** @brief Remove ESL from scanned list by BLE address.
 *
 * This function should be called when AP associates a ESL Tag successful and ESL Tag is synced.
 *
 * @param[in] peer_addr BLE Address of ESL Tag.
 *
 * @retval 0 If remove successful.
 * @retval Otherwise, a negative error code is returned.
 */
int esl_c_scanned_tag_remove_with_ble_addr(const bt_addr_le_t *peer_addr);

/** @brief Remove ESL from scanned list by scanned list index.
 *
 * This function removes ESL Tag in scanned list.
 *
 * @param[in] tag_idx Index of ESL Tag in scanned list. -1 = remove all
 *
 * @retval 0 If the operation was successful.
 * @retval (-ENOBUFS) Special error code used when there is no memory for list node.
 * @retval Otherwise, a negative error code is returned.
 */
int esl_c_scanned_tag_remove(int tag_idx);

/** @brief Get tag_addr_node of ESL Tag.
 *
 * This function should be called when AP want to connect scanned ESL tag.
 * tag_addr_node contains BLE Address and conneted status information.
 *
 * @param[in] tag_idx Index of ESL Tag in scanned list. -1 = Get Tag node status by known BLE
 * Address.
 * @param[in] peer_addr BLE Address of ESL Tag.
 *
 * @retval tag_addr_node of the ESL Tag.
 * @retval NULL, if tag_addr_node is not found.
 */
struct tag_addr_node *esl_c_get_tag_addr(int tag_idx, const bt_addr_le_t *peer_addr);

/** @brief Connect to an ESL Tag.
 *
 * This function should be called when AP wants to connect ESL tag.
 * AP could connect ESL Tag in conventional advertising / scanning or PAwR way.
 *
 * @param[in] peer_addr BT Address of ESL tag.
 * @param[in] esl_addr ESL addr of ESL tag. If broadcast ESL addr, use conventional way to connect.
 * If not, use PAwR way to connect.
 *
 * @retval 0 If the operation was successful.
 * @retval Otherwise, a negative error code is returned.
 */
int esl_c_connect(const bt_addr_le_t *peer_addr, uint16_t esl_addr);

/** @brief Start service discovery.
 *
 * This function should be called when AP wants to discovery service of connected ESL Tag.
 *
 * @param[in] conn_idx Connection index.
 */
void esl_discovery_start(uint8_t conn_idx);

/** @brief Print paired Tags.
 *
 * This function should be called when AP wants to know all bonded devices
 *
 * @param[in] user_data Unused parameter.
 */
void bt_esl_c_bond_dump(void *user_data);

/** @brief Get ESL address for specified connection.
 *
 * This function should be called when AP wants to ESL address of connected ESL Tag.
 *
 * @param[in] conn_idx Connection index.
 *
 * @retval The ESL address of Tag.
 * @retval 0x8000 if there is no connection of index.
 */
uint16_t conn_idx_to_esl_addr(uint8_t conn_idx);

/** @brief Get connection index for a specified ESL address.
 *
 * This function is used to retrieve the connection index associated with a specific ESL address.
 * The connection index can then be used in other functions that require it as a parameter.
 * It is intended to be used when the AP (Access Point) wants to understand the connection details
 * associated with a given ESL tag.
 *
 * @param[in] esl_addr ESL address of the tag for which the connection index is requested.
 *
 * @return The connection index associated with the given ESL address.
 * @retval A negative error code is returned if there is no connection associated with the provided
 * ESL address.
 */
int esl_addr_to_conn_idx(uint16_t esl_addr);

/** @brief Get connection index for a specified BLE address.
 *
 * This function retrieves the connection index associated with a specific BLE (Bluetooth Low
 * Energy) address. The connection index can then be used in other functions that require it as a
 * parameter. It is intended to be used when the application wants to understand the connection
 * details associated with a given BLE device.
 *
 * @param[in] addr BLE address of the device for which the connection index is requested.
 *
 * @return The connection index associated with the given BLE address.
 * @retval A negative error code is returned if there is no connection associated with the provided
 * BLE address.
 */
int ble_addr_to_conn_idx(const bt_addr_le_t *addr);

/** @brief Write ESL address to a specific connection.
 *
 * This function is used to write an ESL address to a connected Tag specified
 * by the connection index. This could be used when the Access Point (AP) needs to update the ESL
 * address of a connected Tag.
 *
 * @param[in] conn_idx The connection index to which the ESL address should be written.
 * @param[in] esl_addr The ESL address that needs to be written to the connection.
 *
 * @return 0 if the operation was successful.
 * @retval A negative error code is returned if there was an error while writing the ESL address to
 * the connection.
 */
int bt_esl_c_write_esl_addr(uint8_t conn_idx, uint16_t esl_addr);

/** @brief Read ESL information characteristics from a specific connection.
 *
 * This function is used to read the information characteristics of an ESL Tag from
 * a specific connection. It could be used when the Access Point (AP) needs to fetch the information
 * characteristics of a connected device.
 * This function calls bt_gatt_read multiple time to read each information characteristics and put
 * in esl_c object
 *
 * @param[in] esl_c A pointer to the instance of the ESL client structure.
 * @param[in] conn_idx The connection index from which the characteristics should be read.
 *
 * @return 0 if the operation was successful.
 * @retval A negative error code is returned if there was an error while reading the characteristics
 * from the connection.
 */
int bt_esl_c_read_chrc(struct bt_esl_client *esl_c, uint8_t conn_idx);

/** @brief Read ESL information characteristics from a specific connection.
 *
 * This function is used to read the information characteristics from an ESL Tag
 * on a specific connection. This function uses the ATT_READ_MULTIPLE_VARIABLE_REQ request to fetch
 * multiple characteristics at once. It could be used when the Access Point (AP) needs to fetch
 * multiple characteristics of a connected ESL Tag simultaneously.
 *
 * @param[in] esl_c A pointer to the instance of the ESL client structure.
 * @param[in] conn_idx The connection index from which the characteristics should be read.
 *
 * @return 0 if the operation was successful.
 * @retval A negative error code is returned if there was an error while reading the characteristics
 * from the connection.
 */
int bt_esl_c_read_chrc_multi(struct bt_esl_client *esl_c, uint8_t conn_idx);

/** @brief Reset the Access Point (AP).
 *
 * This function is used to reset the Access Point (AP). It may be called when the AP needs to be
 * reset to its initial state for debugging purpose.
 * This function will clear all of bond data and ESL Tag database.
 *
 * @return 0 if the operation was successful.
 * @retval A negative error code is returned if there was an error while resetting the AP.
 */
int bt_esl_c_reset_ap(void);

/** @brief Disconnect a peer.
 *
 * This function is used to disconnect a peer connection. It might be called when the Access Point
 * (AP) needs to manually disconnect a connection with a specific peer with reason
 * BT_HCI_ERR_REMOTE_USER_TERM_CONN.
 *
 * @param[in] conn A pointer to the instance of the bt_conn structure representing the peer
 * connection to be disconnected.
 */
void peer_disconnect(struct bt_conn *conn);

/** @brief Dump the GATT flag status.
 *
 * This function prints the current status of the internal GATT flags. This can be used for
 * debugging or informational purposes to understand the state of the GATT flags.
 */
void esl_c_gatt_flag_dump(void);

/** @brief Dump the characteristic data of an ESL Tag.
 *
 * This function is used to print the characteristic data fetched by either
 * \ref bt_esl_c_read_chrc "bt_esl_c_read_chrc()" or \ref bt_esl_c_read_chrc_multi
 * "bt_esl_c_read_chrc_multi()" from an ESL Tag. This can be useful for debugging or informational
 * purposes.
 *
 * @param[in] chrc A pointer to the characteristic data structure.
 */
void esl_c_dump_chrc(const struct bt_esl_chrc_data *chrc);

/** @brief Set or unset a GATT flag.
 *
 * This function is used to set or unset a specific internal GATT
 * flag, useful for debugging or managing the state of the GATT flags.
 *
 * @param[in] flag_idx The index of the flag to be set or unset.
 * @param[in] set_unset The operation to be performed: set (usually non-zero) or
 * unset (usually zero).
 */
void esl_c_gatt_flag_set(uint8_t flag_idx, uint8_t set_unset);

/** @brief Get a free connection index.
 *
 * This function is used to obtain a free connection index. It can be used when a new
 * ESL Tag needs to be connected and a new index is required.
 *
 * @return The index of the free connection.
 * @retval -1 is returned if no free index is available.
 */
int esl_c_get_free_conn_idx(void);

/** @brief Perform the Transition to Synchronized State procedure.
 *
 * According to the ESL profile paragraph 6.1.5, this function is used to
 * perform the Transition to Synchronized State procedure on an ESL Tag.
 * This might be required when the ESL Tag needs to transition to a
 * synchronized state from another state.
 *
 * @param[in] conn_idx The connection index on which to perform the procedure.
 *
 * @return 0 if the operation was successful.
 * @retval A negative error code is returned if there was an error during the procedure.
 */
int esl_c_past(uint8_t conn_idx);

/** @brief Report the synchronization buffer status of a specific ESL group(subevent).
 *
 * This function is used to print or report the status of the synchronization buffer
 * for a specific ESL group. This can be useful for debugging or informational purposes.
 *
 * @param[in] group_id The ID of the group for which to report the buffer status.
 */
void esl_c_sync_buf_status(int8_t group_id);

/** @brief Push ESL command into the synchronization buffer of a specific ESL group.
 *
 * This function is used to push ESL command into the synchronization buffer of a specific
 * group. This can be used when new OP command needs to be synchronized among ESL Tags
 * within a group.
 *
 * @param[in] data The ESL command payload to be pushed into the buffer.
 * @param[in] data_len The length of the data.
 * @param[in] group_id The ID of the ESL group to which the data should be pushed.
 *
 * @return 0 if the operation was successful.
 * @retval A negative error code is returned if there was an error while pushing the data.
 */
int esl_c_push_sync_buf(uint8_t *data, uint8_t data_len, uint8_t group_id);

/** @brief Push response key into the response buffer of a specific group.
 *
 * Response keys are required to decrypt EAD(encrypted advertising data) on-the-fly.
 *
 * This function is used to push a response key into the response buffer of a
 * specific group. This can be used when response keys need to be managed
 * across a group of ESL Tags.
 *
 * @param[in] rsp_buffer The response buffer where the key should be pushed.
 * @param[in] group_id The ID of the group for which the key should be pushed.
 *
 * @return 0 if the operation was successful.
 * @retval A negative error code is returned if there was an error while pushing the key.
 */
int esl_c_push_rsp_key(
	struct bt_esl_rsp_buffer rsp_buffer[CONFIG_ESL_CLIENT_MAX_RESPONSE_SLOT_BUFFER],
	uint8_t group_id);

/** @brief Dump the synchronization buffer of a specific group.
 *
 * This function is used to print the synchronization buffer of a specific group.
 * This can be useful for debugging or informational purposes.
 *
 * @param[in] group_id The ID of the group for which to dump the buffer.
 */
void esl_c_dump_sync_buf(uint8_t group_id);

/** @brief Dump the response buffer of a specific group.
 *
 * This function is used to print the response buffer of a specific group.
 * This can be useful for debugging or informational purposes.
 *
 * @param[in] group_id The ID of the group for which to dump the buffer.
 */
void esl_c_dump_resp_buf(uint8_t group_id);

/** @brief Toggle automatic AP (Access Point) mode.
 *
 * This function is used to turn auto onboarding AP mode on or off. This can be used
 * when there is a need to control the connectivity of ESL Tags.
 *
 * @param[in] onoff The state to set for automatic AP mode (true for on, false for off).
 */
void esl_c_auto_ap_toggle(bool onoff);

/** @brief Write Image to TAG.
 *
 * This function should be called when AP want to transfer Image to connected ESL tag.
 *
 * @param[in] conn_idx		BT connection idx.
 * @param[in] tag_img_idx	Image index(Object index) of Tag is going to be written.
 * @param[in] img_idx		Image index stored in AP storage. Equals to esl_image_<img_idx>.
 *
 */
void esl_c_obj_write(uint16_t conn_idx, uint8_t tag_img_idx, uint16_t img_idx);

/** @brief Write Image to TAG.
 *
 * This function should be called when AP want to transfer Image to connected ESL tag.
 *
 * @param[in] conn_idx		BT connection idx.
 * @param[in] tag_img_idx	Image index(Object index) of Tag is going to be written.
 * @param[in] img_name		Image file name stored in AP storage.
 *
 * @retval 0 If the operation was successful.
 * @retval (-ENOBUFS) Special error code used when there is no memory for list node.
 * @retval Otherwise, a negative error code is returned.
 */
int esl_c_obj_write_by_name(uint16_t conn_idx, uint8_t tag_img_idx, uint8_t *img_name);

uint32_t esl_c_get_abs_time(void);
#ifdef __cplusplus
}
#endif

#endif /* BT_ESL_CLIENT_INTERNAL_H_ */
