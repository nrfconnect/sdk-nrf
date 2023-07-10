/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_ESL_CLIENT_H_
#define BT_ESL_CLIENT_H_

/**
 * @file
 * @defgroup bt_eslc Electronics Shelf Label (ESL) Client API
 * @{
 * @brief Electronics Shelf Label (ESL) GATT Service Client API.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/services/ots.h>
#include <bluetooth/gatt_dm.h>

#include "esl_common.h"
#include "esl.h"

#define MNT_POINT	  "/ots_image"
#define BLE_FOLDER	  "/ble"
#define ESL_FOLDER	  "/esl"
#define TAG_BLE_ADDR_ROOT MNT_POINT BLE_FOLDER
#define TAG_ESL_ADDR_ROOT MNT_POINT ESL_FOLDER

/** @brief Handles Index enum **/
enum ESL_CLIENT_HANDLES {
	/** Handle of the ESL Address Point characteristic, as provided by
	 *  a discovery.
	 */
	HANDLE_ESL_ADDR,
	/** Handle of the AP Sync Key characteristic, as provided by
	 *  a discovery.
	 */
	HANDLE_AP_SYNC_KEY,
	/** Handle of the ESL Response Key characteristic, as provided by
	 *  a discovery.
	 */
	HANDLE_ESL_RESP_KEY,
	/** Handle of the ESL current absolute time characteristic, as provided by
	 *  a discovery.
	 */
	HANDLE_ESL_ABS_TIME,
	/** Handle of the Display Information characteristic, as provided by
	 *  a discovery.
	 */
	HANDLE_DISP_INF,
	/** Handle of the Image Information characteristic, as provided by
	 *  a discovery.
	 */
	HANDLE_IMAGE_INF,
	/** Handle of the Sensor Information characteristic, as provided by
	 *  a discovery.
	 */
	HANDLE_SENSOR_INF,
	/** Handle of the LED Information characteristic, as provided by
	 *  a discovery.
	 */
	HANDLE_LED_INF,
	/** Handle of the ESL Control Point characteristic, as provided by
	 *  a discovery.
	 */
	HANDLE_ECP,
	/** Handle of the CCC descriptor of the ESL Control characteristic,
	 *  as provided by a discovery.
	 */
	HANDLE_ECP_CCC,

	/** Handle of the PNP_ID characteristic,
	 *  as provided by a discovery.
	 */
	HANDLE_DIS_PNPID,
	/** TODO: More Device information characteristic */

	NUMBERS_OF_HANDLE
};

/** @brief Handles on the connected peer device that are needed to interact with the device.  */
struct bt_esl_client_handles {
	uint16_t handles[NUMBERS_OF_HANDLE];
};

/** @brief OTS client handles. */
struct bt_otc_handles {
	uint16_t start_handle;
	uint16_t end_handle;
	uint16_t feature_handle;
	uint16_t obj_name_handle;
	uint16_t obj_type_handle;
	uint16_t obj_size_handle;
	uint16_t obj_properties_handle;
	uint16_t obj_created_handle;
	uint16_t obj_modified_handle;
	uint16_t obj_id_handle;
	uint16_t oacp_handle;
	uint16_t olcp_handle;
};

/** @brief Sync Buffer Status enum. Preserve buffer for another Host to send sync
 * packet buffer for each group. This enum type reflect the status of each buffer.
 */
enum AP_SYNC_BUFFER_STATUS {

	/* The buffer is ready to be filled*/
	SYNC_EMPTY,

	/* The buffer is ready to push to controller */
	SYNC_READY_TO_PUSH,

	/* The buffer has been pushed to controller and wait response*/
	SYNC_PUSHED,

	/* The buffer is sent and responses has received */
	SYNC_RESP_FULL,
};

/** @brief ESL AP ACL state. */
enum {
	ESL_C_SCANNING,
	ESL_C_SCAN_PENDING,
	ESL_C_CONNECTING,
};

/** @brief ESL AP response buffer for each ESL group. */
struct bt_esl_rsp_buffer {
	struct bt_esl_key_material rsp_key;
	uint8_t data[ESL_ENCRTYPTED_DATA_MAX_LEN];
	uint8_t rsp_len;
	uint8_t rsp_slot;
};

/** @brief ESL AP command buffer for each ESL group. */
struct bt_esl_sync_buffer {
	uint8_t data[ESL_ENCRTYPTED_DATA_MAX_LEN];
	uint8_t data_len;
	enum AP_SYNC_BUFFER_STATUS status;
	struct bt_esl_rsp_buffer rsp_buffer[CONFIG_ESL_CLIENT_MAX_RESPONSE_SLOT_BUFFER];
};

/** @brief ESL AP scanned unassociated or unsynchronized ESL tag. */
struct tag_addr_node {
	sys_snode_t node;
	bt_addr_le_t addr;
	bool connected;
};

/** @brief Data to be configured to TAG with auto onboarding feature. */
struct bt_esl_tag_configure_param {
	uint16_t esl_addr;
};

/** @brief Pointers to the callback functions for service events. */
struct bt_esl_client_cb {
	/** @brief Image storage filesystem init callback.
	 *  Storage could be Flash/RAM/SDCARD
	 *  Filesystem could be littlefs/fatfs/ramdisk
	 */
	int (*ap_image_storage_init)(void);

	/** @brief Read img from storage backend to ram callback.
	 * This function is called when AP decides to send image through OTS to ESL.
	 *
	 * @param[in] img_path Full path of image to read in storage backend.
	 * @param[out] data Pointer to hold image data.
	 * @param[out] len Image data size.
	 *
	 * @retval 0 If the operation was successful.
	 * Otherwise, a (negative) error code is returned.
	 */
	int (*ap_read_img_from_storage)(uint8_t *img_path, void *data, size_t *len);

	/** @brief Read img size from storage backend to ram callback.
	 *
	 * @param[in] img_path Full path of image to read in storage backend.
	 *
	 * @retval Actual size of image in storage. Otherwise, 0 is returned.
	 */
	ssize_t (*ap_read_img_size_from_storage)(uint8_t *img_path);
};

/** @brief ESL internal GATT structure. */
struct bt_esl_client_internal {

	/** ECP Notify Subscription state. */
	atomic_t notify_state;

	/** ESL basic state. */
	atomic_t basic_state;

	/** Handles on the connected peer device that are needed
	 * to interact with the device.
	 */
	struct bt_esl_client_handles gatt_handles;

	/** GATT subscribe parameters for ESL Control Point Characteristic. */
	struct bt_gatt_subscribe_params ecp_notif_params;

	/** GATT write parameters for ESL ECP Characteristic. */
	struct bt_gatt_write_params ecp_write_params;

	/** GATT write parameters for ESL Characteristic. */
	struct bt_gatt_write_params esl_write_params;

	/** GATT write parameters for ESL Characteristic. */
	struct bt_gatt_read_params esl_read_params;

	struct bt_esl_chrc_data esl_device;
	/** ESL ECP response*/
	struct esl_ecp_resp resp;

	/** Handles on the OTS service connected peer device that are needed
	 * to interact with the device.
	 */
	struct bt_otc_handles ots_handles;

	struct bt_ots_client otc;

	uint32_t last_image_crc;
#if defined(CONFIG_BT_ESL_AP_AUTO_PAST_TAG)
	bool past_pending;
#endif /* (CONFIG_BT_ESL_AP_AUTO_PAST_TAG) */
};

/** @brief ESL Client structure. */
struct bt_esl_client {

	/** ACL state */
	atomic_t acl_state;

	/** Connection object. */
	struct bt_conn *conn[CONFIG_BT_ESL_PERIPHERAL_MAX];

	struct bt_esl_client_internal gatt[CONFIG_BT_ESL_PERIPHERAL_MAX];

	/* Currently Zephyr OTS clinet only supports 1 instance */
	struct bt_ots_client otc;

	/** Data holding le_addr of scanned unassociated ESL tags */
	sys_slist_t scanned_tag_addr_list;

	/** Scan type, scan only one device or continuously. */
	bool scan_one_shot;

	/** Data holding AP sync key */
	struct bt_esl_key_material esl_ap_key;

	/** Data holding Randomizer */
	uint8_t esl_randomizer[EAD_RANDOMIZER_LEN];

	/** Data Holding Nonce, Randomizer + IV*/
	uint8_t esl_nonce[EAD_RANDOMIZER_LEN + EAD_IV_LEN];

	/** Data holding Current Absolute time */
	uint32_t esl_abs_time;

	int16_t current_esl_count;
	uint16_t scanned_count;
	int16_t pending_unassociated_tag;

	/** Application callbacks. */
	struct bt_esl_client_cb cb;

	/** Buffer from UART to controller PAwR buffer */
	struct bt_esl_sync_buffer sync_buf[CONFIG_ESL_CLIENT_MAX_GROUP];

	uint8_t img_obj_buf[CONFIG_ESL_IMAGE_FILE_SIZE];

	uint16_t esl_addr_start;
};

/** @brief ESL Client initialization structure. */
struct bt_esl_client_init_param {
	/** Callbacks provided by the user. */
	struct bt_esl_client_cb cb;
};

/** @brief Initialize the ESL Client module.
 *
 * This function initializes the ESL Client module with callbacks provided by
 * the user.
 *
 * @param[in] esl_c    ESL Client instance.
 * @param[in] init_param ESL Client initialization parameters.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a negative error code is returned.
 */
int bt_esl_client_init(struct bt_esl_client *esl_c,
		       const struct bt_esl_client_init_param *init_param);

/** @brief Assign handles to the ESL Client instance.
 *
 * This function should be called when a link with a peer has been established
 * to associate the link to this instance of the module. This makes it
 * possible to handle several links and associate each link to a particular
 * instance of this module. The GATT attribute handles are provided by the
 * GATT DB discovery module.
 *
 * @param[in] dm Discovery object.
 * @param[in] esl_c ESL Client instance.
 * @param[in] conn_idx Index of ACL connection to discovery handles.
 *
 * @retval 0 If the operation was successful.
 * @retval (-ENOTSUP) Special error code used when UUID
 *         of the service does not match the expected UUID.
 * @retval Otherwise, a negative error code is returned.
 */
int bt_esl_handles_assign(struct bt_gatt_dm *dm, struct bt_esl_client *esl_c, uint8_t conn_idx);

/** @brief Request the peer to start sending notifications for the ECP Characteristic.
 *
 * This function enables notifications for the ESL Control Point Characteristic at the peer
 * by writing to the CCC descriptor of the ESL Control Point Characteristic Characteristic.
 *
 * @param[in] esl_c ESL Client instance.
 * @param[in] conn_idx Index of ACL connection to subscribe.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a negative error code is returned.
 */
int bt_esl_c_subscribe_receive(struct bt_esl_client *esl_c, uint8_t conn_idx);

/** @brief 6.1.1 Configure or reconfigure an ESL procedure
 *
 * This function configures connected ESL tag mandatory Characteristics.
 * e.g.:esl address / key materials / image data.
 *
 * @param[in] conn_idx Index of ACL connection to configure ESL.
 * @param[in] esl_addr ESL address to be configured.
 *
 */
void bt_esl_configure(uint8_t conn_idx, uint16_t esl_addr);

/** @brief 3.9.2.1 Ping Operation
 *
 * This function is called when AP wants to Ping ESL TAG with ACL connection and ECP.
 *
 * @param[in] esl_c ESL Client instance.
 * @param[in] conn_idx Index of ACL connection to ping.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a negative error code is returned.
 */
int bt_esl_c_ping_op(struct bt_esl_client *esl_c, uint8_t conn_idx);

/** @brief 3.9.2.2 Unassociated to ESL tag
 *
 * Ask ESL TAG to clear bonded data, provisioned data
 * This function is called when AP wants to Unassociated ESL TAG with ACL connection and ECP.
 *
 * @param[in] esl_c ESL Client instance.
 * @param[in] conn_idx Index of ACL connection to unassociate.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a negative error code is returned.
 */
int bt_c_esl_unassociate(struct bt_esl_client *esl_c, uint8_t conn_idx);

/** @brief 3.9.2.3 Service Reset
 *
 * Clear service needed bit on ESL TAG.
 * This function is called when AP wants to reset service ESL TAG with ACL connection and ECP.
 *
 * @param[in] esl_c ESL Client instance.
 * @param[in] conn_idx Index of ACL connection to service reset.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a negative error code is returned.
 **/
int bt_esl_c_service_reset(struct bt_esl_client *esl_c, uint8_t conn_idx);

/** @brief 3.9.2.4 Factory Reset
 *
 * Ask ESL TAG to clear bonded data, provisioned data
 * This function is called when AP wants to Factory reset ESL TAG with ACL connection and ECP.
 *
 * @param[in] esl_c ESL Client instance.
 * @param[in] conn_idx Index of ACL connection to factory reset.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a negative error code is returned.
 */
int bt_esl_c_factory_reset(struct bt_esl_client *esl_c, uint8_t conn_idx);

/** @brief 3.9.2.5 Update Complete
 *
 * Ask ESL TAG to disconnect and sync PAWR of AP.
 * This function is called when AP has completed configuring or updating.
 * Wants to Factory reset ESL TAG with ACL connection and ECP.
 *
 * @param[in] esl_c ESL Client instance.
 * @param[in] conn_idx Index of ACL connection to complete update.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a negative error code is returned.
 */
int bt_esl_c_update_complete(struct bt_esl_client *esl_c, uint8_t conn_idx);

/** @brief 3.10.2.6 Read Sensor Data
 *
 * Ask ESL TAG to read sensor.
 * This function is called when AP wants to read sensor data ESL TAG with ACL connection and ECP.
 *
 * @param[in] esl_c ESL Client instance.
 * @param[in] conn_idx Index of ACL connection to read sensor.
 * @param[in] sensor_idx Index of sensor to be read.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a negative error code is returned.
 */
int bt_esl_c_read_sensor(struct bt_esl_client *esl_c, uint8_t conn_idx, uint8_t sensor_idx);

/** @brief 3.10.2.7 Refresh Display
 *
 * Ask ESL TAG to refresh display device.
 * This function is called when AP wants to refresh display of ESL TAG with ACL connection and ECP.
 *
 * @param[in] esl_c ESL Client instance.
 * @param[in] conn_idx Index of ACL connection to refresh display.
 * @param[in] disp_idx Display index to refresh.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a negative error code is returned.
 */
int bt_esl_c_refresh_display(struct bt_esl_client *esl_c, uint8_t conn_idx, uint8_t disp_idx);

/** @brief 3.9.2.8/3.9.2.9 Control Displayed Image in Synchronized and Updating state
 *
 * Ask ESL TAG to change image on display device.
 * This function is called when AP wants to change image on display of ESL TAG with ACL connection
 * and ECP.
 *
 * @param[in] esl_c ESL Client instance.
 * @param[in] conn_idx Index of ACL connection to refresh display.
 * @param[in] disp_idx Display index to change.
 * @param[in] img_idx Image index to be displyed.
 * @param[in] timed Change image immediately or not.
 * @param[in] abs_time Abolute time when display need to be change. Only valid if timed is True
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a negative error code is returned.
 */
int bt_esl_c_display_control(struct bt_esl_client *esl_c, uint8_t conn_idx, uint8_t disp_idx,
			     uint8_t img_idx, bool timed, uint32_t abs_time);

/** @brief 3.9.2.10/3.9.2.11 Control LED(s) in Synchronized and Updating state
 *
 * Ask ESL TAG to change LED flashing pattern.
 * This function is called when AP wants to change LED flashing pattern on ESL TAG with ACL
 * connection and ECP.
 *
 * @param[in] esl_c ESL Client instance.
 * @param[in] conn_idx Index of ACL connection to refresh display.
 * @param[in] led_idx LED index to change.
 * @param[in] led LED flashing pattern structure.
 * @param[in] timed Change image immediately or not.
 * @param[in] abs_time Abolute time when display need to be change. Only valid if timed is True
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a negative error code is returned.
 *
 */
int bt_esl_c_led_control(struct bt_esl_client *esl_c, uint8_t conn_idx, uint8_t led_idx,
			 struct led_obj *led, bool timed, uint32_t abs_time);

/** @brief Clear bonding data
 *
 * Ask AP to clear bonding data for current ACL connection
 *
 * @param[in] conn_idx Index of ACL connection to remove bond. -1 means remove all of bonding data.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a negative error code is returned.
 */
int bt_c_esl_unbond(int conn_idx);

/** @brief Start/stop to scan ESL service.
 *
 * @param[in] onoff Start/stop to scan ESL service advertising.
 * @param[in] oneshot Scan only one device or continuously.
 *
 */
void esl_c_scan(bool onoff, bool oneshot);

/** @brief Get esl_client object */
struct bt_esl_client *esl_c_get_esl_c_obj(void);

/** @brief Load predefined command buffer to the specified ESL group
 *
 * @param[in] sync_pkt_type Predefined ESL TLV set.
 * @param[in] group_id Index of group to load TLVs.
 *
 */
void esl_dummy_ap_ad_data(uint8_t sync_pkt_type, uint8_t group_id);

#if defined(CONFIG_BT_ESL_TAG_STORAGE)
/** @brief Remove tag information from storage backend.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a negative error code is returned.
 *
 */
int remove_all_tags_in_storage(void);

/** @brief Load tag's response from storage backend.
 *
 * @param[in] group_id Which group of ESL to load.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a negative error code is returned.
 *
 */
int load_all_tags_in_storage(uint8_t group_id);
#endif /* CONFIG_BT_ESL_TAG_STORAGE */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_ESL_CLIENT_H_ */
