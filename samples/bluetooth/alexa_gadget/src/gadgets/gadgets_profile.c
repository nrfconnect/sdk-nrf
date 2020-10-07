/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include "gadgets_profile.h"

#include <drivers/hwinfo.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/services/gadgets.h>
#include <tinycrypt/constants.h>
#include <tinycrypt/sha256.h>
#include <sys/byteorder.h>

#include <stdio.h>

#include "pb_encode.h"
#include "pb_decode.h"
#include "accessories.pb.h"
#include "directiveParser.pb.h"
#include "alexaDiscoveryDiscoverResponseEvent.pb.h"
#include "alexaDiscoveryDiscoverResponseEventPayload.pb.h"

#if CONFIG_GADGETS_CAPABILITY_CUSTOM_ENABLE
#include "custom_event.pb.h"
#endif

#include <logging/log.h>
LOG_MODULE_REGISTER(gadgets_profile, CONFIG_GADGETS_PROFILE_LOG_LEVEL);

#if IS_ENABLED(CONFIG_GADGETS_OTA_ENABLE)
#error OTA Not yet supported
#endif

#define GADGETS_FEATURE_FLAGS \
	(BIT(0)) |	      \
	(BIT(1) * IS_ENABLED(CONFIG_GADGETS_OTA_ENABLE))

/* DSN = Device Serial Number */
#define GADGETS_DSN_LENGTH_BYTES 8
#define GADGETS_DSN_LENGTH_CHARS (GADGETS_DSN_LENGTH_BYTES * 2)
/* Token = Unique device token, derived from DSN and Gadget Secret value */
#define GADGETS_TOKEN_LEN_BYTES TC_SHA256_DIGEST_SIZE
#define GADGETS_TOKEN_LEN_CHARS (GADGETS_TOKEN_LEN_BYTES * 2)
/* PV = Protocol Version */
#define GADGETS_PV_LEN_BYTES 20
#define GADGETS_PV_IDENTIFIER 0xFE03
#define GADGETS_PV_MAJOR_VERSION 0x03
#define GADGETS_PV_MINOR_VERSION 0x00

/* Number of supported Gadget capabilities */
#define GADGETS_CAP_COUNT					      \
	(IS_ENABLED(CONFIG_GADGETS_CAPABILITY_STATELISTENER_ENABLE) + \
	 IS_ENABLED(CONFIG_GADGETS_CAPABILITY_ALERTS_ENABLE) +	      \
	 IS_ENABLED(CONFIG_GADGETS_CAPABILITY_NOTIFICATIONS_ENABLE) + \
	 IS_ENABLED(CONFIG_GADGETS_CAPABILITY_MUSICDATA_ENABLE) +     \
	 IS_ENABLED(CONFIG_GADGETS_CAPABILITY_SPEECHDATA_ENABLE) +    \
	 IS_ENABLED(CONFIG_GADGETS_CAPABILITY_CUSTOM_ENABLE))

#define GADGETS_CAP_DSN_PLACEHOLDER "DSN PLACEHOLDER-"
#define GADGETS_CAP_DEV_TOKEN_PLACEHOLDER \
	"SIXTY-FOUR CHARACTER PLACEHOLDER VALUE FOR DEVICE TOKEN---------"

BUILD_ASSERT(sizeof(GADGETS_CAP_DSN_PLACEHOLDER) ==
	     (GADGETS_DSN_LENGTH_CHARS + 1));
BUILD_ASSERT(sizeof(GADGETS_CAP_DEV_TOKEN_PLACEHOLDER) ==
	     (GADGETS_TOKEN_LEN_CHARS + 1));

static int directive_handler_discovery(
	struct bt_conn *conn, const char *name,
	const uint8_t *data, uint16_t len);
#if CONFIG_GADGETS_CAPABILITY_MUSICDATA_ENABLE
static int directive_handler_musicdata(
	struct bt_conn *conn, const char *name,
	const uint8_t *data, uint16_t len);
#endif
#if CONFIG_GADGETS_CAPABILITY_SPEECHDATA_ENABLE
static int directive_handler_speechdata(
	struct bt_conn *conn, const char *name,
	const uint8_t *data, uint16_t len);
#endif
#if CONFIG_GADGETS_CAPABILITY_STATELISTENER_ENABLE
static int directive_handler_statelistener(
	struct bt_conn *conn, const char *name,
	const uint8_t *data, uint16_t len);
#endif
#if CONFIG_GADGETS_CAPABILITY_NOTIFICATIONS_ENABLE
static int directive_handler_notifications(
	struct bt_conn *conn, const char *name,
	const uint8_t *data, uint16_t len);
#endif
#if CONFIG_GADGETS_CAPABILITY_ALERTS_ENABLE
static int directive_handler_alerts(
	struct bt_conn *conn, const char *name,
	const uint8_t *data, uint16_t len);
#endif
#if CONFIG_GADGETS_CAPABILITY_CUSTOM_ENABLE
static int directive_handler_custom(
	struct bt_conn *conn, const char *name,
	const uint8_t *data, uint16_t len);
#endif


#ifndef CONFIG_GADGETS_CAPABILITY_CUSTOM_NAMESPACE
/* Dummy value for handler list below */
#define CONFIG_GADGETS_CAPABILITY_CUSTOM_NAMESPACE "-"
#endif

/* struct array containing directive namespaces with corresponding handlers */
static const struct {
	const char *namespace;
	int (*handler)(struct bt_conn *conn, const char *name,
		       const uint8_t *data, uint16_t len);
} namespace_handlers[] = {
	{.namespace = "Alexa.Discovery", .handler = directive_handler_discovery},
#if CONFIG_GADGETS_CAPABILITY_MUSICDATA_ENABLE
	{.namespace = "Alexa.Gadget.MusicData", .handler = directive_handler_musicdata},
#endif
#if CONFIG_GADGETS_CAPABILITY_SPEECHDATA_ENABLE
	{.namespace = "Alexa.Gadget.SpeechData", .handler = directive_handler_speechdata},
#endif
#if CONFIG_GADGETS_CAPABILITY_STATELISTENER_ENABLE
	{.namespace = "Alexa.Gadget.StateListener", .handler = directive_handler_statelistener},
#endif
#if CONFIG_GADGETS_CAPABILITY_NOTIFICATIONS_ENABLE
	{.namespace = "Notifications", .handler = directive_handler_notifications},
#endif
#if CONFIG_GADGETS_CAPABILITY_ALERTS_ENABLE
	{.namespace = "Alerts", .handler = directive_handler_alerts},
#endif
#if CONFIG_GADGETS_CAPABILITY_CUSTOM_ENABLE
	{.namespace = CONFIG_GADGETS_CAPABILITY_CUSTOM_NAMESPACE, .handler = directive_handler_custom},
#endif
};

/* Hardcoded template used to save a significant amount of RAM (~14 KiB).
 * This could also be done as a pre-generated protobuf encoded byte array
 * to also save flash memory.
 * It is currently done this way for increased readability compared to
 * magic protobuf encoded array, as well as allowing configuration options
 * via kconfig without additional build scripts.
 * This struct contains two dynamic fields: serial number and device token.
 */
static const alexaDiscovery_DiscoverResponseEventProto
	discovery_response_template = {
	.has_event = true,
	.event.has_header = true,
	.event.header.namespace = "Alexa.Discovery",
	.event.header.name = "Discover.Response",
	.event.has_payload = true,
	.event.payload.endpoints_count = 1,
	.event.payload.endpoints = { {
		/* Placeholder for unique serial number */
		.endpointId = GADGETS_CAP_DSN_PLACEHOLDER,
		.friendlyName = CONFIG_BT_DEVICE_NAME,
		.description = CONFIG_GADGETS_DEVICE_DESCRIPTION,

		/* List of configurable capabilities */
		.capabilities_count = GADGETS_CAP_COUNT,
		.capabilities = {
#if CONFIG_GADGETS_CAPABILITY_STATELISTENER_ENABLE
			{
				.type = "AlexaInterface",
				.interface = "Alexa.Gadget.StateListener",
				.version = "1.0",
				.has_configuration = true,
				.configuration.supportedTypes = {
					{ .name = "timeinfo" },
					{ .name = "timers" },
					{ .name = "alarms" },
					{ .name = "reminders" },
					{ .name = "wakeword" },
				},
				.configuration.supportedTypes_count = 5,
			},
#endif

#if CONFIG_GADGETS_CAPABILITY_ALERTS_ENABLE
			{
				.type = "AlexaInterface",
				.interface = "Alerts",
				.version = "1.1",
				.has_configuration = true,
				.configuration.supportedTypes_count = 0,
			},
#endif

#if CONFIG_GADGETS_CAPABILITY_NOTIFICATIONS_ENABLE
			{
				.type = "AlexaInterface",
				.interface = "Notifications",
				.version = "1.0",
				.has_configuration = true,
				.configuration.supportedTypes_count = 0,
			},
#endif

#if CONFIG_GADGETS_CAPABILITY_MUSICDATA_ENABLE
			{
				.type = "AlexaInterface",
				.interface = "Alexa.Gadget.MusicData",
				.version = "1.0",
				.has_configuration = true,
				.configuration.supportedTypes[0].name = "tempo",
				.configuration.supportedTypes_count = 1,
			},
#endif

#if CONFIG_GADGETS_CAPABILITY_SPEECHDATA_ENABLE
			{
				.type = "AlexaInterface",
				.interface = "Alexa.Gadget.SpeechData",
				.version = "1.0",
				.has_configuration = true,
				.configuration.supportedTypes[0].name = "viseme",
				.configuration.supportedTypes_count = 1,
			},
#endif

#if CONFIG_GADGETS_CAPABILITY_CUSTOM_ENABLE
			{
				.type = "AlexaInterface",
				.interface = CONFIG_GADGETS_CAPABILITY_CUSTOM_NAMESPACE,
				.version = "1.0",
				.has_configuration = true,
				.configuration.supportedTypes_count = 0,
			},
#endif
		},

		.has_additionalIdentification = true,
		.additionalIdentification = {
			.firmwareVersion = "1",
			.deviceToken = GADGETS_CAP_DEV_TOKEN_PLACEHOLDER,
			.deviceTokenEncryptionType = "1",
			.amazonDeviceType = CONFIG_GADGETS_AMAZON_ID,
			.modelName = CONFIG_GADGETS_MODEL_NAME,
			.radioAddress = "default",
		},
	} },
};

/* Users of shared buffer */
enum buf_user {
	BUF_USER_NONE,
	/* Incoming control messages and outgoing replies */
	BUF_USER_CONTROL_STREAM,
	/* Incoming Alexa messages and outgoing replies */
	BUF_USER_ALEXA_STREAM,
	/* Outgoing custom events (via Alexa stream) */
	BUF_USER_CUSTOM_EVT,
};

/* protobuf_encoded must be large enough to hold the discovery reply message.
 * This size of this message depends on the configured capability count.
 * Note that this buffer is used for both incoming and outgoing data.
 * Incoming data must be parsed before encoding the outgoing message
 */
static uint8_t protobuf_encoded[CONFIG_GADGETS_TRANSACTION_BUF_SIZE];
static atomic_t protobuf_encoded_user;
static size_t protobuf_offset;

/* Buffer to hold decoded protobuf structs */
static union {
	/* Control messages */
	ControlEnvelope control_envelope;

	/* Top-level directive */
	directive_DirectiveParserProto directive_parser;

#if CONFIG_GADGETS_CAPABILITY_NOTIFICATIONS_ENABLE
	/* Indications */
	notifications_ClearIndicatorDirectiveProto clear_indicator;
	notifications_SetIndicatorDirectiveProto set_indicator;
#endif

#if CONFIG_GADGETS_CAPABILITY_ALERTS_ENABLE
	/* Alerts */
	alerts_SetAlertDirectiveProto set_alert;
	alerts_DeleteAlertDirectiveProto delete_alert;
#endif


#if CONFIG_GADGETS_CAPABILITY_STATELISTENER_ENABLE
	/* State listener updates */
	alexaGadgetStateListener_StateUpdateDirectiveProto state_update;
#endif

#if CONFIG_GADGETS_CAPABILITY_SPEECHDATA_ENABLE
	/* Speech marks */
	alexaGadgetSpeechData_SpeechmarksDirectiveProto speech_marks;
#endif

#if CONFIG_GADGETS_CAPABILITY_MUSICDATA_ENABLE
	/* Music data */
	alexaGadgetMusicData_TempoDirectiveProto music_tempo;
#endif

#if CONFIG_GADGETS_CAPABILITY_CUSTOM_ENABLE
	custom_event_proto custom_event;
#endif
} protobuf_decoded;

/* Advertising data format as described in Alexa Gadgets tech docs */

/* Scan response payload used when bondable. */
#define SD_VAL_PAIRING\
	0x03, 0xFE, (CONFIG_BT_COMPANY_ID & 0xFF),\
	(CONFIG_BT_COMPANY_ID >> 8 & 0xFF), 0x00, 0xFF, 0x00, 0x01, 0x00, 0x00,\
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

/* Scan response payload used when reconnecting to bonded peer. */
#define SD_VAL_RECONNECT\
	0x03, 0xFE, (CONFIG_BT_COMPANY_ID & 0xFF),\
	(CONFIG_BT_COMPANY_ID >> 8 & 0xFF), 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00,\
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
	0x00, 0x00, 0x00,

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
		(sizeof(CONFIG_BT_DEVICE_NAME) - 1)),
};

static const struct bt_data sd_pairing[] = {
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      BT_UUID_16_ENCODE(BT_UUID_GADGETS_SHORT_VAL)),
	BT_DATA_BYTES(BT_DATA_SVC_DATA16, SD_VAL_PAIRING),
};

static const struct bt_data sd_reconnect[] = {
	BT_DATA_BYTES(BT_DATA_SVC_DATA16, SD_VAL_RECONNECT),
};

static struct {
	struct bt_conn *conn;
	uint8_t cccd_enabled : 1;
	uint8_t encrypted : 1;
	uint8_t mtu_exchanged : 1;
} conn_state;

static gadgets_profile_cb_t callback;

static bool gadgets_alexa_stream_cb(
	struct bt_conn *conn, const uint8_t *const data, uint16_t len,
	bool more_data);
static bool gadgets_control_stream_cb(
	struct bt_conn *conn, const uint8_t *const data, uint16_t len,
	bool more_data);
static void gadgets_sent_cb(
	struct bt_conn *conn, const void *buf, bool success);
static void gadgets_cccd_cb(bool enabled);

static void connected(struct bt_conn *conn, uint8_t err);
static void disconnected(struct bt_conn *conn, uint8_t reason);
static void security_changed(
	struct bt_conn *conn, bt_security_t level, enum bt_security_err err);
static void pairing_confirm(struct bt_conn *conn);
static void exchange_func(
	struct bt_conn *conn, uint8_t err, struct bt_gatt_exchange_params *params);

static struct bt_gadgets_cb gadgets_service_cb = {
	.control_stream_cb = gadgets_control_stream_cb,
	.alexa_stream_cb = gadgets_alexa_stream_cb,
	.sent_cb = gadgets_sent_cb,
	.ccc_cb = gadgets_cccd_cb,
};

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.pairing_confirm = pairing_confirm,
};

static struct bt_gatt_exchange_params exchange_params = {
	.func = exchange_func,
};

/* strstr that ignores null characters in str1 before len is reached.
 * str2 must be properly null terminated
 */
static char *strnstr(const char *str1, size_t len, const char *str2)
{
	size_t len2 = strlen(str2);

	if (len2 > len) {
		return NULL;
	}

	for (int i = 0; i < (len - len2); ++i) {
		if (memcmp(&str1[i], str2, len2) == 0) {
			return (char *) &str1[i];
		}
	}

	return NULL;
}

static void serial_number_string_get(char *serial_number)
{
	uint8_t device_id[GADGETS_DSN_LENGTH_BYTES];
	ssize_t device_id_len;
	int len;

	device_id_len = hwinfo_get_device_id(device_id, sizeof(device_id));
	__ASSERT_NO_MSG(device_id_len == GADGETS_DSN_LENGTH_BYTES);

	/* Expand binary device ID to hex string */
	len = bin2hex(device_id, sizeof(device_id), serial_number,
		      GADGETS_DSN_LENGTH_CHARS + 1);
	__ASSERT_NO_MSG(len == GADGETS_DSN_LENGTH_CHARS);

	LOG_DBG("Serial: %s", log_strdup(serial_number));
}

static void device_token_get(char *device_token)
{
	struct tc_sha256_state_struct sha_ctx;
	uint8_t sha_input[sizeof(CONFIG_GADGETS_DEVICE_SECRET) +
			  GADGETS_DSN_LENGTH_CHARS + 1];
	uint8_t sha_digest[TC_SHA256_DIGEST_SIZE];
	size_t pos;
	size_t len;
	int err;

	/* Device token is the SHA256 digest of serial number + device secret */

	pos = 0;

	serial_number_string_get(sha_input);
	pos += GADGETS_DSN_LENGTH_CHARS;
	strcpy(&sha_input[pos], CONFIG_GADGETS_DEVICE_SECRET);
	pos += strlen(CONFIG_GADGETS_DEVICE_SECRET);

	err = tc_sha256_init(&sha_ctx);
	__ASSERT_NO_MSG(err == TC_CRYPTO_SUCCESS);

	err = tc_sha256_update(&sha_ctx, sha_input, pos);
	__ASSERT_NO_MSG(err == TC_CRYPTO_SUCCESS);

	err = tc_sha256_final(sha_digest, &sha_ctx);
	__ASSERT_NO_MSG(err == TC_CRYPTO_SUCCESS);

	len = bin2hex(sha_digest, sizeof(sha_digest), device_token,
		      (sizeof(sha_digest) * 2 + 1));
	__ASSERT_NO_MSG(len == (TC_SHA256_DIGEST_SIZE * 2));

	LOG_DBG("Device token: %s", log_strdup(device_token));
}

static int gadgets_command_respond(struct bt_conn *conn)
{
	ControlEnvelope control_response;
	Response *response;
	DeviceInformation *dev_info;
	DeviceFeatures *dev_features;
	bool success;

	pb_istream_t stream_in =
		pb_istream_from_buffer(protobuf_encoded, protobuf_offset);

	success = pb_decode(
		&stream_in,
		&ControlEnvelope_msg,
		&protobuf_decoded.control_envelope);
	if (!success) {
		LOG_WRN("pb_decode control failed");
		return -EINVAL;
	}

	memset(&control_response, 0, sizeof(control_response));
	control_response.command = Command_NONE;
	control_response.which_payload = ControlEnvelope_response_tag;

	response = &control_response.payload.response;
	response->error_code = BT_GADGETS_RESULT_CODE_SUCCESS;

	switch (protobuf_decoded.control_envelope.command) {
	case Command_NONE:
		LOG_DBG("Command_NONE");
		break;
	case Command_GET_DEVICE_INFORMATION:
		LOG_DBG("Command_GET_DEVICE_INFORMATION");
		dev_info = &response->payload.device_information;

		serial_number_string_get(dev_info->serial_number);
		strncpy(dev_info->name,
			CONFIG_BT_DEVICE_NAME,
			sizeof(dev_info->name));
		strncpy(dev_info->device_type,
			CONFIG_GADGETS_AMAZON_ID,
			sizeof(dev_info->device_type));
		dev_info->supported_transports[0] =
			Transport_BLUETOOTH_LOW_ENERGY;
		dev_info->supported_transports_count = 1;

		response->which_payload = Response_device_information_tag;
		break;
	case Command_GET_DEVICE_FEATURES:
		LOG_DBG("Command_GET_DEVICE_FEATURES");
		dev_features = &response->payload.device_features;

		dev_features->device_attributes = GADGETS_FEATURE_FLAGS;
		dev_features->device_attributes = 0; /* Reserved */

		response->which_payload = Response_device_features_tag;
		break;
#if CONFIG_GADGETS_OTA_ENABLE
	case Command_UPDATE_COMPONENT_SEGMENT:
		/* Not yet supported */
		LOG_DBG("Command_UPDATE_COMPONENT_SEGMENT");
		break;
	case Command_APPLY_FIRMWARE:
		/* Not yet supported */
		LOG_DBG("Command_APPLY_FIRMWARE");
		break;
#endif
	default:
		LOG_WRN("Unrecognized command: 0x%02X",
			protobuf_decoded.control_envelope.command);
		response->error_code = BT_GADGETS_RESULT_CODE_UNSUPPORTED;
		response->which_payload = Response_error_code_tag;
		break;
	}

	pb_ostream_t stream_out =
		pb_ostream_from_buffer(protobuf_encoded,
				       sizeof(protobuf_encoded));
	success = pb_encode(&stream_out, &ControlEnvelope_msg,
			    &control_response);
	if (!success) {
		LOG_WRN("pb_encode control reply fail");
		return -EINVAL;
	}

	return bt_gadgets_stream_send(
		conn,
		BT_GADGETS_STREAM_ID_CONTROL,
		protobuf_encoded,
		stream_out.bytes_written);
}

/* Transmit Protocol version packet as described in Alexa Gadgets tech docs:
 * https://developer.amazon.com/en-US/docs/alexa/alexa-gadgets-toolkit/bluetooth-le-settings.html#pvp
 */
static void protocol_version_send(void)
{
	uint8_t pv[GADGETS_PV_LEN_BYTES];
	uint16_t att_payload;
	int err;

	LOG_DBG("%s", __func__);

	/* Provide MTU minus ATT overhead */
	att_payload = bt_gatt_get_mtu(conn_state.conn) - 3;

	memset(pv, 0, sizeof(pv));
	sys_put_be16(GADGETS_PV_IDENTIFIER, &pv[0]);
	pv[2] = GADGETS_PV_MAJOR_VERSION;
	pv[3] = GADGETS_PV_MINOR_VERSION;
	/* [effective] MTU */
	sys_put_be16(att_payload, &pv[4]);
	/* Maximum transactional data size */
	sys_put_be16(CONFIG_GADGETS_TRANSACTION_BUF_SIZE, &pv[6]);
	/* Remainder of pv is reserved: leave at value 0 */

	err = bt_gadgets_send(conn_state.conn, pv, sizeof(pv));
	if (err) {
		LOG_WRN("Failed to send PV packet: %d", err);
	}
}

static int encoded_buffer_acquire(int user)
{
	if (atomic_cas(&protobuf_encoded_user, BUF_USER_NONE, user)) {
		return 0;
	} else {
		return -EBUSY;
	}
}

static enum buf_user encoded_buffer_release(void)
{
	return (enum buf_user) atomic_set(&protobuf_encoded_user, BUF_USER_NONE);
}

static int encoded_data_append(const uint8_t *const data, uint16_t len,
			       int user)
{
	/* Check buffer ownership and try to acquire */
	if (atomic_get(&protobuf_encoded_user) != user) {
		int err;

		err = encoded_buffer_acquire(user);
		if (err) {
			return err;
		}
	}

	if ((protobuf_offset + len) > sizeof(protobuf_encoded)) {
		return -ENOMEM;
	}

	memcpy(&protobuf_encoded[protobuf_offset], data, len);
	protobuf_offset += len;

	return 0;
}

static void encoded_data_reset(void)
{
	protobuf_offset = 0;
	encoded_buffer_release();
}

static void gadgets_sent_cb(struct bt_conn *conn, const void *buf, bool success)
{
	enum buf_user user;

	user = encoded_buffer_release();
	if (user == BUF_USER_CUSTOM_EVT) {
		struct gadgets_evt evt = {
			.type = GADGETS_EVT_CUSTOM_SENT,
		};

		callback(&evt);
	}
}

static void gadgets_cccd_cb(bool enabled)
{
	LOG_DBG("%s: %d", __func__, enabled);

	conn_state.cccd_enabled = enabled ? 1 : 0;

	if (!enabled) {
		return;
	}

	if (conn_state.encrypted && conn_state.mtu_exchanged) {
		protocol_version_send();
	}
}

static bool gadgets_control_stream_cb(struct bt_conn *conn,
				      const uint8_t *const data,
				      uint16_t len, bool more_data)
{
	bool ret;
	int err;

	err = encoded_data_append(data, len, BUF_USER_CONTROL_STREAM);
	if (err == -EBUSY) {
		LOG_WRN("Buffer busy");
		return false;
	} else if (err == -ENOMEM) {
		LOG_WRN("Buffer overflow");
		ret = false;
		goto cleanup;
	}

	if (more_data) {
		/* Wait for last piece of data */
		return true;
	}

	err = gadgets_command_respond(conn);
	if (err) {
		LOG_WRN("Failed to reply to command message");
		ret = false;
		goto cleanup;
	}

	ret = true;

cleanup:
	encoded_data_reset();

	return ret;
}

static bool gadgets_alexa_stream_cb(struct bt_conn *conn,
				    const uint8_t *const data,
				    uint16_t len, bool more_data)
{
	bool ret;
	int err;

	err = encoded_data_append(data, len, BUF_USER_ALEXA_STREAM);
	if (err == -EBUSY) {
		LOG_WRN("Buffer busy");
		return false;
	} else if (err == -ENOMEM) {
		LOG_WRN("Buffer overflow");
		ret = false;
		goto cleanup;
	}

	__ASSERT(err == 0, "Unhandled error");

	if (more_data) {
		/* Wait for last piece of data */
		return true;
	}

	pb_istream_t stream_in =
		pb_istream_from_buffer(protobuf_encoded, protobuf_offset);
	bool success = pb_decode(
		&stream_in,
		&directive_DirectiveParserProto_msg,
		&protobuf_decoded.directive_parser);
	if (!success) {
		LOG_WRN("Failed to decode alexa message");
		ret = false;
		goto cleanup;
	}

	const char *name = protobuf_decoded.directive_parser.directive.header.name;
	const char *namespace =
		protobuf_decoded.directive_parser.directive.header.namespace;

	LOG_DBG("Directive name: %s", log_strdup(name));
	LOG_DBG("Directive namespace: %s", log_strdup(namespace));

	ret = false;

	for (int i = 0; i < ARRAY_SIZE(namespace_handlers); ++i) {
		if (strcmp(namespace, namespace_handlers[i].namespace) == 0) {
			/* Reminder:
			 * protobuf_encoded is re-used by handlers
			 * for outoing messages.
			 */
			err = namespace_handlers[i].handler(
				conn, name, protobuf_encoded, protobuf_offset);
			if (err) {
				LOG_ERR("Unsupported %s / %s", log_strdup(name),
					log_strdup(namespace));
			} else {
				ret = true;
			}
			break;
		}
	}

cleanup:
	encoded_data_reset();

	return ret;
}

static int directive_handler_discovery(
	struct bt_conn *conn, const char *name,
	const uint8_t *data, uint16_t len)
{
	int err;

	if (strcmp(name, "Discover") != 0) {
		return -EINVAL;
	}

	/* Construct response to discovery directive from hardcoded template.
	 * Serial number and device token are device-unique values
	 * that must be updated at runtime.
	 */

	pb_ostream_t stream_out =
		pb_ostream_from_buffer(protobuf_encoded, sizeof(protobuf_encoded));
	bool success = pb_encode(
		&stream_out,
		&alexaDiscovery_DiscoverResponseEventProto_msg,
		&discovery_response_template);
	__ASSERT(success, "protobuf_encoded is too small");
	if (!success) {
		return -EINVAL;
	}

	/* Replace serial and token placeholders with actual values */
	char *serial_ptr;
	char *token_ptr;
	char serial_buf[GADGETS_DSN_LENGTH_CHARS + 1];
	char token_buf[GADGETS_TOKEN_LEN_CHARS + 1];

	serial_ptr = strnstr(
		protobuf_encoded,
		stream_out.bytes_written,
		GADGETS_CAP_DSN_PLACEHOLDER);
	__ASSERT_NO_MSG(serial_ptr != NULL);
	serial_number_string_get(serial_buf);
	memcpy(serial_ptr, serial_buf, GADGETS_DSN_LENGTH_CHARS);

	token_ptr = strnstr(
		protobuf_encoded,
		stream_out.bytes_written,
		GADGETS_CAP_DEV_TOKEN_PLACEHOLDER);
	__ASSERT_NO_MSG(token_ptr != NULL);
	device_token_get(token_buf);
	memcpy(token_ptr, token_buf, GADGETS_TOKEN_LEN_CHARS);

	err = bt_gadgets_stream_send(
			conn,
			BT_GADGETS_STREAM_ID_ALEXA,
			protobuf_encoded,
			stream_out.bytes_written);
	if (err == 0) {
		struct gadgets_evt evt = {
			.type = GADGETS_EVT_READY,
		};

		/* Consider Gadget setup complete once Discovery is done */
		callback(&evt);
	}

	return err;
}

#if CONFIG_GADGETS_CAPABILITY_MUSICDATA_ENABLE
static int directive_handler_musicdata(
	struct bt_conn *conn, const char *name,
	const uint8_t *data, uint16_t len)
{
	bool success;
	pb_istream_t stream_in;

	stream_in = pb_istream_from_buffer(data, len);

	if (strcmp(name, "Tempo") == 0) {
		success = pb_decode(
			&stream_in,
			&alexaGadgetMusicData_TempoDirectiveProto_msg,
			&protobuf_decoded.music_tempo);
		if (!success) {
			LOG_ERR("Music tempo decode error");
			return -EINVAL;
		}
		struct gadgets_evt evt = {
			.type = GADGETS_EVT_MUSICTEMPO,
			.parameters.music_tempo = NULL,
		};

		if (protobuf_decoded.music_tempo.has_directive &&
		    protobuf_decoded.music_tempo.directive.has_payload) {
			evt.parameters.music_tempo =
				&protobuf_decoded.music_tempo.directive.payload;
		}

		callback(&evt);
		return 0;
	}

	return -ENOTSUP;
}
#endif

#if CONFIG_GADGETS_CAPABILITY_SPEECHDATA_ENABLE
static int directive_handler_speechdata(
	struct bt_conn *conn, const char *name,
	const uint8_t *data, uint16_t len)
{
	bool success;
	pb_istream_t stream_in;

	stream_in = pb_istream_from_buffer(data, len);

	if (strcmp(name, "Speechmarks") == 0) {
		success = pb_decode(
			&stream_in,
			&alexaGadgetSpeechData_SpeechmarksDirectiveProto_msg,
			&protobuf_decoded.speech_marks);
		if (!success) {
			LOG_ERR("Speechmarks decode error");
			return -EINVAL;
		}
		struct gadgets_evt evt = {
			.type = GADGETS_EVT_SPEECHMARKS,
			.parameters.speech_marks = NULL,
		};

		if (protobuf_decoded.speech_marks.has_directive &&
		    protobuf_decoded.speech_marks.directive.has_payload) {
			evt.parameters.speech_marks =
				&protobuf_decoded.speech_marks.directive.payload;
		}

		callback(&evt);
		return 0;
	}

	return -ENOTSUP;
}
#endif

#if CONFIG_GADGETS_CAPABILITY_STATELISTENER_ENABLE
static int directive_handler_statelistener(
	struct bt_conn *conn, const char *name,
	const uint8_t *data, uint16_t len)
{
	bool success;
	pb_istream_t stream_in;

	stream_in = pb_istream_from_buffer(data, len);

	if (strcmp(name, "StateUpdate") == 0) {
		success = pb_decode(
			&stream_in,
			&alexaGadgetStateListener_StateUpdateDirectiveProto_msg,
			&protobuf_decoded.state_update);
		if (!success) {
			LOG_ERR("StateUpdate decode error");
			return -EINVAL;
		}

		struct gadgets_evt evt = {
			.type = GADGETS_EVT_STATEUPDATE,
			.parameters.state_update = NULL,
		};

		if (protobuf_decoded.state_update.has_directive &&
		    protobuf_decoded.state_update.directive.has_payload) {
			evt.parameters.state_update =
				&protobuf_decoded.state_update.directive.payload;
		}

		callback(&evt);
		return 0;
	}

	return -ENOTSUP;
}
#endif

#if CONFIG_GADGETS_CAPABILITY_NOTIFICATIONS_ENABLE
static int directive_handler_notifications(
	struct bt_conn *conn, const char *name,
	const uint8_t *data, uint16_t len)
{
	bool success;
	pb_istream_t stream_in;

	stream_in = pb_istream_from_buffer(data, len);

	if (strcmp(name, "SetIndicator") == 0) {
		success = pb_decode(
			&stream_in,
			&notifications_SetIndicatorDirectiveProto_msg,
			&protobuf_decoded.set_indicator);
		if (!success) {
			LOG_ERR("SetIndicator decode error");
			return -EINVAL;
		}

		struct gadgets_evt evt = {
			.type = GADGETS_EVT_SETINDICATOR,
			.parameters.set_indicator = NULL,
		};

		if (protobuf_decoded.set_indicator.has_directive &&
		    protobuf_decoded.set_indicator.directive.has_payload) {
			evt.parameters.set_indicator =
				&protobuf_decoded.set_indicator.directive.payload;
		}

		callback(&evt);
		return 0;
	} else if (strcmp(name, "ClearIndicator") == 0) {
		/* No data in payload: don't do protobuf decode */
		struct gadgets_evt evt = {
			.type = GADGETS_EVT_CLEARINDICATOR,
		};

		callback(&evt);
		return 0;
	}

	return -ENOTSUP;
}
#endif

#if CONFIG_GADGETS_CAPABILITY_ALERTS_ENABLE
static int directive_handler_alerts(
	struct bt_conn *conn, const char *name,
	const uint8_t *data, uint16_t len)
{
	bool success;
	pb_istream_t stream_in;

	stream_in = pb_istream_from_buffer(data, len);

	if (strcmp(name, "SetAlert") == 0) {
		success = pb_decode(
			&stream_in,
			&alerts_SetAlertDirectiveProto_msg,
			&protobuf_decoded.set_alert);
		if (!success) {
			LOG_ERR("SetAlert decode error");
			return -EINVAL;
		}

		struct gadgets_evt evt = {
			.type = GADGETS_EVT_SETALERT,
			.parameters.set_alert = NULL,
		};

		if (protobuf_decoded.set_alert.has_directive &&
		    protobuf_decoded.set_alert.directive.has_payload) {
			evt.parameters.set_alert = &protobuf_decoded.set_alert.directive.payload;
		}

		callback(&evt);
		return 0;
	} else if (strcmp(name, "DeleteAlert") == 0) {
		success = pb_decode(
			&stream_in,
			&alerts_DeleteAlertDirectiveProto_msg,
			&protobuf_decoded.delete_alert);
		if (!success) {
			LOG_ERR("DeleteAlert decode error");
			return -EINVAL;
		}

		struct gadgets_evt evt = {
			.type = GADGETS_EVT_DELETEALERT,
			.parameters.delete_alert = NULL,
		};

		if (protobuf_decoded.delete_alert.has_directive &&
		    protobuf_decoded.delete_alert.directive.has_payload) {
			evt.parameters.delete_alert =
				&protobuf_decoded.delete_alert.directive.payload;
		}

		callback(&evt);
		return 0;
	}

	return -ENOTSUP;
}
#endif

#if CONFIG_GADGETS_CAPABILITY_CUSTOM_ENABLE
static int directive_handler_custom(
	struct bt_conn *conn, const char *name,
	const uint8_t *data, uint16_t len)
{
	if (!protobuf_decoded.directive_parser.has_directive) {
		LOG_DBG("Empty custom directive");
		return 0;
	}

	struct gadgets_evt evt = {
		.type = GADGETS_EVT_CUSTOM,
		.parameters.custom_directive.name = name,
		.parameters.custom_directive.payload =
			protobuf_decoded.directive_parser.directive.payload.bytes,
		.parameters.custom_directive.size =
			protobuf_decoded.directive_parser.directive.payload.size,
	};

	callback(&evt);
	return 0;

	return -ENOTSUP;
}
#endif

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_WRN("Connection error: %d", err);
		return;
	}

	conn_state.conn = bt_conn_ref(conn);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (conn_state.conn) {
		bt_conn_unref(conn_state.conn);
	}

	memset(&conn_state, 0, sizeof(conn_state));

	encoded_buffer_release();
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	int mtu_err;

	LOG_DBG("%s: %u", __func__, level);

	if (err) {
		LOG_WRN("Security failed: level %u err %d", level, err);
		return;
	}

	if (level < BT_SECURITY_L2) {
		LOG_WRN("Insuficcient sec level");
		return;
	}

	conn_state.encrypted = 1;

	mtu_err = bt_gatt_exchange_mtu(conn_state.conn, &exchange_params);
	if (mtu_err) {
		LOG_WRN("bt_gatt_exchange_mtu: %d", err);
	}
}

static void pairing_confirm(struct bt_conn *conn)
{
	int err;

	LOG_DBG("%s", __func__);

	err = bt_conn_auth_pairing_confirm(conn);
	if (err) {
		LOG_ERR("bt_conn_auth_pairing_confirm: %d", err);
	}
}

static void exchange_func(struct bt_conn *conn, uint8_t err,
			  struct bt_gatt_exchange_params *params)
{
	if (err) {
		LOG_ERR("%s: %d", __func__, err);
	}

	LOG_DBG("%s", __func__);

	conn_state.mtu_exchanged = 1;

	if (conn_state.encrypted && conn_state.cccd_enabled) {
		protocol_version_send();
	}
}

int gadgets_profile_init(gadgets_profile_cb_t evt_handler)
{
	if (evt_handler == NULL) {
		return -EINVAL;
	}

	callback = evt_handler;

	encoded_buffer_release();

	bt_conn_cb_register(&conn_callbacks);
	bt_conn_auth_cb_register(&conn_auth_callbacks);

	return bt_gadgets_init(&gadgets_service_cb);
}

int gadgets_profile_adv_start(bool bondable_type)
{
	if (bondable_type) {
		return bt_le_adv_start(
			BT_LE_ADV_CONN,
			ad, ARRAY_SIZE(ad),
			sd_pairing, ARRAY_SIZE(sd_pairing));
	} else {
		return bt_le_adv_start(
			BT_LE_ADV_CONN,
			ad, ARRAY_SIZE(ad),
			sd_reconnect, ARRAY_SIZE(sd_reconnect));
	}
}

int gadgets_profile_custom_event_json_send(uint8_t *name, uint8_t *json)
{
#if CONFIG_GADGETS_CAPABILITY_CUSTOM_ENABLE
	int err;
	bool success;

	if (strlen(name) >= sizeof(protobuf_decoded.custom_event.event.header.name) ||
	    strlen(json) >= sizeof(protobuf_decoded.custom_event.event.payload)) {
		return -EINVAL;
	}

	err = encoded_buffer_acquire(BUF_USER_CUSTOM_EVT);
	if (err) {
		return err;
	}

	/* Encapsulate json in protobuf struct, encode, and transmit */

	protobuf_decoded.custom_event.has_event = true;
	protobuf_decoded.custom_event.event.has_header = true;
	/* messageId not used */
	protobuf_decoded.custom_event.event.header.messageId[0] = '\0';
	strcpy(protobuf_decoded.custom_event.event.header.name, name);
	strcpy(protobuf_decoded.custom_event.event.header.namespace,
	       CONFIG_GADGETS_CAPABILITY_CUSTOM_NAMESPACE);
	strcpy(protobuf_decoded.custom_event.event.payload, json);

	pb_ostream_t stream_out =
		pb_ostream_from_buffer(protobuf_encoded, sizeof(protobuf_encoded));
	success = pb_encode(
		&stream_out,
		&custom_event_proto_msg,
		&protobuf_decoded.custom_event);
	if (!success) {
		LOG_WRN("custom event encode fail");
		encoded_buffer_release();
		return -EINVAL;
	}

	err = bt_gadgets_stream_send(
		conn_state.conn,
		BT_GADGETS_STREAM_ID_ALEXA,
		protobuf_encoded,
		stream_out.bytes_written);
	if (err) {
		encoded_buffer_release();
		return err;
	}

	return 0;
#else
	return -ENOTSUP;
#endif
}

int gadgets_profile_custom_event_send(uint8_t *encoded_data, size_t len)
{
#if CONFIG_GADGETS_CAPABILITY_CUSTOM_ENABLE
	int err;

	if (len > sizeof(protobuf_encoded)) {
		return -EINVAL;
	}

	err = encoded_buffer_acquire(BUF_USER_CUSTOM_EVT);
	if (err) {
		return err;
	}

	memcpy(protobuf_encoded, encoded_data, len);

	err = bt_gadgets_stream_send(
		conn_state.conn,
		BT_GADGETS_STREAM_ID_ALEXA,
		protobuf_encoded,
		len);
	if (err) {
		encoded_buffer_release();
		return err;
	}

	return 0;
#else
	return -ENOTSUP;
#endif
}
