/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BT_MGMT_H_
#define _BT_MGMT_H_

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/audio.h>

#define LE_AUDIO_EXTENDED_ADV_NAME                                                                 \
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_USE_NAME,                            \
			CONFIG_BLE_ACL_EXT_ADV_INT_MIN, CONFIG_BLE_ACL_EXT_ADV_INT_MAX, NULL)

#define LE_AUDIO_EXTENDED_ADV_CONN_NAME                                                            \
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_CONNECTABLE |                        \
				BT_LE_ADV_OPT_USE_NAME,                                            \
			CONFIG_BLE_ACL_EXT_ADV_INT_MIN, CONFIG_BLE_ACL_EXT_ADV_INT_MAX, NULL)

#define LE_AUDIO_EXTENDED_ADV_CONN_NAME_FILTER                                                     \
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_CONNECTABLE |                        \
				BT_LE_ADV_OPT_USE_NAME | BT_LE_ADV_OPT_FILTER_CONN,                \
			CONFIG_BLE_ACL_EXT_ADV_INT_MIN, CONFIG_BLE_ACL_EXT_ADV_INT_MAX, NULL)

#define LE_AUDIO_PERIODIC_ADV                                                                      \
	BT_LE_PER_ADV_PARAM(CONFIG_BLE_ACL_PER_ADV_INT_MIN, CONFIG_BLE_ACL_PER_ADV_INT_MAX,        \
			    BT_LE_PER_ADV_OPT_NONE)

#define BT_LE_ADV_FAST_CONN                                                                        \
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE, BT_GAP_ADV_FAST_INT_MIN_1,                      \
			BT_GAP_ADV_FAST_INT_MAX_1, NULL)

/* Broadcast name can be max 32 bytes long, so this will be the limit for both.
 * Add one for '\0' at the end.
 */
#define BLE_SEARCH_NAME_MAX_LEN 33

#if (CONFIG_SCAN_MODE_ACTIVE)
#define NRF5340_AUDIO_GATEWAY_SCAN_TYPE	  BT_LE_SCAN_TYPE_ACTIVE
#define NRF5340_AUDIO_GATEWAY_SCAN_PARAMS BT_LE_SCAN_ACTIVE
#elif (CONFIG_SCAN_MODE_PASSIVE)
#define NRF5340_AUDIO_GATEWAY_SCAN_TYPE	  BT_LE_SCAN_TYPE_PASSIVE
#define NRF5340_AUDIO_GATEWAY_SCAN_PARAMS BT_LE_SCAN_PASSIVE
#else
#error "Select either CONFIG_SCAN_MODE_ACTIVE or CONFIG_SCAN_MODE_PASSIVE"
#endif

enum bt_mgmt_scan_type {
	BT_MGMT_SCAN_TYPE_CONN = 1,
	BT_MGMT_SCAN_TYPE_BROADCAST = 2,
};

#define BRDCAST_ID_NOT_USED (BT_AUDIO_BROADCAST_ID_MAX + 1)

/**
 * @brief	Get the numbers of connected members of a given 'Set Identity Resolving Key' (SIRK).
 *		The SIRK shall be set through bt_mgmt_scan_sirk_set() before calling this function.
 *
 * @param[out]	num_filled	The number of connected set members.
 */
void bt_mgmt_set_size_filled_get(uint8_t *num_filled);

/**
 * @brief	Set 'Set Identity Resolving Key' (SIRK).
 *		Used for searching for other member of the same set.
 *
 * @param[in]	sirk	Pointer to the Set Identity Resolving Key to store.
 */
void bt_mgmt_scan_sirk_set(uint8_t const *const sirk);

/**
 * @brief	Load advertising data into an advertising buffer.
 *
 * @param[out]     adv_buf         Pointer to the advertising  buffer to load.
 * @param[in,out]  index           Next free index in the advertising buffer.
 * @param[in]      adv_buf_vacant  Number of free advertising buffers.
 * @param[in]      data_len        Length of the data.
 * @param[in]      type            Type of the data.
 * @param[in]      data            Data to store in the buffer, can be a pointer or value.
 *
 * @return	0 if success, error otherwise.
 */
int bt_mgmt_adv_buffer_put(struct bt_data *const adv_buf, uint32_t *index, size_t adv_buf_vacant,
			   size_t data_len, uint8_t type, void *data);

/**
 * @brief	Start scanning for advertisements.
 *
 * @param[in]	scan_intvl	Scan interval in units of 0.625ms.
 *				Valid range: 0x4 - 0xFFFF; can be 0.
 * @param[in]	scan_win	Scan window in units of 0.625ms.
 *				Valid range: 0x4 - 0xFFFF; can be 0.
 * @param[in]	type		Type to scan for: ACL connection or broadcaster.
 * @param[in]	name		Name to search for. Depending on @p type of search,
 *				device name or broadcast name. Can be max
 *				BLE_SEARCH_NAME_MAX_LEN long; everything beyond that value
 *				will be cropped. Can be NULL. Shall be '\0' terminated.
 * @param[in]	brdcast_id	Broadcast ID to search for. Only valid if @p type is
 *				BT_MGMT_SCAN_TYPE_BROADCAST. If both @p name and @p brdcast_id are
 *				provided, then brdcast_id will be used.
 *				Set to BRDCAST_ID_NOT_USED if not in use.
 *
 * @note	To restart scanning, call this function with all 0s and NULL, except for @p type.
 *		The same scanning parameters as when bt_mgmt_scan_start was last called will then
 *		be used.
 *
 * @return	0 if success, error otherwise.
 */
int bt_mgmt_scan_start(uint16_t scan_intvl, uint16_t scan_win, enum bt_mgmt_scan_type type,
		       char const *const name, uint32_t brdcast_id);

/**
 * @brief	Add manufacturer ID UUID to the advertisement packet.
 *
 * @param[out]	uuid_buf	Buffer being populated with UUIDs.
 * @param[in]	company_id	16 bit UUID specific to the company.
 *
 * @return	0 for success, error otherwise.
 */
int bt_mgmt_manufacturer_uuid_populate(struct net_buf_simple *uuid_buf, uint16_t company_id);

/**
 * @brief	Create and start advertising for ACL connection.
 *
 * @param[in]	ext_adv_index	Index of the advertising set to start.
 * @param[in]	ext_adv		The data to be put in the extended advertisement.
 * @param[in]	ext_adv_size	Size of @p ext_adv.
 * @param[in]	per_adv		The data for the periodic advertisement; can be NULL.
 * @param[in]	per_adv_size	Size of @p per_adv.
 * @param[in]	connectable	Specify if advertisement should be connectable or not.
 *
 * @note	To restart advertising, call this function with all 0s and NULL, except for
 *		connectable. The same advertising parameters as when bt_mgmt_adv_start was last
 *		called will then be used.
 *
 * @return	0 if success, error otherwise.
 */
int bt_mgmt_adv_start(uint8_t ext_adv_index, const struct bt_data *ext_adv, size_t ext_adv_size,
		      const struct bt_data *per_adv, size_t per_adv_size, bool connectable);

/**
 * @brief	Get the number of active connections.
 *
 * @param[out]	num_conn	The number of active connections.
 */
void bt_mgmt_num_conn_get(uint8_t *num_conn);

/**
 * @brief	Clear all bonded devices.
 *
 * @return	0 if success, error otherwise.
 */
int bt_mgmt_bonding_clear(void);

/**
 * @brief	Delete a periodic advertisement sync.
 *
 * @param[in]	pa_sync	Pointer to the periodic advertisement sync to delete.
 *
 * @return	0 if success, error otherwise.
 */
int bt_mgmt_pa_sync_delete(struct bt_le_per_adv_sync *pa_sync);

/**
 * @brief	Disconnect from a remote device or cancel the pending connection.
 *
 * @param[in]	conn	Connection to disconnect.
 * @param[in]	reason	Reason code for the disconnection, as specified in
 *			HCI Error Codes, BT Core Spec v5.4 [Vol 1, Part F].
 *
 * @return	0 if success, error otherwise.
 */
int bt_mgmt_conn_disconnect(struct bt_conn *conn, uint8_t reason);

/**
 * @brief	Initialize the Bluetooth management module.
 *
 * @return	0 if success, error otherwise.
 */
int bt_mgmt_init(void);

#endif /* _BT_MGMT_H_ */
