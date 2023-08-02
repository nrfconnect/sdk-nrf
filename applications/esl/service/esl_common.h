/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_ESL_COMMON_H_
#define BT_ESL_COMMON_H_

/**
 * @file
 * @defgroup bt_esl Electronics Shelf Label (ESL) GATT Service
 * @{
 * @brief Electronics Shelf Label (ESL) GATT Service API.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Here are Assigned Numbers from
 * https://bitbucket.org/bluetooth-SIG/registry/src/esl_assigned_numbers_request/uuids/characteristic_uuids.yaml
 */
#define BT_UUID_ESL_VAL (0x1857)

/** @brief UUID of the ESL Address Characteristic. **/
#define BT_UUID_ESL_ADDR_VAL (0x2BF6)

/** @brief UUID of the AP Sync Key Characteristic. **/
#define BT_UUID_AP_SYNC_KEY_VAL (0x2BF7)

/** @brief UUID of the ESL Response Key Characteristic. **/
#define BT_UUID_ESL_RESP_KEY_VAL (0x2BF8)

/** @brief UUID of the ESL Current Absolute Time Characteristic. **/
#define BT_UUID_ESL_ABS_TIME_VAL (0x2BF9)

/** @brief UUID of the Display Information Characteristic. **/
#define BT_UUID_DISP_INF_VAL (0x2BFA)

/** @brief UUID of the Image Information Characteristic. **/
#define BT_UUID_IMG_INF_VAL (0x2BFB)

/** @brief UUID of the Sensor Information Characteristic. **/
#define BT_UUID_SENSOR_INF_VAL (0x2BFC)

/** @brief UUID of the LED Information Characteristic. **/
#define BT_UUID_LED_INF_VAL (0x2BFD)

/** @brief UUID of the ESL Control Point Characteristic. **/
#define BT_UUID_ESL_CONT_POINT_VAL (0x2BFE)

#define BT_UUID_ESL_SERVICE    BT_UUID_DECLARE_16(BT_UUID_ESL_VAL)
#define BT_UUID_ESL_ADDR       BT_UUID_DECLARE_16(BT_UUID_ESL_ADDR_VAL)
#define BT_UUID_AP_SYNC_KEY    BT_UUID_DECLARE_16(BT_UUID_AP_SYNC_KEY_VAL)
#define BT_UUID_ESL_RESP_KEY   BT_UUID_DECLARE_16(BT_UUID_ESL_RESP_KEY_VAL)
#define BT_UUID_ESL_ABS_TIME   BT_UUID_DECLARE_16(BT_UUID_ESL_ABS_TIME_VAL)
#define BT_UUID_DISP_INF       BT_UUID_DECLARE_16(BT_UUID_DISP_INF_VAL)
#define BT_UUID_IMG_INF	       BT_UUID_DECLARE_16(BT_UUID_IMG_INF_VAL)
#define BT_UUID_SENSOR_INF     BT_UUID_DECLARE_16(BT_UUID_SENSOR_INF_VAL)
#define BT_UUID_LED_INF	       BT_UUID_DECLARE_16(BT_UUID_LED_INF_VAL)
#define BT_UUID_ESL_CONT_POINT BT_UUID_DECLARE_16(BT_UUID_ESL_CONT_POINT_VAL)

/* AD type from
 * https://bitbucket.org/bluetooth-SIG/registry/src/esl_assigned_numbers_request/core/ad_types.yaml
 */
#define BT_DATA_ESL 0x34 /* ESL  Data */

#ifdef CONFIG_BT_ESL_CLIENT
#define CONFIG_BT_ESL_DISPLAY_MAX 102 /* Max attr value 512/5 = 102 */
#define CONFIG_BT_ESL_SENSOR_MAX  170 /* 512/3 = 170 */
#define CONFIG_BT_ESL_LED_MAX	  256
#endif
/** @brief Constant of ESL service. **/
#define ESL_ADDR_LEN			    2u
#define ESL_ADDR_BROADCAST		    0xFF
#define ESL_ADDR_RFU_BIT		    15
#define ESL_GROUPID_RFU_BIT		    7
#define EAD_SESSION_KEY_LEN		    16u
#define EAD_IV_LEN			    8u
#define EAD_KEY_MATERIAL_LEN		    (EAD_SESSION_KEY_LEN + EAD_IV_LEN)
#define EAD_RANDOMIZER_LEN		    5u
#define EAD_NONCE_LEN			    (EAD_RANDOMIZER_LEN + EAD_IV_LEN)
#define ESL_DISP_CHARA_LEN		    5u
#define ESL_MAX_SENSOR_TYPE_LEN		    4u
#define ESL_LED_TYPE_BIT		    6
#define ESL_LED_SRGB			    0x00
#define ESL_LED_MONO			    0x01
#define ESL_LED_BRIGHTNESS_BIT		    6
#define ESL_LED_BLUE_HI_BIT		    5
#define ESL_LED_BLUE_LO_BIT		    4
#define ESL_LED_GREEN_HI_BIT		    3
#define ESL_LED_GREEN_LO_BIT		    2
#define ESL_LED_RED_HI_BIT		    1
#define ESL_LED_RED_LO_BIT		    0
#define ESL_LED_FLASH_PATTERN_LEN	    5u /* 56 bits ESLS 3.10.2.10.1 Format */
#define ESL_LED_FLASH_PATTERN_START_BIT_IDX (ESL_LED_FLASH_PATTERN_LEN * 8)
#define ESL_ABS_TIME_LEN		    4u	/* 32 bits ESLS 3.10.2.9.2 Format */
#define ESL_REPEAT_DURATION_LEN		    2u	/* 15 bits 3.10.2.11.2 Format */
#define ESL_RESPONSE_SENSOR_HEADER	    2u	/* 1 opcode, 1 sensor_index */
#define ESL_RESPONSE_SENSOR_LEN		    15u /* 120 bits 3.10.3.5 Sensor Value */
#define ESL_RESPONSE_MAX_LEN		    (ESL_RESPONSE_SENSOR_LEN + 2u)
#define ESL_ECP_COMMAND_MAX_LEN		    17u /* 3.10.2.11.2 Format Format ESL service */
/* 1 for LLCAP packet len, 1 for AD TYPE */
#define AD_HEADER_LEN			    2u
#define ESL_SYNC_PKT_PAYLOAD_MAX_LEN	    48u
#define ESL_SYNC_PKT_MIC_LEN		    4u
#define ESL_ENCRTYPTED_DATA_MAX_LEN                                                                \
	(AD_HEADER_LEN + EAD_RANDOMIZER_LEN + AD_HEADER_LEN + ESL_SYNC_PKT_PAYLOAD_MAX_LEN +       \
	 ESL_SYNC_PKT_MIC_LEN)
#define ESL_EAD_PAYLOAD_MAX_LEN	     (ESL_ENCRTYPTED_DATA_MAX_LEN + AD_HEADER_LEN)
#define ESL_ENCRYTPTED_DATA_LEN_IDX  EAD_RANDOMIZER_LEN
#define ESL_RAMDOMIZER_DIRECTION_BIT 7u

#define VALIDATION_EAD

#define ESL_SYNC_PKT_HEADER_FOOTER_LEN (EAD_RANDOMIZER_LEN + ESL_SYNC_PKT_MIC_LEN + AD_HEADER_LEN)
#define ESL_SYNC_PKT_PAYLOAD_IDX       (EAD_RANDOMIZER_LEN + AD_HEADER_LEN)
#define ESL_SYNC_PKT_GROUPID_IDX       ESL_SYNC_PKT_PAYLOAD_IDX + AD_HEADER_LEN
#define SYNC_PKK_MIN_LEN (ESL_SYNC_PKT_HEADER_FOOTER_LEN)

#define ESL_TIMED_ABS_MAX	   4147200000 /* 48days = 48 *24 *3600 *1000 */
#define ESL_AP_PA_SID		   8
#define ESL_AP_SYNC_THRESHOLD	   1
/** @brief Index of Control command parameters */
#define ESL_OP			   0
#define ESL_OP_ESL_ID_IDX	   1
#define ESL_OP_HEADER_LEN	   (ESL_OP_ESL_ID_IDX + 1)
#define ESL_OP_READ_SENSOR_IDX	   (ESL_OP_ESL_ID_IDX + 1)
#define ESL_OP_REFRESH_DISPLAY_IDX (ESL_OP_ESL_ID_IDX + 1)
#define ESL_OP_CONNECTABLE_ADV_IDX (ESL_OP_ESL_ID_IDX + 1)

#define ESL_OP_DISPLAY_IDX	 (ESL_OP_ESL_ID_IDX + 1)
#define ESL_OP_DISPLAY_IMAGE_IDX (ESL_OP_DISPLAY_IDX + 1)

#define ESL_OP_DISPLAY_TIMED_DISPLAY_IDX  (ESL_OP_ESL_ID_IDX + 1)
#define ESL_OP_DISPLAY_TIMED_IMAGE_IDX	  (ESL_OP_DISPLAY_TIMED_DISPLAY_IDX + 1)
#define ESL_OP_DISPLAY_TIMED_ABS_TIME_IDX (ESL_OP_DISPLAY_TIMED_IMAGE_IDX + 1)

#define ESL_OP_LED_CONTROL_LED_IDX	    (ESL_OP_ESL_ID_IDX + 1)
#define ESL_OP_LED_CONTROL_LED_COLOR_IDX    (ESL_OP_LED_CONTROL_LED_IDX + 1)
#define ESL_OP_LED_CONTROL_FLASHING_PATTERN (ESL_OP_LED_CONTROL_LED_COLOR_IDX + 1)
#define ESL_OP_LED_CONTROL_BIT_OFF_PERIOD                                                          \
	(ESL_OP_LED_CONTROL_FLASHING_PATTERN + ESL_LED_FLASH_PATTERN_LEN)
#define ESL_OP_LED_CONTROL_BIT_ON_PERIOD   (ESL_OP_LED_CONTROL_BIT_OFF_PERIOD + 1)
#define ESL_OP_LED_CONTROL_REPEAT_TYPE	   (ESL_OP_LED_CONTROL_BIT_ON_PERIOD + 1)
#define ESL_OP_LED_CONTROL_REPEAT_DURATION (ESL_OP_LED_CONTROL_REPEAT_TYPE + 1)
#define ESL_OP_LED_TIMED_ABS_TIME_IDX                                                              \
	(ESL_OP_LED_CONTROL_REPEAT_TYPE + ESL_REPEAT_DURATION_LEN)
#define OBJECT_NAME_TEMPLETE		   "esl_image_%02X"
#define OBJECT_IDX_FULLNAME_TEMPLETE	   "%s/esl_image_%02X"

#define ESL_CTX_SIZE	    100
#define LOW_BYTE(x)	    ((uint8_t)((x)&0xFF))
#define HIGH_BYTE(x)	    ((uint8_t)(((x) >> 8) & 0xFF))
#define GROUPID_BYTE(x)	    ((uint8_t)(((x) >> 8) & 0x7F))
#define CHECK_BIT(var, pos) ((var) & (1 << (pos)))
#define BIT_UNSET(reg, bit) (reg &= ~(1UL << bit))
#define CREATE_FLAG(flag)   static atomic_t flag = (atomic_t) false
#define SET_FLAG(flag)	    (void)atomic_set(&flag, (atomic_t) true)
#define GET_FLAG(flag)	    atomic_get(&flag)
#define UNSET_FLAG(flag)    (void)atomic_set(&flag, (atomic_t) false)
#define WAIT_FOR_FLAG(flag)                                                                        \
	while (!(bool)atomic_get(&flag)) {                                                         \
		(void)k_sleep(K_MSEC(1));                                                          \
	}
#define ECP_LEN(x) (uint8_t)(((x & 0xf0) >> 4) + 2)
#define ECP_TAG(x) (uint8_t)((x & 0x0f))

#define REPEATS_DURATION_MAX 0x7FFF
/** @brief ESL Display types assigned number.
 * SIG Assigned number
 https://bitbucket.org/bluetooth-SIG/registry/src/release/profiles_and_services/esl/display_types.yaml
 * display_types:
  - name: black white
    value: 0x01
  - name: three gray scale
    value: 0x02
  - name: four gray scale
    value: 0x03
  - name: eight gray scale
    value: 0x04
  - name: sixteen gray scale
    value: 0x05
  - name: red black white
    value: 0x06
  - name: yellow black white
    value: 0x07
  - name: red yellow black white
    value: 0x08
  - name: seven color
    value: 0x09
  - name: sixteen color
    value: 0x0A
  - name: full RGB
    value: 0x0B
 */
enum ESL_DISPLAY_TYPES {
	ESL_DISPLAY_BLACK_WHITE = 0x1,
	ESL_DISPLAY_THREE_GRAY = 0x2,
	ESL_DISPLAY_FOUR_GRAY = 0x3,
	ESL_DISPLAY_EIGHT_GRAY = 0x04,
	ESL_DISPLAY_SIXTEEN_GRAY = 0x5,
	ESL_DISPLAY_RED_BLACK_WHITE = 0x06,
	ESL_DISPLAY_YELLOW_BLACK_WHITE = 0x07,
	ESL_DISPLAY_RED_YELLOW_BLACK_WHITE = 0x08,
	ESL_DISPLAY_SEVEN_COLOR = 0x09,
	ESL_DISPLAY_SIXTEEN_COLOR = 0x0A,
	ESL_DISPLAY_FULL_RGB = 0x0B,
	ESL_DISPLAY_TYPE_MAX = 0xFF
};

/** @brief ESL TLV source */
enum ESL_ACL_SOURCE {
	ACL_NOT_CONNECTED,
	ACL_FROM_SCAN,
	ACL_FROM_PAWR,
};

/** @brief ESL basic state bit field. */
enum ESL_BASIC_STATE_BIT {
	ESL_SERVICE_NEEDED_BIT,
	ESL_SYNCHRONIZED,
	ESL_ACTIVE_LED,
	ESL_PENDING_LED_UPDATE,
	ESL_PENDING_DISPLAY_UPDATE,
	ESL_BASIC_RFU,
};

#define BT_ATT_INVALID_HANDLE		  0x0000
#define DEFAULT_RESPONSE_START		  0
#define AUTO_PAST_RETRY			  5
/* AAD for EAD*/
#define EAD_AES_ADDITIONAL_SIZE		  (1)
/* OTS V10 Table 3.25: List of OLCP Result Codes and Response Parameters */
#define BT_GATT_OTS_OLCP_RES_OUT_OF_BONDS 0x05

#pragma pack(push, 1)
struct esl_disp_inf {
	uint16_t width;
	uint16_t height;
	uint8_t type;
};
#pragma pack(pop)
#define DISP_INF_SIZE sizeof(struct esl_disp_inf)
struct vendor_specific_sensor {
	uint16_t company_id;
	uint16_t sensor_code;
};
#pragma pack(push, 1)
struct esl_sensor_inf {
	uint8_t size;
	union {
		uint16_t property_id;
		struct vendor_specific_sensor vendor_specific;
	};
};
#pragma pack(pop)

struct dis_pnp {
	uint8_t pnp_vid_src;
	uint16_t pnp_vid;
	uint16_t pnp_pid;
	uint16_t pnp_ver;
} __packed;

/** @brief LED information and control structure */
struct led_obj {
	uint8_t led_type;
	uint8_t color_brightness;
	uint8_t flash_pattern[ESL_LED_FLASH_PATTERN_LEN];
	uint8_t bit_off_period;
	uint8_t bit_on_period;
	uint8_t repeat_type;
	uint16_t repeat_duration;
	uint32_t abs_time;
};

struct bt_ead_key_material {
	uint8_t session_key[EAD_SESSION_KEY_LEN];
	uint8_t iv[EAD_IV_LEN];
};

struct bt_esl_key_material {
	union {
		uint8_t key_v[EAD_KEY_MATERIAL_LEN];
		struct bt_ead_key_material key;
	};
};

/** @brief ESL Service characteristicc data structure*/
struct bt_esl_chrc_data {
	/** Data holding ESL address */
	uint16_t esl_addr;

	/** Data holding ESL tag BLE address */
	bt_addr_le_t ble_addr;

	/** Data holding AP sync key */
	struct bt_esl_key_material esl_rsp_key;

	/** Data holding RANDOMIZER */
	uint8_t esl_randomizer[EAD_RANDOMIZER_LEN];

	/** Data holding Current Absolute time */
	uint32_t esl_abs_time;

	/** Data holding max image index */
	uint8_t max_image_index;

	/** Data holding display element */
	struct esl_disp_inf displays[CONFIG_BT_ESL_DISPLAY_MAX];

#ifdef CONFIG_BT_ESL_CLIENT
	bool past_needed;
	uint16_t display_count;
	uint16_t sensor_count;
	uint16_t led_count;
#else
	/** Data holding led element control */
	struct led_obj led_objs[CONFIG_BT_ESL_LED_MAX];

	/** Data holding AP sync key */
	struct bt_esl_key_material esl_ap_key;

#endif /* (CONFIG_BT_ESL_CLIENT) */

	/** Data holding sensor element */
	struct esl_sensor_inf sensors[CONFIG_BT_ESL_SENSOR_MAX];

	/** Data holding static LED information */
	uint8_t led_type[CONFIG_BT_ESL_LED_MAX];

	/** Data holding PNP ID */
	struct dis_pnp dis_pnp_id;

	/** ACL connection source */
	enum ESL_ACL_SOURCE connect_from;
};

/**@brief	Get controller version
 *
 * @param[out]	version Pointer to version
 *
 * @return      0 for success, error otherwise
 */
int ble_ctrl_version_get(uint16_t *version);

#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /* BT_ESL_COMMON_H_ */
