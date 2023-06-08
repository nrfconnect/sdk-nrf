/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>

#include <net/lwm2m_client_utils.h>
#include <modem/modem_key_mgmt.h>
#include <modem/lte_lc.h>
#include <zephyr/settings/settings.h>
#include "stubs.h"
#include "lwm2m_object.h"

#define MY_ID "my_id"
#define MY_ENDPOINT "my_endpoint"
#define MY_PSK "0a0b0c0d"
#define MY_PSK_BIN {0x0A, 0x0B, 0x0C, 0x0D}
#define MY_CERTIFICATE "--BEGIN CERT--PLAAPLAAPLAA"
#define MY_ROOT_CA "--BEGIN CERT--root cert"
#define MY_PRIVATE_KEY "s3cret"

struct lwm2m_ctx ctx = {
	.bootstrap_mode = true,
	.sec_obj_inst = 1, /* Use 1 here to distinct from default security object */
	.srv_obj_inst = 1,
	.use_dtls = true,
};

static uint8_t my_psk_bin[] = MY_PSK_BIN;
static uint8_t my_buf[256];
static uint16_t my_data_len;
static bool certificate_cleared;
static bool ca_cleared;
static bool key_cleared;
static bool keys_exist;

static int lwm2m_get_res_buf_custom_fake(const struct lwm2m_obj_path *path,
					 void **buffer_ptr, uint16_t *buffer_len,
					 uint16_t *data_len, uint8_t *data_flags)
{
	if (buffer_ptr)
		*buffer_ptr = my_buf;
	if (buffer_len)
		*buffer_len = sizeof(my_buf);
	if (data_len)
		*data_len = my_data_len;
	return 0;
}

static void setup(void)
{
	/* Register resets */
	DO_FOREACH_FAKE(RESET_FAKE);

	/* reset common FFF internal structures */
	FFF_RESET_HISTORY();
	certificate_cleared = false;
	ca_cleared = false;
	key_cleared = false;
	my_data_len = 0;
	keys_exist = false;

	lwm2m_get_res_buf_fake.custom_fake = lwm2m_get_res_buf_custom_fake;
}

static int modem_key_mgmt_exists_custom_fake(nrf_sec_tag_t sec_tag,
					     enum modem_key_mgmt_cred_type cred_type, bool *exists)
{
	*exists = keys_exist;
	return 0;
}

static int lwm2m_set_opaque_custom_fake_PSK(const struct lwm2m_obj_path *path, const char *data,
					    uint16_t len)
{
	if (path->obj_id == 0 && path->obj_inst_id == 0 && path->res_id == 0) {
		/* URI */
		zassert_equal(strncmp(data, CONFIG_LWM2M_CLIENT_UTILS_SERVER, len), 0, "Wrong URI");

	} else if (path->obj_id == 0 && path->obj_inst_id == 0 && path->res_id == 2) {
		/* MODE */
		zassert_equal(*(uint8_t *)data, 0, "Wrong security type");

	} else if (path->obj_id == 0 && path->obj_inst_id == 0 && path->res_id == 3) {
		/*  ID */
		zassert_equal(strncmp(data, MY_ID, len), 0, "Wrong ID");

	} else if (path->obj_id == 0 && path->obj_inst_id == 0 && path->res_id == 5) {
		/* PSK */
		zassert_equal(memcmp(data, my_psk_bin, len), 0, "Wrong PSK");
	} else {
		return -EINVAL;
	}
	return 0;
}

static int lwm2m_set_opaque_custom_fake_CERT(const struct lwm2m_obj_path *path, const char *data,
					     uint16_t len)
{
	if (path->obj_id == 0 && path->obj_inst_id == 0 && path->res_id == 0) {
		/* URI */
		zassert_equal(strncmp(data, CONFIG_LWM2M_CLIENT_UTILS_SERVER, len), 0, "Wrong URI");

	} else if (path->obj_id == 0 && path->obj_inst_id == 0 && path->res_id == 2) {
		/* MODE */
		zassert_equal(*(uint8_t *)data, 2, "Wrong security type");

	} else if (path->obj_id == 0 && path->obj_inst_id == 0 && path->res_id == 3) {
		/*  certificate */
		zassert_equal(strncmp(data, MY_CERTIFICATE, len), 0, "Wrong certificate");

	} else if (path->obj_id == 0 && path->obj_inst_id == 0 && path->res_id == 4) {
		/* CA chain */
		zassert_equal(strncmp(data, MY_ROOT_CA, len), 0, "Wrong CA certificate");

	} else if (path->obj_id == 0 && path->obj_inst_id == 0 && path->res_id == 5) {
		/* private key */
		zassert_equal(strncmp(data, MY_PRIVATE_KEY, len), 0, "Wrong private key");
	} else {
		return -EINVAL;
	}
	return 0;
}

/* This stub should end up being called only if given resource is cleared.
 * or if default instance sets the server URI
 */
static int lwm2m_set_res_data_len_custom_fake_CERT(const struct lwm2m_obj_path *path,
						   uint16_t len)
{
	if (path->obj_id == 0 && path->obj_inst_id == 0 && path->res_id == 0) {
		/* URI */
		zassert_equal(len, sizeof(CONFIG_LWM2M_CLIENT_UTILS_SERVER), "Wrong URI");
		return 0;
	}

	zassert_equal(len, 0, "Len should be zero");
	if (path->obj_id == 0 && path->obj_inst_id == 0 && path->res_id == 3) {
		/*  certificate */
		certificate_cleared = true;

	} else if (path->obj_id == 0 && path->obj_inst_id == 0 && path->res_id == 4) {
		/* CA chain */
		ca_cleared = true;

	} else if (path->obj_id == 0 && path->obj_inst_id == 0 && path->res_id == 5) {
		/* private key */
		key_cleared = true;
	} else {
		zassert_unreachable("Unknown resource cleared");
	}
	return 0;
}

static int get_mode_PSK(const struct lwm2m_obj_path *path, uint8_t *mode)
{
	if (path->obj_id != 0 || path->obj_inst_id != 1 || path->res_id != 2) {
		zassert_unreachable("wrong path for mode %d/%d/%d", path->obj_id, path->obj_inst_id,
				    path->res_id);
	}
	*mode = 0;
	return 0;
}

static int get_mode_CERT(const struct lwm2m_obj_path *path, uint8_t *mode)
{
	if (path->obj_id != 0 || path->obj_inst_id != 1 || path->res_id != 2) {
		zassert_unreachable("wrong path for mode %d/%d/%d", path->obj_id, path->obj_inst_id,
				    path->res_id);
	}
	*mode = 2;
	return 0;
}

/* Return PSK for sec obj 1, certificate for sec obj 2 */
static int get_mode_both(const struct lwm2m_obj_path *path, uint8_t *mode)
{
	if (path->obj_id == 0 && path->obj_inst_id == 1 && path->res_id == 2) {
		*mode = 0;
	} else if (path->obj_id == 0 && path->obj_inst_id == 2 && path->res_id == 2) {
		*mode = 2;
	} else {
		zassert_unreachable("Requesting mode for unknown sec obj %d/%d/%d", path->obj_id,
				    path->obj_inst_id, path->res_id);
	}
	return 0;
}

static int get_psk_buf(const struct lwm2m_obj_path *path, void **buffer_ptr, uint16_t *buffer_len,
		       uint16_t *data_len, uint8_t *data_flags)
{
	if (path->obj_id == 0 && path->obj_inst_id == 1 && path->res_id == 5) {
		/* PSK */
		if (buffer_ptr)
			*buffer_ptr = my_psk_bin;
		if (buffer_len)
			*buffer_len = sizeof(my_psk_bin);
		if (data_len)
			*data_len = sizeof(my_psk_bin);
		return 0;
	} else if (path->obj_id == 0 && path->obj_inst_id == 1 && path->res_id == 3) {
		if (buffer_ptr)
			*buffer_ptr = MY_ID;
		if (buffer_len)
			*buffer_len = sizeof(MY_ID);
		if (data_len)
			*data_len = sizeof(MY_ID);
		return 0;
	}
	zassert_unreachable("Requesting unknown data");
	return -EINVAL;
}

static int get_cert_buf(const struct lwm2m_obj_path *path, void **buffer_ptr, uint16_t *buffer_len,
			uint16_t *data_len, uint8_t *data_flags)
{
	if (path->obj_id == 0 && path->obj_inst_id == 2 && path->res_id == 3) {
		if (buffer_ptr)
			*buffer_ptr = MY_CERTIFICATE;
		if (buffer_len)
			*buffer_len = sizeof(MY_CERTIFICATE);
		if (data_len)
			*data_len = sizeof(MY_CERTIFICATE);
		return 0;
	} else if (path->obj_id == 0 && path->obj_inst_id == 2 && path->res_id == 4) {
		if (buffer_ptr)
			*buffer_ptr = MY_ROOT_CA;
		if (buffer_len)
			*buffer_len = sizeof(MY_ROOT_CA);
		if (data_len)
			*data_len = sizeof(MY_ROOT_CA);
		return 0;
	} else if (path->obj_id == 0 && path->obj_inst_id == 2 && path->res_id == 5) {
		if (buffer_ptr)
			*buffer_ptr = MY_PRIVATE_KEY;
		if (buffer_len)
			*buffer_len = sizeof(MY_PRIVATE_KEY);
		if (data_len)
			*data_len = sizeof(MY_PRIVATE_KEY);
		return 0;
	}

	zassert_unreachable("Requesting unknown data (%d/%d/%d)", path->obj_id, path->obj_inst_id,
			    path->res_id);
	return -EINVAL;
}

static int get_cert_wrapper(const struct lwm2m_obj_path *path, void **buffer_ptr,
			    uint16_t *buffer_len, uint16_t *data_len, uint8_t *data_flags)
{
	if (path->obj_id == 0 && path->obj_inst_id == 1)
		return get_psk_buf(path, buffer_ptr, buffer_len, data_len, data_flags);
	else
		return get_cert_buf(path, buffer_ptr, buffer_len, data_len, data_flags);
}

static int get_uri_wrapper(const struct lwm2m_obj_path *path, void **buffer_ptr,
			   uint16_t *buffer_len, uint16_t *data_len, uint8_t *data_flags)
{
	if (path->obj_id == 0 && path->obj_inst_id == 1)
		return get_psk_buf(path, buffer_ptr, buffer_len, data_len, data_flags);
	else
		return lwm2m_get_res_buf_custom_fake(path, buffer_ptr, buffer_len, data_len,
						     data_flags);
}

static int write_to_modem(nrf_sec_tag_t sec_tag, enum modem_key_mgmt_cred_type cred_type,
			  const void *buf, size_t len)
{
	if (cred_type == MODEM_KEY_MGMT_CRED_TYPE_PSK) {
		zassert_equal(sec_tag, 200, "Wrong security tag");
		zassert_mem_equal(buf, MY_PSK, sizeof(MY_PSK), "Wrong PSK key (len=%d, buf=%s)",
				  len, buf);
	} else if (cred_type == MODEM_KEY_MGMT_CRED_TYPE_IDENTITY) {
		zassert_equal(sec_tag, 200, "Wrong security tag");
		zassert_mem_equal(buf, MY_ID, sizeof(MY_ID), "Wrong content (len=%d, buf=%s)", len,
				  buf);
	} else if (cred_type == MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT) {
		zassert_equal(sec_tag, 100, "Wrong security tag");
		zassert_mem_equal(buf, MY_CERTIFICATE, sizeof(MY_CERTIFICATE),
				  "Wrong content (len=%d, buf=%s)", len, buf);
	} else if (cred_type == MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN) {
		zassert_equal(sec_tag, 100, "Wrong security tag");
		zassert_mem_equal(buf, MY_ROOT_CA, sizeof(MY_ROOT_CA),
				  "Wrong content (len=%d, buf=%s)", len, buf);
	} else if (cred_type == MODEM_KEY_MGMT_CRED_TYPE_PRIVATE_CERT) {
		zassert_equal(sec_tag, 100, "Wrong security tag");
		zassert_mem_equal(buf, MY_PRIVATE_KEY, sizeof(MY_PRIVATE_KEY),
				  "Wrong content (len=%d, buf=%s)", len, buf);
	} else {
		zassert_unreachable("Writing unknown data");
	}
	return 0;
}

static int get_empty_CERT_buf(const struct lwm2m_obj_path *path, void **buffer_ptr,
			      uint16_t *buffer_len, uint16_t *data_len, uint8_t *data_flags)
{
	if (path->obj_id == 0 && path->obj_inst_id == 1 && path->res_id == 5) {
		/* PSK */
		if (buffer_ptr)
			*buffer_ptr = "";
		if (buffer_len)
			*buffer_len = 100;
		if (data_len)
			*data_len = 0;
		return 0;
	} else if (path->obj_id == 0 && path->obj_inst_id == 1 && path->res_id == 3) {
		if (buffer_ptr)
			*buffer_ptr = "";
		if (buffer_len)
			*buffer_len = 100;
		if (data_len)
			*data_len = 0;
		return 0;
	}
	zassert_unreachable("Requesting unknown data");
	return -EINVAL;
}

static int modem_X509_exists(nrf_sec_tag_t sec_tag, enum modem_key_mgmt_cred_type cred_type,
			     bool *exists)
{
	if (ctx.bootstrap_mode) {
		zassert_equal(sec_tag, 100, "Wrong security tag");
	} else {
		zassert_equal(sec_tag, 200, "Wrong security tag");
	}
	if (cred_type == MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN) {
		*exists = true;
	} else if (cred_type == MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT) {
		*exists = true;
	} else if (cred_type == MODEM_KEY_MGMT_CRED_TYPE_PRIVATE_CERT) {
		*exists = true;
	} else {
		zassert_unreachable("Requesting unknown data");
	}
	return 0;
}

ZTEST_SUITE(lwm2m_client_utils_security, NULL, NULL, NULL, NULL, NULL);

/*
 * Tests for initializing the module and setting the credentials
 */

ZTEST(lwm2m_client_utils_security, test_init_settings_fails)
{
	int rc;

	setup();
	/* First, test if settings fail */
	settings_subsys_init_fake.return_val = -1;
	rc = lwm2m_init_security(&ctx, MY_ENDPOINT, NULL);
	zassert_equal(rc, -1, "wrong return value");
}

ZTEST(lwm2m_client_utils_security, test_settings_reg_fails)
{
	/* Test if registering settings fails */
	int rc;

	setup();

	settings_register_fake.return_val = -2;
	rc = lwm2m_init_security(&ctx, MY_ENDPOINT, NULL);
	zassert_equal(settings_register_fake.call_count, 1, "Settings not registered");
	zassert_equal(rc, -2, "wrong return value");
}

ZTEST(lwm2m_client_utils_security, test_settings_load_fails)
{
	/* Test if loading fails */
	int rc;

	setup();

	settings_load_subtree_fake.return_val = -3;
	rc = lwm2m_init_security(&ctx, MY_ENDPOINT, NULL);
	zassert_equal(settings_load_subtree_fake.call_count, 1, "Settings not loaded");
	zassert_equal(rc, -3, "wrong return value");
}

ZTEST(lwm2m_client_utils_security, test_init_get_buf_fails)
{
	/* Test if getting buffer fails */
	int rc;

	setup();

	lwm2m_get_res_buf_fake.custom_fake = NULL;
	lwm2m_get_res_buf_fake.return_val = -4;
	rc = lwm2m_init_security(&ctx, MY_ENDPOINT, NULL);
	zassert_equal(rc, -4, "wrong return value");
}

ZTEST(lwm2m_client_utils_security, test_init)
{
	/* Normal case */
	int rc;

	setup();

	lwm2m_get_res_buf_fake.custom_fake = lwm2m_get_res_buf_custom_fake;
	rc = lwm2m_init_security(&ctx, MY_ENDPOINT, NULL);
	zassert_equal(rc, 0, "wrong return value");
	zassert_equal(ctx.tls_tag, 100, "Bootstrap TLS tag not selected");
	zassert_equal(lwm2m_delete_object_inst_fake.call_count, 1,
		      "Default server object not deleted");
	zassert_equal(lwm2m_register_create_callback_fake.call_count, 2,
		      "Callbacks not registered");
	zassert_equal(lwm2m_register_delete_callback_fake.call_count, 2,
		      "Callbacks not registered");
	zassert_equal(settings_subsys_init_fake.call_count, 1, "Settings subsys not initialized");
	zassert_equal(lwm2m_set_res_data_len_fake.call_count, 3, "Buffer size not set");

	/* check that bootstrap mode is set */
	zassert_equal(lwm2m_set_u8_fake.arg1_val, 1, "Bootstrap mode not set");
}

ZTEST(lwm2m_client_utils_security, test_need_bootstrap)
{
	int rc;

	setup();
	modem_key_mgmt_exists_fake.custom_fake = modem_key_mgmt_exists_custom_fake;

	rc = lwm2m_init_security(&ctx, MY_ENDPOINT, NULL);
	zassert_equal(rc, 0, "wrong return value");

	zassert_equal(lwm2m_security_needs_bootstrap(), true, "Should need bootstrap");
}

ZTEST(lwm2m_client_utils_security, test_lwm2m_security_set_psk)
{
	int rc;

	setup();

	lwm2m_set_opaque_fake.custom_fake = lwm2m_set_opaque_custom_fake_PSK;

	rc = lwm2m_security_set_psk(0, MY_PSK, sizeof(MY_PSK), true, MY_ID);
	zassert_equal(rc, 0, "wrong return value");
}

ZTEST(lwm2m_client_utils_security, test_lwm2m_security_set_certificate)
{
	int rc;

	setup();

	lwm2m_set_opaque_fake.custom_fake = lwm2m_set_opaque_custom_fake_CERT;
	lwm2m_set_res_data_len_fake.custom_fake =
		lwm2m_set_res_data_len_custom_fake_CERT;

	rc = lwm2m_security_set_certificate(0, MY_CERTIFICATE, sizeof(MY_CERTIFICATE),
					    MY_PRIVATE_KEY, sizeof(MY_PRIVATE_KEY), MY_ROOT_CA,
					    sizeof(MY_ROOT_CA));
	zassert_equal(rc, 0, "wrong return value");
	zassert_equal(certificate_cleared, false, "Certificate was cleared");
	zassert_equal(key_cleared, false, "Key was cleared");
	zassert_equal(ca_cleared, false, "CA was cleared");
}

/* In this case, certificate is already provisioned to modem */
ZTEST(lwm2m_client_utils_security, test_lwm2m_security_set_certificate_provisioned)
{
	int rc;

	setup();

	lwm2m_set_opaque_fake.custom_fake = lwm2m_set_opaque_custom_fake_CERT;
	lwm2m_set_res_data_len_fake.custom_fake =
		lwm2m_set_res_data_len_custom_fake_CERT;

	rc = lwm2m_security_set_certificate(0, NULL, 0, NULL, 0, NULL, 0);
	zassert_equal(rc, 0, "wrong return value");
	zassert_equal(certificate_cleared, true, "Certificate was not cleared");
	zassert_equal(key_cleared, true, "Key was not cleared");
	zassert_equal(ca_cleared, true, "CA was not cleared");
}

/*
 * Tests for loading the credentials to modem
 */

ZTEST(lwm2m_client_utils_security, test_load_credentials_PSK)
{
	int rc;

	setup();
	rc = lwm2m_init_security(&ctx, MY_ENDPOINT, NULL);
	zassert_equal(rc, 0, "wrong return value");

	zassert_not_null(ctx.load_credentials, "load_credentials callback not set");

	lwm2m_get_u8_fake.custom_fake = get_mode_PSK;
	lwm2m_get_res_buf_fake.custom_fake = get_psk_buf;
	modem_key_mgmt_write_fake.custom_fake = write_to_modem;
	modem_key_mgmt_exists_fake.custom_fake = modem_key_mgmt_exists_custom_fake;
	ctx.bootstrap_mode = false;

	rc = ctx.load_credentials(&ctx);
	zassert_equal(rc, 0, "wrong return value");
	zassert_equal(modem_key_mgmt_exists_fake.call_count, 5, "Did not check existing (%d)",
		      modem_key_mgmt_exists_fake.call_count);
	zassert_equal(modem_key_mgmt_write_fake.call_count, 2, "Did not write PSK");
	zassert_equal(lte_lc_func_mode_set_fake.call_count, 1, "Did not set mode");
	zassert_equal(lte_lc_func_mode_set_fake.arg0_val, LTE_LC_FUNC_MODE_OFFLINE,
		      "Did not go into offline");
	zassert_equal(lte_lc_connect_fake.call_count, 1, "Did not connect LTE");
}

ZTEST(lwm2m_client_utils_security, test_load_credentials_CERT_provisioned)
{
	int rc;

	setup();
	rc = lwm2m_init_security(&ctx, MY_ENDPOINT, NULL);
	zassert_equal(rc, 0, "wrong return value");

	zassert_not_null(ctx.load_credentials, "load_credentials callback not set");

	lwm2m_get_u8_fake.custom_fake = get_mode_CERT;
	lwm2m_get_res_buf_fake.custom_fake = get_empty_CERT_buf;
	modem_key_mgmt_exists_fake.custom_fake = modem_X509_exists;
	rc = ctx.load_credentials(&ctx);
	zassert_equal(rc, 0, "wrong return value");
	zassert_equal(modem_key_mgmt_exists_fake.call_count, 6, "Did not check existing (%d)",
		      modem_key_mgmt_exists_fake.call_count);
	zassert_equal(modem_key_mgmt_write_fake.call_count, 0, "Tried to write");
	zassert_equal(lte_lc_func_mode_set_fake.call_count, 0, "Changed LTE mode");
	zassert_equal(lte_lc_connect_fake.call_count, 0, "Changed LTE mode");
}

/*
 * Tests for handling the loading of settings from storage
 */

static struct settings_handler *handler;

static int copy_settings_hanler(struct settings_handler *cf)
{
	handler = cf;
	return 0;
}

static struct lwm2m_engine_obj_inst *get_pk_obj(const struct lwm2m_obj_path *path)
{
	static struct lwm2m_engine_obj_inst obj = { .obj_inst_id = 1, .resource_count = 1 };

	return &obj;
}

static ssize_t read_cb(void *cb_arg, void *data, size_t len)
{
	memcpy(data, cb_arg, len);
	return len;
}

static int get_bs_flag(const struct lwm2m_obj_path *path, bool *value)
{
	if (path->obj_id != 0 || path->obj_inst_id != 2 || path->res_id != 1) {
		zassert_unreachable("Wrong path %d/%d/%d", path->obj_id, path->obj_inst_id,
				    path->res_id);
	}
	*value = true;
	return 0;
}

ZTEST(lwm2m_client_utils_security, test_load_from_flash)
{
	int rc;

	setup();
	settings_register_fake.custom_fake = copy_settings_hanler;
	lwm2m_engine_get_obj_inst_fake.custom_fake = get_pk_obj;
	lwm2m_get_res_buf_fake.custom_fake = lwm2m_get_res_buf_custom_fake;
	modem_key_mgmt_exists_fake.custom_fake = modem_key_mgmt_exists_custom_fake;

	rc = lwm2m_init_security(&ctx, MY_ENDPOINT, NULL);
	zassert_equal(rc, 0, "wrong return value");

	zassert_equal(settings_load_subtree_fake.call_count, 1, "Did not load settings");
	zassert_not_null(handler, "Did not set handler");
	zassert_not_null(handler->h_set, "Did not set handler");

	handler->h_set("0/1/3", sizeof(MY_ID), read_cb, MY_ID);
	zassert_mem_equal(my_buf, MY_ID, sizeof(MY_ID), "Incorrect ID");

	handler->h_set("0/1/5", sizeof(my_psk_bin), read_cb, my_psk_bin);
	zassert_mem_equal(my_buf, my_psk_bin, sizeof(my_psk_bin), "Incorrect PSK");

	keys_exist = true;
	zassert_equal(lwm2m_security_needs_bootstrap(), false, "Still needs bootstrap");
}

static int test_bs_loading(const char *subtree)
{
	uint8_t flag = 1;

	handler->h_set("0/2/1", sizeof(flag), read_cb, &flag);

	zassert_equal(lwm2m_get_bool_fake.call_count, 1, "Did not read the flag");
	return 0;
}

ZTEST(lwm2m_client_utils_security, test_load_boostrap_from_flash)
{
	int rc;

	setup();
	settings_register_fake.custom_fake = copy_settings_hanler;
	lwm2m_engine_get_obj_inst_fake.custom_fake = get_pk_obj;
	lwm2m_get_res_buf_fake.custom_fake = lwm2m_get_res_buf_custom_fake;
	lwm2m_get_bool_fake.custom_fake = get_bs_flag;

	settings_load_subtree_fake.custom_fake = test_bs_loading;

	rc = lwm2m_init_security(&ctx, MY_ENDPOINT, NULL);
	zassert_equal(rc, 0, "wrong return value");

	zassert_equal(lwm2m_delete_object_inst_fake.call_count, 2,
		      "Did not delete extra security&server instances");
}

/*
 * Test for writing data to flash
 */

static lwm2m_engine_user_cb_t sec_cb;
static int copy_sec_cb(uint16_t obj_id, lwm2m_engine_user_cb_t cb)
{
	if (obj_id == 0) {
		sec_cb = cb;
	}
	return 0;
}

static lwm2m_engine_set_data_cb_t set_uri_cb;
static int copy_uri_write_cb(const struct lwm2m_obj_path *path, lwm2m_engine_set_data_cb_t cb)
{
	if (path->obj_id == 0 && path->obj_inst_id == 1 && path->res_id == 0) {
		set_uri_cb = cb;
	}
	return 0;
}

static int check_server_addr(const char *name, const void *value, size_t val_len)
{
	zassert_not_null(name, "NULL");
	zassert_not_null(value, "NULL");
	zassert_equal(val_len, sizeof(CONFIG_LWM2M_CLIENT_UTILS_SERVER), "Wrong length");
	zassert_equal(strcmp(name, "lwm2m:sec/0/1/0"), 0, "Wrong path");
	zassert_equal(strcmp(value, CONFIG_LWM2M_CLIENT_UTILS_SERVER), 0, "Wrong path");
	return 0;
}

ZTEST(lwm2m_client_utils_security, test_store_to_flash)
{
	int rc;

	setup();

	lwm2m_register_create_callback_fake.custom_fake = copy_sec_cb;
	settings_save_one_fake.custom_fake = check_server_addr;

	rc = lwm2m_init_security(&ctx, MY_ENDPOINT, NULL);
	zassert_equal(rc, 0, "wrong return value");
	zassert_not_null(sec_cb, "Did not set callback");

	lwm2m_register_post_write_callback_fake.custom_fake = copy_uri_write_cb;
	rc = sec_cb(1);
	zassert_equal(rc, 0, "wrong return value");
	zassert_not_null(set_uri_cb, "Did not set callback");

	rc = set_uri_cb(1, 0, 0, CONFIG_LWM2M_CLIENT_UTILS_SERVER,
			sizeof(CONFIG_LWM2M_CLIENT_UTILS_SERVER), true,
			sizeof(CONFIG_LWM2M_CLIENT_UTILS_SERVER));
	zassert_equal(rc, 0, "wrong return value");
	zassert_equal(settings_save_one_fake.call_count, 1, "Did not store");

}

ZTEST(lwm2m_client_utils_security, test_remove_from_flash)
{
	int rc;

	setup();

	lwm2m_register_delete_callback_fake.custom_fake = copy_sec_cb;

	rc = lwm2m_init_security(&ctx, MY_ENDPOINT, NULL);
	zassert_equal(rc, 0, "wrong return value");
	zassert_not_null(sec_cb, "Did not set callback");

	rc = sec_cb(1);
	zassert_equal(settings_delete_fake.call_count, 6, "Deleted only %d resources",
		      settings_delete_fake.call_count);
}

/*
 * Special cases
 */

/* Test a case where during a bootstrap, we receive new PSK credentials normally
 * But also we receive new certificates for bootstrap
 */
ZTEST(lwm2m_client_utils_security, test_bs_X509)
{
	int rc;

	setup();
	keys_exist = true;
	lwm2m_register_create_callback_fake.custom_fake = copy_sec_cb;
	lwm2m_register_post_write_callback_fake.custom_fake = copy_uri_write_cb;
	lwm2m_get_u8_fake.custom_fake = get_mode_both;
	modem_key_mgmt_write_fake.custom_fake = write_to_modem;
	modem_key_mgmt_exists_fake.custom_fake = modem_key_mgmt_exists_custom_fake;
	lwm2m_get_res_buf_fake.custom_fake = lwm2m_get_res_buf_custom_fake;

	rc = lwm2m_init_security(&ctx, MY_ENDPOINT, NULL);
	lwm2m_get_res_buf_fake.custom_fake = get_cert_wrapper;
	zassert_equal(rc, 0, "wrong return value");

	rc = sec_cb(1);
	zassert_equal(rc, 0, "wrong return value");

	uint8_t flag = 1;

	rc = set_uri_cb(2, 1, 0, &flag, sizeof(flag), true, sizeof(flag));
	zassert_equal(rc, 0, "wrong return value");

	ctx.sec_obj_inst = 1;
	ctx.bootstrap_mode = false;
	rc = ctx.load_credentials(&ctx);
	zassert_equal(rc, 0, "wrong return value");

}

/* Test a case where during a bootstrap, we receive new PSK credentials normally
 * But also we receive new data for bootstrap, but not certificates (For example new URI)
 */
ZTEST(lwm2m_client_utils_security, test_bs_URI)
{
	int rc;

	setup();
	keys_exist = true;
	lwm2m_register_create_callback_fake.custom_fake = copy_sec_cb;
	lwm2m_register_post_write_callback_fake.custom_fake = copy_uri_write_cb;
	lwm2m_get_u8_fake.custom_fake = get_mode_both;
	modem_key_mgmt_write_fake.custom_fake = write_to_modem;
	modem_key_mgmt_exists_fake.custom_fake = modem_key_mgmt_exists_custom_fake;
	lwm2m_get_res_buf_fake.custom_fake = lwm2m_get_res_buf_custom_fake;

	rc = lwm2m_init_security(&ctx, MY_ENDPOINT, NULL);
	lwm2m_get_res_buf_fake.custom_fake = get_uri_wrapper;
	zassert_equal(rc, 0, "wrong return value");

	rc = sec_cb(1);
	zassert_equal(rc, 0, "wrong return value");

	uint8_t flag = 1;

	rc = set_uri_cb(2, 1, 0, &flag, sizeof(flag), true, sizeof(flag));
	zassert_equal(rc, 0, "wrong return value");

	ctx.sec_obj_inst = 1;
	ctx.bootstrap_mode = false;
	rc = ctx.load_credentials(&ctx);
	zassert_equal(rc, 0, "wrong return value");

	zassert_equal(modem_key_mgmt_write_fake.call_count, 2, "Written more than just PSK");
}
