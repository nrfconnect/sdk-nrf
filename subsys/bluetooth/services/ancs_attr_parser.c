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

#include <zephyr/sys/byteorder.h>
#include <bluetooth/services/ancs_client.h>
#include "ancs_client_internal.h"
#include "ancs_attr_parser.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(ancs_c, CONFIG_BT_ANCS_CLIENT_LOG_LEVEL);

static bool all_req_attrs_parsed(struct bt_ancs_client *ancs_c)
{
	return !ancs_c->parse_info.expected_number_of_attrs;
}

static bool attr_is_requested(struct bt_ancs_client *ancs_c,
			      struct bt_ancs_attr attr)
{
	return ancs_c->parse_info.attr_list[attr.attr_id].get;
}

/**@brief Function for parsing command id and notification id.
 *        Used in the @ref parse_get_notif_attrs_response state machine.
 *
 * @details UID and command ID will be received only once at the beginning of the first
 *          GATT notification of a new attribute request for a given iOS notification.
 *
 * @param[in] ancs_c   Pointer to an ANCS instance to which the event belongs.
 * @param[in] data_src Pointer to data that was received from the Notification Provider.
 * @param[in] index    Pointer to an index that helps us keep track of the current data
 *                     to be parsed.
 *
 * @return The next parse state.
 */
static enum bt_ancs_parse_state command_id_parse(struct bt_ancs_client *ancs_c,
						 const uint8_t *data_src,
						 uint32_t *index)
{
	enum bt_ancs_parse_state parse_state;

	ancs_c->parse_info.command_id =
		(enum bt_ancs_cmd_id_val)data_src[(*index)++];

	switch (ancs_c->parse_info.command_id) {
	case BT_ANCS_COMMAND_ID_GET_NOTIF_ATTRIBUTES:
		ancs_c->attr_response.command_id = BT_ANCS_COMMAND_ID_GET_NOTIF_ATTRIBUTES;
		ancs_c->parse_info.attr_list = ancs_c->ancs_notif_attr_list;
		ancs_c->parse_info.attr_count = BT_ANCS_NOTIF_ATTR_COUNT;
		parse_state = BT_ANCS_PARSE_STATE_NOTIF_UID;
		break;

	case BT_ANCS_COMMAND_ID_GET_APP_ATTRIBUTES:
		ancs_c->attr_response.command_id = BT_ANCS_COMMAND_ID_GET_APP_ATTRIBUTES;
		ancs_c->parse_info.attr_list = ancs_c->ancs_app_attr_list;
		ancs_c->parse_info.attr_count = BT_ANCS_APP_ATTR_COUNT;
		ancs_c->parse_info.current_app_id_index = 0;
		parse_state = BT_ANCS_PARSE_STATE_APP_ID;
		break;

	default:
		/* no valid command_id, abort the rest of the parsing procedure. */
		LOG_DBG("Invalid Command ID");
		parse_state = BT_ANCS_PARSE_STATE_DONE;
		break;
	}
	return parse_state;
}

static enum bt_ancs_parse_state notif_uid_parse(struct bt_ancs_client *ancs_c,
						const uint8_t *data_src,
						uint32_t *index)
{
	ancs_c->attr_response.notif_uid = sys_get_le32(&data_src[*index]);
	*index += sizeof(uint32_t);
	return BT_ANCS_PARSE_STATE_ATTR_ID;
}

static enum bt_ancs_parse_state app_id_parse(struct bt_ancs_client *ancs_c,
					     const uint8_t *data_src,
					     uint32_t *index)
{
	if (ancs_c->parse_info.current_app_id_index >= sizeof(ancs_c->attr_response.app_id)) {
		LOG_WRN("App ID cannot be stored in response buffer.");
		return BT_ANCS_PARSE_STATE_DONE;
	}

	ancs_c->attr_response.app_id[ancs_c->parse_info.current_app_id_index] =
		data_src[(*index)++];

	if (ancs_c->attr_response.app_id[ancs_c->parse_info.current_app_id_index] !=
	    '\0') {
		ancs_c->parse_info.current_app_id_index++;
		return BT_ANCS_PARSE_STATE_APP_ID;
	} else {
		return BT_ANCS_PARSE_STATE_ATTR_ID;
	}
}

/**@brief Function for parsing the id of an iOS attribute.
 *        Used in the @ref parse_get_notif_attrs_response state machine.
 *
 * @details We only request attributes that are registered with @ref bt_ancs_attr_add
 *          once they have been reveiced we stop parsing.
 *
 * @param[in] ancs_c   Pointer to an ANCS instance to which the event belongs.
 * @param[in] data_src Pointer to data that was received from the Notification Provider.
 * @param[in] index    Pointer to an index that helps us keep track of the current data
 *                     to be parsed.
 *
 * @return The next parse state.
 */
static enum bt_ancs_parse_state attr_id_parse(struct bt_ancs_client *ancs_c,
					      const uint8_t *data_src,
					      uint32_t *index)
{
	ancs_c->attr_response.attr.attr_id = data_src[(*index)++];

	if (ancs_c->attr_response.attr.attr_id >= ancs_c->parse_info.attr_count) {
		LOG_DBG("Attribute ID Invalid.");
		return BT_ANCS_PARSE_STATE_DONE;
	}
	ancs_c->attr_response.attr.attr_data =
		ancs_c->parse_info.attr_list[ancs_c->attr_response.attr.attr_id]
			.attr_data;

	if (all_req_attrs_parsed(ancs_c)) {
		LOG_DBG("All requested attributes received. ");
		return BT_ANCS_PARSE_STATE_DONE;
	}

	if (attr_is_requested(ancs_c, ancs_c->attr_response.attr)) {
		ancs_c->parse_info.expected_number_of_attrs--;
	}
	LOG_DBG("Attribute ID %i ", ancs_c->attr_response.attr.attr_id);
	return BT_ANCS_PARSE_STATE_ATTR_LEN1;
}

/**@brief Function for parsing the length of an iOS attribute.
 *        Used in the @ref parse_get_notif_attrs_response state machine.
 *
 * @details The Length is 2 bytes. Since there is a chance we reveice the bytes in two different
 *          GATT notifications, we parse only the first byte here and then set the state machine
 *          ready to parse the next byte.
 *
 * @param[in] ancs_c   Pointer to an ANCS instance to which the event belongs.
 * @param[in] data_src Pointer to data that was received from the Notification Provider.
 * @param[in] index    Pointer to an index that helps us keep track of the current data
 *                     to be parsed.
 *
 * @return The next parse state.
 */
static enum bt_ancs_parse_state attr_len1_parse(struct bt_ancs_client *ancs_c,
						const uint8_t *data_src,
						uint32_t *index)
{
	ancs_c->attr_response.attr.attr_len = data_src[(*index)++];
	return BT_ANCS_PARSE_STATE_ATTR_LEN2;
}

/**@brief Function for parsing the length of an iOS attribute.
 *        Used in the @ref parse_get_notif_attrs_response state machine.
 *
 * @details Second byte of the length field. If the length is zero, it means that the attribute
 *          is not present and the state machine is set to parse the next attribute.
 *
 * @param[in] ancs_c   Pointer to an ANCS instance to which the event belongs.
 * @param[in] data_src Pointer to data that was received from the Notification Provider.
 * @param[in] index    Pointer to an index that helps us keep track of the current data
 *                     to be parsed.
 *
 * @return The next parse state.
 */
static enum bt_ancs_parse_state attr_len2_parse(struct bt_ancs_client *ancs_c,
						const uint8_t *data_src,
						uint32_t *index)
{
	ancs_c->attr_response.attr.attr_len |= (data_src[(*index)++] << 8);
	ancs_c->parse_info.current_attr_index = 0;

	if (ancs_c->attr_response.attr.attr_len != 0) {
		/* If the attribute has a length but there is no allocated
		 * space for this attribute
		 */
		if (!ancs_c->parse_info.attr_list[ancs_c->attr_response.attr.attr_id].attr_len ||
		    !ancs_c->parse_info.attr_list[ancs_c->attr_response.attr.attr_id].attr_data) {
			return BT_ANCS_PARSE_STATE_ATTR_SKIP;
		} else {
			return BT_ANCS_PARSE_STATE_ATTR_DATA;
		}
	} else {
		LOG_DBG("Attribute LEN %i ", ancs_c->attr_response.attr.attr_len);
		if (attr_is_requested(ancs_c, ancs_c->attr_response.attr)) {
			bt_ancs_do_ds_notif_cb(ancs_c, &ancs_c->attr_response);
		}
		if (all_req_attrs_parsed(ancs_c)) {
			return BT_ANCS_PARSE_STATE_DONE;
		} else {
			return BT_ANCS_PARSE_STATE_ATTR_ID;
		}
	}
}

/**@brief Function for parsing the data of an iOS attribute.
 *        Used in the @ref parse_get_notif_attrs_response state machine.
 *
 * @details Read the data of the attribute into our local buffer.
 *
 * @param[in] ancs_c   Pointer to an ANCS instance to which the event belongs.
 * @param[in] data_src Pointer to data that was received from the Notification Provider.
 * @param[in] index    Pointer to an index that helps us keep track of the current data
 *                     to be parsed.
 *
 * @return The next parse state.
 */
static enum bt_ancs_parse_state attr_data_parse(struct bt_ancs_client *ancs_c,
						const uint8_t *data_src,
						uint32_t *index)
{
	/* We have not reached the end of the attribute, nor our max allocated internal size.
	 * Proceed with copying data over to our buffer.
	 */
	if ((ancs_c->parse_info.current_attr_index <
	     ancs_c->parse_info.attr_list[ancs_c->attr_response.attr.attr_id].attr_len) &&
	    (ancs_c->parse_info.current_attr_index <
	     ancs_c->attr_response.attr.attr_len)) {
		/* Un-comment this line to see every byte of an attribute as it is parsed.
		 * Commented out by default since it can overflow the uart buffer.
		 */
		/* LOG_DBG("Byte copied to buffer: %c", data_src[(*index)]); */
		ancs_c->attr_response.attr
			.attr_data[ancs_c->parse_info.current_attr_index++] =
			data_src[(*index)++];
	}

	/* We have reached the end of the attribute, or our max allocated internal size.
	 * Stop copying data over to our buffer. NUL-terminate at the current index.
	 */
	if ((ancs_c->parse_info.current_attr_index ==
	     ancs_c->attr_response.attr.attr_len) ||
	    (ancs_c->parse_info.current_attr_index ==
	     ancs_c->parse_info.attr_list[ancs_c->attr_response.attr.attr_id].attr_len -
		     1)) {
		if (attr_is_requested(ancs_c, ancs_c->attr_response.attr)) {
			ancs_c->attr_response.attr.attr_data
				[ancs_c->parse_info.current_attr_index] = '\0';
		}

		/* If our max buffer size is smaller than the remaining attribute data, we must
		 * increase index to skip the data until the start of the next attribute.
		 */
		if (ancs_c->parse_info.current_attr_index <
		    ancs_c->attr_response.attr.attr_len) {
			return BT_ANCS_PARSE_STATE_ATTR_SKIP;
		}
		LOG_DBG("Attribute finished!");
		if (attr_is_requested(ancs_c, ancs_c->attr_response.attr)) {
			bt_ancs_do_ds_notif_cb(ancs_c, &ancs_c->attr_response);
		}
		if (all_req_attrs_parsed(ancs_c)) {
			return BT_ANCS_PARSE_STATE_DONE;
		} else {
			return BT_ANCS_PARSE_STATE_ATTR_ID;
		}
	}
	return BT_ANCS_PARSE_STATE_ATTR_DATA;
}

static enum bt_ancs_parse_state attr_skip(struct bt_ancs_client *ancs_c,
					  const uint8_t *data_src,
					  uint32_t *index)
{
	/* We have not reached the end of the attribute, nor our max allocated internal size.
	 * Proceed with copying data over to our buffer.
	 */
	if (ancs_c->parse_info.current_attr_index < ancs_c->attr_response.attr.attr_len) {
		ancs_c->parse_info.current_attr_index++;
		(*index)++;
	}
	/* At the end of the attribute, determine if it should be passed to event handler and
	 * continue parsing the next attribute ID if we are not done with all the attributes.
	 */
	if (ancs_c->parse_info.current_attr_index ==
	    ancs_c->attr_response.attr.attr_len) {
		if (attr_is_requested(ancs_c, ancs_c->attr_response.attr)) {
			bt_ancs_do_ds_notif_cb(ancs_c, &ancs_c->attr_response);
		}
		if (all_req_attrs_parsed(ancs_c)) {
			return BT_ANCS_PARSE_STATE_DONE;
		} else {
			return BT_ANCS_PARSE_STATE_ATTR_ID;
		}
	}
	return BT_ANCS_PARSE_STATE_ATTR_SKIP;
}

void bt_ancs_parse_get_attrs_response(struct bt_ancs_client *ancs_c,
				      const uint8_t *data_src,
				      const uint16_t data_len)
{
	uint32_t index;

	for (index = 0; index < data_len;) {
		switch (ancs_c->parse_info.parse_state) {
		case BT_ANCS_PARSE_STATE_COMMAND_ID:
			ancs_c->parse_info.parse_state =
				command_id_parse(ancs_c, data_src, &index);
			break;

		case BT_ANCS_PARSE_STATE_NOTIF_UID:
			ancs_c->parse_info.parse_state =
				notif_uid_parse(ancs_c, data_src, &index);
			break;

		case BT_ANCS_PARSE_STATE_APP_ID:
			ancs_c->parse_info.parse_state =
				app_id_parse(ancs_c, data_src, &index);
			break;

		case BT_ANCS_PARSE_STATE_ATTR_ID:
			ancs_c->parse_info.parse_state =
				attr_id_parse(ancs_c, data_src, &index);
			break;

		case BT_ANCS_PARSE_STATE_ATTR_LEN1:
			ancs_c->parse_info.parse_state =
				attr_len1_parse(ancs_c, data_src, &index);
			break;

		case BT_ANCS_PARSE_STATE_ATTR_LEN2:
			ancs_c->parse_info.parse_state =
				attr_len2_parse(ancs_c, data_src, &index);
			break;

		case BT_ANCS_PARSE_STATE_ATTR_DATA:
			ancs_c->parse_info.parse_state =
				attr_data_parse(ancs_c, data_src, &index);
			break;

		case BT_ANCS_PARSE_STATE_ATTR_SKIP:
			ancs_c->parse_info.parse_state =
				attr_skip(ancs_c, data_src, &index);
			break;

		case BT_ANCS_PARSE_STATE_DONE:
			LOG_DBG("Parse state: Done ");
			index = data_len;
			break;

		default:
			/* Default case will never trigger intentionally.
			 * Go to the DONE state to minimize the consequences.
			 */
			ancs_c->parse_info.parse_state = BT_ANCS_PARSE_STATE_DONE;
			break;
		}
	}
}
