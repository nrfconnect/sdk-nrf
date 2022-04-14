/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <net/lwm2m.h>
#include <net/lwm2m_client_utils.h>
#include <lwm2m_engine.h>
#include <modem/modem_key_mgmt.h>
#include <modem/lte_lc.h>
#include <settings/settings.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(lwm2m_security, CONFIG_LWM2M_CLIENT_UTILS_LOG_LEVEL);

/* LWM2M_OBJECT_SECURITY_ID */
#define SECURITY_SERVER_URI_ID 0
#define SECURITY_MODE_ID 2
#define SECURITY_CLIENT_PK_ID 3
#define SECURITY_SERVER_PK_ID 4
#define SECURITY_SECRET_KEY_ID 5
#define SECURITY_SHORT_SERVER_ID 10
/* LWM2M_OBJECT_SERVER_ID */
#define SERVER_SHORT_SERVER_ID 0
#define SERVER_LIFETIME_ID 1

#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
#define SETTINGS_PREFIX "lwm2m:sec"

static bool have_permanently_stored_keys;
static bool loading_in_progress = true;

static int write_to_settings(int obj, int inst, int res, uint8_t *data, uint16_t data_len);

static int write_credential_type(struct lwm2m_ctx *ctx, int res_id,
				 enum modem_key_mgmt_cred_type type)
{
	int ret;
	void *cred = NULL;
	uint16_t cred_len;
	uint8_t cred_flags;
	char pathstr[sizeof("0/0/0")];
	char psk_hex[65];

	snprintk(pathstr, sizeof(pathstr), "0/%d/%d", ctx->sec_obj_inst, res_id);
	ret = lwm2m_engine_get_res_data(pathstr, &cred, &cred_len, &cred_flags);
	if (ret < 0) {
		LOG_ERR("Unable to get resource data for '%s'", log_strdup(pathstr));
		return ret;
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

	ret = modem_key_mgmt_write(ctx->tls_tag, type, cred, cred_len);
	if (ret < 0) {
		LOG_ERR("Unable to write credentials to modem (%d)", ret);
		return ret;
	}
	LOG_DBG("Written sec_tag %d, type %d", ctx->tls_tag, type);
	return 0;
}

static bool sec_obj_has_credentials(struct lwm2m_ctx *ctx)
{
	int ret;
	void *cred = NULL;
	uint16_t cred_len;
	uint8_t cred_flags;
	char pathstr[sizeof("0/0/0")];

	snprintk(pathstr, sizeof(pathstr), "0/%d/%d", ctx->sec_obj_inst, SECURITY_SECRET_KEY_ID);
	ret = lwm2m_engine_get_res_data(pathstr, &cred, &cred_len, &cred_flags);
	if (ret < 0) {
		LOG_ERR("Unable to get resource data for '%s'", log_strdup(pathstr));
		return false;
	}

	return cred_len != 0;
}

static int load_credentials_to_modem(struct lwm2m_ctx *ctx)
{
	int ret;
	bool exist;
	bool has_credentials;
	enum lte_lc_func_mode fmode;

	if (ctx->bootstrap_mode) {
		ctx->tls_tag = CONFIG_LWM2M_CLIENT_UTILS_BOOTSTRAP_TLS_TAG;
	} else {
		ctx->tls_tag = CONFIG_LWM2M_CLIENT_UTILS_SERVER_TLS_TAG;
	}

	ret = modem_key_mgmt_exists(ctx->tls_tag, MODEM_KEY_MGMT_CRED_TYPE_PSK, &exist);
	if (ret < 0) {
		LOG_ERR("modem_key_mgmt_exists() failed %d", ret);
		return ret;
	}

	has_credentials = sec_obj_has_credentials(ctx);

	/* If we have credentials already in modem, and we have loaded settings from the flash
	 * assume that the ones in modem are correct.
	 * Never overwrite bootstrap keys.
	 */
	if (exist &&
	    (have_permanently_stored_keys ||
	     ctx->tls_tag == CONFIG_LWM2M_CLIENT_UTILS_BOOTSTRAP_TLS_TAG || !has_credentials)) {
		LOG_DBG("Existing credentials found on modem");
		return 0;
	}

	if (!exist && !has_credentials) {
		LOG_ERR("No security credentials provisioned");
		return -ENOENT;
	}

	if (lte_lc_func_mode_get(&fmode)) {
		LOG_ERR("Failed to read modem functional mode");
		return -EFAULT;
	}

	if (fmode != LTE_LC_FUNC_MODE_POWER_OFF || fmode != LTE_LC_FUNC_MODE_OFFLINE) {
		LOG_INF("Need to write credentials, switch LTE offline");
		lte_lc_offline();
	}

	ret = write_credential_type(ctx, SECURITY_CLIENT_PK_ID, MODEM_KEY_MGMT_CRED_TYPE_IDENTITY);
	if (ret) {
		goto out;
	}

	ret = write_credential_type(ctx, SECURITY_SECRET_KEY_ID, MODEM_KEY_MGMT_CRED_TYPE_PSK);
	if (ret) {
		goto out;
	}

	/* Mark that we have now written those keys, so
	 * reconnection or rebooting does not cause another rewrite
	 */
	if (!have_permanently_stored_keys) {
		uint8_t mode = 0; /* Hard coded to PSK */

		write_to_settings(LWM2M_OBJECT_SECURITY_ID, ctx->sec_obj_inst, SECURITY_MODE_ID,
				  &mode, sizeof(mode));
		have_permanently_stored_keys = true;
	}

out:
	if (fmode != LTE_LC_FUNC_MODE_POWER_OFF || fmode != LTE_LC_FUNC_MODE_OFFLINE) {
		LOG_INF("Restoring modem connection");
		/* I need to use the blocking call, because in next step
		 * LwM2M engine would create socket and call connect()
		 */
		int err = lte_lc_connect();

		if (err) {
			LOG_ERR("lte_lc_connect() failed %d", err);
		}
	}

	return ret;
}

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

	if (!key) {
		return -ENOENT;
	}

	LOG_DBG("Loading \"%s\"", log_strdup(key));

	int obj = key[0] - '0';
	int inst = key[2] - '0';

	/* Create object instance, if it does not exist */
	if (!object_instance_exist(obj, inst)) {
		int ret;
		char o_path[] = "0/0";

		/* I only need object number (1 char), and resource (1 char) */
		o_path[0] = key[0];
		o_path[2] = key[2];
		ret = lwm2m_engine_create_obj_inst(o_path);
		if (ret) {
			LOG_ERR("Failed to create object instance %s", log_strdup(o_path));
			return ret;
		}
	}

	if (lwm2m_engine_get_res_data((char *)key, &buf, &len, &flags) != 0) {
		LOG_ERR("Failed to get data pointer");
		return -ENOENT;
	}

	len = read_cb(cb_arg, buf, len);
	if (len <= 0) {
		LOG_ERR("Failed to read data");
		return -ENOENT;
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
		LOG_ERR("Failed to store %s", log_strdup(path));
	}
	LOG_DBG("Permanently stored %s", log_strdup(path));
	return 0;
}

#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)

static void delete_from_storage(int obj, int inst, int res)
{
	char path[sizeof(SETTINGS_PREFIX "/0/0/0")];

	snprintk(path, sizeof(path), SETTINGS_PREFIX "/%d/%d/%d", obj, inst, res);
	settings_delete(path);
	LOG_DBG("Deleted %s", log_strdup(path));
}



static int write_cb_sec(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id, uint8_t *data,
			uint16_t data_len, bool last_block, size_t total_size)
{
	return write_to_settings(LWM2M_OBJECT_SECURITY_ID, obj_inst_id, res_id, data, data_len);
}
static int write_cb_srv(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id, uint8_t *data,
			uint16_t data_len, bool last_block, size_t total_size)
{
	return write_to_settings(LWM2M_OBJECT_SERVER_ID, obj_inst_id, res_id, data, data_len);
}

static void register_write_cb(int obj, int inst, int res)
{
	char path[sizeof("/0/0/10")];
	lwm2m_engine_set_data_cb_t cb;

	if (obj == LWM2M_OBJECT_SECURITY_ID) {
		cb = write_cb_sec;
	} else {
		cb = write_cb_srv;
	}

	snprintk(path, sizeof(path), "%d/%d/%d", obj, inst, res);
	lwm2m_engine_register_post_write_callback(path, cb);
}

static int server_deleted(uint16_t id)
{
	delete_from_storage(LWM2M_OBJECT_SERVER_ID, id, SERVER_LIFETIME_ID);
	delete_from_storage(LWM2M_OBJECT_SERVER_ID, id, SERVER_SHORT_SERVER_ID);
	return 0;
}

static int server_created(uint16_t id)
{
	if (!loading_in_progress) {
		register_write_cb(LWM2M_OBJECT_SERVER_ID, id, SERVER_LIFETIME_ID);
		register_write_cb(LWM2M_OBJECT_SERVER_ID, id, SERVER_SHORT_SERVER_ID);
	}
	return 0;
}

static int security_deleted(uint16_t id)
{
	delete_from_storage(LWM2M_OBJECT_SECURITY_ID, id, SECURITY_SERVER_URI_ID);
	delete_from_storage(LWM2M_OBJECT_SECURITY_ID, id, SECURITY_MODE_ID);
	delete_from_storage(LWM2M_OBJECT_SECURITY_ID, id, SECURITY_CLIENT_PK_ID);
	delete_from_storage(LWM2M_OBJECT_SECURITY_ID, id, SECURITY_SERVER_PK_ID);
	delete_from_storage(LWM2M_OBJECT_SECURITY_ID, id, SECURITY_SECRET_KEY_ID);
	delete_from_storage(LWM2M_OBJECT_SECURITY_ID, id, SECURITY_SHORT_SERVER_ID);

	have_permanently_stored_keys = false;
	return 0;
}

static int security_created(uint16_t id)
{
	if (!loading_in_progress) {
		register_write_cb(LWM2M_OBJECT_SECURITY_ID, id, SECURITY_SERVER_URI_ID);
		register_write_cb(LWM2M_OBJECT_SECURITY_ID, id, SECURITY_MODE_ID);
		register_write_cb(LWM2M_OBJECT_SECURITY_ID, id, SECURITY_CLIENT_PK_ID);
		register_write_cb(LWM2M_OBJECT_SECURITY_ID, id, SECURITY_SERVER_PK_ID);
		register_write_cb(LWM2M_OBJECT_SECURITY_ID, id, SECURITY_SECRET_KEY_ID);
		register_write_cb(LWM2M_OBJECT_SECURITY_ID, id, SECURITY_SHORT_SERVER_ID);
	}
	return 0;
}

static bool server_keys_exist_in_modem(void)
{
	int ret;
	bool exist;

	ret = modem_key_mgmt_exists(CONFIG_LWM2M_CLIENT_UTILS_SERVER_TLS_TAG,
				    MODEM_KEY_MGMT_CRED_TYPE_PSK, &exist);
	if (ret || !exist) {
		return false;
	}

	ret = modem_key_mgmt_exists(CONFIG_LWM2M_CLIENT_UTILS_SERVER_TLS_TAG,
				    MODEM_KEY_MGMT_CRED_TYPE_IDENTITY, &exist);
	if (ret || !exist) {
		return false;
	}
	return true;
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

int lwm2m_init_security(struct lwm2m_ctx *ctx, char *endpoint)
{
	int ret;
	char *server_url;
	uint16_t server_url_len;
	uint8_t server_url_flags;

	have_permanently_stored_keys = false;

	settings_register(&lwm2m_security_settings);

	/* setup SECURITY object */

	/* Server URL */
	ret = lwm2m_engine_get_res_data(
		LWM2M_PATH(LWM2M_OBJECT_SECURITY_ID, 0, SECURITY_SERVER_URI_ID),
		(void **)&server_url, &server_url_len, &server_url_flags);
	if (ret < 0) {
		return ret;
	}

	snprintk(server_url, server_url_len, "%s", CONFIG_LWM2M_CLIENT_UTILS_SERVER);

	/* Security Mode */
	lwm2m_engine_set_u8(LWM2M_PATH(LWM2M_OBJECT_SECURITY_ID, 0, SECURITY_MODE_ID),
			    IS_ENABLED(CONFIG_LWM2M_DTLS_SUPPORT) ? 0 : 3);
	/* Empty the PSK key as data length of uninitialized opaque resources is not zero */
	lwm2m_engine_set_opaque(LWM2M_PATH(LWM2M_OBJECT_SECURITY_ID, 0, SECURITY_SECRET_KEY_ID),
				NULL, 0);
#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
	ctx->tls_tag = IS_ENABLED(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP) ?
				  CONFIG_LWM2M_CLIENT_UTILS_BOOTSTRAP_TLS_TAG :
				  CONFIG_LWM2M_CLIENT_UTILS_SERVER_TLS_TAG;
	ctx->load_credentials = load_credentials_to_modem;

	lwm2m_engine_set_string(LWM2M_PATH(LWM2M_OBJECT_SECURITY_ID, 0, SECURITY_CLIENT_PK_ID),
				endpoint);
#endif /* CONFIG_LWM2M_DTLS_SUPPORT */

#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)
	/* Mark 1st instance of security object as a bootstrap server */
	lwm2m_engine_set_u8("0/0/1", 1);

#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
	/* Bootsrap server might delete or create our security&server object,
	 * so I need to be aware of those.
	 */
	lwm2m_engine_register_create_callback(LWM2M_OBJECT_SECURITY_ID, security_created);
	lwm2m_engine_register_delete_callback(LWM2M_OBJECT_SECURITY_ID, security_deleted);
	lwm2m_engine_register_create_callback(LWM2M_OBJECT_SERVER_ID, server_created);
	lwm2m_engine_register_delete_callback(LWM2M_OBJECT_SERVER_ID, server_deleted);
#endif
#else
	/* Security and Server object need matching Short Server ID value. */
	lwm2m_engine_set_u16("0/0/10", 101);
	lwm2m_engine_set_u16("1/0/0", 101);
#endif

	return ret;
}
