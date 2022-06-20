/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/shell/shell.h>

#include <zboss_api.h>
#include <zigbee/zigbee_error_handler.h>
#include <zb_nrf_platform.h>
#include <zigbee/zigbee_shell.h>
#include "zigbee_shell_utils.h"

#define IC_ADD_HELP \
	("Add install code for device with given eui64.\n" \
	"Usage: add <h:install_code> <h:eui64>")

#define IC_LIST_HELP \
	("Read install codes stored at the device.\n" \
	"Usage: list")

#define IC_POLICY_HELP \
	("Set Trust Center install code policy.\n" \
	"Usage: policy <enable|disable>")

#define IC_SET_HELP \
	("Add install code for the device.\n" \
	"Usage: set <h:install_code>")

#define CHANNEL_HELP \
	("Set/get channel.\n" \
	"Usage: channel [<n>]\n" \
	"If n is [11:26], set to that channel. Otherwise, treat n as bitmask.")

#define CHILD_MAX_HELP \
	("Set max_child number.\n" \
	"Usage: child_max <d:children_nbr>")

#define EXTPANID_HELP \
	("Set/get extpanid.\n" \
	"Usage: extpanid [<h:id>]")

#define NWKKEY_HELP \
	("Set network key.\n" \
	"Usage: nwkkey <h:key>")

#define PANID_HELP \
	("Set/get panid.\n" \
	"Usage: panid [<h:id>]")

/* Forward declarations. */
#ifndef ZB_ED_ROLE
static void zb_install_code_list_read(zb_uint8_t buffer, zb_uint16_t start_index);
#endif

/* Install code list entry structure. */
struct __packed zb_secur_ic_list_entry {
	/* Partner address */
	zb_ieee_addr_t device_address;
	zb_uint8_t options;
	zb_uint8_t reserved;
	/* 16b installcode +2b crc. */
	zb_uint8_t installcode[ZB_CCM_KEY_SIZE + ZB_CCM_KEY_CRC_SIZE];
};

struct ic_cmd_ctx {
	volatile bool taken;
	zb_ieee_addr_t addr;
	zb_uint8_t ic[ZB_CCM_KEY_SIZE + ZB_CCM_KEY_CRC_SIZE];
	const struct shell *shell;
};

struct ic_cmd_list_ctx {
	volatile bool taken;
	const struct shell *shell;
};

static zb_bool_t legacy_mode = ZB_FALSE;
#ifndef ZB_ED_ROLE
static struct ic_cmd_ctx ic_add_ctx = {0};
static struct ic_cmd_list_ctx ic_list_ctx = {0};
static zb_nwk_device_type_t default_role = ZB_NWK_DEVICE_TYPE_ROUTER;
#else
static zb_nwk_device_type_t default_role = ZB_NWK_DEVICE_TYPE_ED;
#endif

/**@brief Get Zigbee role set for the device.
 *
 * @return Zigbee role already set or role planned to set if stack is not yet started.
 */
static zb_nwk_device_type_t zb_shell_get_network_role(void)
{
	if (zigbee_is_stack_started()) {
		return zb_get_network_role();
	} else {
		return default_role;
	}
}

/**@brief Get Zigbee role of the device.
 *
 * @code
 * bdb role
 * @endcode
 *
 * @pre Reading only after @ref start "bdb start".
 *
 */
static int cmd_zb_role(const struct shell *shell, size_t argc, char **argv)
{
	zb_nwk_device_type_t role;

	if (!zigbee_is_stack_started()
	    && zigbee_is_nvram_initialised()
#ifdef CONFIG_ZIGBEE_SHELL_DEBUG_CMD
	    && zb_shell_nvram_enabled()
#endif /* CONFIG_ZIGBEE_SHELL_DEBUG_CMD */
	    ) {
		shell_warn(
			shell,
			"Zigbee stack has been configured in the past.\r\n"
			"Please start the Zigbee stack to check the configured role.");

		zb_shell_print_error(
			shell,
			"Can't get role before stack is started - NVRAM not empty",
			ZB_FALSE);
		return -ENOEXEC;
	}

	role = zb_shell_get_network_role();

	if (role == ZB_NWK_DEVICE_TYPE_COORDINATOR) {
		shell_print(shell, "zc");
	} else if (role == ZB_NWK_DEVICE_TYPE_ROUTER) {
		shell_print(shell, "zr");
	} else if (role == ZB_NWK_DEVICE_TYPE_ED) {
		shell_print(shell, "zed");
	}

	zb_shell_print_done(shell, ZB_FALSE);
	return 0;
}

/**@brief Set Zigbee coordinator role of the device.
 *
 * @code
 * bdb role zc
 * @endcode
 *
 * @pre Setting only before @ref start "bdb start".
 *
 */
static int cmd_zb_role_zc(const struct shell *shell, size_t argc, char **argv)
{
	if (zigbee_is_stack_started()) {
		zb_shell_print_error(shell, "Stack already started", ZB_FALSE);
		return -ENOEXEC;
	}

#ifdef ZB_ED_ROLE
	zb_shell_print_error(shell, "Role unsupported", ZB_FALSE);
	return -ENOEXEC;
#else
	if (zigbee_is_nvram_initialised()
#ifdef CONFIG_ZIGBEE_SHELL_DEBUG_CMD
	    && zb_shell_nvram_enabled()
#endif /* CONFIG_ZIGBEE_SHELL_DEBUG_CMD */
	    ) {
		shell_warn(
			shell,
			"Zigbee stack has been configured in the past.\r\n"
			"Please use the same role or disable NVRAM to change the Zigbee role.");
	} else {
		shell_info(
			shell,
			"Zigbee shell does not erase the NVRAM between reboots, but is not aware of the previously configured role.\r\n"
			"Remember to set the coordinator role after rebooting the device.");
	}

	default_role = ZB_NWK_DEVICE_TYPE_COORDINATOR;
	shell_print(shell, "Coordinator set");
	zb_shell_print_done(shell, ZB_FALSE);
	return 0;
#endif
}

/**@brief Set Zigbee End Device role of the device.
 *
 * @code
 * bdb role zed
 * @endcode
 *
 * @pre Setting only before @ref start "bdb start".
 *
 * @note Zigbee End Device role is not currently supported to be switched to
 *       from Router or Coordinator role.
 *
 */
static int cmd_zb_role_zed(const struct shell *shell, size_t argc, char **argv)
{
	if (zigbee_is_stack_started()) {
		zb_shell_print_error(shell, "Stack already started", ZB_FALSE);
		return -ENOEXEC;
	}

#ifdef ZB_ED_ROLE
	default_role = ZB_NWK_DEVICE_TYPE_ED;
	shell_print(shell, "End Device role set");
	zb_shell_print_done(shell, ZB_FALSE);
	return 0;
#else
	zb_shell_print_error(shell, "Role unsupported", ZB_FALSE);
	return -ENOEXEC;
#endif
}

/**@brief Set Zigbee Router role of the device.
 *
 * @code
 * bdb role zr
 * @endcode
 *
 * @pre Setting only before @ref start "bdb start".
 *
 */
static int cmd_zb_role_zr(const struct shell *shell, size_t argc, char **argv)
{
	if (zigbee_is_stack_started()) {
		zb_shell_print_error(shell, "Stack already started", ZB_FALSE);
		return -ENOEXEC;
	}

#ifdef ZB_ED_ROLE
	zb_shell_print_error(shell, "Role unsupported", ZB_FALSE);
	return -ENOEXEC;
#else
	if (zigbee_is_nvram_initialised()
#ifdef CONFIG_ZIGBEE_SHELL_DEBUG_CMD
	    && zb_shell_nvram_enabled()
#endif /* CONFIG_ZIGBEE_SHELL_DEBUG_CMD */
	    ) {
		shell_warn(
			shell,
			"Zigbee stack has been configured in the past.\r\n"
			"Please use the same role or disable NVRAM to change the Zigbee role.");
	}

	default_role = ZB_NWK_DEVICE_TYPE_ROUTER;
	shell_print(shell, "Router role set");
	zb_shell_print_done(shell, ZB_FALSE);
	return 0;
#endif
}

/**@brief Start bdb top level commissioning process.
 *
 * This command can be called multiple times.
 *
 * If the ZBOSS thread has not been created yet, `zigbee_enable` is called to
 * create the ZBOSS thread and the Zigbee stack is started. The subsequent
 * behavior of the device depends on the implementation of the
 * `zboss_signal_handler` function. If `zigbee_default_signal_handler` is used
 * to handle a ZBOSS signals, the device will attempt to join or form a network
 * (depending on the role) after the Zigbee stack is started.
 *
 * If the device is not on a network but the ZBOSS thread has been created
 * (so the Zigbee stack is started), then it will attempt to join
 * or form a network (depending on the role).
 *
 * If the device is on the network then the command will open the network for
 * new devices to join.
 *
 * See Base Device Behaviour specification for details.
 *
 * @code
 * > bdb start
 * Started coordinator
 * Done
 * @endcode
 */
static int cmd_zb_start(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	uint32_t channel;
	zb_bool_t ret = ZB_TRUE;
	zb_uint8_t mode_mask = ZB_BDB_NETWORK_STEERING;

	if ((!zigbee_is_stack_started())) {
		channel = zb_get_bdb_primary_channel_set();

		switch (default_role) {
#ifndef ZB_ED_ROLE
		case ZB_NWK_DEVICE_TYPE_ROUTER:
			zb_set_network_router_role(channel);
			shell_print(shell, "Started router");
			break;

		case ZB_NWK_DEVICE_TYPE_COORDINATOR:
			zb_set_network_coordinator_role(channel);
			shell_print(shell, "Started coordinator");
			break;
#else
		case ZB_NWK_DEVICE_TYPE_ED:
			zb_set_network_ed_role(channel);
			shell_print(shell, "Started End Device");
			break;
#endif
		default:
			zb_shell_print_error(shell, "Role unsupported", ZB_FALSE);
			return -ENOEXEC;
		}

		zigbee_enable();
	} else {
		/* Handle case where Zigbee coordinator has left the network
		 * and a new network needs to be formed.
		 */
		if ((default_role == ZB_NWK_DEVICE_TYPE_COORDINATOR) && !ZB_JOINED()) {
			mode_mask = ZB_BDB_NETWORK_FORMATION;
		}
		ret = bdb_start_top_level_commissioning(mode_mask);
	}

	if (ret) {
		zb_shell_print_done(shell, ZB_FALSE);
		return 0;
	} else {
		zb_shell_print_error(shell, "Could not start top level commissioning", ZB_FALSE);
		return -ENOEXEC;
	}
}

/**@brief Set or get the Zigbee Extended Pan ID value.
 *
 * @code
 * bdb extpanid [<h:id>]
 * @endcode
 *
 * @pre Setting only before @ref start "bdb start". Reading only after
 *      @ref start "bdb start".
 *
 * If the optional argument is not provided, gets the extended PAN ID
 * of the joined network.
 *
 * If the optional argument is provided, gets the extended PAN ID to `id`.
 *
 *
 */
static int cmd_zb_extpanid(const struct shell *shell, size_t argc, char **argv)
{
	zb_ext_pan_id_t extpanid;

	if (argc == 1) {
		zb_get_extended_pan_id(extpanid);
		zb_shell_print_eui64(shell, extpanid);
		/* Prepend newline because `zb_shell_print_eui64` does not
		 * print LF.
		 */
		zb_shell_print_done(shell, ZB_TRUE);
	} else if (argc == 2) {
		if (zigbee_is_stack_started()) {
			zb_shell_print_error(shell, "Stack already started", ZB_FALSE);
			return -ENOEXEC;
		}

		if (zigbee_is_nvram_initialised()
#ifdef CONFIG_ZIGBEE_SHELL_DEBUG_CMD
		    && zb_shell_nvram_enabled()
#endif /* CONFIG_ZIGBEE_SHELL_DEBUG_CMD */
		    ) {
			shell_warn(
				shell,
				"Zigbee stack has been configured in the past.\r\n"
				"Please disable NVRAM to change the Extended PAN ID.");

			zb_shell_print_error(
				shell,
				"Can't change extpanid - NVRAM not empty",
				ZB_FALSE);
			return -ENOEXEC;
		}

		if (parse_long_address(argv[1], extpanid)) {
			zb_set_extended_pan_id(extpanid);
			zb_shell_print_done(shell, ZB_FALSE);
		} else {
			zb_shell_print_error(shell, "Failed to parse extpanid", ZB_FALSE);
			return -EINVAL;
		}
	}

	return 0;
}

/**@brief Set or get the Zigbee Pan ID value.
 *
 * @code
 * bdb panid [<h:id>]
 * @endcode
 *
 * @pre Setting only before @ref start "bdb start". Reading only
 *      after @ref start "bdb start".
 *
 * If the optional argument is not provided, gets the PAN ID
 * of the joined network.
 * If the optional argument is provided, sets the PAN ID to `id`.
 *
 */
static int cmd_zb_panid(const struct shell *shell, size_t argc, char **argv)
{
	zb_uint16_t pan_id;

	if (argc == 1) {
		shell_print(shell, "%0X", ZB_PIBCACHE_PAN_ID());
		zb_shell_print_done(shell, ZB_FALSE);
	} else if (argc == 2) {
		if (zigbee_is_stack_started()) {
			zb_shell_print_error(shell, "Stack already started", ZB_FALSE);
			return -ENOEXEC;
		}

		if (zigbee_is_nvram_initialised()
#ifdef CONFIG_ZIGBEE_SHELL_DEBUG_CMD
		    && zb_shell_nvram_enabled()
#endif /* CONFIG_ZIGBEE_SHELL_DEBUG_CMD */
		    ) {
			shell_warn(
				shell,
				"Zigbee stack has been configured in the past.\r\n"
				"Please disable NVRAM to change the PAN ID.");

			zb_shell_print_error(
				shell,
				"Can't change PAN ID - NVRAM not empty",
				ZB_FALSE);
			return -ENOEXEC;
		}

		if (parse_hex_u16(argv[1], &pan_id)) {
			ZB_PIBCACHE_PAN_ID() = pan_id;
			zb_shell_print_done(shell, ZB_FALSE);
		} else {
			zb_shell_print_error(shell, "Failed to parse PAN ID", ZB_FALSE);
			return -EINVAL;
		}
	} else {
		zb_shell_print_error(shell, "Unsupported format. Expected panid <h:id>", ZB_FALSE);
		return -EINVAL;
	}

	return 0;
}

/**@brief Set or get 802.15.4 channel.
 *
 * @code
 * bdb channel [<n>]
 * @endcode
 *
 *  @pre Setting only before @ref start "bdb start".
 *
 * If the optional argument is not provided, get the current number
 * and bitmask of the channel.
 *
 * If the optional argument is provided:
 *   - If `n` is in [11:26] range, set to that channel.
 *   - Otherwise, treat `n` as bitmask (logical or of a single bit shifted
 *     by channel number).
 *
 *
 * Example:
 * @code
 * > bdb channel  0x110000
 * Setting channel bitmask to 110000
 * Done
 * @endcode
 */
static int cmd_zb_channel(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t chan[2];
	uint32_t channel_number = 0;
	uint32_t channel_mask = 0;

	if (argc == 1) {
		int i;
		int c;

		chan[0] = zb_get_bdb_primary_channel_set();
		chan[1] = zb_get_bdb_secondary_channel_set();

		/* Check for case in which channel mask can not be read. */
		if (zigbee_is_nvram_initialised() && !zigbee_is_stack_started()
#ifdef CONFIG_ZIGBEE_SHELL_DEBUG_CMD
		    && zb_shell_nvram_enabled()
#endif /* CONFIG_ZIGBEE_SHELL_DEBUG_CMD */
		   ) {
			shell_warn(
				shell,
				"Zigbee stack has been configured in the past.\r\n"
				"Please start the Zigbee stack to check the configured channels.");

			zb_shell_print_error(
				shell,
				"Can't get channel mask - NVRAM not empty",
				ZB_FALSE);
			return -ENOEXEC;
		}

		/* If channel lists are empty, the default channel will be used.
		 * Set it to correct value so it can be printed correctly.
		 */
		if (chan[0] == 0) {
#if defined CONFIG_ZIGBEE_CHANNEL_SELECTION_MODE_SINGLE
			chan[0] = (1U << CONFIG_ZIGBEE_CHANNEL);
			chan[1] = (1U << CONFIG_ZIGBEE_CHANNEL);
#elif defined CONFIG_ZIGBEE_CHANNEL_SELECTION_MODE_MULTI
			chan[0] = CONFIG_ZIGBEE_CHANNEL_MASK;
			chan[1] = CONFIG_ZIGBEE_CHANNEL_MASK;
#endif
		}

		/* Print for both channels. */
		for (c = 0; c < 2; c++) {
			shell_fprintf(shell, SHELL_NORMAL, "%s channel(s):",
				      c == 0 ? "Primary" : "Secondary");
			for (i = 11; i <= 26; i++) {
				if ((1 << i) & chan[c]) {
					shell_fprintf(shell, SHELL_NORMAL,
						      " %d", i);
				}
			}
			shell_print(shell, "");
		}

		zb_shell_print_done(shell, ZB_FALSE);
		return 0;
	} else if (argc == 2) {
		if (zigbee_is_stack_started()) {
			zb_shell_print_error(shell, "Stack already started", ZB_FALSE);
			return -ENOEXEC;
		}

		if (zigbee_is_nvram_initialised()
#ifdef CONFIG_ZIGBEE_SHELL_DEBUG_CMD
		    && zb_shell_nvram_enabled()
#endif /* CONFIG_ZIGBEE_SHELL_DEBUG_CMD */
		    ) {
			shell_warn(
				shell,
				"Zigbee stack has been configured in the past.\r\n"
				"Please disable NVRAM to change the channel mask.");

			zb_shell_print_error(
				shell,
				"Can't change channel mask - NVRAM not empty",
				ZB_FALSE);
			return -ENOEXEC;
		}

		zb_shell_sscan_uint(argv[1], (uint8_t *)&channel_number, 4, 10);
		if (channel_number < 11 || channel_number > 26) {
			/* Treat as a bitmask. */
			channel_number = 0;
			zb_shell_sscan_uint(argv[1], (uint8_t *)&channel_mask, 4, 16);

			if ((!(channel_mask & 0x7FFF800)) || (channel_mask & (~0x7FFF800))) {
				zb_shell_print_error(shell, "Bitmask invalid", ZB_FALSE);
				return -EINVAL;
			}
		} else {
			/* Treat as number. */
			channel_mask = 1 << channel_number;
		}

		if (zb_get_bdb_primary_channel_set() != channel_mask) {
			if (channel_number) {
				shell_print(shell, "Setting channel to %d",
					    channel_number);
			} else {
				shell_print(shell,
					    "Setting channel bitmask to %x",
					    channel_mask);
			}

			zb_set_bdb_primary_channel_set(channel_mask);
			zb_set_bdb_secondary_channel_set(channel_mask);
			zb_set_channel_mask(channel_mask);
		}

		zb_shell_print_done(shell, ZB_FALSE);
	}
	return 0;
}

#if !defined(ZB_ED_ROLE)

static void zb_secur_ic_add_cb(zb_ret_t status)
{
	if (status != RET_OK) {
		zb_shell_print_error(ic_add_ctx.shell, "Failed to add IC", ZB_FALSE);
	} else {
		zb_shell_print_done(ic_add_ctx.shell, ZB_FALSE);
	}

	ic_add_ctx.taken = false;
}

/**@brief Function adding install code, to be executed in Zigbee thread context.
 *
 * @param[in] param Unused param.
 */
void zb_install_code_add(zb_uint8_t param)
{
	ARG_UNUSED(param);

	if (zb_get_network_role() != ZB_NWK_DEVICE_TYPE_COORDINATOR) {
		zb_shell_print_error(
			ic_add_ctx.shell,
			"Failed to add IC. Device must be a coordinator",
			ZB_FALSE);
		return;
	}

	zb_secur_ic_add(ic_add_ctx.addr, ZB_IC_TYPE_128, ic_add_ctx.ic, zb_secur_ic_add_cb);
}

/**@brief Function to print install codes list table header.
 */
static inline void zb_install_code_list_print_header(void)
{
	shell_print(ic_list_ctx.shell,
		    "[idx] EUI64:           IC:                                  options:");
}

/**@brief Function to print number of entries in the install codes table.
 *
 * @param[in] total_rows    Number of rows present in the install code table.
 */
static inline void zb_install_code_list_print_summary(zb_uint8_t total_entries)
{
	shell_print(ic_list_ctx.shell,
		    "Total entries for the install codes table: %d",
		    total_entries);
}

/**@brief Function to print install codes list table row.
 *
 * @param[in] ic            Pointer to zb_secur_ic_list_entry structure to read install codes from.
 * @param[in] index         Index of install code in the install code table.
 */
static void zb_install_code_list_print_row(struct zb_secur_ic_list_entry *ic, zb_uint8_t index)
{
	/* Print row index. */
	shell_fprintf(ic_list_ctx.shell, SHELL_NORMAL, "[%3d] ", index);
	/* Print device long address. */
	zb_shell_print_eui64(ic_list_ctx.shell, ic->device_address);
	/* Print space to separate device address and install code fields. */
	shell_fprintf(ic_list_ctx.shell, SHELL_NORMAL, " ");
	/* Print the install code. */
	zb_shell_print_hexdump(
		ic_list_ctx.shell,
		ic->installcode,
		(ZB_CCM_KEY_SIZE + ZB_CCM_KEY_CRC_SIZE),
		false);
	/* Print space to separate install code and options fields. */
	shell_fprintf(ic_list_ctx.shell, SHELL_NORMAL, " ");
	/* Print options with additional newline character. */
	shell_print(ic_list_ctx.shell, "%#02x", ic->options);
}

/**@brief Callback function called when install codes are read and ready to be printed.
 *        To be executed in Zigbee thread context.
 *
 * @param[in] buffer        Buffer ID.
 */
static void zb_install_code_list_read_cb(zb_uint8_t buffer)
{
	zb_uint8_t ic_entries_left_to_read = 0;
	struct zb_secur_ic_list_entry *ic = NULL;
	zb_secur_ic_get_list_resp_t *ic_list_resp = NULL;

	if (ic_list_ctx.taken == false) {
		goto exit;
	}

	/* Check if buffer id is correct. */
	if (buffer == ZB_BUF_INVALID) {
		zb_shell_print_error(
			ic_list_ctx.shell,
			"Invalid buffer ID received in cb",
			ZB_FALSE);
		goto exit;
	}

	ic_list_resp = ZB_BUF_GET_PARAM(buffer, zb_secur_ic_get_list_resp_t);

	if (ic_list_resp->status != RET_OK) {
		zb_shell_print_error(
			ic_list_ctx.shell,
			"Error status when reading IC list.",
			ZB_FALSE);
		goto exit;
	}

	/* Set pointer to first ic struct in the buffer. */
	ic = zb_buf_begin(buffer);

	/* If zero index row is to be printed, print table header first. */
	if (ic_list_resp->start_index == 0) {
		zb_install_code_list_print_header();
	}

	/* Print every ic in the separate row. */
	for (zb_uint8_t id = 0; id < ic_list_resp->ic_table_list_count; id++) {
		zb_install_code_list_print_row((ic + id), (ic_list_resp->start_index + id));
	}

	/* Calculate how many entries left to be read. */
	ic_entries_left_to_read = ic_list_resp->ic_table_entries;
	ic_entries_left_to_read -= (ic_list_resp->start_index + ic_list_resp->ic_table_list_count);

	if (ic_entries_left_to_read > 0) {
		zb_uint8_t new_start_index = (ic_list_resp->ic_table_entries
						- ic_entries_left_to_read);

		/* Reuse the same buffer to read remaining install codes. */
		zb_buf_reuse(buffer);
		zb_install_code_list_read(buffer, new_start_index);
		return;
	}

	/* All install codes have been read, print summary and done msg and clean up. */
	zb_install_code_list_print_summary(ic_list_resp->ic_table_entries);
	zb_shell_print_done(ic_list_ctx.shell, ZB_FALSE);

exit:
	ic_list_ctx.taken = false;
	if (buffer != ZB_BUF_INVALID) {
		zb_buf_free(buffer);
	}
}

/**@brief Function to start reading install codes stored at the device.
 *        To be executed in Zigbee thread context.
 *
 * @param[in] buffer        Buffer ID.
 * @param[in] start_index   Index of IC list to start reading install codes from.
 */
static void zb_install_code_list_read(zb_uint8_t buffer, zb_uint16_t start_index)
{
	zb_secur_ic_get_list_req_t *ic_list_req = NULL;

	/* Check if buffer id is correct. */
	if (buffer == ZB_BUF_INVALID) {
		zb_shell_print_error(ic_list_ctx.shell, "Invalid buffer ID received", ZB_FALSE);
		ic_list_ctx.taken = false;
		return;
	}

	/* Fill request data in the param. */
	ic_list_req = ZB_BUF_GET_PARAM(buffer, zb_secur_ic_get_list_req_t);
	ic_list_req->start_index = start_index;
	ic_list_req->response_cb = zb_install_code_list_read_cb;

	/* Call API to request IC list. */
	zb_secur_ic_get_list_req(buffer);
}
#endif /* !defined ZB_ED_ROLE */

/**@brief Set install code on the device, add information about the install code
 *  on the trust center, set the trust center install code policy.
 *
 * @code
 * bdb ic add <h:install_code> <h:eui64>
 * bdb ic set <h:install_code>
 * bdb ic policy <enable|disable>
 * bdb ic list
 * @endcode
 *
 * @pre Adding and reading install codes only after @ref start "bdb start".
 *
 * <tt>bdb ic set</tt> must only be used on a joining device.
 *
 * <tt>bdb ic add</tt> must only be used on a coordinator.
 *                     For <tt><h:eui64></tt>, use the address
 *                     of the joining device.
 *
 * <tt>bdb ic policy</tt> must only be used on a coordinator.
 *
 * <tt>bdb ic list</tt> must only be used on a coordinator.
 *
 * Provide the install code as an ASCII-encoded hex including CRC16.
 *
 * For production devices, an install code must be installed by the production
 * configuration present in flash.
 *
 *
 * Example:
 * @code
 * > bdb ic add 83FED3407A939723A5C639B26916D505C3B5 0B010E2F79E9DBFA
 * Done
 * @endcode
 */
static int cmd_zb_install_code(const struct shell *shell, size_t argc,
			       char **argv)
{
	const char *err_msg = NULL;

	if ((argc == 2) && (strcmp(argv[0], "set") == 0)) {
		zb_uint8_t ic[ZB_CCM_KEY_SIZE + ZB_CCM_KEY_CRC_SIZE];

		if (zb_shell_get_network_role() == ZB_NWK_DEVICE_TYPE_COORDINATOR) {
			zb_shell_print_error(shell, "Device can't be a coordinator", ZB_FALSE);
			return -ENOEXEC;
		}

		if (zigbee_is_stack_started()) {
			zb_shell_print_error(shell, "Stack already started", ZB_FALSE);
			return -ENOEXEC;
		}

		if (!parse_hex_str(argv[1], strlen(argv[1]), ic, sizeof(ic),
				   false)) {
			err_msg = "Failed to parse IC";
			goto exit;
		}

		if (zb_secur_ic_set(ZB_IC_TYPE_128, ic) != RET_OK) {
			err_msg = "Failed to set IC";
			goto exit;
		}
#ifndef ZB_ED_ROLE
	} else if ((argc == 3) && (strcmp(argv[0], "add") == 0)) {
		/* Check if stack is initialized as Install Code can not
		 * be added until production config is initialised.
		 */
		if (zb_shell_get_network_role() != ZB_NWK_DEVICE_TYPE_COORDINATOR) {
			zb_shell_print_error(shell, "Device must be a coordinator", ZB_FALSE);
			return -ENOEXEC;
		}

		if (!zigbee_is_stack_started()) {
			zb_shell_print_error(shell, "Stack not started", ZB_FALSE);
			return -ENOEXEC;
		}

		ic_add_ctx.shell = shell;

		if (ic_add_ctx.taken == true) {
			err_msg = "Can not get ctx to store install code";
			goto exit;
		}

		ic_add_ctx.taken = true;

		if (!parse_hex_str(argv[1], strlen(argv[1]), ic_add_ctx.ic,
				   sizeof(ic_add_ctx.ic), false)) {
			err_msg = "Failed to parse IC";
			ic_add_ctx.taken = false;
			goto exit;
		}

		if (!parse_long_address(argv[2], ic_add_ctx.addr)) {
			err_msg = "Failed to parse eui64";
			ic_add_ctx.taken = false;
			goto exit;
		}

		if (ZB_SCHEDULE_APP_CALLBACK(zb_install_code_add, 0) !=
		    RET_OK) {
			err_msg = "Can not add install code";
			ic_add_ctx.taken = false;
			goto exit;
		}
		return 0;

	} else if ((argc == 2) && (strcmp(argv[0], "policy") == 0)) {
		if (zb_shell_get_network_role() != ZB_NWK_DEVICE_TYPE_COORDINATOR) {
			zb_shell_print_error(shell, "Device must be a coordinator", ZB_FALSE);
			return -ENOEXEC;
		}

		if (strcmp(argv[1], "enable") == 0) {
			zb_set_installcode_policy(ZB_TRUE);
		} else if (strcmp(argv[1], "disable") == 0) {
			zb_set_installcode_policy(ZB_FALSE);
		} else {
			err_msg = "Syntax error";
			goto exit;
		}
	} else if ((argc == 1) && (strcmp(argv[0], "list") == 0)) {
		zb_ret_t ret_val = RET_OK;

		if (zb_shell_get_network_role() != ZB_NWK_DEVICE_TYPE_COORDINATOR) {
			zb_shell_print_error(shell, "Device must be a coordinator", ZB_FALSE);
			return -ENOEXEC;
		}

		if (!zigbee_is_stack_started()) {
			zb_shell_print_error(shell, "Stack not started", ZB_FALSE);
			return -ENOEXEC;
		}

		if (ic_list_ctx.taken == true) {
			zb_shell_print_error(
				shell,
				"Can't start reading IC list - already in progress.",
				ZB_FALSE);
			return -ENOEXEC;
		}

		ic_list_ctx.taken = true;
		ic_list_ctx.shell = shell;

		/* Schedule reading IC list immediately, start from index 0. */
		ret_val = zb_buf_get_out_delayed_ext(zb_install_code_list_read, 0, 0);

		if (ret_val != RET_OK) {
			ic_list_ctx.taken = false;
			zb_shell_print_error(
				shell,
				"Couldn't get buffer for reading IC list.",
				ZB_FALSE);
			return -ENOEXEC;
		}

		return 0;
#endif
	} else {
		err_msg = "Syntax error";
	}

exit:
	if (err_msg) {
		zb_shell_print_error(shell, err_msg, ZB_FALSE);
		return -EINVAL;
	} else {
		zb_shell_print_done(shell, ZB_FALSE);
		return 0;
	}
}

/**@brief Get the state of the legacy device support.
 *
 * @code
 * bdb legacy
 * @endcode
 *
 * Example:
 * @code
 * > bdb legacy
 * on
 * Done
 * @endcode
 */
static int cmd_zb_legacy(const struct shell *shell, size_t argc, char **argv)
{
	if (zb_shell_get_network_role() != ZB_NWK_DEVICE_TYPE_COORDINATOR) {
		zb_shell_print_error(shell, "Device must be a coordinator", ZB_FALSE);
		return -ENOEXEC;
	}

	shell_print(shell, "%s", legacy_mode ? "on" : "off");
	zb_shell_print_done(shell, ZB_FALSE);
	return 0;
}

/**@brief Enable the legacy device support.
 *
 * @code
 * bdb legacy enable
 * @endcode
 *
 * Allow pre-r21 devices on the Zigbee network.
 *
 * @pre Use only after @ref start "bdb start".
 *
 * Example:
 * @code
 * > bdb legacy enable
 * Done
 * @endcode
 */
static int cmd_zb_legacy_enable(const struct shell *shell, size_t argc,
				char **argv)
{
	if (!zigbee_is_stack_started()) {
		zb_shell_print_error(shell, "Stack not started", ZB_FALSE);
		return -ENOEXEC;
	}

	if (zb_get_network_role() != ZB_NWK_DEVICE_TYPE_COORDINATOR) {
		zb_shell_print_error(shell, "Device must be a coordinator", ZB_FALSE);
		return -ENOEXEC;
	}

	zb_bdb_set_legacy_device_support(1);
	legacy_mode = ZB_TRUE;

	if (ZB_SCHEDULE_APP_CALLBACK(zb_bdb_set_legacy_device_support, legacy_mode)) {
		zb_shell_print_error(shell, "Can not execute command", ZB_FALSE);
		return -ENOEXEC;
	}

	zb_shell_print_done(shell, ZB_FALSE);
	return 0;
}

/**@brief Disable the legacy device support.
 *
 * @code
 * bdb legacy disable
 * @endcode
 *
 * Disallow legacy pre-r21 devices on the Zigbee network.
 *
 * @pre Use only after @ref start "bdb start".
 *
 * Example:
 * @code
 * > bdb legacy disable
 * Done
 * @endcode
 */
static int cmd_zb_legacy_disable(const struct shell *shell, size_t argc,
				 char **argv)
{
	if (!zigbee_is_stack_started()) {
		zb_shell_print_error(shell, "Stack not started", ZB_FALSE);
		return -ENOEXEC;
	}

	if (zb_get_network_role() != ZB_NWK_DEVICE_TYPE_COORDINATOR) {
		zb_shell_print_error(shell, "Device must be a coordinator", ZB_FALSE);
		return -ENOEXEC;
	}

	zb_bdb_set_legacy_device_support(0);
	legacy_mode = ZB_FALSE;

	if (ZB_SCHEDULE_APP_CALLBACK(zb_bdb_set_legacy_device_support, legacy_mode)) {
		zb_shell_print_error(shell, "Can not execute command", ZB_FALSE);
		return -ENOEXEC;
	}

	zb_shell_print_done(shell, ZB_FALSE);
	return 0;
}

/**@brief Set network key.
 *
 * @code
 * bdb nwkkey <h:key>
 * @endcode
 *
 * Set a pre-defined network key instead of a random one.
 *
 * @pre Setting only before @ref start "bdb start".
 *
 * Example:
 * @code
 * > bdb nwkkey 00112233445566778899aabbccddeeff
 * Done
 * @endcode
 */
static int cmd_zb_nwkkey(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	if (zigbee_is_stack_started()) {
		zb_shell_print_error(shell, "Stack already started", ZB_FALSE);
		return -ENOEXEC;
	}

	if (zigbee_is_nvram_initialised()
#ifdef CONFIG_ZIGBEE_SHELL_DEBUG_CMD
	    && zb_shell_nvram_enabled()
#endif /* CONFIG_ZIGBEE_SHELL_DEBUG_CMD */
	    ) {
		shell_warn(
			shell,
			"Zigbee stack has been configured in the past.\r\n"
			"Please disable NVRAM to change the preconfigured network key.");

		zb_shell_print_error(shell, "Can't change NWK key - NVRAM not empty", ZB_FALSE);
		return -ENOEXEC;
	}

	zb_uint8_t key[ZB_CCM_KEY_SIZE];

	if (parse_hex_str(argv[1], strlen(argv[1]), key, sizeof(key), false)) {
		zb_secur_setup_nwk_key(key, 0);
	} else {
		zb_shell_print_error(shell, "Failed to parse key", ZB_FALSE);
		return -EINVAL;
	}

	zb_shell_print_done(shell, ZB_FALSE);
	return 0;
}

/**@brief Perform a factory reset via local action
 *
 * See Base Device Behavior specification chapter 9.5 for details.
 *
 * @code
 * > bdb factory_reset
 * Done
 * @endcode
 */
static int cmd_zb_factory_reset(const struct shell *shell, size_t argc,
				char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Do not allow to call factory_reset before the ZBOSS stack is started. */
	if (!zigbee_is_stack_started()) {
		zb_shell_print_error(shell, "Stack not started", ZB_FALSE);
		return -ENOEXEC;
	}

	ZB_SCHEDULE_APP_CALLBACK(zb_bdb_reset_via_local_action, 0);
	zb_shell_print_done(shell, ZB_FALSE);
	return 0;
}

/**@brief Set amount of the child devices which can be connected to the device.
 *
 * @code
 * bdb child_max <d:children_nbr>
 * @endcode
 *
 *  @pre Setting only before @ref start "bdb start".
 *
 * If the argument is provided and `children_nbr` is in [0:32] range,
 * set to that number. Otherwise, return error.
 *
 *
 * Example:
 * @code
 * > bdb child_max 16
 * Setting max children to: 16
 * Done
 * @endcode
 */
#ifndef ZB_ED_ROLE
static int cmd_child_max(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	uint32_t child_max = 0xFFFFFFFF;

	/* Two argc - set the amount of the max_children. */
	if (zigbee_is_stack_started()) {
		zb_shell_print_error(shell, "Stack already started", ZB_FALSE);
		return -ENOEXEC;
	}

	zb_shell_sscan_uint(argv[1], (uint8_t *)&child_max, 4, 10);
	if (child_max > 32) {
		zb_shell_print_error(
			shell,
			"Children device number must be within [0:32]",
			ZB_FALSE);
		return -EINVAL;
	} else {
		/* Set the value by calling ZBOSS API. */
		zb_set_max_children(child_max);
		shell_print(shell, "Setting max children to: %d", child_max);
	}

	zb_shell_print_done(shell, ZB_FALSE);
	return 0;
}
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(sub_ic,
#ifndef ZB_ED_ROLE
	SHELL_CMD_ARG(add, NULL, IC_ADD_HELP, cmd_zb_install_code, 3, 0),
	SHELL_CMD_ARG(list, NULL, IC_LIST_HELP, cmd_zb_install_code, 1, 0),
	SHELL_CMD_ARG(policy, NULL, IC_POLICY_HELP, cmd_zb_install_code, 2, 0),
#endif
	SHELL_CMD_ARG(set, NULL, IC_SET_HELP, cmd_zb_install_code, 2, 0),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_legacy,
	SHELL_CMD_ARG(disable, NULL, "Disable legacy mode.",
		      cmd_zb_legacy_disable, 1, 0),
	SHELL_CMD_ARG(enable, NULL, "Enable legacy mode.",
		      cmd_zb_legacy_enable, 1, 0),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_role,
	SHELL_CMD_ARG(zc, NULL, "Set Coordinator role.",
		      cmd_zb_role_zc, 1, 0),
	SHELL_CMD_ARG(zed, NULL, "Set End Device role.",
		      cmd_zb_role_zed, 1, 0),
	SHELL_CMD_ARG(zr, NULL, "Set Router role.",
		      cmd_zb_role_zr, 1, 0),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_bdb,
	SHELL_CMD_ARG(channel, NULL, CHANNEL_HELP, cmd_zb_channel, 1, 1),
#ifndef ZB_ED_ROLE
	SHELL_CMD_ARG(child_max, NULL, CHILD_MAX_HELP, cmd_child_max, 2, 0),
#endif
	SHELL_CMD_ARG(extpanid, NULL, EXTPANID_HELP, cmd_zb_extpanid, 1, 1),
	SHELL_CMD_ARG(factory_reset, NULL, "Perform factory reset.",
				  cmd_zb_factory_reset, 1, 0),
	SHELL_CMD(ic, &sub_ic, "Install code manipulation.", NULL),
	SHELL_CMD_ARG(legacy, &sub_legacy, "Get legacy mode.", cmd_zb_legacy,
		      1, 0),
	SHELL_CMD_ARG(nwkkey, NULL, NWKKEY_HELP, cmd_zb_nwkkey, 2, 0),
	SHELL_CMD_ARG(panid, NULL, PANID_HELP, cmd_zb_panid, 1, 1),
	SHELL_CMD_ARG(role, &sub_role, ("Set/get role."), cmd_zb_role, 1, 1),
	SHELL_CMD_ARG(start, NULL, "Start commissionning.", cmd_zb_start, 1, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(bdb, &sub_bdb, "Base device behaviour manipulation.", NULL);
