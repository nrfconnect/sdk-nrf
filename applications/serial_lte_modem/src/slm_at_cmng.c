/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <logging/log.h>
#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <net/socket.h>
#include <modem/modem_info.h>
#include <modem/modem_key_mgmt.h>
#include "slm_util.h"
#include "slm_at_host.h"
#include "slm_native_tls.h"
#include "slm_at_cmng.h"

LOG_MODULE_REGISTER(cmng, CONFIG_SLM_LOG_LEVEL);

/**@brief List of supported AT commands. */
enum slm_cmng_at_cmd {
	AT_XCMNG,
	AT_CMNG_MAX
};

/**@brief List of supported opcode */
enum slm_cmng_opcode {
	AT_CMNG_OP_WRITE,
	AT_CMNG_OP_LIST,
	AT_CMNG_OP_READ,
	AT_CMNG_OP_DELETE
};

/**@brief List of supported type */
enum slm_cmng_type {
	AT_CMNG_TYPE_CA_CERT,
	AT_CMNG_TYPE_CERT,
	AT_CMNG_TYPE_PRIV,
	AT_CMNG_TYPE_PSK,
	AT_CMNG_TYPE_PSK_ID,
};

/* global functions defined in different files */
void rsp_send(const uint8_t *str, size_t len);

/** forward declaration of cmd handlers **/
static int handle_at_xcmng(enum at_cmd_type cmd_type);

/**@brief SLM AT Command list type. */
static slm_at_cmd_list_t cmng_at_list[AT_CMNG_MAX] = {
	{AT_XCMNG, "AT%CMNG", handle_at_xcmng},
};

/* global variable defined in different files */
extern struct at_param_list at_param_list;
extern char rsp_buf[CONFIG_AT_CMD_RESPONSE_MAX_LEN];

/**@brief handle AT#XCMNG commands
 *  AT#XCMNG=<opcode>[,<sec_tag>[,<type>[,<content>]]]
 *  AT#XCMNG? READ command not supported
 *  AT#XCMNG=? READ command not supported
 */
static int handle_at_xcmng(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t op, type;
	nrf_sec_tag_t sec_tag;
	uint8_t *content;
	size_t len = CONFIG_AT_CMD_RESPONSE_MAX_LEN;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		if (at_params_valid_count_get(&at_param_list) < 2) {
			LOG_ERR("Parameter missed");
			return -EINVAL;
		}
		err = at_params_short_get(&at_param_list, 1, &op);
		if (err < 0) {
			LOG_ERR("Fail to get op parameter: %d", err);
			return err;
		}
		if (op > AT_CMNG_OP_DELETE) {
			LOG_ERR("Wrong XCMNG operation: %d", op);
			return -EPERM;
		}
		if (op == AT_CMNG_OP_LIST) {
			/* Currently not support list command */
			LOG_ERR("XCMNG List is not supported");
			return -EPERM;
		}
		if (at_params_valid_count_get(&at_param_list) < 4) {
			/* READ, WRITE, DELETE requires sec_tag and type */
			LOG_ERR("Parameter missed");
			return -EINVAL;
		}
		err = at_params_int_get(&at_param_list, 2, &sec_tag);
		if (err < 0) {
			LOG_ERR("Fail to get sec_tag parameter: %d", err);
			return err;
		};
		if (sec_tag > MAX_SLM_SEC_TAG) {
			LOG_ERR("Invalid security tag: %d", sec_tag);
			return -EINVAL;
		}
		err = at_params_short_get(&at_param_list, 3, &type);
		if (err < 0) {
			LOG_ERR("Fail to get type parameter: %d", err);
			return err;
		};
		if (op == AT_CMNG_OP_WRITE) {
			if (at_params_valid_count_get(&at_param_list) < 5) {
				/* WRITE requires sec_tag, type and content */
				LOG_ERR("Parameter missed");
				return -EINVAL;
			}
			content = k_malloc(CONFIG_AT_CMD_RESPONSE_MAX_LEN);
			err = util_string_get(&at_param_list, 4, content,
						   &len);
			if (err != 0) {
				LOG_ERR("Failed to get content");
				k_free(content);
				return err;
			}
			err = modem_key_mgmt_write(
				slm_tls_map_sectag(sec_tag, type),
				0, content, len);
			if (err != 0) {
				LOG_ERR("FAILED! modem_key_mgmt_write() = %d",
					err);
			}
			k_free(content);
		} else if (op == AT_CMNG_OP_READ) {
			if (type == AT_CMNG_TYPE_CERT ||
			   type == AT_CMNG_TYPE_PRIV ||
			   type == AT_CMNG_TYPE_PSK) {
				/* Not supported */
				LOG_ERR("Not support READ for type: %d", type);
				return -EPERM;
			}
			content = k_malloc(CONFIG_AT_CMD_RESPONSE_MAX_LEN);
			err = modem_key_mgmt_read(
				slm_tls_map_sectag(sec_tag, type),
				0, content, &len);
			if (err != 0) {
				LOG_ERR("FAILED! modem_key_mgmt_read() = %d",
					err);
			} else {
				*(content + len) = '\0';
				sprintf(rsp_buf, "%%CMNG: %d,%d,\"\","
					"\"%s\"\r\n", sec_tag, type, content);
				rsp_send(rsp_buf, strlen(rsp_buf));
			}
			k_free(content);
		} else if (op == AT_CMNG_OP_DELETE) {
			err = modem_key_mgmt_delete(
				slm_tls_map_sectag(sec_tag, type), 0);
			if (err != 0) {
				LOG_ERR("FAILED! modem_key_mgmt_delete() = %d",
					err);
			}
		}
		break;

	default:
		break;
	}

	return err;
}


/**@brief API to handle SLM credential management AT commands
 */
int slm_at_cmng_parse(const char *at_cmd)
{
	int ret = -ENOENT;
	enum at_cmd_type type;

	for (int i = 0; i < AT_CMNG_MAX; i++) {
		if (slm_util_cmd_casecmp(at_cmd, cmng_at_list[i].string)) {
			ret = at_parser_params_from_str(at_cmd, NULL,
						&at_param_list);
			if (ret < 0) {
				LOG_ERR("Failed to parse AT command %d", ret);
				return -EINVAL;
			}
			type = at_parser_cmd_type_get(at_cmd);
			ret = cmng_at_list[i].handler(type);
			break;
		}
	}

	return ret;
}

/**@brief API to list CMNG AT commands
 */
void slm_at_cmng_clac(void)
{
	/* Let modem list CMNG command. */
}

/**@brief API to initialize CMNG AT commands handler
 */
int slm_at_cmng_init(void)
{
	return 0;
}

/**@brief API to uninitialize CMNG AT commands handler
 */
int slm_at_cmng_uninit(void)
{
	return 0;
}
