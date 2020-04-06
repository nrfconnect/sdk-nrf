/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>
#include <string.h>
#include <zephyr.h>
#include <modem/at_cmd.h>
#include <modem/at_cmd_parser.h>
#include <modem/at_params.h>
#include <bsd_limits.h>
#include <modem/modem_key_mgmt.h>
#include <logging/log.h>

#define MODEM_KEY_MGMT_OP_LS "AT%CMNG=1"
#define MODEM_KEY_MGMT_OP_RD "AT%CMNG=2"
#define MODEM_KEY_MGMT_OP_WR "AT%CMNG=0"
#define MODEM_KEY_MGMT_OP_RM "AT%CMNG=3"

#define AT_CMNG_PARAMS_COUNT 5
#define AT_CMNG_CONTENT_INDEX 4

LOG_MODULE_REGISTER(modem_key_mgmt, CONFIG_MODEM_KEY_MGMT_LOG_LEVEL);

static char scratch_buf[4096];

static int cmee_is_active(void)
{
	int err;
	char response[sizeof("+CMEE: X\r\n")];

	err = at_cmd_write("AT+CMEE?", response, sizeof(response), NULL);
	if (err) {
		return err;
	}

#define CMEE_STATUS 7

	return (response[CMEE_STATUS] == '1');
}

static int cmee_enable(void)
{
	return at_cmd_write("AT+CMEE=1", NULL, 0, NULL);
}

static int cmee_disable(void)
{
	return at_cmd_write("AT+CMEE=0", NULL, 0, NULL);
}

static int translate_error(int err, enum at_cmd_state state)
{
	/* In case of CME error translate the error value to
	 * an errno value.
	 */
	if ((err > 0) && (state == AT_CMD_ERROR_CME)) {
		switch (err) {
		case 513:
			return -ENOENT;
		case 514:
			return -EPERM;
		case 515:
			return -ENOMEM;
		case 518:
			return -EACCES;
		default:
			/* Catch unexpected CME errors.
			 * Return a magic value to make sure this
			 * situation is clearly distinguishable.
			 */
			__ASSERT(false, "Untranslated CME error %d!", err);
			return 0xBAADBAAD;
		}
	}

	return err;
}

static int write_at_cmd_with_cme_enabled(char *cmd, char *buf, size_t buf_len,
					 enum at_cmd_state *state)
{
	int err;
	int cmee_was_active = cmee_is_active();

	if (cmee_was_active < 0) {
		return -EFAULT;
	}

	if (!cmee_was_active) {
		err = cmee_enable();
		if (err) {
			return err;
		}
	}

	err = at_cmd_write(cmd, buf, buf_len, state);

	if (!cmee_was_active) {
		err = cmee_disable();
		if (err) {
			return err;
		}
	}

	return err;
}

int modem_key_mgmt_write(nrf_sec_tag_t sec_tag,
			 enum modem_key_mgnt_cred_type cred_type,
			 const void *buf, size_t len)
{
	int err;
	int written;
	enum at_cmd_state state;

	if (buf == NULL || len == 0) {
		return -EINVAL;
	}

	written = snprintf(scratch_buf, sizeof(scratch_buf), "%s,%d,%d,\"",
			   MODEM_KEY_MGMT_OP_WR, sec_tag, cred_type);

	if (written < 0 || written >= sizeof(scratch_buf)) {
		return -ENOBUFS;
	}

	if (written + len + sizeof("\"") > sizeof(scratch_buf)) {
		return -ENOBUFS;
	}

	memcpy(&scratch_buf[written], buf, len);
	written += len;

	memcpy(&scratch_buf[written], "\"", sizeof("\""));

	err = write_at_cmd_with_cme_enabled(scratch_buf, NULL, 0, &state);

	return translate_error(err, state);
}

int modem_key_mgmt_read(nrf_sec_tag_t sec_tag,
			enum modem_key_mgnt_cred_type cred_type, void *buf,
			u16_t *len)
{
	int err;
	int written;
	enum at_cmd_state state;

	if ((buf == NULL) || (len == NULL) || (*len == 0)) {
		return -EINVAL;
	}

	written = snprintf(scratch_buf, sizeof(scratch_buf),
			   "%s,%u,%u\r\n", MODEM_KEY_MGMT_OP_RD,
			   (u32_t)sec_tag, (u8_t)cred_type);

	if ((written < 0) || (written >= sizeof(scratch_buf))) {
		return -ENOBUFS;
	}

	err = write_at_cmd_with_cme_enabled(scratch_buf, scratch_buf,
					    sizeof(scratch_buf), &state);
	if (err == 0) {
		size_t size;
		struct at_param_list cmng_list;

		at_params_list_init(&cmng_list, AT_CMNG_PARAMS_COUNT);
		at_parser_params_from_str(scratch_buf, NULL, &cmng_list);
		at_params_size_get(&cmng_list, AT_CMNG_CONTENT_INDEX, &size);

		if (size < *len) {
			at_params_list_free(&cmng_list);
			return -ENOMEM;
		}

		at_params_string_get(&cmng_list, AT_CMNG_CONTENT_INDEX, buf,
				     &size);
		*len = size;
		at_params_list_free(&cmng_list);
	}

	return translate_error(err, state);
}

int modem_key_mgmt_delete(nrf_sec_tag_t sec_tag,
			  enum modem_key_mgnt_cred_type cred_type)
{
	int err;
	int written;
	enum at_cmd_state state;

	written = snprintf(scratch_buf, sizeof(scratch_buf), "%s,%d,%d",
			   MODEM_KEY_MGMT_OP_RM, sec_tag, cred_type);

	if (written < 0 || written >= sizeof(scratch_buf)) {
		return -ENOBUFS;
	}

	err = write_at_cmd_with_cme_enabled(scratch_buf, NULL, 0, &state);

	return translate_error(err, state);
}

int modem_key_mgmt_permission_set(nrf_sec_tag_t sec_tag,
				  enum modem_key_mgnt_cred_type cred_type,
				  u8_t perm_flags)
{
	return -EOPNOTSUPP;
}

int modem_key_mgmt_exists(nrf_sec_tag_t sec_tag,
			  enum modem_key_mgnt_cred_type cred_type,
			  bool *exists, u8_t *perm_flags)
{
	int err;
	int written;
	char cmd[32];
	enum at_cmd_state state;

	if (exists == NULL || perm_flags == NULL) {
		return -EINVAL;
	}

	written = snprintf(cmd, sizeof(cmd), "%s,%d,%d",
			   MODEM_KEY_MGMT_OP_LS, sec_tag, cred_type);

	if (written < 0 || written >= sizeof(cmd)) {
		return -ENOBUFS;
	}

	err = write_at_cmd_with_cme_enabled(cmd, scratch_buf,
					    sizeof(scratch_buf), &state);
	if (err) {
		return translate_error(err, state);
	}

	if (strlen(scratch_buf) > 0) {
		*exists = true;
		*perm_flags = 0;
	} else {
		*exists = false;
	}

	return 0;
}
