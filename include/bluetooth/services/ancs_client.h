/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_ANCS_CLIENT_H_
#define BT_ANCS_CLIENT_H_

/** @file
 *
 * @defgroup bt_ancs_client Apple Notification Service Client
 * @{
 *
 * @brief Apple Notification Center Service Client module.
 */

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>
#include <bluetooth/gatt_dm.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Apple Notification Center Service UUID. */
#define BT_UUID_ANCS_VAL                                                      \
	BT_UUID_128_ENCODE(0x7905f431, 0xb5ce, 0x4e99, 0xa40f, 0x4b1e122d00d0)

/** @brief Notification Source Characteristic UUID. */
#define BT_UUID_ANCS_NOTIFICATION_SOURCE_VAL                                     \
	BT_UUID_128_ENCODE(0x9fbf120d, 0x6301, 0x42d9, 0x8c58, 0x25e699a21dbd)

/** @brief Control Point Characteristic UUID. */
#define BT_UUID_ANCS_CONTROL_POINT_VAL                                           \
	BT_UUID_128_ENCODE(0x69d1d8f3, 0x45e1, 0x49a8, 0x9821, 0x9bbdfdaad9d9)

/** @brief Data Source Characteristic UUID. */
#define BT_UUID_ANCS_DATA_SOURCE_VAL                                             \
	BT_UUID_128_ENCODE(0x22eac6e9, 0x24d6, 0x4bb5, 0xbe44, 0xb36ace7c7bfb)

#define BT_UUID_ANCS BT_UUID_DECLARE_128(BT_UUID_ANCS_VAL)
#define BT_UUID_ANCS_NOTIFICATION_SOURCE                                       \
	BT_UUID_DECLARE_128(BT_UUID_ANCS_NOTIFICATION_SOURCE_VAL)
#define BT_UUID_ANCS_CONTROL_POINT                                             \
	BT_UUID_DECLARE_128(BT_UUID_ANCS_CONTROL_POINT_VAL)
#define BT_UUID_ANCS_DATA_SOURCE BT_UUID_DECLARE_128(BT_UUID_ANCS_DATA_SOURCE_VAL)

/** Maximum data length of an iOS notification attribute. */
#define BT_ANCS_ATTR_DATA_MAX 32
/** Number of iOS notification categories: Other, Incoming Call, Missed Call,
 *  Voice Mail, Social, Schedule, Email, News, Health and Fitness, Business and
 *  Finance, Location, Entertainment.
 */
#define BT_ANCS_CATEGORY_ID_COUNT 12
/** Number of iOS notification attributes: AppIdentifier, Title, Subtitle,
 *  Message, MessageSize, Date, PositiveActionLabel, NegativeActionLabel.
 */
#define BT_ANCS_NOTIF_ATTR_COUNT 8
/** Number of iOS application attributes: DisplayName. */
#define BT_ANCS_APP_ATTR_COUNT 1
/** Number of iOS notification events: Added, Modified, Removed. */
#define BT_ANCS_EVT_ID_COUNT 3

/** @brief Length of the iOS notification data. */
#define BT_ANCS_NOTIF_DATA_LENGTH 8

/** 0b.......1 Silent: First (LSB) bit is set. All flags can be active
 *  at the same time.
 */
#define BT_ANCS_EVENT_FLAG_SILENT 0
/** 0b......1. Important: Second (LSB) bit is set. All flags can be active
 *  at the same time.
 */
#define BT_ANCS_EVENT_FLAG_IMPORTANT 1
/** 0b.....1.. Pre-existing: Third (LSB) bit is set. All flags can be active
 *  at the same time.
 */
#define BT_ANCS_EVENT_FLAG_PREEXISTING 2
/** 0b....1... Positive action: Fourth (LSB) bit is set. All flags can be active
 *  at the same time.
 */
#define BT_ANCS_EVENT_FLAG_POSITIVE_ACTION 3
/** 0b...1.... Negative action: Fifth (LSB) bit is set. All flags can be active
 *  at the same time.
 */
#define BT_ANCS_EVENT_FLAG_NEGATIVE_ACTION 4

/** @defgroup BT_ATT_ANCS_NP_ERROR_CODES Notification Provider (iOS) Error Codes
 * @{
 */
/** The command ID is unknown to the NP. */
#define BT_ATT_ERR_ANCS_NP_UNKNOWN_COMMAND 0xA0
/** The command format is invalid. */
#define BT_ATT_ERR_ANCS_NP_INVALID_COMMAND 0xA1
/** One or more parameters do not exist in the NP. */
#define BT_ATT_ERR_ANCS_NP_INVALID_PARAMETER 0xA2
/** The action failed to be performed by the NP. */
#define BT_ATT_ERR_ANCS_NP_ACTION_FAILED 0xA3
/** @} */

/**@brief Category IDs for iOS notifications. */
enum bt_ancs_category_id_val {
	/** The iOS notification belongs to the "Other" category.  */
	BT_ANCS_CATEGORY_ID_OTHER,
	/** The iOS notification belongs to the "Incoming Call" category. */
	BT_ANCS_CATEGORY_ID_INCOMING_CALL,
	/** The iOS notification belongs to the "Missed Call" category. */
	BT_ANCS_CATEGORY_ID_MISSED_CALL,
	/** The iOS notification belongs to the "Voice Mail" category. */
	BT_ANCS_CATEGORY_ID_VOICE_MAIL,
	/** The iOS notification belongs to the "Social" category. */
	BT_ANCS_CATEGORY_ID_SOCIAL,
	/** The iOS notification belongs to the "Schedule" category. */
	BT_ANCS_CATEGORY_ID_SCHEDULE,
	/** The iOS notification belongs to the "Email" category. */
	BT_ANCS_CATEGORY_ID_EMAIL,
	/** The iOS notification belongs to the "News" category. */
	BT_ANCS_CATEGORY_ID_NEWS,
	/** The iOS notification belongs to the "Health and Fitness" category. */
	BT_ANCS_CATEGORY_ID_HEALTH_AND_FITNESS,
	/** The iOS notification belongs to the "Business and Finance" category. */
	BT_ANCS_CATEGORY_ID_BUSINESS_AND_FINANCE,
	/** The iOS notification belongs to the "Location" category. */
	BT_ANCS_CATEGORY_ID_LOCATION,
	/** The iOS notification belongs to the "Entertainment" category. */
	BT_ANCS_CATEGORY_ID_ENTERTAINMENT
};

/**@brief Event IDs for iOS notifications. */
enum bt_ancs_evt_id_values {
	/** The iOS notification was added. */
	BT_ANCS_EVENT_ID_NOTIFICATION_ADDED,
	/** The iOS notification was modified. */
	BT_ANCS_EVENT_ID_NOTIFICATION_MODIFIED,
	/** The iOS notification was removed. */
	BT_ANCS_EVENT_ID_NOTIFICATION_REMOVED
};

/**@brief Control point command IDs that the Notification Consumer can send
 *        to the Notification Provider.
 */
enum bt_ancs_cmd_id_val {
	/** Requests attributes to be sent from the NP to the NC for a given
	 *  notification.
	 */
	BT_ANCS_COMMAND_ID_GET_NOTIF_ATTRIBUTES,
	/** Requests attributes to be sent from the NP to the NC for a given
	 *  iOS app.
	 */
	BT_ANCS_COMMAND_ID_GET_APP_ATTRIBUTES,
	/** Requests an action to be performed on a given notification.
	 *  For example, dismiss an alarm.
	 */
	BT_ANCS_COMMAND_ID_PERFORM_NOTIF_ACTION
};

/**@brief IDs for actions that can be performed for iOS notifications. */
enum bt_ancs_action_id_values {
	/** Positive action. */
	BT_ANCS_ACTION_ID_POSITIVE = 0,
	/** Negative action. */
	BT_ANCS_ACTION_ID_NEGATIVE
};

/**@brief App attribute ID values.
 * @details Currently, only one value is defined. However, the number of app
 * attributes might increase. For this reason, they are stored in an enumeration.
 */
enum bt_ancs_app_attr_id_val {
	/** Command used to get the display name for an app identifier. */
	BT_ANCS_APP_ATTR_ID_DISPLAY_NAME = 0
};

/**@brief IDs for iOS notification attributes. */
enum bt_ancs_notif_attr_id_val {
	/** Identifies that the attribute data is of an "App Identifier" type. */
	BT_ANCS_NOTIF_ATTR_ID_APP_IDENTIFIER = 0,
	/** Identifies that the attribute data is a "Title". */
	BT_ANCS_NOTIF_ATTR_ID_TITLE,
	/** Identifies that the attribute data is a "Subtitle". */
	BT_ANCS_NOTIF_ATTR_ID_SUBTITLE,
	/** Identifies that the attribute data is a "Message". */
	BT_ANCS_NOTIF_ATTR_ID_MESSAGE,
	/** Identifies that the attribute data is a "Message Size". */
	BT_ANCS_NOTIF_ATTR_ID_MESSAGE_SIZE,
	/** Identifies that the attribute data is a "Date". */
	BT_ANCS_NOTIF_ATTR_ID_DATE,
	/** The notification has a "Positive action" that can be executed associated with it. */
	BT_ANCS_NOTIF_ATTR_ID_POSITIVE_ACTION_LABEL,
	/** The notification has a "Negative action" that can be executed associated with it. */
	BT_ANCS_NOTIF_ATTR_ID_NEGATIVE_ACTION_LABEL
};

/**@brief Flags for iOS notifications. */
struct bt_ancs_notif_flags {
	/** If this flag is set, the notification has a low priority. */
	uint8_t silent : 1;
	/** If this flag is set, the notification has a high priority. */
	uint8_t important : 1;
	/** If this flag is set, the notification is pre-existing. */
	uint8_t pre_existing : 1;
	/** If this flag is set, the notification has a positive action that can be taken. */
	uint8_t positive_action : 1;
	/** If this flag is set, the notification has a negative action that can be taken. */
	uint8_t negative_action : 1;
};

/**@brief Parsing states for received iOS notification and app attributes. */
enum bt_ancs_parse_state {
	/** Parsing the command ID. */
	BT_ANCS_PARSE_STATE_COMMAND_ID,
	/** Parsing the notification UID. */
	BT_ANCS_PARSE_STATE_NOTIF_UID,
	/** Parsing app ID. */
	BT_ANCS_PARSE_STATE_APP_ID,
	/** Parsing attribute ID. */
	BT_ANCS_PARSE_STATE_ATTR_ID,
	/** Parsing the LSB of the attribute length. */
	BT_ANCS_PARSE_STATE_ATTR_LEN1,
	/** Parsing the MSB of the attribute length. */
	BT_ANCS_PARSE_STATE_ATTR_LEN2,
	/** Parsing the attribute data. */
	BT_ANCS_PARSE_STATE_ATTR_DATA,
	/** Parsing is skipped for the rest of an attribute (or entire attribute). */
	BT_ANCS_PARSE_STATE_ATTR_SKIP,
	/** Parsing for one attribute is done. */
	BT_ANCS_PARSE_STATE_DONE
};

/**@brief iOS notification structure. */
struct bt_ancs_evt_notif {
	/** Notification UID. */
	uint32_t notif_uid;
	/** Whether the notification was added, removed, or modified. */
	enum bt_ancs_evt_id_values evt_id;
	/** Bitmask to signal whether a special condition applies to the
	 *  notification. For example, "Silent" or "Important".
	 */
	struct bt_ancs_notif_flags evt_flags;
	/** Classification of the notification type. For example, email or
	 *  location.
	 */
	enum bt_ancs_category_id_val category_id;
	/** Current number of active notifications for this category ID. */
	uint8_t category_count;
};

/**@brief iOS attribute structure. This type is used for both notification
 *        attributes and app attributes.
 */
struct bt_ancs_attr {
	/** Length of the received attribute data. */
	uint16_t attr_len;
	/** Classification of the attribute type. For example, "Title" or "Date". */
	uint32_t attr_id;
	/** Pointer to where the memory is allocated for storing incoming attributes. */
	uint8_t *attr_data;
};

/**@brief iOS notification attribute content requested by the application. */
struct bt_ancs_attr_list {
	/** Boolean to determine whether this attribute will be requested from
	 *  the Notification Provider.
	 */
	bool get;
	/** Attribute ID: AppIdentifier(0), Title(1), Subtitle(2), Message(3),
	 *  MessageSize(4), Date(5), PositiveActionLabel(6),
	 *  NegativeActionLabel(7).
	 */
	uint32_t attr_id;
	/** Length of the attribute. If more data is received from the
	 *  Notification Provider, all the data beyond this length is discarded.
	 */
	uint16_t attr_len;
	/** Pointer to where the memory is allocated for storing incoming attributes. */
	uint8_t *attr_data;
};

struct bt_ancs_client;

/**@brief Attribute response structure. */
struct bt_ancs_attr_response {
	/** Command ID. */
	enum bt_ancs_cmd_id_val command_id;
	/** iOS notification attribute or app attribute, depending on the
	 *  Command ID.
	 */
	struct bt_ancs_attr attr;
	/** Notification UID. */
	uint32_t notif_uid;
	/** App identifier. */
	uint8_t app_id[BT_ANCS_ATTR_DATA_MAX];
};

/**@brief Notification Source notification callback function.
 *
 * @param[in] ancs_c    ANCS client instance.
 * @param[in] err       0 if the notification is valid.
 *                      Otherwise, contains a (negative) error code.
 * @param[in] notif     iOS notification structure.
 */
typedef void (*bt_ancs_ns_notif_cb)(struct bt_ancs_client *ancs_c, int err,
				    const struct bt_ancs_evt_notif *notif);

/**@brief Data Source notification callback function.
 *
 * @param[in] ancs_c    ANCS client instance.
 * @param[in] response  Attribute response structure.
 */
typedef void (*bt_ancs_ds_notif_cb)(struct bt_ancs_client *ancs_c,
				    const struct bt_ancs_attr_response *response);

/**@brief Write response callback function.
 *
 * @param[in] ancs_c  ANCS client instance.
 * @param[in] err     ATT error code.
 */
typedef void (*bt_ancs_write_cb)(struct bt_ancs_client *ancs_c,
				 uint8_t err);

struct bt_ancs_parse_sm {
	/** The current list of attributes that are being parsed. This will
	 *  point to either ancs_notif_attr_list or ancs_app_attr_list in
	 *  struct bt_ancs_client.
	 */
	struct bt_ancs_attr_list *attr_list;
	/** Number of possible attributes. When parsing begins, it is set to
	 *  either @ref BT_ANCS_NOTIF_ATTR_COUNT or @ref BT_ANCS_APP_ATTR_COUNT.
	 */
	uint32_t attr_count;
	/** The number of attributes expected upon receiving attributes. Keeps
	 *  track of when to stop reading incoming attributes.
	 */
	uint32_t expected_number_of_attrs;
	/** ANCS notification attribute parsing state. */
	enum bt_ancs_parse_state parse_state;
	/** Variable to keep track of what command type is being parsed
	 *  ( @ref BT_ANCS_COMMAND_ID_GET_NOTIF_ATTRIBUTES or
	 *  @ref BT_ANCS_COMMAND_ID_GET_APP_ATTRIBUTES).
	 */
	enum bt_ancs_cmd_id_val command_id;
	/** Attribute that the parsed data is copied into. */
	uint8_t *data_dest;
	/** Variable to keep track of the parsing progress, for the given attribute. */
	uint16_t current_attr_index;
	/** Variable to keep track of the parsing progress, for the given app identifier. */
	uint32_t current_app_id_index;
};

/**@brief ANCS client instance, which contains various status information. */
struct bt_ancs_client {
	/** Connection object. */
	struct bt_conn *conn;

	/** Internal state. */
	atomic_t state;

	/** Handle of the Control Point Characteristic. */
	uint16_t handle_cp;

	/** Handle of the Notification Source Characteristic. */
	uint16_t handle_ns;

	/** Handle of the CCCD of the Notification Source Characteristic. */
	uint16_t handle_ns_ccc;

	/** Handle of the Data Source Characteristic. */
	uint16_t handle_ds;

	/** Handle of the CCCD of the Data Source Characteristic. */
	uint16_t handle_ds_ccc;

	/** GATT write parameters for Control Point Characteristic. */
	struct bt_gatt_write_params cp_write_params;

	/** Callback function for Control Point GATT write. */
	bt_ancs_write_cb cp_write_cb;

	/** Data buffer for Control Point GATT write. */
	uint8_t cp_data[CONFIG_BT_ANCS_CLIENT_CP_BUFF_SIZE];

	/** GATT subscribe parameters for Notification Source Characteristic. */
	struct bt_gatt_subscribe_params ns_notif_params;

	/** Callback function for Notification Source notification. */
	bt_ancs_ns_notif_cb ns_notif_cb;

	/** GATT subscribe parameters for Data Source Characteristic. */
	struct bt_gatt_subscribe_params ds_notif_params;

	/** Callback function for Data Source notification. */
	bt_ancs_ds_notif_cb ds_notif_cb;

	/** For all attributes: contains information about whether the
	 *  attributes are to be requested upon attribute request, and
	 *  the length and buffer of where to store attribute data.
	 */
	struct bt_ancs_attr_list
		ancs_notif_attr_list[BT_ANCS_NOTIF_ATTR_COUNT];

	/** For all app attributes: contains information about whether the
	 *  attributes are to be requested upon attribute request, and the
	 *  length and buffer of where to store attribute data.
	 */
	struct bt_ancs_attr_list
		ancs_app_attr_list[BT_ANCS_APP_ATTR_COUNT];

	/** The number of attributes that are to be requested when an iOS
	 *  notification attribute request is made.
	 */
	uint32_t number_of_requested_attr;

	/** Structure containing different information used to parse incoming
	 *  attributes correctly (from data_source characteristic).
	 */
	struct bt_ancs_parse_sm parse_info;

	/** Allocate memory for the attribute response here.
	 */
	struct bt_ancs_attr_response attr_response;
};

/**@brief Function for initializing the ANCS client.
 *
 * @param[out] ancs_c       ANCS client instance. This structure must be
 *                          supplied by the application. It is initialized by
 *                          this function and will later be used to identify
 *                          this particular client instance.
 *
 * @retval 0 If the client was initialized successfully.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_ancs_client_init(struct bt_ancs_client *ancs_c);

/**@brief Function for writing to the CCCD to enable notifications from
 *        the Apple Notification Service.
 *
 * @param[in] ancs_c  ANCS client instance.
 * @param[in] func    Callback function for handling notification.
 *
 * @retval 0 If writing to the CCCD was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_ancs_subscribe_notification_source(struct bt_ancs_client *ancs_c,
					  bt_ancs_ns_notif_cb func);

/**@brief Function for writing to the CCCD to enable data source notifications from the ANCS.
 *
 * @param[in] ancs_c  ANCS client instance.
 * @param[in] func    Callback function for handling notification.
 *
 * @retval 0 If writing to the CCCD was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_ancs_subscribe_data_source(struct bt_ancs_client *ancs_c,
				  bt_ancs_ds_notif_cb func);

/**@brief Function for writing to the CCCD to disable notifications from the ANCS.
 *
 * @param[in] ancs_c  ANCS client instance
 *
 * @retval 0 If writing to the CCCD was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_ancs_unsubscribe_notification_source(struct bt_ancs_client *ancs_c);

/**@brief Function for writing to the CCCD to disable data source notifications from the ANCS.
 *
 * @param[in] ancs_c  ANCS client instance.
 *
 * @retval 0 If writing to the CCCD was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_ancs_unsubscribe_data_source(struct bt_ancs_client *ancs_c);

/**@brief Function for registering attributes that will be requested when
 *        @ref bt_ancs_request_attrs is called.
 *
 * @param[in] ancs_c ANCS client instance on which the attribute is registered.
 * @param[in] id     ID of the attribute that is added.
 * @param[in] data   Pointer to the buffer where the data of the attribute can be stored.
 * @param[in] len    Length of the buffer where the data of the attribute can be stored.
 *
 * @retval 0 If all operations are successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_ancs_register_attr(struct bt_ancs_client *ancs_c,
			  const enum bt_ancs_notif_attr_id_val id,
			  uint8_t *data, const uint16_t len);

/**@brief Function for registering attributes that will be requested when
 *        @ref bt_ancs_request_app_attr is called.
 *
 * @param[in] ancs_c ANCS client instance on which the attribute is registered.
 * @param[in] id     ID of the attribute that is added.
 * @param[in] data   Pointer to the buffer where the data of the attribute can be stored.
 * @param[in] len    Length of the buffer where the data of the attribute can be stored.
 *
 * @retval 0 If all operations are successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_ancs_register_app_attr(struct bt_ancs_client *ancs_c,
			      const enum bt_ancs_app_attr_id_val id,
			      uint8_t *data, const uint16_t len);

/**@brief Function for requesting attributes for a notification.
 *
 * @param[in] ancs_c   ANCS client instance.
 * @param[in] notif    Pointer to the notification whose attributes will be requested from
 *                     the Notification Provider.
 * @param[in] func     Callback function for handling NP write response.
 *
 * @retval 0 If all operations are successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_ancs_request_attrs(struct bt_ancs_client *ancs_c,
			  const struct bt_ancs_evt_notif *notif,
			  bt_ancs_write_cb func);

/**@brief Function for requesting attributes for a given app.
 *
 * @param[in] ancs_c   ANCS client instance.
 * @param[in] app_id   App identifier of the app for which the app attributes are requested.
 * @param[in] len      Length of the app identifier.
 * @param[in] func     Callback function for handling NP write response.
 *
 * @retval 0 If all operations are successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_ancs_request_app_attr(struct bt_ancs_client *ancs_c,
			     const uint8_t *app_id, uint32_t len,
			     bt_ancs_write_cb func);

/**@brief Function for performing a notification action.
 *
 * @param[in] ancs_c    ANCS client instance.
 * @param[in] uuid      The UUID of the notification for which to perform the action.
 * @param[in] action_id Perform a positive or negative action.
 * @param[in] func      Callback function for handling NP write response.
 *
 * @retval 0 If the operation is successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_ancs_notification_action(struct bt_ancs_client *ancs_c, uint32_t uuid,
				enum bt_ancs_action_id_values action_id,
				bt_ancs_write_cb func);

/**@brief Function for assigning handles to ANCS client instance.
 *
 * @details Call this function when a link has been established with a peer to
 *          associate the link to this instance of the module. This makes it
 *          possible to handle several links and associate each link to a particular
 *          instance of this module.
 *
 * @param[in]     dm     Discovery object.
 * @param[in,out] ancs_c ANCS client instance for associating the link.
 *
 * @retval 0 If the operation is successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_ancs_handles_assign(struct bt_gatt_dm *dm,
			   struct bt_ancs_client *ancs_c);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_ANCS_CLIENT_H_ */
