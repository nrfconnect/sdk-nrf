/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/net/socket.h>
#include <zephyr/settings/settings.h>
#include "slm_native_tls.h"
#include "slm_uart_handler.h"
#include "slm_at_cmng.h"

LOG_MODULE_REGISTER(slm_native_tls, CONFIG_SLM_LOG_LEVEL);

/* Stores certificates and other credentials in the Zephyr settings storage.
 * Storage is organized as slm-keys/<sec_tag>/<type>.
 * When native TLS connection is created, credentials under <sec_tag> are loaded to
 * TLS credential management for Mbed TLS.
 */

#define SLM_KEYS_MAX_SIZE sizeof("slm-keys/2147483647/0")

/* Loaded for TLS credential management. */
struct tls_cred_buf {
	uint8_t buf[CONFIG_SLM_NATIVE_TLS_CREDENTIAL_BUFFER_SIZE]; /* Credentials. */
	sec_tag_t sec_tag; /* sec_tag of the credentials in buf. */
	uint8_t type_flags; /* Credential types loaded in buf (1 << slm_cmng_type). */
};
static struct tls_cred_buf cred_buf[CONFIG_SLM_NATIVE_TLS_CREDENTIAL_BUFFER_COUNT] = {
	[0 ... CONFIG_SLM_NATIVE_TLS_CREDENTIAL_BUFFER_COUNT - 1] = {
		.sec_tag = -1
	}
};
static uint8_t cred_buf_next; /* Index of next cred_buf to use. */

/* Parameter for load_credential_cb. */
struct load_credential_params {
	sec_tag_t sec_tag;
	uint8_t *credential_flags;
	uint8_t *buf;
	size_t buf_len;
};

enum tls_credential_type slm_cmng_to_tls_cred(enum slm_cmng_type type)
{
	switch (type) {
	case SLM_AT_CMNG_TYPE_CA_CERT: return TLS_CREDENTIAL_CA_CERTIFICATE;
	case SLM_AT_CMNG_TYPE_CLIENT_CERT: return TLS_CREDENTIAL_SERVER_CERTIFICATE;
	case SLM_AT_CMNG_TYPE_CLIENT_KEY: return TLS_CREDENTIAL_PRIVATE_KEY;
	case SLM_AT_CMNG_TYPE_PSK: return TLS_CREDENTIAL_PSK;
	case SLM_AT_CMNG_TYPE_PSK_ID: return TLS_CREDENTIAL_PSK_ID;
	default: return TLS_CREDENTIAL_NONE;
	}
}

/* Load credentials to TLS credential management.
 * Called for every (slm-keys/<sec_tag>/)<type>.
 */
static int load_credential_cb(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg,
				void *param)
{
	int err;
	size_t setting_length;
	unsigned long type;
	struct load_credential_params *lcp = (struct load_credential_params *)param;

	type = strtoul(key, NULL, 10);
	if (type >= SLM_AT_CMNG_TYPE_COUNT) {
		return -EFAULT;
	}
	if (len > lcp->buf_len) {
		return -E2BIG;
	}

	setting_length = read_cb(cb_arg, lcp->buf, len);
	if (setting_length != len) {
		return -EFAULT;
	}

	/* PEM formatted certificate and keys need to be null-terminated for Mbed TLS. */
	if (type <= SLM_AT_CMNG_TYPE_CLIENT_KEY && lcp->buf[setting_length - 1] != '\0' &&
	    lcp->buf_len > setting_length) {
		lcp->buf[setting_length] = '\0';
		setting_length++;
	}

	err = tls_credential_add(lcp->sec_tag, slm_cmng_to_tls_cred(type), lcp->buf,
				 setting_length);
	if (err) {
		LOG_ERR("Failed tls_credential_add: %d,%lu", lcp->sec_tag, type);
		return err;
	}

	/* Keep track of the loaded credentials types. */
	*lcp->credential_flags |= 1 << type;

	/* Update buf to accommodate other credentials under the sec_tag. */
	lcp->buf += setting_length;
	lcp->buf_len -= setting_length;

	LOG_DBG("Loaded credential: %d,%lu", lcp->sec_tag, type);

	return 0;
}

/* List SLM credentials stored in Zephyr settings storage.
 * Called for every (slm-keys/)<sec_tag>.
 */
static int list_credentials_cb(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg,
				void *param)
{
	ARG_UNUSED(len);
	ARG_UNUSED(read_cb);
	ARG_UNUSED(cb_arg);
	ARG_UNUSED(param);

	int err;
	char buf[sizeof("#XCMNG: 2147483647,0\r\n")];

	err = snprintf(buf, sizeof(buf), "#XCMNG: %s\r\n", key);
	if (err < 0) {
		return err;
	} else if (err >= sizeof(buf)) {
		return -E2BIG;
	}

	char *ptr = buf;

	/* Format similarly to %CMNG. */
	while (*ptr) {
		if (*ptr == '/') {
			*ptr = ',';
		}
		ptr++;
	}

	err = slm_uart_tx_write(buf, strlen(buf), false, false);
	if (err) {
		LOG_ERR("Failed slm_uart_tx_write: %d", err);
	}

	return err;
}

/* Called for every (slm-keys/<sec_tag>/)<type>. */
static int get_setting_len_cb(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg,
				void *param)
{
	ARG_UNUSED(key);
	ARG_UNUSED(read_cb);
	ARG_UNUSED(cb_arg);

	size_t *param_len = param;
	*param_len += len;

	return 0;
}

static int get_credentials_len(sec_tag_t sec_tag, size_t *len)
{
	char key[SLM_KEYS_MAX_SIZE];
	const int err = snprintf(key, sizeof(key), "slm-keys/%d", sec_tag);

	if (err < 0) {
		return err;
	}

	*len = 0;
	return settings_load_subtree_direct(key, get_setting_len_cb, len);
}

static struct tls_cred_buf *get_tls_cred_buf(sec_tag_t sec_tag)
{
	for (int i = 0; i < ARRAY_SIZE(cred_buf); ++i) {
		if (cred_buf[i].sec_tag == sec_tag) {
			return &cred_buf[i];
		}
	}

	return NULL;
}

static int unload_tls_cred_buf(sec_tag_t sec_tag)
{
	struct tls_cred_buf *cred = get_tls_cred_buf(sec_tag);

	if (cred == NULL || sec_tag == -1) {
		return 0;
	}

	/* Delete previously loaded credentials from the credential store. */
	for (enum slm_cmng_type i = SLM_AT_CMNG_TYPE_CA_CERT; i != SLM_AT_CMNG_TYPE_COUNT; ++i) {
		if (cred->type_flags & (1 << i)) {
			const int err =
				tls_credential_delete(cred->sec_tag, slm_cmng_to_tls_cred(i));

			if (err) {
				LOG_ERR("Failed tls_credential_delete: %d, credential: %d,%d",
					err, cred->sec_tag, i);

				return err;
			}
		}
	}
	cred->sec_tag = -1;
	cred->type_flags = 0;

	return 0;
}

int slm_native_tls_store_credential(sec_tag_t sec_tag, uint16_t type, const void *data, size_t len)
{
	char key[SLM_KEYS_MAX_SIZE];
	int err;
	size_t cred_len = 0;

	err = snprintf(key, sizeof(key), "slm-keys/%d/%d", sec_tag, type);
	if (err < 0) {
		return err;
	}

	err = get_credentials_len(sec_tag, &cred_len);
	if (err) {
		return err;
	}

	/* Verify that the stored credentials could be loaded. */
	const uint8_t null_termination = type <= SLM_AT_CMNG_TYPE_CLIENT_KEY ? 1 : 0;

	if ((cred_len + len + null_termination) > CONFIG_SLM_NATIVE_TLS_CREDENTIAL_BUFFER_SIZE) {
		LOG_ERR("Increase CONFIG_SLM_NATIVE_TLS_CREDENTIAL_BUFFER_SIZE.");
		return -E2BIG;
	}

	err = settings_save_one(key, data, len);
	if (err) {
		LOG_ERR("Failed settings_save_one: %d", err);
		return err;
	}

	return unload_tls_cred_buf(sec_tag);
}

int slm_native_tls_list_credentials(void)
{
	return settings_load_subtree_direct("slm-keys", list_credentials_cb, NULL);
}

int slm_native_tls_load_credentials(sec_tag_t sec_tag)
{
	int err;
	struct tls_cred_buf *cred = get_tls_cred_buf(sec_tag);

	if (cred) {
		/* Already loaded. */
		return 0;
	}

	/* Take the next credential buf to use. */
	cred = &cred_buf[cred_buf_next];
	cred_buf_next = (cred_buf_next + 1) % ARRAY_SIZE(cred_buf);
	err = unload_tls_cred_buf(cred->sec_tag);
	if (err) {
		return err;
	}
	cred->sec_tag = sec_tag;

	char key[SLM_KEYS_MAX_SIZE];

	err = snprintf(key, sizeof(key), "slm-keys/%d", cred->sec_tag);
	if (err < 0) {
		return err;
	}

	struct load_credential_params params = {
		.sec_tag = cred->sec_tag,
		.credential_flags = &(cred->type_flags),
		.buf = cred->buf,
		.buf_len = sizeof(cred->buf)
	};

	return settings_load_subtree_direct(key, load_credential_cb, (void *)&params);
}

int slm_native_tls_delete_credential(sec_tag_t sec_tag, uint16_t type)
{
	int err;
	char key[SLM_KEYS_MAX_SIZE];

	err = snprintf(key, sizeof(key), "slm-keys/%d/%d", sec_tag, type);
	if (err < 0) {
		return err;
	}

	err = settings_delete(key);
	if (err) {
		return err;
	}

	return unload_tls_cred_buf(sec_tag);
}
