/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <nrf_modem_at.h>
#include <modem/modem_key_mgmt.h>
#include <zephyr/logging/log.h>

#define ENABLE 1
#define DISABLE 0

LOG_MODULE_REGISTER(modem_key_mgmt, CONFIG_MODEM_KEY_MGMT_LOG_LEVEL);

static char scratch_buf[4096];

static bool cmee_is_active(void)
{
	int err;
	int active;

	err = nrf_modem_at_scanf("AT+CMEE?", "+CMEE: %d", &active);
	if (err < 0) {
		LOG_WRN("Failed to retrieve CMEE status, err %d", err);
		return false;
	}

	return active ? true : false;
}

static int cmee_control(int state)
{
	return nrf_modem_at_printf("AT+CMEE=%d", state);
}

static void cmee_enable(bool *was_enabled)
{
	if (!cmee_is_active()) {
		*was_enabled = false;
		cmee_control(ENABLE);
	} else {
		*was_enabled = true;
	}
}

static void cmee_disable(void)
{
	cmee_control(DISABLE);
}

static int translate_error(int err)
{
	if (err < 0) {
		/* Command did not reach modem,
		 * return error from nrf_modem_at_cmd() directly
		 */
		return err;
	}

	/* In case of CME error translate to an errno value */
	switch (nrf_modem_at_err(err)) {
	case 513: /* not found */
		LOG_WRN("Key not found");
		return -ENOENT;
	case 514: /* no access */
		LOG_WRN("Key access refused");
		return -EACCES;
	case 515: /* memory full */
		LOG_WRN("Key storage memory full");
		return -ENOMEM;
	case 518: /* not allowed in active state */
		LOG_WRN("Not allowed when LTE connection is active");
		return -EPERM;
	case 519: /* already exists */
		LOG_WRN("Key already exists");
		return -EALREADY;
	case 528: /* not allowed in power off warning */
		LOG_WRN("Not allowed when power off warning is active");
		return -ECANCELED;
	default:
		/* Catch unexpected CME errors.
		 * Return a magic value to make sure this
		 * situation is clearly distinguishable.
		 */
		LOG_ERR("Untranslated CME error %d", nrf_modem_at_err(err));
		__ASSERT(false, "Untranslated CME error %d", nrf_modem_at_err(err));
		return 0xBAADBAAD;
	}
}

/* Read the given credential into the static buffer */
static int key_fetch(nrf_sec_tag_t tag,
		     enum modem_key_mgmt_cred_type cred_type)
{
	int err;
	bool cmee_was_active;

	cmee_enable(&cmee_was_active);

	err = nrf_modem_at_cmd(scratch_buf, sizeof(scratch_buf),
			       "AT%%CMNG=2,%u,%d", tag, cred_type);

	if (!cmee_was_active) {
		cmee_disable();
	}

	if (err) {
		return translate_error(err);
	}

	return 0;
}

int modem_key_mgmt_write(nrf_sec_tag_t sec_tag,
			 enum modem_key_mgmt_cred_type cred_type,
			 const void *buf, size_t len)
{
	int err;
	bool cmee_was_enabled;

	if (buf == NULL || len == 0) {
		return -EINVAL;
	}

	cmee_enable(&cmee_was_enabled);

	err = nrf_modem_at_printf("AT%%CMNG=0,%u,%d,\"%.*s\"",
				  sec_tag, cred_type, len, (const char *)buf);

	if (!cmee_was_enabled) {
		cmee_disable();
	}

	if (err) {
		return translate_error(err);
	}

	return 0;
}

int modem_key_mgmt_read(nrf_sec_tag_t sec_tag,
			enum modem_key_mgmt_cred_type cred_type,
			void *buf, size_t *len)
{
	int err;
	char *begin, *end;

	if (buf == NULL || len == NULL) {
		return -EINVAL;
	}

	err = key_fetch(sec_tag, cred_type);
	if (err) {
		return err;
	}

	begin = scratch_buf;
	for (size_t i = 0; i < 3; i++) {
		begin = strchr(begin, '\"');
		if (!begin) {
			return -ENOENT;
		}
		begin++;
	}

	end = strchr(begin, '\"');
	if (!end) {
		return -ENOENT;
	}

	if (end - begin > *len) {
		return -ENOMEM;
	}

	memcpy(buf, begin, end - begin);
	*len = end - begin;

	return err;
}

int modem_key_mgmt_cmp(nrf_sec_tag_t sec_tag,
		       enum modem_key_mgmt_cred_type cred_type,
		       const void *buf, size_t len)
{
	int err;
	char *begin, *end;

	if (buf == NULL) {
		return -EINVAL;
	}

	err = key_fetch(sec_tag, cred_type);
	if (err) {
		return err;
	}

	begin = scratch_buf;
	for (size_t i = 0; i < 3; i++) {
		begin = strchr(begin, '\"');
		if (!begin) {
			return -ENOENT;
		}
		begin++;
	}

	end = strchr(begin, '\"');
	if (!end) {
		return -ENOENT;
	}

	if (end - begin != len) {
		LOG_DBG("Credential length mismatch");
		return 1;
	}

	if (memcmp(begin, buf, len)) {
		LOG_DBG("Credential data mismatch");
		return 1;
	}

	return 0;
}

int modem_key_mgmt_delete(nrf_sec_tag_t sec_tag,
			  enum modem_key_mgmt_cred_type cred_type)
{
	int err;
	bool cmee_was_enabled;

	cmee_enable(&cmee_was_enabled);

	err = nrf_modem_at_printf("AT%%CMNG=3,%u,%d", sec_tag, cred_type);

	if (!cmee_was_enabled) {
		cmee_disable();
	}

	if (err) {
		return translate_error(err);
	}

	return 0;
}

int modem_key_mgmt_exists(nrf_sec_tag_t sec_tag,
			  enum modem_key_mgmt_cred_type cred_type,
			  bool *exists)
{
	int err;
	bool cmee_was_active;

	if (exists == NULL) {
		return -EINVAL;
	}

	cmee_enable(&cmee_was_active);

	scratch_buf[0] = '\0';
	err = nrf_modem_at_cmd(scratch_buf, sizeof(scratch_buf),
			       "AT%%CMNG=1,%u,%d", sec_tag, cred_type);

	if (!cmee_was_active) {
		cmee_disable();
	}

	if (err) {
		return translate_error(err);
	}

	if (strlen(scratch_buf) > strlen("OK\r\n")) {
		*exists = true;
	} else {
		*exists = false;
	}

	return 0;
}
