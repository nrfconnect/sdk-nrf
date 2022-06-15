/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <zephyr/types.h>
#include <zephyr/shell/shell.h>

#include <zboss_api.h>
#include <zigbee/zigbee_error_handler.h>
#include <zigbee/zigbee_logger_eprxzcl.h>
#include "zigbee_shell_utils.h"


LOG_MODULE_DECLARE(zigbee_shell, CONFIG_ZIGBEE_SHELL_LOG_LEVEL);

extern zb_uint8_t zb_shell_ep_handler_attr(zb_bufid_t bufid);
extern zb_uint8_t zb_shell_ep_handler_generic_cmd(zb_bufid_t bufid);
extern zb_uint8_t zb_shell_ep_handler_report(zb_bufid_t bufid);
extern zb_uint8_t zb_shell_ep_handler_ping(zb_bufid_t bufid);
extern zb_uint8_t zb_shell_ep_handler_groups_cmd(zb_bufid_t bufid);

static zb_device_handler_t zb_ep_handlers[] = {
	zb_shell_ep_handler_attr,
	zb_shell_ep_handler_generic_cmd,
	zb_shell_ep_handler_report,
	zb_shell_ep_handler_ping,
	zb_shell_ep_handler_groups_cmd
};

zb_uint8_t zb_shell_ep_handler(zb_bufid_t bufid)
{
	unsigned int idx;
	uint8_t ep_handler_cnt = (sizeof(zb_ep_handlers) /
				  sizeof(zb_device_handler_t));

	if (IS_ENABLED(CONFIG_ZIGBEE_LOGGER_EP)) {
		(void)(zigbee_logger_eprxzcl_ep_handler(bufid));
	}

	for (idx = 0; idx < ep_handler_cnt; idx++) {
		if ((zb_ep_handlers[idx])(bufid) == ZB_TRUE) {
			return ZB_TRUE;
		}
	}

	return ZB_FALSE;
}

int zb_shell_zcl_attr_to_str(char *str_buf, uint16_t buf_len, zb_uint16_t attr_type,
			     zb_uint8_t *attr)
{
	int bytes_written = 0;
	int string_len;
	int i;

	if ((str_buf == NULL) || (attr == NULL)) {
		return -1;
	}

	switch (attr_type) {
	/* Boolean. */
	case ZB_ZCL_ATTR_TYPE_BOOL:
		bytes_written = snprintf(str_buf, buf_len, "%s",
					 *((zb_bool_t *)attr) ? "True"
					  : "False");
		break;

	/* 1 byte. */
	case ZB_ZCL_ATTR_TYPE_8BIT:
	case ZB_ZCL_ATTR_TYPE_8BITMAP:
	case ZB_ZCL_ATTR_TYPE_U8:
	case ZB_ZCL_ATTR_TYPE_8BIT_ENUM:
		bytes_written = snprintf(str_buf, buf_len, "%hu",
					 *((zb_uint8_t *)attr));
		break;

	case ZB_ZCL_ATTR_TYPE_S8:
		bytes_written = snprintf(str_buf, buf_len, "%hd",
					 *((zb_int8_t *)attr));
		break;

	/* 2 bytes. */
	case ZB_ZCL_ATTR_TYPE_16BIT:
	case ZB_ZCL_ATTR_TYPE_16BITMAP:
	case ZB_ZCL_ATTR_TYPE_U16:
	case ZB_ZCL_ATTR_TYPE_16BIT_ENUM:
		bytes_written = snprintf(str_buf, buf_len, "%hu",
					 *((zb_uint16_t *)attr));
		break;

	case ZB_ZCL_ATTR_TYPE_S16:
		bytes_written = snprintf(str_buf, buf_len, "%hd",
					 *((zb_int16_t *)attr));
		break;

	/* 4 bytes. */
	case ZB_ZCL_ATTR_TYPE_32BIT:
	case ZB_ZCL_ATTR_TYPE_32BITMAP:
	case ZB_ZCL_ATTR_TYPE_U32:
		bytes_written = snprintf(str_buf, buf_len, "%u",
					 *((zb_uint32_t *)attr));
		break;

	case ZB_ZCL_ATTR_TYPE_S32:
		bytes_written = snprintf(str_buf, buf_len, "%d",
					 *((zb_int32_t *)attr));
		break;

	/* String. */
	case ZB_ZCL_ATTR_TYPE_CHAR_STRING:
		string_len = attr[0];
		attr++;

		if ((buf_len - bytes_written) < (string_len + 1)) {
			return -1;
		}

		for (i = 0; i < string_len; i++) {
			str_buf[bytes_written + i] = ((char *)attr)[i];
		}
		str_buf[bytes_written + i] = '\0';
		bytes_written += string_len + 1;
		break;

	case ZB_ZCL_ATTR_TYPE_IEEE_ADDR:
		bytes_written = to_hex_str(str_buf, buf_len,
					   (const uint8_t *)attr,
					   sizeof(zb_64bit_addr_t), true);
		break;

	default:
		bytes_written = snprintf(str_buf, buf_len,
					 "Value type 0x%x unsupported",
					 attr_type);
		break;
	}

	return bytes_written;
}

zb_bool_t zb_shell_is_zcl_cmd_response(zb_zcl_parsed_hdr_t *zcl_hdr, struct ctx_entry *entry)
{
	zb_uint16_t remote_node_short = 0;
	struct zcl_packet_info *packet_info = &entry->zcl_data.pkt_info;

	if (zcl_hdr->addr_data.common_data.source.addr_type != ZB_ZCL_ADDR_TYPE_SHORT) {
		return ZB_FALSE;
	}

	if (packet_info->dst_addr_mode == ZB_APS_ADDR_MODE_64_ENDP_PRESENT) {
		remote_node_short = zb_address_short_by_ieee(packet_info->dst_addr.addr_long);
	} else {
		remote_node_short = packet_info->dst_addr.addr_short;
	}

	if (zcl_hdr->addr_data.common_data.source.u.short_addr != remote_node_short) {
		return ZB_FALSE;
	}

	if (zcl_hdr->profile_id != packet_info->prof_id) {
		return ZB_FALSE;
	}

	if (zcl_hdr->addr_data.common_data.src_endpoint != packet_info->dst_ep) {
		return ZB_FALSE;
	}

	return ZB_TRUE;
}

void zb_shell_zcl_cmd_timeout_cb(zb_uint8_t index)
{
	struct ctx_entry *entry = ctx_mgr_get_entry_by_index(index);

	if (entry == NULL) {
		LOG_ERR("Couldn't get attr entry %d - entry not found", index);
		return;
	}

	zb_shell_print_error(entry->shell, "Request timed out", ZB_FALSE);
	ctx_mgr_delete_entry(entry);
}

int zb_shell_sscan_uint8(const char *bp, uint8_t *value)
{
	/* strtoul() used as a replacement for lacking sscanf() and its output
	 * is validated to ensure that conversion was successful.
	 */
	char *end = NULL;
	unsigned long tmp_val;

	tmp_val = strtoul(bp, &end, 10);

	if (((tmp_val == 0) && (bp == end)) || (tmp_val > UINT8_MAX)) {
		return 0;
	}

	*value = tmp_val & 0xFF;

	return 1;
}

int zb_shell_sscan_uint(const char *bp, uint8_t *value, uint8_t size, uint8_t base)
{
	char *end = NULL;
	unsigned long tmp_val;

	tmp_val = strtoul(bp, &end, base);

	/* Validation steps:
	 *   - check if returned tmp_val is not zero - strtoul returns zero
	 *     if failed to convert string.
	 *   - check if end is not equal `bp` - end is set to point
	 *     to the first character after the number or to `pb`
	 *     if nothing is matched.
	 *   - check if returned tmp_val can be stored in variable of length
	 *     given by `size` argument.
	 */
	if ((tmp_val == 0) && (bp == end)) {
		return 0;
	}
	if (size == 4) {
		*((uint32_t *)value) = tmp_val & ((1 << (size * 8)) - 1);
	} else if (size == 2) {
		*((uint16_t *)value) = tmp_val & ((1 << (size * 8)) - 1);
	} else {
		*value = tmp_val & ((1 << (size * 8)) - 1);
	}

	return 1;
}

void zb_shell_print_hexdump(const struct shell *shell, const uint8_t *data, uint8_t size,
			    bool reverse)
{
	char addr_buf[2 * size + 1];
	int bytes_written = 0;

	memset(addr_buf, 0, sizeof(addr_buf));

	bytes_written = to_hex_str(addr_buf, (uint16_t)sizeof(addr_buf), data,
				   size, reverse);
	if (bytes_written < 0) {
		shell_fprintf(shell, SHELL_ERROR, "%s", "Unable to print hexdump");
	} else {
		shell_fprintf(shell, SHELL_NORMAL, "%s", addr_buf);
	}
}
