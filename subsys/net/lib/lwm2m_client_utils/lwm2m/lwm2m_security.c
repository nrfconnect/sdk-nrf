/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net/lwm2m.h>
#include <net/lwm2m_client_utils.h>
#include <lwm2m_engine.h>
#include <lwm2m_rd_client.h>
#include <lwm2m_util.h>
#include <modem/modem_key_mgmt.h>
#include <modem/lte_lc.h>
#include <zephyr/settings/settings.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_ncs.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(lwm2m_security, CONFIG_LWM2M_CLIENT_UTILS_LOG_LEVEL);

/* LWM2M_OBJECT_SECURITY_ID */
#define SECURITY_SERVER_URI_ID 0
#define SECURITY_BOOTSTRAP_FLAG_ID 1
#define SECURITY_MODE_ID 2
#define SECURITY_CLIENT_PK_ID 3
#define SECURITY_SERVER_PK_ID 4
#define SECURITY_SECRET_KEY_ID 5
#define SECURITY_SHORT_SERVER_ID 10
/* LWM2M_OBJECT_SERVER_ID */
#define SERVER_SHORT_SERVER_ID 0
#define SERVER_LIFETIME_ID 1

enum security_mode {
	SEC_MODE_PSK = 0,
	SEC_MODE_CERTIFICATE = 2,
	SEC_MODE_NO_SEC = 3,
};

static struct modem_mode_change mm;

int lwm2m_modem_mode_cb(enum lte_lc_func_mode new_mode, void *user_data)
{
	enum lte_lc_func_mode fmode;
	int ret;

	if (lte_lc_func_mode_get(&fmode)) {
		LOG_ERR("Failed to read modem functional mode");
		ret = -EFAULT;
		return ret;
	}

	if (new_mode == LTE_LC_FUNC_MODE_NORMAL) {
		/* I need to use the blocking call, because in next step
		 * LwM2M engine would create socket and call connect()
		 */
		ret = lte_lc_connect();

		if (ret) {
			LOG_ERR("lte_lc_connect() failed %d", ret);
		}
		LOG_INF("Modem connection restored");
	} else {
		ret = lte_lc_func_mode_set(new_mode);
		if (ret == 0) {
			LOG_DBG("Modem set to requested state %d", new_mode);
		}
	}

	return ret;
}

#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
#define SETTINGS_PREFIX "lwm2m:sec"

static bool have_permanently_stored_keys;
static int bootstrap_settings_loaded_inst = -1;
static bool loading_in_progress;

static int write_credential_type(int sec_obj_inst, int sec_tag, int res_id,
				 enum modem_key_mgmt_cred_type type)
{
	int ret;
	void *cred = NULL;
	uint16_t cred_len;
	char psk_hex[65];

	ret = lwm2m_get_res_buf(&LWM2M_OBJ(0, sec_obj_inst, res_id), &cred, NULL, &cred_len,  NULL);
	if (ret < 0) {
		LOG_ERR("Unable to get resource data for '%d/%d/%d'", 0, sec_obj_inst, res_id);
		return ret;
	}

	if (cred_len == 0) {
		bool exist;

		ret = modem_key_mgmt_exists(sec_tag, type, &exist);
		if (ret) {
			return ret;
		}
		if (exist) {
			return -EEXIST;
		}
		return -ENOENT;
	}

	/* Convert binary PSK key to hex format, which is expect by modem */
	if (type == MODEM_KEY_MGMT_CRED_TYPE_PSK) {
		cred_len = bin2hex(cred, cred_len, psk_hex, sizeof(psk_hex));
		if (cred_len == 0) {
			LOG_ERR("PSK is too large to convert (%d)", -EOVERFLOW);
			return -EOVERFLOW;
		}
		cred = psk_hex;
	}

	ret = modem_key_mgmt_write(sec_tag, type, cred, cred_len);
	if (ret < 0) {
		LOG_ERR("Unable to write credentials to modem (%d)", ret);
		return ret;
	}
	LOG_DBG("Written sec_tag %d, type %d", sec_tag, type);
	return 0;
}

static int write_sec_obj_to_sec_tag(int sec_obj_inst, int sec_tag, int mode)
{
	int ret;

	if (mode == SEC_MODE_PSK) {
		ret = write_credential_type(sec_obj_inst, sec_tag, SECURITY_CLIENT_PK_ID,
					    MODEM_KEY_MGMT_CRED_TYPE_IDENTITY);
		if (ret) {
			goto out;
		}

		ret = write_credential_type(sec_obj_inst, sec_tag, SECURITY_SECRET_KEY_ID,
					    MODEM_KEY_MGMT_CRED_TYPE_PSK);
		if (ret) {
			goto out;
		}
	} else if (mode == SEC_MODE_CERTIFICATE) {
		/* Don't fail if we already have a given data in the modem and we did not receive
		 * that as part of bootstrap. It might have been written as part of EST process.
		 */
		ret = write_credential_type(sec_obj_inst, sec_tag, SECURITY_SERVER_PK_ID,
					    MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);
		if (ret && ret != -EEXIST) {
			goto out;
		}
		ret = write_credential_type(sec_obj_inst, sec_tag, SECURITY_CLIENT_PK_ID,
					    MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT);
		if (ret && ret != -EEXIST) {
			goto out;
		}
		ret = write_credential_type(sec_obj_inst, sec_tag, SECURITY_SECRET_KEY_ID,
					    MODEM_KEY_MGMT_CRED_TYPE_PRIVATE_CERT);
		if (ret && ret != -EEXIST) {
			goto out;
		}
	} else {
		ret = -ENOTSUP;
	}
out:
	LOG_DBG("write_sec_obj_to_sec_tag(%d, %d, %d) ret = %d", sec_obj_inst, sec_tag, mode, ret);
	return ret;
}

static int sec_mode(int sec_obj_inst)
{
	uint8_t mode;
	int ret;

	ret = lwm2m_get_u8(&LWM2M_OBJ(0, sec_obj_inst, SECURITY_MODE_ID), &mode);
	if (ret < 0) {
		return ret;
	}
	return mode;
}

static bool sec_obj_has_credentials(int sec_obj_inst)
{
	int ret;
	void *cred = NULL;
	uint16_t cred_len;
	int mode;
	struct lwm2m_obj_path path;

	mode = sec_mode(sec_obj_inst);
	if (mode < 0) {
		return false;
	}
	if (mode != SEC_MODE_CERTIFICATE && mode != SEC_MODE_PSK) {
		return false;
	}

	path = LWM2M_OBJ(0, sec_obj_inst, SECURITY_CLIENT_PK_ID);
	ret = lwm2m_get_res_buf(&path, &cred, NULL, &cred_len, NULL);
	if (ret < 0) {
		goto fail;
	}
	if (cred_len == 0) {
		return false;
	}

	path.res_id = SECURITY_SECRET_KEY_ID;
	ret = lwm2m_get_res_buf(&path, &cred, NULL, &cred_len, NULL);
	if (ret < 0) {
		goto fail;
	}

	return cred_len != 0;
fail:
	LOG_ERR("Unable to get resource data for '%d/%d/%d', rc = %d", path.obj_id,
		path.obj_inst_id, path.res_id, ret);
	return false;
}

static bool modem_has_credentials(int sec_tag, int mode)
{
	bool exist;
	int ret;

	if (mode == SEC_MODE_CERTIFICATE) {
		bool val1, val2, val3;

		ret = modem_key_mgmt_exists(sec_tag, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, &val1);
		if (ret < 0) {
			return false;
		}
		ret = modem_key_mgmt_exists(sec_tag, MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT, &val2);
		if (ret < 0) {
			return false;
		}
		ret = modem_key_mgmt_exists(sec_tag, MODEM_KEY_MGMT_CRED_TYPE_PRIVATE_CERT, &val3);
		if (ret < 0) {
			return false;
		}
		exist = val1 && val2 && val3;
	} else if (mode == SEC_MODE_PSK) {
		bool val1, val2;

		ret = modem_key_mgmt_exists(sec_tag, MODEM_KEY_MGMT_CRED_TYPE_PSK, &val1);
		if (ret < 0) {
			return false;
		}

		ret = modem_key_mgmt_exists(sec_tag, MODEM_KEY_MGMT_CRED_TYPE_IDENTITY, &val2);
		if (ret < 0) {
			return false;
		}
		exist = val1 && val2;
	} else if (mode == SEC_MODE_NO_SEC) {
		return true;
	} else {
		return false;
	}
	return exist;
}

static int load_credentials_to_modem(struct lwm2m_ctx *ctx)
{
	int ret;
	bool exist;
	bool has_credentials;
	int mode;

	if (ctx->bootstrap_mode) {
		ctx->tls_tag = CONFIG_LWM2M_CLIENT_UTILS_BOOTSTRAP_TLS_TAG;
	} else {
		ctx->tls_tag = CONFIG_LWM2M_CLIENT_UTILS_SERVER_TLS_TAG;
	}

	mode = sec_mode(ctx->sec_obj_inst);
	if (mode < 0) {
		return mode;
	}

	exist = modem_has_credentials(ctx->tls_tag, mode);
	has_credentials = sec_obj_has_credentials(ctx->sec_obj_inst);

	/* If we have credentials already in modem, and we have loaded settings from the flash
	 * assume that the ones in modem are correct.
	 * Don't overwrite bootstrap keys with hardcoded keys.
	 */
	if (exist &&
	    (have_permanently_stored_keys ||
	     ctx->tls_tag == CONFIG_LWM2M_CLIENT_UTILS_BOOTSTRAP_TLS_TAG || !has_credentials)) {
		LOG_DBG("Existing credentials found on modem");
		return 0;
	}

	if (!has_credentials) {
		LOG_ERR("No security credentials provisioned");
		return -ENOENT;
	}

	LOG_INF("Need to write credentials, requesting LTE and GNSS offline...");

	while ((ret = mm.cb(LTE_LC_FUNC_MODE_OFFLINE, mm.user_data)) != 0) {
		if (ret < 0) {
			return ret; /* Error */
		}

		/* Block the LwM2M engine until mode switch is OK */
		k_sleep(K_SECONDS(ret));
	}

	ret = write_sec_obj_to_sec_tag(ctx->sec_obj_inst, ctx->tls_tag, mode);
	if (ret < 0) {
		LOG_ERR("Failed to write credentials to modem, err %d", ret);
		goto out;
	}

	/* Do I need to update bootstrap keys as well?
	 * These keys should only be updated when server keys are updated as well,
	 * so this is after the bootstrap has finished.
	 */
	if (bootstrap_settings_loaded_inst != -1 &&
	    sec_obj_has_credentials(bootstrap_settings_loaded_inst)) {
		int bs_mode = sec_mode(bootstrap_settings_loaded_inst);

		ret = write_sec_obj_to_sec_tag(bootstrap_settings_loaded_inst,
					       CONFIG_LWM2M_CLIENT_UTILS_BOOTSTRAP_TLS_TAG,
					       bs_mode);
		if (ret < 0) {
			LOG_ERR("Failed to write Boostrap credentials to modem, err %d", ret);
			goto out;
		}
		/* Prevent rewriting the same key on next reconnect or next boostrap */
		bootstrap_settings_loaded_inst = -1;
	}

	/* Mark that we have now written those keys, so
	 * reconnection or rebooting does not cause another rewrite
	 */
	if (!have_permanently_stored_keys) {
		have_permanently_stored_keys = true;
	}

out:
	LOG_INF("Requesting LTE and GNSS online");

	while ((ret = mm.cb(LTE_LC_FUNC_MODE_NORMAL, mm.user_data)) != 0) {
		if (ret < 0) {
			return ret; /* Error */
		}

		LOG_DBG("Reconnection delayed by %d seconds", ret);
		/* Block the LwM2M engine until mode switch is OK */
		k_sleep(K_SECONDS(ret));
	}

	return ret;
}

#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)

static bool object_instance_exist(int obj, int inst)
{
	struct lwm2m_obj_path path = {
		.level = LWM2M_PATH_LEVEL_OBJECT_INST,
		.obj_id = obj,
		.obj_inst_id = inst,
	};

	return lwm2m_engine_get_obj_inst(&path) != NULL;
}

static int loaded(void)
{
	loading_in_progress = false;
	return 0;
}

static int set(const char *key, size_t len_rd, settings_read_cb read_cb, void *cb_arg)
{
	uint16_t len;
	void *buf;
	uint8_t flags;
	struct lwm2m_obj_path path;
	int ret;
	uint8_t level;

	if (!key) {
		return -ENOENT;
	}

	LOG_DBG("Loading \"%s\"", key);

	ret = lwm2m_string_to_path(key, &path, '/');

	if (ret) {
		return ret;
	}

	level = path.level;
	path.level = LWM2M_PATH_LEVEL_OBJECT_INST;

	/* Create object instance, if it does not exist */
	if (!object_instance_exist(path.obj_id, path.obj_inst_id)) {
		ret = lwm2m_create_object_inst(&path);
		if (ret) {
			LOG_ERR("Failed to create object instance %d/%d", path.obj_id,
				path.obj_inst_id);
			return ret;
		}
	}

	path.level = level;

	if (lwm2m_get_res_buf(&path, &buf, &len, NULL, &flags) != 0) {
		LOG_ERR("Failed to get data pointer");
		return -ENOENT;
	}

	len = read_cb(cb_arg, buf, len_rd);
	if (len <= 0) {
		LOG_ERR("Failed to read data");
		return -ENOENT;
	}

	lwm2m_set_res_data_len(&path,  len);

	if (path.obj_id == LWM2M_OBJECT_SECURITY_ID && path.res_id == SECURITY_BOOTSTRAP_FLAG_ID) {
		bool is_bootstrap;

		lwm2m_get_bool(&path, &is_bootstrap);
		if (is_bootstrap) {
			bootstrap_settings_loaded_inst = path.obj_inst_id;
		}
	}

	/* TODO: check that all has been loaded */
	have_permanently_stored_keys = true;

	return 0;
}
static struct settings_handler lwm2m_security_settings = {
	.name = SETTINGS_PREFIX,
	.h_set = set,
	.h_commit = loaded,
};

static int write_to_settings(int obj, int inst, int res, uint8_t *data, uint16_t data_len)
{
	char path[sizeof(SETTINGS_PREFIX "/0/0/10")];

	snprintk(path, sizeof(path), SETTINGS_PREFIX "/%d/%d/%d", obj, inst, res);
	if (settings_save_one(path, data, data_len)) {
		LOG_ERR("Failed to store %s", path);
	}
	LOG_DBG("Permanently stored %s", path);
	return 0;
}

static void delete_from_storage(int obj, int inst, int res)
{
	char path[sizeof(SETTINGS_PREFIX "/0/0/0")];

	snprintk(path, sizeof(path), SETTINGS_PREFIX "/%d/%d/%d", obj, inst, res);
	settings_delete(path);
	LOG_DBG("Deleted %s", path);
}



static int write_cb_sec(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id, uint8_t *data,
			uint16_t data_len, bool last_block, size_t total_size)
{
	if (loading_in_progress) {
		return 0;
	}
	/* We need to know which instance have new Boostrap keys */
	if (res_id == SECURITY_BOOTSTRAP_FLAG_ID) {
		if (*((bool *)data) == true) {
			bootstrap_settings_loaded_inst = obj_inst_id;
		}
	}
	return write_to_settings(LWM2M_OBJECT_SECURITY_ID, obj_inst_id, res_id, data, data_len);
}

static int write_cb_srv(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id, uint8_t *data,
			uint16_t data_len, bool last_block, size_t total_size)
{
	if (loading_in_progress) {
		return 0;
	}
	if (res_id == SERVER_LIFETIME_ID) {
		engine_trigger_update(false);
	}
	return write_to_settings(LWM2M_OBJECT_SERVER_ID, obj_inst_id, res_id, data, data_len);
}

static void register_write_cb(int obj, int inst, int res)
{
	lwm2m_engine_set_data_cb_t cb;

	if (obj == LWM2M_OBJECT_SECURITY_ID) {
		cb = write_cb_sec;
	} else {
		cb = write_cb_srv;
	}

	lwm2m_register_post_write_callback(&LWM2M_OBJ(obj, inst, res), cb);
}

static int server_deleted(uint16_t id)
{
	delete_from_storage(LWM2M_OBJECT_SERVER_ID, id, SERVER_LIFETIME_ID);
	delete_from_storage(LWM2M_OBJECT_SERVER_ID, id, SERVER_SHORT_SERVER_ID);
	return 0;
}

static int server_created(uint16_t id)
{
	register_write_cb(LWM2M_OBJECT_SERVER_ID, id, SERVER_LIFETIME_ID);
	register_write_cb(LWM2M_OBJECT_SERVER_ID, id, SERVER_SHORT_SERVER_ID);
	return 0;
}

static int security_deleted(uint16_t id)
{
	if (!loading_in_progress) {
		delete_from_storage(LWM2M_OBJECT_SECURITY_ID, id, SECURITY_SERVER_URI_ID);
		delete_from_storage(LWM2M_OBJECT_SECURITY_ID, id, SECURITY_BOOTSTRAP_FLAG_ID);
		delete_from_storage(LWM2M_OBJECT_SECURITY_ID, id, SECURITY_MODE_ID);
		delete_from_storage(LWM2M_OBJECT_SECURITY_ID, id, SECURITY_CLIENT_PK_ID);
		delete_from_storage(LWM2M_OBJECT_SECURITY_ID, id, SECURITY_SERVER_PK_ID);
		delete_from_storage(LWM2M_OBJECT_SECURITY_ID, id, SECURITY_SHORT_SERVER_ID);
		have_permanently_stored_keys = false;
	}
	return 0;
}

static int security_created(uint16_t id)
{
	register_write_cb(LWM2M_OBJECT_SECURITY_ID, id, SECURITY_SERVER_URI_ID);
	register_write_cb(LWM2M_OBJECT_SECURITY_ID, id, SECURITY_BOOTSTRAP_FLAG_ID);
	register_write_cb(LWM2M_OBJECT_SECURITY_ID, id, SECURITY_MODE_ID);
	register_write_cb(LWM2M_OBJECT_SECURITY_ID, id, SECURITY_CLIENT_PK_ID);
	register_write_cb(LWM2M_OBJECT_SECURITY_ID, id, SECURITY_SERVER_PK_ID);
	register_write_cb(LWM2M_OBJECT_SECURITY_ID, id, SECURITY_SHORT_SERVER_ID);
	return 0;
}

static bool server_keys_exist_in_modem(void)
{
	return modem_has_credentials(CONFIG_LWM2M_CLIENT_UTILS_SERVER_TLS_TAG, SEC_MODE_PSK);
}

bool lwm2m_security_needs_bootstrap(void)
{
	return !have_permanently_stored_keys || !server_keys_exist_in_modem();
}
#else /* CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP */
bool lwm2m_security_needs_bootstrap(void)
{
	return false;
}
#endif /* CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP */
#endif /* CONFIG_LWM2M_DTLS_SUPPORT */

static int init_default_security_obj(struct lwm2m_ctx *ctx, char *endpoint)
{
	int ret;
	char *server_url;
	uint16_t server_url_len;

	/* Server URL */
	ret = lwm2m_get_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_SECURITY_ID, 0,
					   SECURITY_SERVER_URI_ID),
				(void **)&server_url, &server_url_len, NULL, NULL);
	if (ret < 0) {
		return ret;
	}

	server_url_len = snprintk(server_url, server_url_len, "%s",
				  CONFIG_LWM2M_CLIENT_UTILS_SERVER);

	lwm2m_set_res_data_len(&LWM2M_OBJ(LWM2M_OBJECT_SECURITY_ID, 0,
					  SECURITY_SERVER_URI_ID), server_url_len + 1);

	/* Security Mode, default to PSK with key written by application */
	if (IS_ENABLED(CONFIG_LWM2M_DTLS_SUPPORT)) {
		/* At minimum, we are storing endpoint name */
		/* This works on PSK where credentials are already in modem */
		lwm2m_security_set_psk(0, NULL, 0, false, endpoint);
		/* But if modem has certificates, change the mode to match */
		if (modem_has_credentials(ctx->tls_tag, SEC_MODE_CERTIFICATE)) {
			lwm2m_set_u8(&LWM2M_OBJ(LWM2M_OBJECT_SECURITY_ID, 0, SECURITY_MODE_ID),
				     SEC_MODE_CERTIFICATE);
		}
	} else {
		lwm2m_set_u8(&LWM2M_OBJ(LWM2M_OBJECT_SECURITY_ID, 0, SECURITY_MODE_ID),
			     SEC_MODE_NO_SEC);
	}

#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)
	/* Mark 1st instance of security object as a bootstrap server */
	lwm2m_set_u8(&LWM2M_OBJ(0, 0, 1), 1);
#else
	/* Security and Server object need matching Short Server ID value. */
	lwm2m_set_u16(&LWM2M_OBJ(0, 0, 10), 101);
	lwm2m_set_u16(&LWM2M_OBJ(1, 0, 0), 101);
#endif
	return 0;
}

static int lwm2m_security_set_opaque_res(uint16_t sec_obj_inst, uint16_t resource, const void *buf,
					 int len)
{
	if (buf && len) {
		return lwm2m_set_opaque(&LWM2M_OBJ(0, sec_obj_inst, resource), (void *) buf, len);
	} else {
		return lwm2m_set_res_data_len(&LWM2M_OBJ(0, sec_obj_inst, resource), 0);
	}
}

static int lwm2m_security_set(uint16_t sec_obj_inst, uint8_t mode, const void *pubkey,
			      int pubkeylen, const void *serverkey, int serverkeylen,
			      const void *secretkey, int secretkeylen)
{
	int rc;

	rc = lwm2m_security_set_opaque_res(sec_obj_inst, SECURITY_MODE_ID, &mode, sizeof(mode));
	if (rc) {
		return rc;
	}

	rc = lwm2m_security_set_opaque_res(sec_obj_inst, SECURITY_CLIENT_PK_ID, pubkey, pubkeylen);
	if (rc) {
		return rc;
	}

	rc = lwm2m_security_set_opaque_res(sec_obj_inst, SECURITY_SERVER_PK_ID, serverkey,
					   serverkeylen);
	if (rc) {
		return rc;
	}

	rc = lwm2m_security_set_opaque_res(sec_obj_inst, SECURITY_SECRET_KEY_ID, secretkey,
					   secretkeylen);
	return rc;
}

int lwm2m_security_set_psk(uint16_t sec_obj_inst, const void *psk, int psk_len, bool psk_is_hex,
			   const char *psk_id)
{
	char buf[1 + psk_len / 2];

	if (!IS_ENABLED(CONFIG_LWM2M_DTLS_SUPPORT)) {
		return -ENOTSUP;
	}

	if (psk && psk_len && psk_is_hex) {
		/* Need to skip the nul terminator from string */
		size_t len = hex2bin(psk, psk_len - 1, buf, sizeof(buf));

		if (len <= 0) {
			return -EINVAL;
		}

		psk_len = len;
		psk = buf;
	}

	return lwm2m_security_set(sec_obj_inst, SEC_MODE_PSK, psk_id, strlen(psk_id) + 1, NULL, 0,
			      psk, psk_len);
}

int lwm2m_security_set_certificate(uint16_t sec_obj_inst, const void *cert, int cert_len,
				   const void *private_key, int key_len, const void *ca_chain,
				   int ca_len)
{
	if (!IS_ENABLED(CONFIG_LWM2M_DTLS_SUPPORT)) {
		return -ENOTSUP;
	}
	return lwm2m_security_set(sec_obj_inst, SEC_MODE_CERTIFICATE, cert, cert_len, ca_chain,
				  ca_len, private_key, key_len);
}

static int set_socketoptions(struct lwm2m_ctx *ctx)
{
	int ret;

	if (IS_ENABLED(CONFIG_LWM2M_CLIENT_UTILS_DTLS_CID)) {
		/* Enable CID */
		uint32_t dtls_cid = NRF_SO_SEC_DTLS_CID_ENABLED;

		ret = zsock_setsockopt(ctx->sock_fd, SOL_TLS, TLS_DTLS_CID, &dtls_cid,
				       sizeof(dtls_cid));
		if (ret) {
			ret = -errno;
			LOG_ERR("Failed to enable TLS_DTLS_CID: %d", ret);
		}
	}
	return lwm2m_set_default_sockopt(ctx);
}

int lwm2m_init_security(struct lwm2m_ctx *ctx, char *endpoint, struct modem_mode_change *mmode)
{
#if defined(CONFIG_LWM2M_DTLS_SUPPORT)

	have_permanently_stored_keys = false;
	bootstrap_settings_loaded_inst = -1;
	loading_in_progress = false;

	/* Restore the default if not a callback function */
	if (!mmode) {
		mm.cb = lwm2m_modem_mode_cb;
		mm.user_data = NULL;
	} else {
		mm.cb = mmode->cb;
		mm.user_data = mmode->user_data;
	}

	ctx->tls_tag = IS_ENABLED(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP) ?
			       CONFIG_LWM2M_CLIENT_UTILS_BOOTSTRAP_TLS_TAG :
				     CONFIG_LWM2M_CLIENT_UTILS_SERVER_TLS_TAG;
	ctx->load_credentials = load_credentials_to_modem;
	ctx->set_socketoptions = set_socketoptions;


#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)
	/* If bootstrap is enabled, we should delete the default server instance,
	 * because it is the bootstrap server that creates it for us.
	 * Then on a next boot, we load it from flash.
	 */
	lwm2m_delete_object_inst(&LWM2M_OBJ(1, 0));

	/* Bootsrap server might delete or create our security&server object,
	 * so I need to be aware of those.
	 */
	lwm2m_register_create_callback(LWM2M_OBJECT_SECURITY_ID, security_created);
	lwm2m_register_delete_callback(LWM2M_OBJECT_SECURITY_ID, security_deleted);
	lwm2m_register_create_callback(LWM2M_OBJECT_SERVER_ID, server_created);
	lwm2m_register_delete_callback(LWM2M_OBJECT_SERVER_ID, server_deleted);

	int ret = settings_subsys_init();

	if (ret) {
		LOG_ERR("Failed to initialize settings subsystem, %d", ret);
		return ret;
	}

	ret = settings_register(&lwm2m_security_settings);
	if (ret) {
		LOG_ERR("Failed to register settings, %d", ret);
		return ret;
	}

	ret = settings_load_subtree(SETTINGS_PREFIX);
	if (ret) {
		LOG_ERR("Failed to load settings, %d", ret);
		return ret;
	}

	if (have_permanently_stored_keys && bootstrap_settings_loaded_inst > 0) {
		/* I don't need the default security instance 0 anymore */
		loading_in_progress = true;
		lwm2m_delete_object_inst(&LWM2M_OBJ(0, 0));
		loading_in_progress = false;
		return 0;
	}

#endif /* CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP */
#endif /* CONFIG_LWM2M_DTLS_SUPPORT */

	return init_default_security_obj(ctx, endpoint);
}
