/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/kernel.h>
#include <zephyr/toolchain/common.h>
#include "desh_native_tls.h"
#include "desh_at_cmng.h"

#include "desh_print.h"

#if defined(CONFIG_NRF_CLOUD_SEC_TAG)
#define DESH_KEYS_MAX_SIZE (sizeof("desh-keys/" STRINGIFY(CONFIG_NRF_CLOUD_SEC_TAG) "/0"))
#define DESH_CMNG_BUF_SIZE (sizeof("%CMNG: " STRINGIFY(CONFIG_NRF_CLOUD_SEC_TAG) ",0\r\n"))
#else
#define DESH_KEYS_MAX_SIZE sizeof("desh-keys/2147483647/0")
#define DESH_CMNG_BUF_SIZE (sizeof("%CMNG: 2147483647,0\r\n") + 1)
#endif

#if defined(CONFIG_SAMPLE_DESH_CLOUD_MQTT)
#include <zephyr/net/socket.h>
#include <zephyr/settings/settings.h>

#include "at_cmd_desh_custom.h"

/* Stores certificates and other credentials in the Zephyr settings storage.
 * Storage is organized as desh-keys/<sec_tag>/<type>.
 * When native TLS connection is created, credentials under <sec_tag> are loaded to
 * TLS credential management for Mbed TLS.
 */

/* Loaded for TLS credential management. */
struct tls_cred_buf {
	uint8_t buf[CONFIG_SAMPLE_DESH_NATIVE_TLS_CREDENTIAL_BUFFER_SIZE]; /* Credentials. */
	sec_tag_t sec_tag;  /* sec_tag of the credentials in buf. */
	uint8_t type_flags; /* Credential types loaded in buf (1 << desh_cmng_type). */
};
static struct tls_cred_buf cred_buf[CONFIG_SAMPLE_DESH_NATIVE_TLS_CREDENTIAL_BUFFER_COUNT] = {
	[0 ... CONFIG_SAMPLE_DESH_NATIVE_TLS_CREDENTIAL_BUFFER_COUNT - 1] = {.sec_tag = -1}};
static uint8_t cred_buf_next; /* Index of next cred_buf to use. */

/* Parameter for load_credential_cb. */
struct load_credential_params {
	sec_tag_t sec_tag;
	uint8_t *credential_flags;
	uint8_t *buf;
	size_t buf_len;
};
#endif

enum tls_credential_type desh_cmng_to_tls_cred(enum desh_cmng_type type)
{
	switch (type) {
	case DESH_AT_CMNG_TYPE_CA_CERT:
		return TLS_CREDENTIAL_CA_CERTIFICATE;
	case DESH_AT_CMNG_TYPE_CLIENT_CERT:
		return TLS_CREDENTIAL_SERVER_CERTIFICATE;
	case DESH_AT_CMNG_TYPE_CLIENT_KEY:
		return TLS_CREDENTIAL_PRIVATE_KEY;
	case DESH_AT_CMNG_TYPE_PSK:
		return TLS_CREDENTIAL_PSK;
	case DESH_AT_CMNG_TYPE_PSK_ID:
		return TLS_CREDENTIAL_PSK_ID;
	default:
		return TLS_CREDENTIAL_NONE;
	}
}

#if defined(CONFIG_SAMPLE_DESH_CLOUD_MQTT)
/* Load credentials to TLS credential management.
 * Called for every (desh-keys/<sec_tag>/)<type>.
 */
static int load_credential_cb(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg,
			      void *param)
{
	int err;
	size_t setting_length;
	unsigned long type;
	struct load_credential_params *lcp = (struct load_credential_params *)param;

	type = strtoul(key, NULL, 10);
	if (type >= DESH_AT_CMNG_TYPE_COUNT) {
		printk("%s: invalid type: %lu\n", (__func__), type);
		return -EFAULT;
	}
	if (len > lcp->buf_len) {
		printk("%s: buffer too small: %zu > %zu\n", (__func__), len, lcp->buf_len);
		return -E2BIG;
	}

	setting_length = read_cb(cb_arg, lcp->buf, len);
	if (setting_length != len) {
		printk("%s: read_cb failed: %zu != %zu\n", (__func__), setting_length, len);
		return -EFAULT;
	}

	/* PEM formatted certificate and keys need to be null-terminated for Mbed TLS. */
	if (type <= DESH_AT_CMNG_TYPE_CLIENT_KEY && lcp->buf[setting_length - 1] != '\0' &&
	    lcp->buf_len > setting_length) {
		lcp->buf[setting_length] = '\0';
		setting_length++;
	}

	err = tls_credential_add(lcp->sec_tag, desh_cmng_to_tls_cred(type), lcp->buf,
				 setting_length);
	if (err) {
		printk("Failed tls_credential_add: %d,%lu\n", lcp->sec_tag, type);
		return err;
	}

	/* Keep track of the loaded credentials types. */
	*lcp->credential_flags |= 1 << type;

	/* Update buf to accommodate other credentials under the sec_tag. */
	lcp->buf += setting_length;
	lcp->buf_len -= setting_length;

	printk("Loaded credential: %d,%lu\n", lcp->sec_tag, type);

	return 0;
}

/* List desh credentials stored in Zephyr settings storage.
 * Called for every (desh-keys/)<sec_tag>.
 */
static int list_credentials_cb(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg,
			       void *param)
{
	ARG_UNUSED(len);
	ARG_UNUSED(read_cb);
	ARG_UNUSED(cb_arg);
	ARG_UNUSED(param);

	int err;
	char buf[DESH_CMNG_BUF_SIZE];

	err = snprintf(buf, sizeof(buf), "%%CMNG: %s\r\n", key);
	if (err < 0) {
		printk("Failed snprintf: %d\n", err);
		return err;
	} else if (err >= sizeof(buf)) {
		printk("snprintf buffer too small: %d\n", err);
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
	printk("%s", buf);

	return 0;
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
	for (enum desh_cmng_type i = DESH_AT_CMNG_TYPE_CA_CERT; i != DESH_AT_CMNG_TYPE_COUNT; ++i) {
		if (cred->type_flags & (1 << i)) {
			const int err =
				tls_credential_delete(cred->sec_tag, desh_cmng_to_tls_cred(i));

			if (err) {
				printk("Failed tls_credential_delete: %d, credential: %d,%d\n", err,
				       cred->sec_tag, i);

				return err;
			}
		}
	}
	cred->sec_tag = -1;
	cred->type_flags = 0;

	return 0;
}
int desh_native_tls_store_credential(sec_tag_t sec_tag, uint16_t type, const void *data, size_t len)
{
	char key[DESH_KEYS_MAX_SIZE];
	int err;
	size_t cred_len = 0;

	err = snprintf(key, sizeof(key), "desh-keys/%d/%d", sec_tag, type);
	if (err < 0) {
		return err;
	}
	/* Verify that the stored credentials could be loaded. */
	const uint8_t null_termination = type <= DESH_AT_CMNG_TYPE_CLIENT_KEY ? 1 : 0;

	if ((cred_len + len + null_termination) >
	     CONFIG_SAMPLE_DESH_NATIVE_TLS_CREDENTIAL_BUFFER_SIZE) {
		printk("Increase CONFIG_SAMPLE_DESH_NATIVE_TLS_CREDENTIAL_BUFFER_SIZE. "
			"cred_len %d, len %d, null %d\n",
		       cred_len, len, null_termination);
		return -E2BIG;
	}

	err = settings_save_one(key, data, len);
	if (err) {
		printk("Failed settings_save_one: %d (len %d)\n", err, len);
		return err;
	}

	return unload_tls_cred_buf(sec_tag);
}

int desh_native_tls_list_credentials(void)
{
	int foo = 1;
	static const char *key = "desh-keys";

	return settings_load_subtree_direct(key, list_credentials_cb, &foo);
}

int desh_native_tls_load_credentials(sec_tag_t sec_tag)
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

	char key[DESH_KEYS_MAX_SIZE];

	err = snprintf(key, sizeof(key), "desh-keys/%d", cred->sec_tag);
	if (err < 0) {
		return err;
	}

	struct load_credential_params params = {.sec_tag = cred->sec_tag,
						.credential_flags = &(cred->type_flags),
						.buf = cred->buf,
						.buf_len = sizeof(cred->buf)};

	return settings_load_subtree_direct(key, load_credential_cb, (void *)&params);
}

int desh_native_tls_delete_credential(sec_tag_t sec_tag, uint16_t type)
{
	int err;
	char key[DESH_KEYS_MAX_SIZE];

	err = snprintf(key, sizeof(key), "desh-keys/%d/%d", sec_tag, type);
	if (err < 0) {
		return err;
	}

	err = settings_delete(key);
	if (err) {
		return err;
	}

	return unload_tls_cred_buf(sec_tag);
}

int desh_native_tls_init(void)
{
	int err;

	err = settings_subsys_init();
	if (err) {
		printk("Failed settings_subsys_init: %d\n", err);
		return err;
	}
	return 0;
}

SYS_INIT(desh_native_tls_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#else
/* With CoAP: store credentials to secure storage. */

int desh_native_tls_store_credential(sec_tag_t sec_tag, uint16_t type, const void *data, size_t len)
{
	static uint8_t buf[CONFIG_SAMPLE_DESH_NATIVE_TLS_CREDENTIAL_BUFFER_SIZE];
	int err;
	const void *cred = data;

	/* PEM formatted certificate and keys need to be null-terminated for Mbed TLS. */
	if ((type <= DESH_AT_CMNG_TYPE_CLIENT_KEY) && (((const char *)data)[len - 1] != '\0')) {
		if (len + 1 > sizeof(buf)) {
			/* Cannot fit a null-terminator in the buffer. */
			return -E2BIG;
		}

		/* Copy into buffer and null-terminate. */
		err = snprintf(buf, sizeof(buf), "%.*s", len, (const char *)data);
		if ((err < 0) || (err >= sizeof(buf))) {
			printk("Failed to null-terminate the credential: %d,%u\n", sec_tag, type);
			return err;
		}

		len++;
		cred = buf;
	}

	/* Delete the credential in case it already exists. */
	err = tls_credential_delete(sec_tag, desh_cmng_to_tls_cred(type));
	if (err) {
		printk("Failed tls_credential_delete: %d,%u\n", sec_tag, type);
	}

	err = tls_credential_add(sec_tag, desh_cmng_to_tls_cred(type), cred, len);
	if (err) {
		printk("Failed tls_credential_add: %d,%u\n", sec_tag, type);
	}

	/* Clear the credential from RAM. */
	memset(buf, 0, sizeof(buf));

	return err;
}

int desh_native_tls_list_credentials(void)
{
	int err;
	char buf[DESH_CMNG_BUF_SIZE];
	size_t credlen = 0;

	for (enum desh_cmng_type type = DESH_AT_CMNG_TYPE_CA_CERT; type < DESH_AT_CMNG_TYPE_COUNT;
		type++) {
		err = tls_credential_get(CONFIG_NRF_CLOUD_SEC_TAG, type, NULL, &credlen);
		if (err == -ENOENT) {
			continue;
		}

		err = snprintf(buf, sizeof(buf), "%%CMNG: %d,%d\r\n", CONFIG_NRF_CLOUD_SEC_TAG,
			type);
		if (err < 0) {
			printk("Failed snprintf: %d\n", err);
			return err;
		} else if (err >= sizeof(buf)) {
			printk("snprintf buffer too small: %d\n", err);
			return -E2BIG;
		}

		printk("%s", buf);
	}

	return 0;
}

int desh_native_tls_delete_credential(sec_tag_t sec_tag, uint16_t type)
{
	return tls_credential_delete(sec_tag, desh_cmng_to_tls_cred(type));
}
#endif
