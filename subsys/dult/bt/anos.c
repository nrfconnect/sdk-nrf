/*
 * Copyright (c) 2024-2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/gatt.h>

#include <dult/dult.h>
#include <dult/bt.h>
#include <dult/multi_user.h>
#include "dult_battery.h"
#include "dult_bt_anos.h"
#include "dult_user.h"
#include "dult_user_slot.h"
#include "dult_near_owner_state.h"
#include "dult_id.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dult_bt_anos, CONFIG_DULT_LOG_LEVEL);

/* Accessory non-owner GATT service UUIDs defined by the DULT specification. */
#define BT_UUID_ACCESSORY_NON_OWNER_SERVICE \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x15190001, 0x12F4, 0xC226, 0x88ED, 0x2AC5579F2A85))
#define BT_UUID_ACCESSORY_NON_OWNER_CHARACTERISTIC \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x8E0C0001, 0x1D68, 0xFB92, 0xBF61, 0x48377421680E))

/* DULT Accessory Non-Owner Service (ANOS) opcode groups. */
enum dult_bt_anos_opcode_group {
	DULT_BT_ANOS_OPCODE_GROUP_ACCESSORY_INFO,
	DULT_BT_ANOS_OPCODE_GROUP_NON_OWNER_CONTROL,
};

/* DULT opcodes for Accessory Information writes. */
enum anos_chrc_accessory_info_write_opcode {
	ANOS_CHRC_ACCESSORY_INFO_WRITE_OPCODE_GET_PRODUCT_DATA				= 0x003,
	ANOS_CHRC_ACCESSORY_INFO_WRITE_OPCODE_GET_MANUFACTURER_NAME			= 0x004,
	ANOS_CHRC_ACCESSORY_INFO_WRITE_OPCODE_GET_MODEL_NAME				= 0x005,
	ANOS_CHRC_ACCESSORY_INFO_WRITE_OPCODE_GET_ACCESSORY_CATEGORY			= 0x006,
	ANOS_CHRC_ACCESSORY_INFO_WRITE_OPCODE_GET_PROTOCOL_IMPLEMENTATION_VERSION	= 0x007,
	ANOS_CHRC_ACCESSORY_INFO_WRITE_OPCODE_GET_ACCESSORY_CAPABILITIES		= 0x008,
	ANOS_CHRC_ACCESSORY_INFO_WRITE_OPCODE_GET_NETWORK_ID				= 0x009,
	ANOS_CHRC_ACCESSORY_INFO_WRITE_OPCODE_GET_FIRMWARE_VERSION			= 0x00A,
	ANOS_CHRC_ACCESSORY_INFO_WRITE_OPCODE_GET_BATTERY_TYPE				= 0x00B,
	ANOS_CHRC_ACCESSORY_INFO_WRITE_OPCODE_GET_BATTERY_LEVEL				= 0x00C,
	ANOS_CHRC_ACCESSORY_INFO_WRITE_OPCODE_GET_NETWORK_VERSION			= 0x00D,
};

/* DULT opcodes for Non-owner control writes. */
enum anos_chrc_non_owner_control_write_opcode {
	ANOS_CHRC_NON_OWNER_CONTROL_WRITE_OPCODE_SOUND_START	= 0x300,
	ANOS_CHRC_NON_OWNER_CONTROL_WRITE_OPCODE_SOUND_STOP	= 0x301,
	ANOS_CHRC_NON_OWNER_CONTROL_WRITE_OPCODE_GET_ID		= 0x404,
};

#define ANOS_CHRC_ACCESSORY_INFO_INDICATION_OPCODE(anos_chrc_accessory_info_write_opcode)	\
	(0x800 | (anos_chrc_accessory_info_write_opcode))

/* DULT opcodes for Non-owner control indications. */
enum anos_chrc_non_owner_control_indication_opcode {
	ANOS_CHRC_NON_OWNER_CONTROL_INDICATION_OPCODE_COMMAND_RESPONSE		= 0x302,
	ANOS_CHRC_NON_OWNER_CONTROL_INDICATION_OPCODE_SOUND_COMPLETED		= 0x303,
	ANOS_CHRC_NON_OWNER_CONTROL_INDICATION_OPCODE_GET_ID_RESPONSE		= 0x405,
};

/* DULT command response status. */
enum anos_chrc_cmd_response_status {
	ANOS_CHRC_CMD_RESPONSE_STATUS_SUCCESS			= 0x0000,
	ANOS_CHRC_CMD_RESPONSE_STATUS_INVALID_STATE		= 0x0001,
	ANOS_CHRC_CMD_RESPONSE_STATUS_INVALID_CONFIGURATION	= 0x0002,
	ANOS_CHRC_CMD_RESPONSE_STATUS_INVALID_LENGTH		= 0x0003,
	ANOS_CHRC_CMD_RESPONSE_STATUS_INVALID_PARAM		= 0x0004,
	ANOS_CHRC_CMD_RESPONSE_STATUS_INVALID_COMMAND		= 0xFFFF,
};

/* DULT ANOS sound states. */
enum anos_sound_state {
	ANOS_SOUND_STATE_IDLE,
	ANOS_SOUND_STATE_START_REQUEST,
	ANOS_SOUND_STATE_START_ACK,
	ANOS_SOUND_STATE_STOP_REQUEST,
	ANOS_SOUND_STATE_EXTERNAL_TAKEOVER,
};

/* ANOS opcode descriptor. */
struct anos_opcode_entry {
	/* ANOS opcode. */
	uint16_t opcode;

	/* ANOS opcode group. */
	enum dult_bt_anos_opcode_group group;

	/* Optional precondition check for an ANOS opcode.
	 *
	 * Returns ANOS_CHRC_CMD_RESPONSE_STATUS_SUCCESS when the opcode may be
	 * processed, or a non-success status that the dispatcher reports back to the
	 * peer via a Command Response indication. A NULL verify handler means the
	 * opcode has no precondition and is always processed.
	 */
	enum anos_chrc_cmd_response_status (*verify)(struct bt_conn *conn,
						     const struct dult_user *dult_user);

	/* Build and send the indication payload for an ANOS opcode.
	 *
	 * Return error value is only used to forward the error from the GATT indication
	 * sending API to the dispatcher.
	 */
	int (*process)(struct bt_conn *conn,
		       const struct bt_gatt_attr *attr,
		       const struct dult_user *dult_user);
};

#define ANOS_ACCESSORY_INFO_OPCODE_ENTRY(_name, _verify, _process)		\
	{									\
		.opcode = ANOS_CHRC_ACCESSORY_INFO_WRITE_OPCODE_##_name,	\
		.group = DULT_BT_ANOS_OPCODE_GROUP_ACCESSORY_INFO,		\
		.verify = (_verify),						\
		.process = (_process),						\
	}

#define ANOS_NON_OWNER_CONTROL_OPCODE_ENTRY(_name, _verify, _process)		\
	{									\
		.opcode = ANOS_CHRC_NON_OWNER_CONTROL_WRITE_OPCODE_##_name,	\
		.group = DULT_BT_ANOS_OPCODE_GROUP_NON_OWNER_CONTROL,		\
		.verify = (_verify),						\
		.process = (_process),						\
	}

#define ANOS_CHRC_OPCODE_LEN	sizeof(uint16_t)
#define ANOS_CHRC_OPERAND_LEN_MAX	\
	MAX(DULT_USER_STR_PARAM_LEN_MAX, CONFIG_DULT_BT_ANOS_ID_PAYLOAD_LEN_MAX)
#define ANOS_CHRC_DATA_LEN_MAX	(ANOS_CHRC_OPCODE_LEN + ANOS_CHRC_OPERAND_LEN_MAX)
#define ANOS_CHRC_INDICATION_LEN(_operand_len)	(ANOS_CHRC_OPCODE_LEN + (_operand_len))

#define SLAB_ALIGN	4
#define SLAB_BLOCK_SIZE	ROUND_UP(sizeof(struct bt_gatt_indicate_params), SLAB_ALIGN)

K_MEM_SLAB_DEFINE_STATIC(indicate_params_slab, SLAB_BLOCK_SIZE,
			 CONFIG_DULT_BT_ANOS_INDICATION_COUNT, SLAB_ALIGN);

static const struct bt_gatt_attr *anos_chrc_indicate_attr;

static enum anos_sound_state anos_sound_state;
static struct bt_conn *sound_conn;
static const struct dult_bt_anos_sound_cb *anos_sound_cb;

/* Per-user memory reference holding the user's ANOS access callback.  */
static size_t anos_cb_id = DULT_USER_SLOT_MEM_REF_ID_UNSET;

static const struct dult_bt_anos_cb *anos_cb_get(const struct dult_user *user)
{
	void *ref = NULL;

	if (anos_cb_id == DULT_USER_SLOT_MEM_REF_ID_UNSET) {
		return NULL;
	}

	(void) dult_user_slot_mem_ref_get(user, anos_cb_id, &ref);

	return ref;
}

static int anos_cb_set(const struct dult_user *user, const struct dult_bt_anos_cb *cb)
{
	return dult_user_slot_mem_ref_set(user, anos_cb_id, (void *)cb);
}

/* Connection ownership table used to route ANOS operations to the right user
 * during the multi-user pre-association window. Indexed by bt_conn_index(), it
 * stores the user that claimed each connection. Each DULT user claims the
 * connections it owns from its Bluetooth connected callback with
 * dult_multi_user_conn_claim(); the claim is dropped by this layer's own
 * Bluetooth disconnected callback (see disconnected()), so users never release
 * connections explicitly. This keeps the DULT core free of any Bluetooth
 * connection dependency. The table is only exercised with the v2 API; the
 * accessors are reached exclusively from IS_ENABLED(CONFIG_DULT_API_VARIANT_V2)
 * guarded paths.
 */
static const struct dult_user *conn_claims[CONFIG_BT_MAX_CONN];

static const struct dult_user *conn_user_lookup(struct bt_conn *conn)
{
	return conn_claims[bt_conn_index(conn)];
}

int dult_multi_user_conn_claim(const struct dult_user *user, struct bt_conn *conn)
{
	int err;
	uint8_t idx;
	struct bt_conn_info conn_info;

	if (!IS_ENABLED(CONFIG_DULT_API_VARIANT_V2)) {
		return -ENOTSUP;
	}

	if (!user || !conn) {
		return -EINVAL;
	}

	if (!dult_user_is_registered(user)) {
		return -EACCES;
	}

	err = bt_conn_get_info(conn, &conn_info);
	if (err) {
		LOG_ERR("Failed to get conn info %d", err);
		return err;
	}

	if (conn_info.state != BT_CONN_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	idx = bt_conn_index(conn);

	/* Re-claiming by the same user is a no-op; a different user cannot steal
	 * an already claimed connection.
	 */
	if (conn_claims[idx] && (conn_claims[idx] != user)) {
		return -EACCES;
	}

	conn_claims[idx] = user;

	return 0;
}

static bool is_mtu_sufficient(struct bt_conn *conn, uint16_t data_len)
{
	static const size_t att_header_len = 3;
	uint16_t mtu;

	BUILD_ASSERT((ANOS_CHRC_DATA_LEN_MAX + att_header_len) <=
		     MIN(BT_L2CAP_TX_MTU, BT_L2CAP_RX_MTU));

	mtu = bt_gatt_get_mtu(conn);

	return (data_len + att_header_len) <= mtu;
}

static void indicate_params_destroy_cb(struct bt_gatt_indicate_params *params)
{
	k_mem_slab_free(&indicate_params_slab, params);
	LOG_DBG("Indicate params destroy callback");
}

static int gatt_indicate(struct bt_conn *conn,
			 const struct bt_gatt_attr *attr,
			 const void *data,
			 uint16_t data_len)
{
	int err;
	struct bt_gatt_indicate_params *indicate_params;

	if (!is_mtu_sufficient(conn, data_len)) {
		LOG_WRN("Too small MTU to send GATT indication");
		return -EINVAL;
	}

	err = k_mem_slab_alloc(&indicate_params_slab, (void **)&indicate_params, K_NO_WAIT);
	if (err) {
		LOG_ERR("Failed to allocate memory slab: err=%d", err);
		return -ENOMEM;
	}

	*indicate_params = (struct bt_gatt_indicate_params) {
		.uuid = NULL,
		.attr = attr,
		.func = NULL,
		.destroy = indicate_params_destroy_cb,
		.data = data,
		.len = data_len,
	};

	err = bt_gatt_indicate(conn, indicate_params);
	if (err) {
		k_mem_slab_free(&indicate_params_slab, indicate_params);
		LOG_ERR("Failed to send GATT indication: err=%d", err);
		return err;
	}

	return 0;
}

static int process_get_product_data(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				    const struct dult_user *dult_user)
{
	static const uint16_t indication_opcode =
		ANOS_CHRC_ACCESSORY_INFO_INDICATION_OPCODE(
					ANOS_CHRC_ACCESSORY_INFO_WRITE_OPCODE_GET_PRODUCT_DATA);
	NET_BUF_SIMPLE_DEFINE(buf, ANOS_CHRC_INDICATION_LEN(DULT_PRODUCT_DATA_LEN));

	net_buf_simple_add_le16(&buf, indication_opcode);
	net_buf_simple_add_mem(&buf, dult_user->product_data, DULT_PRODUCT_DATA_LEN);

	return gatt_indicate(conn, attr, buf.data, buf.len);
}

static int process_get_manufacturer_name(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					 const struct dult_user *dult_user)
{
	static const uint16_t indication_opcode =
		ANOS_CHRC_ACCESSORY_INFO_INDICATION_OPCODE(
				ANOS_CHRC_ACCESSORY_INFO_WRITE_OPCODE_GET_MANUFACTURER_NAME);
	/* NULL terminating character is not sent. */
	NET_BUF_SIMPLE_DEFINE(buf, ANOS_CHRC_INDICATION_LEN(DULT_USER_STR_PARAM_LEN_MAX));
	size_t name_len;

	name_len = strnlen(dult_user->manufacturer_name, DULT_USER_STR_PARAM_LEN_MAX + 1);
	__ASSERT_NO_MSG((name_len > 0) && (name_len <= DULT_USER_STR_PARAM_LEN_MAX));

	net_buf_simple_add_le16(&buf, indication_opcode);
	net_buf_simple_add_mem(&buf, dult_user->manufacturer_name, name_len);

	return gatt_indicate(conn, attr, buf.data, buf.len);
}

static int process_get_model_name(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				  const struct dult_user *dult_user)
{
	static const uint16_t indication_opcode =
		ANOS_CHRC_ACCESSORY_INFO_INDICATION_OPCODE(
					ANOS_CHRC_ACCESSORY_INFO_WRITE_OPCODE_GET_MODEL_NAME);
	/* NULL terminating character is not sent. */
	NET_BUF_SIMPLE_DEFINE(buf, ANOS_CHRC_INDICATION_LEN(DULT_USER_STR_PARAM_LEN_MAX));
	size_t name_len;

	name_len = strnlen(dult_user->model_name, DULT_USER_STR_PARAM_LEN_MAX + 1);
	__ASSERT_NO_MSG((name_len > 0) && (name_len <= DULT_USER_STR_PARAM_LEN_MAX));

	net_buf_simple_add_le16(&buf, indication_opcode);
	net_buf_simple_add_mem(&buf, dult_user->model_name, name_len);

	return gatt_indicate(conn, attr, buf.data, buf.len);
}

static int process_get_accessory_category(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					  const struct dult_user *dult_user)
{
	static const size_t accessory_category_len = 8;
	static const uint16_t indication_opcode =
		ANOS_CHRC_ACCESSORY_INFO_INDICATION_OPCODE(
				ANOS_CHRC_ACCESSORY_INFO_WRITE_OPCODE_GET_ACCESSORY_CATEGORY);
	NET_BUF_SIMPLE_DEFINE(buf, ANOS_CHRC_INDICATION_LEN(accessory_category_len));
	size_t reserved_len;

	net_buf_simple_add_le16(&buf, indication_opcode);
	net_buf_simple_add_u8(&buf, dult_user->accessory_category);
	/* Fill remaining reserved bytes with zeros. */
	reserved_len = net_buf_simple_tailroom(&buf);
	memset(net_buf_simple_add(&buf, reserved_len), 0, reserved_len);

	return gatt_indicate(conn, attr, buf.data, buf.len);
}

static int process_get_protocol_implementation_version(struct bt_conn *conn,
						       const struct bt_gatt_attr *attr,
						       const struct dult_user *dult_user)
{
	static const uint16_t indication_opcode =
		ANOS_CHRC_ACCESSORY_INFO_INDICATION_OPCODE(
			ANOS_CHRC_ACCESSORY_INFO_WRITE_OPCODE_GET_PROTOCOL_IMPLEMENTATION_VERSION);
	NET_BUF_SIMPLE_DEFINE(buf, ANOS_CHRC_INDICATION_LEN(sizeof(uint32_t)));

	ARG_UNUSED(dult_user);

	net_buf_simple_add_le16(&buf, indication_opcode);
	net_buf_simple_add_le32(&buf, CONFIG_DULT_PROTOCOL_IMPLEMENTATION_VERSION);

	return gatt_indicate(conn, attr, buf.data, buf.len);
}

static int process_get_accessory_capabilities(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					      const struct dult_user *dult_user)
{
	static const uint16_t indication_opcode =
		ANOS_CHRC_ACCESSORY_INFO_INDICATION_OPCODE(
				ANOS_CHRC_ACCESSORY_INFO_WRITE_OPCODE_GET_ACCESSORY_CAPABILITIES);
	NET_BUF_SIMPLE_DEFINE(buf, ANOS_CHRC_INDICATION_LEN(sizeof(uint32_t)));

	net_buf_simple_add_le16(&buf, indication_opcode);
	net_buf_simple_add_le32(&buf, dult_user->accessory_capabilities);

	return gatt_indicate(conn, attr, buf.data, buf.len);
}

static int process_get_network_id(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				  const struct dult_user *dult_user)
{
	static const uint16_t indication_opcode =
		ANOS_CHRC_ACCESSORY_INFO_INDICATION_OPCODE(
					ANOS_CHRC_ACCESSORY_INFO_WRITE_OPCODE_GET_NETWORK_ID);
	NET_BUF_SIMPLE_DEFINE(buf, ANOS_CHRC_INDICATION_LEN(sizeof(uint8_t)));

	net_buf_simple_add_le16(&buf, indication_opcode);
	net_buf_simple_add_u8(&buf, dult_user->network_id);

	return gatt_indicate(conn, attr, buf.data, buf.len);
}

static inline uint32_t version_encode(struct dult_version version)
{
	return ((((uint32_t)(version.major) & 0xFFFF) << 16) |
		(((uint32_t)(version.minor) & 0xFF) << 8) |
		((uint32_t)(version.revision) & 0xFF));
}

static int handle_get_version(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			      enum anos_chrc_accessory_info_write_opcode write_opcode,
			      const struct dult_version version)
{
	const uint16_t indication_opcode =
		ANOS_CHRC_ACCESSORY_INFO_INDICATION_OPCODE(write_opcode);
	NET_BUF_SIMPLE_DEFINE(buf, ANOS_CHRC_INDICATION_LEN(sizeof(uint32_t)));

	net_buf_simple_add_le16(&buf, indication_opcode);
	net_buf_simple_add_le32(&buf, version_encode(version));

	return gatt_indicate(conn, attr, buf.data, buf.len);
}

static int process_get_firmware_version(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					const struct dult_user *dult_user)
{
	return handle_get_version(conn, attr,
				  ANOS_CHRC_ACCESSORY_INFO_WRITE_OPCODE_GET_FIRMWARE_VERSION,
				  dult_user->firmware_version);
}

#if defined(CONFIG_DULT_BATTERY)
static int process_get_battery_type(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				    const struct dult_user *dult_user)
{
	static const uint16_t indication_opcode =
		ANOS_CHRC_ACCESSORY_INFO_INDICATION_OPCODE(
					ANOS_CHRC_ACCESSORY_INFO_WRITE_OPCODE_GET_BATTERY_TYPE);
	NET_BUF_SIMPLE_DEFINE(buf, ANOS_CHRC_INDICATION_LEN(sizeof(uint8_t)));

	ARG_UNUSED(dult_user);

	net_buf_simple_add_le16(&buf, indication_opcode);
	net_buf_simple_add_u8(&buf, dult_battery_type_encode());

	return gatt_indicate(conn, attr, buf.data, buf.len);
}

static enum anos_chrc_cmd_response_status verify_get_battery_level(
	struct bt_conn *conn, const struct dult_user *dult_user)
{
	ARG_UNUSED(conn);

	if (!dult_battery_level_is_set(dult_user)) {
		return ANOS_CHRC_CMD_RESPONSE_STATUS_INVALID_COMMAND;
	}

	return ANOS_CHRC_CMD_RESPONSE_STATUS_SUCCESS;
}

static int process_get_battery_level(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				     const struct dult_user *dult_user)
{
	static const uint16_t indication_opcode =
		ANOS_CHRC_ACCESSORY_INFO_INDICATION_OPCODE(
					ANOS_CHRC_ACCESSORY_INFO_WRITE_OPCODE_GET_BATTERY_LEVEL);
	NET_BUF_SIMPLE_DEFINE(buf, ANOS_CHRC_INDICATION_LEN(sizeof(uint8_t)));

	net_buf_simple_add_le16(&buf, indication_opcode);
	net_buf_simple_add_u8(&buf, dult_battery_level_encode(dult_user));

	return gatt_indicate(conn, attr, buf.data, buf.len);
}
#endif /* defined(CONFIG_DULT_BATTERY) */

static enum anos_chrc_cmd_response_status verify_get_network_version(
	struct bt_conn *conn, const struct dult_user *dult_user)
{
	ARG_UNUSED(conn);

	/* Get_Network_Version is OPTIONAL per the DULT specification. */
	if (!dult_user->network_version) {
		return ANOS_CHRC_CMD_RESPONSE_STATUS_INVALID_COMMAND;
	}

	return ANOS_CHRC_CMD_RESPONSE_STATUS_SUCCESS;
}

static int process_get_network_version(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				       const struct dult_user *dult_user)
{
	/* verify_get_network_version() guarantees network_version is non-NULL. */
	return handle_get_version(conn, attr,
				  ANOS_CHRC_ACCESSORY_INFO_WRITE_OPCODE_GET_NETWORK_VERSION,
				  *dult_user->network_version);
}

static int command_response_send(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				 uint16_t opcode_write, enum anos_chrc_cmd_response_status status)
{
	uint16_t response_status = status;
	static const size_t cmd_response_operand_len = ANOS_CHRC_OPCODE_LEN +
						       sizeof(response_status);
	NET_BUF_SIMPLE_DEFINE(buf, ANOS_CHRC_INDICATION_LEN(cmd_response_operand_len));

	net_buf_simple_add_le16(&buf,
				ANOS_CHRC_NON_OWNER_CONTROL_INDICATION_OPCODE_COMMAND_RESPONSE);
	net_buf_simple_add_le16(&buf, opcode_write);
	net_buf_simple_add_le16(&buf, response_status);

	return gatt_indicate(conn, attr, buf.data, buf.len);
}

static bool capability_verify(const struct dult_user *dult_user,
			      enum dult_accessory_capability capability)
{
	return (dult_user->accessory_capabilities & BIT(capability));
}

static void capability_assert(const struct dult_user *dult_user,
			      enum dult_accessory_capability capability)
{
	__ASSERT_NO_MSG(dult_user->accessory_capabilities & BIT(capability));
}

static enum anos_chrc_cmd_response_status verify_get_id(struct bt_conn *conn,
							const struct dult_user *dult_user)
{
	ARG_UNUSED(conn);

	if (!capability_verify(dult_user, DULT_ACCESSORY_CAPABILITY_ID_LOOKUP_BLE_BIT_POS)) {
		LOG_WRN("Identifier lookup (BLE) capability not supported - "
			"identifier read blocked");
		return ANOS_CHRC_CMD_RESPONSE_STATUS_INVALID_COMMAND;
	}

	if (!dult_id_is_in_read_state()) {
		LOG_WRN("Accessory not in identifier read state - identifier read blocked");
		return ANOS_CHRC_CMD_RESPONSE_STATUS_INVALID_COMMAND;
	}

	return ANOS_CHRC_CMD_RESPONSE_STATUS_SUCCESS;
}

static int process_get_id(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  const struct dult_user *dult_user)
{
	static const size_t payload_len_max = CONFIG_DULT_BT_ANOS_ID_PAYLOAD_LEN_MAX;
	size_t payload_len = payload_len_max;
	int err;

	NET_BUF_SIMPLE_DEFINE(buf, ANOS_CHRC_INDICATION_LEN(payload_len_max));

	ARG_UNUSED(dult_user);

	net_buf_simple_add_le16(&buf,
				ANOS_CHRC_NON_OWNER_CONTROL_INDICATION_OPCODE_GET_ID_RESPONSE);

	err = dult_id_payload_get(net_buf_simple_add(&buf, payload_len_max), &payload_len);
	if (err) {
		LOG_ERR("Unable to get identifier payload: err=%d", err);
		return err;
	}
	__ASSERT_NO_MSG(payload_len <= payload_len_max);
	/* Remove unnecessarily added memory. */
	net_buf_simple_remove_mem(&buf, payload_len_max - payload_len);

	return gatt_indicate(conn, attr, buf.data, buf.len);
}

static int sound_command_response_send(struct bt_conn *conn,
				       const struct bt_gatt_attr *attr,
				       bool sound_start,
				       enum anos_chrc_cmd_response_status response_status)
{
	int err;
	enum anos_chrc_non_owner_control_write_opcode sound_opcode;

	if (sound_start) {
		sound_opcode = ANOS_CHRC_NON_OWNER_CONTROL_WRITE_OPCODE_SOUND_START;
	} else {
		sound_opcode = ANOS_CHRC_NON_OWNER_CONTROL_WRITE_OPCODE_SOUND_STOP;

		__ASSERT_NO_MSG(response_status != ANOS_CHRC_CMD_RESPONSE_STATUS_SUCCESS);
	}

	err = command_response_send(conn, attr, sound_opcode, response_status);
	if (err) {
		LOG_ERR("DULT ANOS: sound %s request: failed to send response: %d "
			"with status: 0x%04X", sound_start ? "start" : "stop",
			err, response_status);
		return err;
	}

	return 0;
}

static enum anos_chrc_cmd_response_status verify_sound_start(struct bt_conn *conn,
							     const struct dult_user *dult_user)
{
	ARG_UNUSED(conn);

	if (IS_ENABLED(CONFIG_DULT_ACCESSORY_TYPE_SMALL)) {
		capability_assert(dult_user, DULT_ACCESSORY_CAPABILITY_PLAY_SOUND_BIT_POS);
	} else {
		if (!capability_verify(dult_user, DULT_ACCESSORY_CAPABILITY_PLAY_SOUND_BIT_POS)) {
			LOG_WRN("Sound capability not supported - sound start blocked");
			return ANOS_CHRC_CMD_RESPONSE_STATUS_INVALID_COMMAND;
		}
	}

	return (anos_sound_state != ANOS_SOUND_STATE_IDLE) ?
		ANOS_CHRC_CMD_RESPONSE_STATUS_INVALID_STATE :
		ANOS_CHRC_CMD_RESPONSE_STATUS_SUCCESS;
}

static int process_sound_start(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			       const struct dult_user *dult_user)
{
	ARG_UNUSED(attr);
	ARG_UNUSED(dult_user);

	/* State is validated in verify_sound_start(). The ANOS GATT write runs to
	 * completion in a single context, so no re-check is needed here.
	 */
	anos_sound_state = ANOS_SOUND_STATE_START_REQUEST;
	sound_conn = conn;

	__ASSERT(anos_sound_cb && anos_sound_cb->sound_start,
		 "DULT ANOS: sound start callback is not populated");

	anos_sound_cb->sound_start();

	return 0;
}

static enum anos_chrc_cmd_response_status verify_sound_stop(struct bt_conn *conn,
							    const struct dult_user *dult_user)
{
	if (IS_ENABLED(CONFIG_DULT_ACCESSORY_TYPE_SMALL)) {
		capability_assert(dult_user, DULT_ACCESSORY_CAPABILITY_PLAY_SOUND_BIT_POS);
	} else {
		if (!capability_verify(dult_user, DULT_ACCESSORY_CAPABILITY_PLAY_SOUND_BIT_POS)) {
			LOG_WRN("Sound capability not supported - sound stop blocked");
			return ANOS_CHRC_CMD_RESPONSE_STATUS_INVALID_COMMAND;
		}
	}

	return ((anos_sound_state != ANOS_SOUND_STATE_START_ACK) || (sound_conn != conn)) ?
		ANOS_CHRC_CMD_RESPONSE_STATUS_INVALID_STATE :
		ANOS_CHRC_CMD_RESPONSE_STATUS_SUCCESS;
}

static int process_sound_stop(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			      const struct dult_user *dult_user)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(attr);
	ARG_UNUSED(dult_user);

	/* State is validated in verify_sound_stop(). The ANOS GATT write runs to
	 * completion in a single context, so no re-check is needed here.
	 */
	anos_sound_state = ANOS_SOUND_STATE_STOP_REQUEST;

	__ASSERT(anos_sound_cb && anos_sound_cb->sound_stop,
		 "DULT ANOS: sound stop callback is not populated");

	anos_sound_cb->sound_stop();

	return 0;
}

static inline void write_accessory_non_owner_exit_log(ssize_t res, struct bt_conn *conn)
{
	LOG_DBG("Accessory non-owner write: res=%d conn=%p", res, (void *)conn);
}

static ssize_t write_accessory_non_owner_err_to_att_err_map(int err)
{
	__ASSERT_NO_MSG(err);

	if (err == -ENOMEM) {
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	} else {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}
}

static const struct anos_opcode_entry anos_opcode_map[] = {
	ANOS_ACCESSORY_INFO_OPCODE_ENTRY(GET_PRODUCT_DATA,
					 NULL, process_get_product_data),
	ANOS_ACCESSORY_INFO_OPCODE_ENTRY(GET_MANUFACTURER_NAME,
					 NULL, process_get_manufacturer_name),
	ANOS_ACCESSORY_INFO_OPCODE_ENTRY(GET_MODEL_NAME,
					 NULL, process_get_model_name),
	ANOS_ACCESSORY_INFO_OPCODE_ENTRY(GET_ACCESSORY_CATEGORY,
					 NULL, process_get_accessory_category),
	ANOS_ACCESSORY_INFO_OPCODE_ENTRY(GET_PROTOCOL_IMPLEMENTATION_VERSION,
					 NULL, process_get_protocol_implementation_version),
	ANOS_ACCESSORY_INFO_OPCODE_ENTRY(GET_ACCESSORY_CAPABILITIES,
					 NULL, process_get_accessory_capabilities),
	ANOS_ACCESSORY_INFO_OPCODE_ENTRY(GET_NETWORK_ID,
					 NULL, process_get_network_id),
	ANOS_ACCESSORY_INFO_OPCODE_ENTRY(GET_FIRMWARE_VERSION,
					 NULL, process_get_firmware_version),
#if defined(CONFIG_DULT_BATTERY)
	ANOS_ACCESSORY_INFO_OPCODE_ENTRY(GET_BATTERY_TYPE,
					 NULL, process_get_battery_type),
	ANOS_ACCESSORY_INFO_OPCODE_ENTRY(GET_BATTERY_LEVEL,
					 verify_get_battery_level, process_get_battery_level),
#endif /* defined(CONFIG_DULT_BATTERY) */
	ANOS_ACCESSORY_INFO_OPCODE_ENTRY(GET_NETWORK_VERSION,
					 verify_get_network_version, process_get_network_version),
	ANOS_NON_OWNER_CONTROL_OPCODE_ENTRY(SOUND_START,
					    verify_sound_start, process_sound_start),
	ANOS_NON_OWNER_CONTROL_OPCODE_ENTRY(SOUND_STOP,
					    verify_sound_stop, process_sound_stop),
	ANOS_NON_OWNER_CONTROL_OPCODE_ENTRY(GET_ID,
					    verify_get_id, process_get_id),
};

static const struct anos_opcode_entry *opcode_entry_get(uint16_t opcode)
{
	ARRAY_FOR_EACH(anos_opcode_map, i) {
		if (anos_opcode_map[i].opcode == opcode) {
			return &anos_opcode_map[i];
		}
	}

	return NULL;
}

static ssize_t write_accessory_non_owner_reject(struct bt_conn *conn,
						const struct bt_gatt_attr *attr,
						uint16_t opcode_write,
						uint16_t write_len,
						enum anos_chrc_cmd_response_status status)
{
	ssize_t res = write_len;
	int err;

	err = command_response_send(conn, attr, opcode_write, status);
	if (err) {
		res = write_accessory_non_owner_err_to_att_err_map(err);
	}

	write_accessory_non_owner_exit_log(res, conn);
	return res;
}

static bool near_owner_state_is_separated(void)
{
	return dult_near_owner_state_get() == DULT_NEAR_OWNER_STATE_MODE_SEPARATED;
}

static ssize_t write_accessory_non_owner(struct bt_conn *conn,
					 const struct bt_gatt_attr *attr,
					 const void *buf,
					 uint16_t len,
					 uint16_t offset,
					 uint8_t flags)
{
	int err = 0;
	ssize_t res = len;
	uint16_t opcode_write;
	const struct anos_opcode_entry *entry;
	const struct dult_user *dult_user;
	const struct dult_bt_anos_cb *anos_cb;
	bool anos_access_verify_cb_present;

	/* Lazily cache the ANOS attribute so writes arriving during the
	 * pre-association window can be handled. With the v1 API the not-enabled
	 * guard below rejects pre-enable writes anyway.
	 */
	if (!anos_chrc_indicate_attr) {
		anos_chrc_indicate_attr = attr;
	}

	if (!IS_ENABLED(CONFIG_DULT_API_VARIANT_V2) && !dult_is_any_associated()) {
		res = BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		LOG_INF("Accessory non-owner write: res=%d conn=%p, "
			"Return error because DULT is not enabled", res, (void *)conn);
		return res;
	}

	__ASSERT_NO_MSG(attr == anos_chrc_indicate_attr);

	if (offset != 0) {
		LOG_WRN("Invalid offset: off=%" PRIu16 " (Accessory non-owner write)", offset);
		res = BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
		write_accessory_non_owner_exit_log(res, conn);
		return res;
	}

	if (len != sizeof(opcode_write)) {
		LOG_WRN("Invalid length: len=%" PRIu16 " (Accessory non-owner write)", len);
		res = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		write_accessory_non_owner_exit_log(res, conn);
		return res;
	}

	/* Resolve the user: the associated user first, then (v2 only) the
	 * connection lookup for the pre-association window. With the v1 API there
	 * is always an associated user here thanks to the not-enabled guard above.
	 */
	dult_user = dult_user_get_associated();
	if (!dult_user && IS_ENABLED(CONFIG_DULT_API_VARIANT_V2)) {
		dult_user = conn_user_lookup(conn);
	}

	if (!dult_user) {
		res = BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		LOG_WRN("Accessory non-owner write: res=%d conn=%p, "
			"Return error because DULT user cannot be resolved",
			res, (void *)conn);
		return res;
	}

	/* The ANOS access callback is per-user: consult the callback bound to
	 * the resolved user. NULL means the built-in fallback policy applies.
	 */
	anos_cb = anos_cb_get(dult_user);
	anos_access_verify_cb_present = anos_cb && anos_cb->access_verify;

	opcode_write = sys_get_le16(buf);
	LOG_DBG("Received following opcode: %#05X (Accessory non-owner write)", opcode_write);

	/* Reject opcodes that are not defined by the DULT specification early,
	 * without consulting the access_verify callback. The DULT specification
	 * mandates the Invalid_command status for any opcode outside the
	 * defined set.
	 */
	entry = opcode_entry_get(opcode_write);
	if (!entry) {
		LOG_INF("Unknown opcode: %#05X (Accessory non-owner write)", opcode_write);
		return write_accessory_non_owner_reject(
			conn, attr, opcode_write, len,
			ANOS_CHRC_CMD_RESPONSE_STATUS_INVALID_COMMAND);
	}

	if (anos_access_verify_cb_present) {
		/* The Non-owner Control opcodes always remain gated on the separated near-owner
		 * state.
		 */
		if ((entry->group == DULT_BT_ANOS_OPCODE_GROUP_NON_OWNER_CONTROL) &&
		    !near_owner_state_is_separated()) {
			LOG_WRN("Non-owner control blocked outside separated state: opcode=%#05X "
				"(Accessory non-owner write)",
				opcode_write);
			return write_accessory_non_owner_reject(
				conn, attr, opcode_write, len,
				ANOS_CHRC_CMD_RESPONSE_STATUS_INVALID_COMMAND);
		}
	} else if (!near_owner_state_is_separated()) {
		/* Reject every opcode outside the separated state. */
		LOG_WRN("Invalid near-owner state mode: opcode=%#05X (Accessory non-owner write)",
			opcode_write);
		return write_accessory_non_owner_reject(
			conn, attr, opcode_write, len,
			ANOS_CHRC_CMD_RESPONSE_STATUS_INVALID_COMMAND);
	}

	/* Opcode-specific precondition check. On failure the peer is informed with
	 * a Command Response indication carrying the reported status.
	 */
	if (entry->verify) {
		enum anos_chrc_cmd_response_status status = entry->verify(conn, dult_user);

		if (status != ANOS_CHRC_CMD_RESPONSE_STATUS_SUCCESS) {
			return write_accessory_non_owner_reject(conn, attr, opcode_write, len,
								status);
		}
	}

	/* The OPTIONAL access_verify callback is the last gate before the opcode is processed. */
	if (anos_access_verify_cb_present && !anos_cb->access_verify(conn)) {
		LOG_WRN("ANOS access denied by access_verify callback: opcode=%#05X "
			"(Accessory non-owner write)",
			opcode_write);
		return write_accessory_non_owner_reject(
			conn, attr, opcode_write, len,
			ANOS_CHRC_CMD_RESPONSE_STATUS_INVALID_STATE);
	}

	err = entry->process(conn, attr, dult_user);
	if (err == -ENOMEM) {
		/* Do not send a Command Response to avoid overloading the indication buffer. */
		LOG_WRN("No more indication buffers available (Accessory non-owner write)");
	} else if (err) {
		LOG_WRN("Sending indication failed: err=%d (Accessory non-owner write)", err);
	}

	if (err) {
		res = write_accessory_non_owner_err_to_att_err_map(err);
	}

	write_accessory_non_owner_exit_log(res, conn);
	return res;
}

BT_GATT_SERVICE_DEFINE(dult_accessory_non_owner_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_ACCESSORY_NON_OWNER_SERVICE),
	BT_GATT_CHARACTERISTIC(BT_UUID_ACCESSORY_NON_OWNER_CHARACTERISTIC,
		BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE,
		BT_GATT_PERM_WRITE,
		NULL, write_accessory_non_owner, NULL),
	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

static void sound_state_reset(void)
{
	sound_conn = NULL;
	anos_sound_state = ANOS_SOUND_STATE_IDLE;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (conn == sound_conn) {
		sound_state_reset();
	}

	if (IS_ENABLED(CONFIG_DULT_API_VARIANT_V2)) {
		/* Drop any per-connection user claim recorded for ANOS routing so
		 * the connection index can be safely reused. This is the single
		 * cleanup point for the conn_claims table; DULT users only claim
		 * connections and never release them explicitly.
		 */
		conn_claims[bt_conn_index(conn)] = NULL;
	}
}

BT_CONN_CB_DEFINE(beacon_actions_conn_callbacks) = {
	.disconnected = disconnected,
};

static void sound_start_notify(bool native)
{
	enum anos_chrc_cmd_response_status response_status;
	struct bt_conn *conn = sound_conn;

	if (anos_sound_state == ANOS_SOUND_STATE_IDLE) {
		/* The DULT GATT layer does not own this sound action:
		 * No need to perform further actions.
		 */
		return;
	}

	if ((anos_sound_state == ANOS_SOUND_STATE_START_ACK) ||
	    (anos_sound_state == ANOS_SOUND_STATE_STOP_REQUEST)) {
		/* The DULT GATT layer sound action was overridden by the external source
		 * when the it was already acknowledged by the ANOS module.
		 */
		__ASSERT(!native,
			 "DULT ANOS: native sound cannot reclaim the external sound action");

		anos_sound_state = ANOS_SOUND_STATE_EXTERNAL_TAKEOVER;

		return;
	}

	__ASSERT(anos_sound_state != ANOS_SOUND_STATE_EXTERNAL_TAKEOVER,
		 "DULT ANOS: invalid state: sound start transition skipped");

	if (native) {
		response_status = ANOS_CHRC_CMD_RESPONSE_STATUS_SUCCESS;
		anos_sound_state = ANOS_SOUND_STATE_START_ACK;
	} else {
		/* The DULT GATT layer sound start request was overidden
		 * by the external sound source.
		 */
		response_status = ANOS_CHRC_CMD_RESPONSE_STATUS_INVALID_STATE;
		sound_state_reset();
	}

	/* Connection object cannot be NULL if we are not in the IDLE state at the
	 * beginning of this function.
	 */
	__ASSERT_NO_MSG(conn);

	/* Cached by the sound-start write that drives this notification. */
	__ASSERT_NO_MSG(anos_chrc_indicate_attr);

	(void) sound_command_response_send(conn, anos_chrc_indicate_attr, true, response_status);
}

static void sound_completed_indicate(struct bt_conn *conn, const struct bt_gatt_attr *attr)
{
	int err;
	static const uint16_t indication_opcode =
		ANOS_CHRC_NON_OWNER_CONTROL_INDICATION_OPCODE_SOUND_COMPLETED;
	NET_BUF_SIMPLE_DEFINE(buf, ANOS_CHRC_INDICATION_LEN(0));

	net_buf_simple_add_le16(&buf, indication_opcode);

	err = gatt_indicate(conn, attr, buf.data, buf.len);
	if (err) {
		LOG_ERR("DULT ANOS: sound completed indicate: failed to send response: %d",
			err);
	}
}

static void sound_stop_notify(void)
{
	if (anos_sound_state == ANOS_SOUND_STATE_IDLE) {
		/* The DULT GATT layer does not own this sound action:
		 * No need to perform further actions.
		 */
		return;
	}

	__ASSERT(anos_sound_state != ANOS_SOUND_STATE_START_REQUEST,
		 "DULT ANOS: invalid state: sound start transition skipped");

	/* Connection object cannot be NULL if we are not in the IDLE state at the
	 * beginning of this function.
	 */
	__ASSERT_NO_MSG(sound_conn);

	/* Cached by the sound-start write that drives this notification. */
	__ASSERT_NO_MSG(anos_chrc_indicate_attr);

	sound_completed_indicate(sound_conn, anos_chrc_indicate_attr);

	sound_state_reset();
}

void dult_bt_anos_sound_state_change_notify(bool active, bool native)
{
	if (active) {
		sound_start_notify(native);
	} else {
		sound_stop_notify();
	}
}

void dult_bt_anos_sound_cb_register(const struct dult_bt_anos_sound_cb *cb)
{
	__ASSERT(!anos_sound_cb,
		 "DULT ANOS: sound callback already registered");
	__ASSERT(cb && cb->sound_start && cb->sound_stop,
		 "DULT ANOS: input callback structure with invalid parameters");

	anos_sound_cb = cb;
}

int dult_bt_anos_cb_register(const struct dult_user *user, const struct dult_bt_anos_cb *cb)
{
	int err;

	if (!user) {
		return -EINVAL;
	}

	if (!cb || !cb->access_verify) {
		return -EINVAL;
	}

	if (!dult_user_is_registered(user)) {
		return -EACCES;
	}

	if (!IS_ENABLED(CONFIG_DULT_API_VARIANT_V2)) {
		/* v1 API: callbacks may only be registered while DULT is disabled. */
		if (dult_is_any_associated()) {
			LOG_ERR("DULT ANOS: module must be disabled to register callbacks");
			return -EACCES;
		}
	}

	if (anos_cb_get(user)) {
		LOG_ERR("DULT ANOS: callback already registered");
		return -EALREADY;
	}

	if (anos_cb_id == DULT_USER_SLOT_MEM_REF_ID_UNSET) {
		err = dult_user_slot_mem_ref_register(&anos_cb_id);
		if (err) {
			return err;
		}
	}

	/* Stored in the user's slot and consulted for both phases. In multi-user
	 * builds it persists across dult_reset() and is cleared by
	 * dult_user_unregister().
	 */
	return anos_cb_set(user, cb);
}

int dult_bt_anos_reset(void)
{
	if (anos_sound_state != ANOS_SOUND_STATE_IDLE) {
		/* Connection object cannot be NULL if we are not in the IDLE state at the
		 * beginning of this function.
		 */
		__ASSERT_NO_MSG(sound_conn);
		__ASSERT_NO_MSG(anos_chrc_indicate_attr);

		if (anos_sound_state == ANOS_SOUND_STATE_START_REQUEST) {
			(void) sound_command_response_send(
				sound_conn,
				anos_chrc_indicate_attr,
				true,
				ANOS_CHRC_CMD_RESPONSE_STATUS_INVALID_STATE);
		} else {
			sound_completed_indicate(sound_conn, anos_chrc_indicate_attr);
		}

		sound_state_reset();
	}

	return 0;
}
