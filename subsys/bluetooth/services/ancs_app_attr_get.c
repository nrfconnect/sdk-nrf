/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Disclaimer: This client implementation of the Apple Notification Center
 * Service can and will be changed at any time by Nordic Semiconductor ASA.
 * Server implementations such as the ones found in iOS can be changed at any
 * time by Apple and may cause this client implementation to stop working.
 */

#include <bluetooth/services/ancs_client.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include "ancs_client_internal.h"
#include "ancs_app_attr_get.h"

LOG_MODULE_DECLARE(ancs_c, CONFIG_BT_ANCS_CLIENT_LOG_LEVEL);

/**@brief Enumeration for keeping track of the state-based encoding while requesting
 *        app attributes.
 */
enum encode_app_attr {
	/** Currently encoding the command ID. */
	APP_ATTR_COMMAND_ID,
	/** Currently encoding the app ID. */
	APP_ATTR_APP_ID,
	/** Currently encoding the attribute ID. */
	APP_ATTR_ATTR_ID,
	/** Encoding done. */
	APP_ATTR_DONE,
	/** Encoding aborted. */
	APP_ATTR_ABORT
};

/**@brief Function for determining whether an attribute is requested.
 *
 * @param[in] ancs_c iOS notification structure. This structure must be supplied by
 *                   the application. It identifies the particular client instance to use.
 *
 * @return True  If it is requested
 * @return False If it is not requested.
 */
static bool app_attr_is_requested(struct bt_ancs_client *ancs_c,
				  uint32_t attr_id)
{
	return ancs_c->ancs_app_attr_list[attr_id].get;
}

/**@brief Function for encoding the command ID as part of assembling a "get app attributes" command.
 *
 * @param[in,out] buf        Pointer to the BLE GATT write buffer structure.
 */
static enum encode_app_attr app_attr_encode_cmd_id(struct net_buf_simple *buf)
{
	LOG_DBG("Encoding command ID.");

	if (net_buf_simple_tailroom(buf) < 1) {
		return APP_ATTR_ABORT;
	}

	/* Encode Command ID. */
	net_buf_simple_add_u8(buf, BT_ANCS_COMMAND_ID_GET_APP_ATTRIBUTES);

	return APP_ATTR_APP_ID;
}

/**@brief Function for encoding the app ID as part of assembling a "get app attributes" command.
 *
 * @param[in,out] buf        Pointer to the BLE GATT write buffer structure.
 * @param[in]     app_id     The app ID of the app for which to request app attributes.
 * @param[in]     app_id_len Length of the app ID.
 */
static enum encode_app_attr app_attr_encode_app_id(struct net_buf_simple *buf,
						   const uint8_t *app_id,
						   const uint32_t app_id_len)
{
	LOG_DBG("Encoding app ID.");

	if (net_buf_simple_tailroom(buf) < app_id_len + 1) {
		return APP_ATTR_ABORT;
	}

	/* Encode app identifier. */
	net_buf_simple_add_mem(buf, app_id, app_id_len);
	net_buf_simple_add_u8(buf, '\0');

	return APP_ATTR_ATTR_ID;
}

/**@brief Function for encoding the attribute ID as part of assembling a
 *        "get app attributes" command.
 *
 * @param[in]     ancs_c     iOS notification structure. This structure must be supplied by
 *                           the application. It identifies the particular client instance to use.
 * @param[in,out] buf        Pointer to the BLE GATT write buffer structure.
 */
static enum encode_app_attr app_attr_encode_attr_id(
		struct bt_ancs_client *ancs_c, struct net_buf_simple *buf)
{
	uint32_t i;

	LOG_DBG("Encoding attribute ID.");

	ancs_c->number_of_requested_attr = 0;

	/* Encode Attribute ID. */
	for (i = 0; i < BT_ANCS_APP_ATTR_COUNT; i++) {
		if (app_attr_is_requested(ancs_c, i)) {
			if (net_buf_simple_tailroom(buf) < 1) {
				return APP_ATTR_ABORT;
			}
			net_buf_simple_add_u8(buf, i);
			ancs_c->number_of_requested_attr++;
		}
	}

	return APP_ATTR_DONE;
}

/**@brief Function for sending a "get app attributes" request.
 *
 * @details Since the app ID may not fit in a single write, long write
 *          with a state machine is used to encode the "get app attributes" request.
 *
 * @param[in] ancs_c     iOS notification structure. This structure must be supplied by
 *                       the application. It identifies the particular client instance to use.
 * @param[in] app_id     The app ID of the app for which to request app attributes.
 * @param[in] app_id_len Length of the app ID.
 */
static int app_attr_get(struct bt_ancs_client *ancs_c, const uint8_t *app_id,
			uint32_t app_id_len, bt_ancs_write_cb func)
{
	enum encode_app_attr state = APP_ATTR_COMMAND_ID;
	int err;

	if (atomic_test_and_set_bit(&ancs_c->state, ANCS_CP_WRITE_PENDING)) {
		return -EBUSY;
	}

	struct net_buf_simple buf;

	net_buf_simple_init_with_data(&buf, ancs_c->cp_data, sizeof(ancs_c->cp_data));
	net_buf_simple_reset(&buf);

	while (state != APP_ATTR_DONE && state != APP_ATTR_ABORT) {
		switch (state) {
		case APP_ATTR_COMMAND_ID:
			state = app_attr_encode_cmd_id(&buf);
			break;
		case APP_ATTR_APP_ID:
			state = app_attr_encode_app_id(&buf,
						       app_id, app_id_len);
			break;
		case APP_ATTR_ATTR_ID:
			state = app_attr_encode_attr_id(ancs_c, &buf);
			break;
		case APP_ATTR_DONE:
			break;
		case APP_ATTR_ABORT:
			break;
		default:
			break;
		}
	}

	if (state == APP_ATTR_DONE) {
		err = bt_ancs_cp_write(ancs_c, buf.len, func);

		ancs_c->parse_info.expected_number_of_attrs =
			ancs_c->number_of_requested_attr;
	} else {
		err = -ENOMEM;
	}

	return err;
}

int bt_ancs_app_attr_request(struct bt_ancs_client *ancs_c,
			     const uint8_t *app_id, uint32_t len,
			     bt_ancs_write_cb func)
{
	if (!len) {
		return -EINVAL;
	}

	ancs_c->parse_info.parse_state = BT_ANCS_PARSE_STATE_COMMAND_ID;

	return app_attr_get(ancs_c, app_id, len, func);
}
