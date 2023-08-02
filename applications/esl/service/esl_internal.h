/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_ESL_INTERNAL_H_
#define BT_ESL_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @file
 * @brief ESL service internal file
 */

#include <zephyr/smf.h>
#include <zephyr/bluetooth/services/ots.h>

#include "esl.h"

#if !defined(CONFIG_BT_ESL_IMAGE_MAX)
#define CONFIG_BT_ESL_IMAGE_MAX 0
#endif

/** @brief ESL Configuring state. **/
enum esl_configuring_state_bit {
	ESL_ADDR_SET_BIT,
	ESL_AP_SYNC_KEY_SET_BIT,
	ESL_RESPONSE_KEY_SET_BIT,
	ESL_ABS_TIME_SET_BIT,
	ESL_UPDATE_COMPLETE_SET_BIT,
};

/** @brief ESL packet source. **/
enum esl_ecp_command_src {
	ESL_ECP,
	ESL_PAWR,
};

/** @brief ESL tag advertising mode. **/
enum esl_adv_mode {
	ESL_ADV_DEMO,
	ESL_ADV_DEFAULT,
	ESL_ADV_UNASSOCIATED,
	ESL_ADV_UNSYNCED,
};

/** @brief ESL tag timeout mode. **/
enum esl_timeout_mode {
	ESL_UNSYNCED_TIMEOUT_MODE,
	ESL_UNASSOCIATED_TIMEOUT_MODE,
};

/** @brief ESL State Machine. */
enum BT_ESL_STATE {
	/** Unassociated state. */
	SM_UNASSOCIATED,
	/** Configuring state. */
	SM_CONFIGURING,
	/** Configured state.
	 * interal state to indicate all data provisioned includes UPDATE_COMPLETE
	 */
	SM_CONFIGURED,
	/** Synchronized state. */
	SM_SYNCHRONIZED,
	/** Updating state. */
	SM_UPDATING,
	/** Unsynchronized state. */
	SM_UNSYNCHRONIZED,
};

/** @brief ESL operation code. */
enum ESL_OP_CODE {
	OP_PING,
	OP_UNASSOCIATE,
	OP_SERVICE_RESET,
	OP_FACTORY_RESET,
	OP_UPDATE_COMPLETE,
	OP_READ_SENSOR_DATA = 0x10,
	OP_REFRESH_DISPLAY = 0x11,
	OP_DISPLAY_IMAGE = 0x20,
	OP_DISPLAY_TIMED_IMAGE = 0x60,
	OP_LED_CONTROL = 0xB0,
	OP_LED_TIMED_CONTROL = 0xF0,
};

/** @brief ESL response operation code. ESLS 3.10.3 Response behavior */
enum ESL_RESP_CODE {
	OP_ERR,
	OP_LED_STATE,
	OP_BASIC_STATE = 0x10,
	OP_DISPLAY_STATE = 0x11,
	OP_SENSOR_VALUE_STATE_MASK = 0x0E,
	OP_VENDOR_SPECIFIC_STATE_MASK = 0x0F,
};

/** @brief ESL response error code. */
enum ESL_RSP_ERR_CODE {
	ERR_RFU,
	ERR_UNSPEC,
	ERR_INV_OPCODE,
	ERR_INV_STATE,
	ERR_INV_IMG_IDX,
	ERR_IMG_NOT_AVAIL,
	ERR_INV_PARAMS,
	ERR_CAP_LIMIT,
	ERR_INSUFF_BAT,
	ERR_INSUFF_RES,
	ERR_RETRY,
	ERR_QUEUE_FULL,
	ERR_IMPLAUSIBLE_ABS,
	ERR_RFU_2,
	ERR_VENDOR_SPEC = 0xf0,
};

/** @brief ESL notification events. */
enum BT_ESL_NOTIFY_EVT {
	/** Notification enabled event. */
	BT_ESL_CCCD_EVT_NOTIFY_ENABLED,
	/** Notification disabled event. */
	BT_ESL_CCCD_EVT_NOTIFY_DISABLED,
};
/** @brief ESL state machine object. **/
struct esl_sm_object {
	struct smf_ctx ctx;
	/* All User Defined Data Follows */
	enum BT_ESL_STATE sm;
};

/** @brief Advertising ESL Service @ref bt_esl_adv_start. */
void bt_esl_adv_start(enum esl_adv_mode);

/** @brief Stop advertising ESL Service @ref bt_esl_adv_stop. */
void bt_esl_adv_stop(void);

/** @brief Unassociated to AP @ref bt_esl_unassociate.
 * remove all bonding information with the AP and delete the
 * value of the AP Sync Key in internal storage
 * 3.10.2.2 Unassociate from AP
 */
void bt_esl_unassociate(void);

/** @brief Disconnect ACL for development. */
void bt_esl_disconnect(void);

/** @brief Print bonded AP address. */
void bt_esl_bond_dump(void *user_data);

/** @brief Update ESL basic state. */
void bt_esl_basic_state_update(void);

/** @brief Transist ESL state machine. */
void bt_esl_state_transition(uint8_t state);

/** @brief Get ESL image OTS object. */
struct ots_img_object *bt_esl_otc_inst(void);

/** @brief Stop sync PAwR. */
void esl_stop_sync_pawr(void);

/** @brief Check if ESL is in configuring state. */
bool esl_is_configuring_state(struct bt_esls *esl_obj);

/** @brief Calculate checksum of transferred image data from OTS.
 * This function is called if ESL wants to check image integrity.
 *
 * @param[in] ots    Object Transfer Service Instance.
 * @param[in] conn   Pointer to connection object that has received data.
 * @param[in] id     48bit Object IDs to be checked.
 * @param[in] offset Offset of Object to be checked.
 * @param[in] len   Length of Object.
 * @param[out] data  Pointer to hold Object data.
 *
 * @retval 0 If retrieve Object data is successful.
 *           Otherwise, a negative value is returned.
 */
int ots_obj_cal_checksum(struct bt_ots *ots, struct bt_conn *conn, uint64_t id, off_t offset,
			 size_t len, void **data);

/** @brief Update ESL absolute time anchor. */
void esl_abs_time_update(uint32_t new_abs_time);

/** @brief Get ESL absolute time.
 * This function is called when ESL wants to know current absolute time.
 *
 * @retval 32bit Absolute time set by Access Point, increasing with 1 ms resolution.
 */
uint32_t esl_get_abs_time(void);

/** @brief Unpack LED repeat type and duration from ESL LED Control Command.
 *
 * This function is used to extract the repeat type and duration from a given ESL LED Control
 * Command. The unpacked values are useful for applications that need to understand or manipulate
 * the repeat behavior of certain processes or tasks.
 *
 * @param[in] data Pointer to the data from which to extract repeat type and duration.
 * @param[out] repeat_type Pointer to a variable where the extracted repeat type will be stored.
 * @param[out] repeat_duration Pointer to a variable where the extracted repeat duration will be
 * stored.
 */
void unpack_repeat_type_duration(uint8_t *data, uint8_t *repeat_type, uint16_t *repeat_duration);

/** @brief Pack repeat type and duration into ESL LED Control Command.
 *
 * This function is used to pack the repeat type and duration into a given LED Control Command.
 * The packed data can then be used for transmitting these details over a GATT or PAwR.
 *
 * @param[out] data Pointer to the data buffer where the packed repeat type and duration will be
 * stored.
 * @param[in] repeat_type The repeat type to be packed into the data.
 * @param[in] repeat_duration The repeat duration to be packed into the data.
 */
void pack_repeat_type_duration(uint8_t *data, uint8_t repeat_type, uint16_t repeat_duration);

/** @brief Print display information.
 *
 * This function is used to dump display information from a given raw data buffer.
 * The information may include details about displayed images or other related details.
 * This can be useful for debugging or informational purposes.
 *
 * @param[in] raw Pointer to the raw data from which display information will be dumped.
 * @param[in] len The length of the raw data.
 * @param[in] displayed_imgs Pointer to the variable which image is displayed on display device.
 */
void dump_disp_inf(uint8_t *raw, uint16_t len, uint8_t *displayed_imgs);

/** @brief Print LED information.
 *
 * This function is used to dump LED information from a given raw data buffer.
 * The information may include details about LED states, color, brightness or other related details.
 * This can be useful for debugging or informational purposes.
 *
 * @param[in] raw Pointer to the raw data from which LED information will be dumped.
 * @param[in] len The length of the raw data.
 */
void dump_led_infs(uint8_t *raw, uint16_t len);

/** @brief Parse and return sensor count from raw data.
 *
 * This function is used to dump sensor information from a given raw data buffer
 * and store it in a struct. It also returns the count of sensors found in the data.
 * This can be useful when working with multiple sensors and the data needs to be processed
 * efficiently.
 *
 * @param[out] sensors Pointer to the array of sensor information structures where parsed
 * information will be stored.
 * @param[in] raw Pointer to the raw data from which sensor information will be parsed.
 * @param[in] len The length of the raw data.
 *
 * @return The count of sensors found in the raw data.
 */
uint8_t parse_sensor_info_from_raw(struct esl_sensor_inf *sensors, uint8_t *raw, uint16_t len);

/** @brief Dump sensor information.
 *
 * This function is used to dump sensor information from a given esl_sensor_inf structure.
 * The information may include sensor details and parameters.
 * This can be useful for debugging or informational purposes.
 *
 * @param[in] sensors Pointer to the array of sensor information structures to be dumped.
 * @param[in] count The number of sensors in the array.
 */
void dump_sensor_inf(const struct esl_sensor_inf *sensors, uint8_t count);

/** @brief Dump sensor data.
 *
 * This function is used to dump sensor data from a given raw data buffer.
 * The data could include measurements.
 * This can be useful for debugging or informational purposes.
 *
 * @param[in] raw Pointer to the raw data from which sensor data will be dumped.
 * @param[in] len The length of the raw data.
 */
void dump_sensor_data(const uint8_t *raw, uint16_t len);

/** @brief Print data in hexadecimal string format.
 *
 * This function is used to print a given buffer of data in hexadecimal format with leading 0x to
 * UART console. This can be useful for debugging or when you need to visually inspect the data.
 *
 * @param[in] hex Pointer to the data to be printed in hexadecimal format.
 * @param[in] len The length of the data.
 */
void print_hex(const uint8_t *hex, size_t len);

/** @brief Print data in binary string.
 *
 * This function is used to print a given hexadecimal number in binary format. General used to print
 * basic state of ESL Tag. This can be useful for debugging or when you need to visually inspect the
 * binary representation of the data.
 *
 * @param[in] hex The hexadecimal number to be printed in binary format.
 * @param[in] len The number of bits to print, starting from the least significant bit.
 */
void print_binary(uint64_t hex, uint8_t len);

/** @brief Dump OTS client handles.
 *
 * This function is used to dump OTS (Object Transfer Service) client handles from a given
 * discovered OTS client. The dumped handles are stored in the provided buffer. This function also
 * returns the length of the data written to the buffer. This can be useful for debugging or
 * informational purposes.
 *
 * @param[out] buf Pointer to the buffer where the dumped handles will be stored.
 * @param[in] otc The OTS client structure from which handles will be dumped.
 *
 * @return The length of the data written to the buffer.
 */
uint16_t dump_ots_client_handles(char *buf, struct bt_ots_client otc);

/** @brief Check if BLE (Bluetooth Low Energy) connection existed.
 *
 * This function checks whether a given BLE connection is established or not.
 * It can be used to ensure that a BLE connection is active before performing operations that
 * require connectivity.
 *
 * @param[in] conn Pointer to the BLE connection to be checked.
 *
 * @return True if the connection is established, false otherwise.
 */
bool check_ble_connection(struct bt_conn *conn);

/** @brief Convert ESL state to string.
 *
 * This function converts a given ESL state into a string representation.
 * It can be used for easier logging or display of ESL states.
 *
 * @param[in] state The ESL state to be converted.
 *
 * @return A string representation of the given ESL state.
 */
uint8_t *esl_state_to_string(uint8_t state);


/** @brief Convert ESL response error code to string.
 *
 * This function converts a given ESL response error code into a string representation.
 * It can be used for easier logging or display of ESL response error codes.
 *
 * @param[in] err_code The ESL response error code to be converted.
 *
 * @return A string representation of the given ESL response error code.
 */
uint8_t *esl_rcp_error_to_string(enum ESL_RSP_ERR_CODE err_code);


/** @brief Print ESL basic state.
 *
 * This function prints a given ESL basic state.
 * This can be useful for debugging or informational purposes.
 *
 * @param[in] state The basic state to be printed.
 */
void print_basic_state(atomic_t state);

/** @brief Generate ESL AP PAWR AD Data header
 *
 * This function generate Encrypted Data placeholder by the following procedure.
 * 1. Copy randomizer of this packet.
 * 2. Copy ESL payload plain text.
 * 3. Fill length of ESL TAG AD type.
 * 4. Fill ESL TAG AD type.
 *
 * @param[in] randomizer Randomizer of this packet for Encrypted Advertising Data.
 * @param[in,out] ad_data AD data placeholder.
 * @param[in] esl_payload_len ESL Payload length without ad type.
 *
 * @retval Length of AD header + randomizer + ESL payload. Should be 7 + ESL payload length
 * @retval (-EINVAL) Parameter null or zero.
 * @retval Otherwise, a negative error code is returned.
 */
int esl_generate_ad_data_header(uint8_t *randomizer, uint8_t *ad_data, uint8_t esl_payload_len);

/** @brief Helper function to combine randomizer and IV to nonce for AES-CCM use.
 *
 *
 * @param[in] randomizer First part of AES-CCM nonce. Normally came along with PAWR sync packet or
 * response.
 * @param[in] iv ESL key material initial vector part. Normally came from AP SYNC key material
 * or Response key material.
 * @param[out] nonce Nonce of this packet. ESL Randomizer + ESL Key material IV.
 *
 */
void esl_generate_nonce(uint8_t *randomizer, uint8_t *iv, uint8_t *nonce);

/** @brief Initialize the randomizer for EAD.
 *
 * This function is used to initialize the randomizer with a given value.
 * This is typically used before generating random numbers to ensure
 * that the sequence of generated numbers is pseudorandom.
 *
 * @param[in] randomizer The initial value for the randomizer.
 */
void esl_init_randomizer(uint8_t *randomizer);

/** @brief Update the randomizer and nonce for encrypted advertising data.
 *
 * This function is used to update the randomizer with new values and combine IV(initial vector) of
 * key material to nonce. According to the ESL protocol, the randomizer should be updated when a new
 * packet is sent. This helps maintain the pseudorandom nature of the generated numbers.
 *
 * @param[in, out] randomizer The randomizer to be updated.
 * @param[in] key The key material contains IV to update the randomizer.
 * @param[out] nonce The nonce to be update with the randomizer and IV.
 */
void esl_update_randomizer(uint8_t *randomizer, struct bt_esl_key_material key, uint8_t *nonce);

/** @brief Generate EAD (Encrypted Advertising Data) from ESL payload.
 *
 * This function is used to generate EAD from the provided ESL payload. The function
 * takes an ESL payload and a randomizer as input, and generates EAD as output.
 *
 * @param[out] pa_ad_data Pointer to the array where the generated EAD will be stored.
 * @param[in] esl_payload Pointer to the ESL payload from which the EAD will be generated.
 * @param[in] esl_payload_len The length of the ESL payload.
 * @param[in] randomizer The randomizer used in the generation process.
 * @param[in] key The key material used in the generation process.
 *
 * @return The length of the generated EAD.
 */
uint8_t esl_compose_ad_data(uint8_t *pa_ad_data, uint8_t *esl_payload, uint8_t esl_payload_len,
			    uint8_t *randomizer, struct bt_esl_key_material key);

/** @brief Unregister a BT OTS client.
 *
 * This function is part of OTS service client which prototype is not declared.
 * This function is used to unregister a Bluetooth Object Transfer Service (OTS) client.
 * This could be used when the client is no longer needed, to free up resources.
 *
 * @param[in] index The index of the BT OTS client to be unregistered.
 *
 * @return 0 if the operation was successful.
 * @retval A negative error code is returned if there was an error while unregistering the client.
 */
int bt_ots_client_unregister(uint8_t index);

#ifdef __cplusplus
}
#endif

#endif /* BT_ESL_INTERNAL_H_ */
