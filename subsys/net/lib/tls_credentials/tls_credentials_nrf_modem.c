/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stddef.h>
/* stddef must be included before tls_credentials.h*/
#include <zephyr/net/tls_credentials.h>
#include "tls_internal.h"
#include <modem/nrf_modem_lib.h>

#include <string.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <modem/modem_key_mgmt.h>
#include <zephyr/sys/base64.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tls_credentials, CONFIG_TLS_CREDENTIALS_LOG_LEVEL);

/* Global pool of credentials shared among TLS contexts. */
static struct tls_credential credentials[CONFIG_TLS_MAX_CREDENTIALS_NUMBER];

static uint8_t workaround_buf;

/* A mutex for protecting access to the credentials array. */
static struct k_mutex credential_lock;

static struct tls_credential *unused_credential_get(void)
{
	for (int i = 0; i < ARRAY_SIZE(credentials); i++) {
		if (credentials[i].type == TLS_CREDENTIAL_NONE) {
			return &credentials[i];
		}
	}

	return NULL;
}

struct tls_credential *credential_get(sec_tag_t tag,
					  enum tls_credential_type type)
{
	for (int i = 0; i < ARRAY_SIZE(credentials); i++) {
		if (credentials[i].type == type && credentials[i].tag == tag) {
			return &credentials[i];
		}
	}

	return NULL;
}

struct tls_credential *credential_next_get(sec_tag_t tag,
					   struct tls_credential *iter)
{
	if (!iter) {
		iter = credentials;
	} else {
		iter++;
	}

	for (int i = ARRAY_INDEX(credentials, iter); i < ARRAY_SIZE(credentials); i++) {
		if (credentials[i].type != TLS_CREDENTIAL_NONE &&
		    credentials[i].tag == tag) {
			return &credentials[i];
		}
	}

	return NULL;
}

static enum modem_key_mgmt_cred_type modem_key_mgmt_type_get(enum tls_credential_type type)
{
	switch (type) {
	case TLS_CREDENTIAL_CA_CERTIFICATE:
		return MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN;
	case TLS_CREDENTIAL_SERVER_CERTIFICATE:
		return MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT;
	case TLS_CREDENTIAL_PRIVATE_KEY:
		return MODEM_KEY_MGMT_CRED_TYPE_PRIVATE_CERT;
	default:
		return -1;
	}
}

static enum tls_credential_type tls_credential_type_get(enum modem_key_mgmt_cred_type cred_type)
{
	switch (cred_type) {
	case MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN:
		return TLS_CREDENTIAL_CA_CERTIFICATE;
	case MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT:
		return TLS_CREDENTIAL_SERVER_CERTIFICATE;
	case MODEM_KEY_MGMT_CRED_TYPE_PRIVATE_CERT:
		return TLS_CREDENTIAL_PRIVATE_KEY;
	default:
		/* TLS credentials do not support the rest of the types. */
		return TLS_CREDENTIAL_NONE;
	}
}

sec_tag_t credential_next_tag_get(sec_tag_t iter)
{
	int i;
	sec_tag_t lowest = TLS_SEC_TAG_NONE;

	/* Scan all slots and find lowest sectag greater than iter */
	for (i = 0; i < ARRAY_SIZE(credentials); i++) {
		/* Skip empty slots. */
		if (credentials[i].type == TLS_CREDENTIAL_NONE) {
			continue;
		}

		/* Skip any slots containing sectags not greater than iter */
		if (credentials[i].tag <= iter && iter != TLS_SEC_TAG_NONE) {
			continue;
		}

		/* Find the lowest of such slots */
		if (lowest == TLS_SEC_TAG_NONE || credentials[i].tag < lowest) {
			lowest = credentials[i].tag;
		}
	}

	return lowest;
}

int credential_digest(struct tls_credential *credential, void *dest, size_t *len)
{
	uint8_t digest_buf[MODEM_KEY_MGMT_DIGEST_SIZE];
	int written;
	int err;

	err = modem_key_mgmt_digest(credential->tag, modem_key_mgmt_type_get(credential->type),
				    digest_buf, sizeof(digest_buf));
	if (err < 0) {
		return err;
	}

	err = base64_encode(dest, *len, &written, digest_buf, sizeof(digest_buf));
	*len = err ? 0 : written;

	return err;
}

void credentials_lock(void)
{
	k_mutex_lock(&credential_lock, K_FOREVER);
}

void credentials_unlock(void)
{
	k_mutex_unlock(&credential_lock);
}

static int credential_add(sec_tag_t tag, enum tls_credential_type type)
{
	struct tls_credential *credential;
	int ret = 0;

	if (type == TLS_CREDENTIAL_NONE) {
		return -EINVAL;
	}

	credential = credential_get(tag, type);
	if (credential != NULL) {
		return -EEXIST;
	}

	credential = unused_credential_get();
	if (credential == NULL) {
		return -ENOMEM;
	}

	credential->tag = tag;
	credential->type = type;

	/* Set buf to something else than NULL to avoid tls_credential 'cred del' command from
	 * crashing the system. See: https://github.com/zephyrproject-rtos/zephyr/pull/84449.
	 */
	credential->buf = &workaround_buf;
	credential->len = 0;

	return ret;
}

static int credential_verify_or_write(struct tls_credential *credential,
				       const void *cred, size_t credlen)
{
	enum modem_key_mgmt_cred_type cred_type = modem_key_mgmt_type_get(credential->type);
	int err;

	err = modem_key_mgmt_cmp(credential->tag, cred_type, cred, credlen);
	if (err == -ENOENT) {
		LOG_WRN("Credential did not exist, but should have");
		err = modem_key_mgmt_write(credential->tag, cred_type, cred, credlen);
		if (err < 0) {
			return err;
		}
		return 0;
	}

	/* Any other error code is an error. */
	if (err < 0) {
		return err;
	}

	/* err == 1 means the certificates did not match */
	if (err == 1) {
		return -EEXIST;
	}

	/* err == 0 means the certificate matched. */
	LOG_DBG("Security tag %d verified", credential->tag);

	return 0;
}

int tls_credential_add(sec_tag_t tag, enum tls_credential_type type,
		       const void *cred, size_t credlen)
{
	struct tls_credential *credential;
	int err = 0;

	if (type == TLS_CREDENTIAL_NONE) {
		return -EINVAL;
	}

	credentials_lock();

	credential = credential_get(tag, type);
	if (credential != NULL) {
		err = credential_verify_or_write(credential, cred, credlen);
		goto exit;
	}

	credential = unused_credential_get();
	if (credential == NULL) {
		err = -ENOMEM;
		goto exit;
	}

	err = modem_key_mgmt_write(tag, modem_key_mgmt_type_get(type), cred, credlen);
	if (err < 0) {
		goto exit;
	}

	credential->tag = tag;
	credential->type = type;

	/* keep the reference to the original buffer in case the user needs it, for example the
	 * shell 'cred add' command allocates this on heap, and frees it in 'cred del'.
	 */
	credential->buf = cred;
	credential->len = credlen;

exit:
	credentials_unlock();

	return err;
}

int tls_credential_get(sec_tag_t tag, enum tls_credential_type type,
		       void *cred, size_t *credlen)
{
	struct tls_credential *credential;
	int ret = 0;

	credentials_lock();

	credential = credential_get(tag, type);
	if (credential == NULL) {
		ret = -ENOENT;
		goto exit;
	}

	if (credential->len > *credlen) {
		ret = -EFBIG;
		goto exit;
	}

	/* Must be read from the modem. */
	if (credential->len == 0) {
		ret = modem_key_mgmt_read(tag, modem_key_mgmt_type_get(type), cred, credlen);
		goto exit;
	}

	*credlen = credential->len;
	memcpy(cred, credential->buf, credential->len);

exit:
	credentials_unlock();

	return ret;
}

int tls_credential_delete(sec_tag_t tag, enum tls_credential_type type)
{
	struct tls_credential *credential;
	int ret = 0;

	credentials_lock();

	credential = credential_get(tag, type);

	if (!credential) {
		ret = -ENOENT;
		goto exit;
	}

	ret = modem_key_mgmt_delete(credential->tag, modem_key_mgmt_type_get(type));
	if (ret < 0) {
		goto exit;
	}

	credential->type = TLS_CREDENTIAL_NONE;

exit:
	credentials_unlock();

	return ret;
}

static void credential_entry_init_callback(nrf_sec_tag_t sec_tag,
					   enum modem_key_mgmt_cred_type cred_type)
{
	int err;

	LOG_DBG("Got credential: tag: %d, type %d", sec_tag, cred_type);

	enum tls_credential_type type = tls_credential_type_get(cred_type);

	if (type == TLS_CREDENTIAL_NONE) {
		LOG_DBG("Skip unsupported TLS credential, modem credential type: %d", cred_type);
		return;
	}

	err = credential_add(sec_tag, type);
	if (err) {
		LOG_ERR("Failed to add credential: %d", err);
	}
}

void credentials_init(void)
{
	int err;

	(void)memset(credentials, 0, sizeof(credentials));

	k_mutex_init(&credential_lock);

	err = modem_key_mgmt_list(credential_entry_init_callback);
	if (err < 0) {
		LOG_ERR("Failed to list credentials");
	}
}

#ifdef CONFIG_ZTEST
void credentials_on_modem_init(int ret, void *ctx)
#else
NRF_MODEM_LIB_ON_INIT(credentials_modem_init_hook, credentials_on_modem_init, NULL);
static void credentials_on_modem_init(int ret, void *ctx)
#endif
{
	static bool credentials_loaded;

	ARG_UNUSED(ctx);

	if (ret != 0) {
		LOG_ERR("Modem library did not initialize: %d", ret);
		return;
	}

	if (!credentials_loaded) {
		credentials_init();
		credentials_loaded = true;
	}
}
