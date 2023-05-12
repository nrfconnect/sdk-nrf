/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BT_MGMT_H_
#define _BT_MGMT_H_

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>

#define LE_AUDIO_EXTENDED_ADV_NAME                                                                 \
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_USE_NAME,                            \
			CONFIG_BLE_ACL_EXT_ADV_INT_MIN, CONFIG_BLE_ACL_EXT_ADV_INT_MAX, NULL)

#define LE_AUDIO_EXTENDED_ADV_CONN_NAME                                                            \
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_CONNECTABLE |                        \
				BT_LE_ADV_OPT_USE_NAME,                                            \
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
 *				will be cropped. Can be NULL.
 *
 * @note	To restart scanning, call this function with all 0s and NULL, except for @p type.
 *		The same scanning parameters as when bt_mgmt_scan_start was last called will then
 *		be used.
 *
 * @return	0 if success, error otherwise.
 */
int bt_mgmt_scan_start(uint16_t scan_intvl, uint16_t scan_win, enum bt_mgmt_scan_type type,
		       char const *const name);
/**
 * @brief	Create and start advertising for ACL connection.
 *
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
int bt_mgmt_adv_start(const struct bt_data *ext_adv, size_t ext_adv_size,
		      const struct bt_data *per_adv, size_t per_adv_size, bool connectable);

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
 */
void bt_mgmt_conn_disconnect(struct bt_conn *conn, uint8_t reason);

/**
 * @brief	Initialize the Bluetooth management module.
 *
 * @return	0 if success, error otherwise.
 */
int bt_mgmt_init(void);

#endif /* _BT_MGMT_H_ */
