/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/coap.h>
#include <zephyr/net/coap_client.h>
#if defined(CONFIG_POSIX_API)
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/netdb.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/poll.h>
#include <zephyr/posix/sys/socket.h>
#else
#include <zephyr/net/socket.h>
#endif
#include <modem/lte_lc.h>
#include <zephyr/random/random.h>
#if defined(CONFIG_NRF_MODEM_LIB)
#include <nrf_socket.h>
#include <nrf_modem_at.h>
#endif
#include <date_time.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_codec.h>
#include <net/nrf_cloud_coap.h>
#include <cJSON.h>
#include <version.h>
#include "nrf_cloud_codec_internal.h"
#include "nrfc_dtls.h"
#include "coap_codec.h"
#include "nrf_cloud_coap_transport.h"
#include "nrf_cloud_mem.h"
#include "nrf_cloud_credentials.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nrf_cloud_coap_transport, CONFIG_NRF_CLOUD_COAP_LOG_LEVEL);

/** @TODO: figure out whether to make this a Kconfig value or place in a header */
#define CDDL_VERSION "1"
#define MAX_COAP_PATH 256
#define MAX_RETRIES 10
#define JWT_BUF_SZ 700
#define VER_STRING_FMT "mver=%s&cver=%s&dver=%s"
#define VER_STRING_FMT2 "cver=" CDDL_VERSION "&dver=" BUILD_VERSION_STR
#define BUILD_VERSION_STR STRINGIFY(BUILD_VERSION)
#define NON_RESP_WAIT_S 3
#define MAX_XFERS (CONFIG_COAP_CLIENT_MAX_INSTANCES * CONFIG_COAP_CLIENT_MAX_REQUESTS)

#define NRF_CLOUD_COAP_AUTH_RSC "auth/jwt"

/* CoAP client transfer data */
struct cc_xfer_data {
	struct nrf_cloud_coap_client *nrfc_cc;
	coap_client_response_cb_t cb;
	void *user_data;
	int result_code;
	struct k_sem *sem;
	atomic_t used;
};

/* Semaphore to be used with internal coap_client requests */
static K_SEM_DEFINE(cb_sem, 0, 1);
/* Semaphore to be used when doing authorization with an external coap_client */
static K_SEM_DEFINE(ext_cc_sem, 0, 1);

static struct nrf_cloud_coap_client internal_cc = {0};

#if defined(CONFIG_NRF_CLOUD_COAP_LOG_LEVEL_DBG)
static const char *const coap_method_str[] = {
	NULL,		/* 0 */
	"GET",		/* COAP_METHOD_GET = 1 */
	"POST",		/* COAP_METHOD_POST = 2 */
	"PUT",		/* COAP_METHOD_PUT = 3 */
	"DELETE",	/* COAP_METHOD_DELETE = 4 */
	"FETCH",	/* COAP_METHOD_FETCH = 5 */
	"PATCH",	/* COAP_METHOD_PATCH = 6 */
	"IPATCH",	/* COAP_METHOD_IPATCH = 7 */
	NULL
};

#define METHOD_NAME(method) \
	(((method >= COAP_METHOD_GET) && (method <= COAP_METHOD_IPATCH)) ?\
	 coap_method_str[method] : "N/A")

struct format {
	enum coap_content_format fmt;
	const char *name;
};

const struct format formats[] = {
	{COAP_CONTENT_FORMAT_TEXT_PLAIN, "PLAIN TEXT"},
	{COAP_CONTENT_FORMAT_APP_LINK_FORMAT, "APP LINK"},
	{COAP_CONTENT_FORMAT_APP_XML, "XML"},
	{COAP_CONTENT_FORMAT_APP_OCTET_STREAM, "OCTET STRM"},
	{COAP_CONTENT_FORMAT_APP_EXI, "EXI"},
	{COAP_CONTENT_FORMAT_APP_JSON, "JSON"},
	{COAP_CONTENT_FORMAT_APP_JSON_PATCH_JSON, "JSON PATCH"},
	{COAP_CONTENT_FORMAT_APP_MERGE_PATCH_JSON, "JSON MRG PATCH"},
	{COAP_CONTENT_FORMAT_APP_CBOR, "CBOR"}
};

static const char *fmt_name(enum coap_content_format fmt)
{
	for (int i = 0; i < ARRAY_SIZE(formats); i++) {
		if (formats[i].fmt == fmt) {
			return formats[i].name;
		}
	}
	return "N/A";
}
#endif /* CONFIG_NRF_CLOUD_COAP_LOG_LEVEL_DBG */

/* Create a pool of CoAP transfer structures. Memory passed to coap_client needs to
 * persist after any calls to client_transfer() in case a server response to a NON message
 * returns after timeout in client_transfer(), or in case nrf_cloud_coap_transport_disconnect
 * is called while coap_client is waiting for a packet or timeout from the socket.
 */
static struct cc_xfer_data xfer_ctx_pool[MAX_XFERS];

static struct cc_xfer_data *xfer_ctx_take(void)
{
	for (int i = 0; i < ARRAY_SIZE(xfer_ctx_pool); i++) {
		if (!atomic_test_and_set_bit(&xfer_ctx_pool[i].used, 0)) {
			return &xfer_ctx_pool[i];
		}
	}
	return NULL;
}

static void xfer_ctx_release(struct cc_xfer_data *ctx)
{
	if (ctx) {
		atomic_clear_bit(&ctx->used, 0);
	}
}

static struct cc_xfer_data *xfer_data_init(struct nrf_cloud_coap_client *cc,
					   coap_client_response_cb_t cb,
					   void *user,
					   struct k_sem *sem)
{
	struct cc_xfer_data *xfer = xfer_ctx_take();

	if (!xfer) {
		LOG_ERR("Maximum number of CoAP transfers are already in progress");
		return NULL;
	}
	xfer->nrfc_cc = cc;
	xfer->cb = cb;
	xfer->user_data = user;
	xfer->result_code = -ECANCELED;
	xfer->sem = sem;
	return xfer;
}

bool nrf_cloud_coap_is_connected(void)
{
	return internal_cc.authenticated && !internal_cc.paused;
}

bool nrf_cloud_coap_keepopen_is_supported(void)
{
	return nrfc_keepopen_is_supported();
}

static inline bool is_internal(struct nrf_cloud_coap_client const *const client)
{
	return client == &internal_cc;
}

static int add_creds(void)
{
	int err = 0;

#if defined(CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES)
	err = nrf_cloud_credentials_provision();
#endif
	return err;
}

/**@brief Initialize the CoAP client */
int nrf_cloud_coap_init(void)
{
	int err;

	internal_cc.authenticated = false;

	if (!internal_cc.initialized) {
		err = add_creds();
		if (err) {
			return err;
		}

		(void)nrf_cloud_codec_init(NULL);

#if defined(CONFIG_MODEM_INFO)
		err = modem_info_init();
		if (err) {
			return err;
		}
#endif
		err = nrf_cloud_coap_transport_init(&internal_cc);
		if (err) {
			return err;
		}
	}

	return 0;
}

static int update_configured_info_sections(const char * const app_ver)
{
	static bool updated;

	if (updated) {
		return -EALREADY;
	}

	/* Create a JSON object to contain shadow info sections */
	NRF_CLOUD_OBJ_JSON_DEFINE(info_obj);
	int err = nrf_cloud_obj_init(&info_obj);

	if (err) {
		LOG_ERR("Failed to initialize object: %d", err);
		return err;
	}

	/* Encode the enabled info sections */
	err = nrf_cloud_enabled_info_sections_json_encode(info_obj.json, app_ver);
	if (err) {
		(void)nrf_cloud_obj_free(&info_obj);
		if (err == -ENODEV) {
			/* No info sections are enabled */
			err = 0;
		} else {
			LOG_ERR("Error encoding info sections: %d", err);
		}
		return err;
	}

	/* Encode the object for the cloud */
	err = nrf_cloud_obj_cloud_encode(&info_obj);
	/* Free the JSON object; the encoded data remains */
	(void)nrf_cloud_obj_free(&info_obj);

	if (err) {
		LOG_ERR("Error encoding data for the cloud: %d", err);
		return err;
	}

	/* Send the shadow update */
	err = nrf_cloud_coap_shadow_state_update((const char *)info_obj.encoded_data.ptr);
	/* Free the encoded data */
	nrf_cloud_obj_cloud_encoded_free(&info_obj);

	if (err) {
		LOG_ERR("Failed to update info sections in shadow, error: %d", err);
		return -EIO;
	}

	updated = true;
	return 0;
}

static void update_control_section(void)
{
	static bool updated;

	if (updated) {
		return;
	}

	struct nrf_cloud_ctrl_data device_ctrl = {0};
	struct nrf_cloud_data data_out = {0};
	int err;

	/* Get the device control settings, encode it, and send to shadow */
	nrf_cloud_device_control_get(&device_ctrl);

	err = nrf_cloud_shadow_control_response_encode(&device_ctrl, true, &data_out);
	if (err) {
		LOG_ERR("Failed to encode control section, error: %d", err);
		return;
	}

	/* If there is a difference with the desired settings in the shadow a delta
	 * event will be sent to the device
	 */
	err = nrf_cloud_coap_shadow_state_update(data_out.ptr);
	if (err) {
		LOG_ERR("Failed to update control section in device shadow, error: %d", err);
	} else {
		updated = true;
	}

	nrf_cloud_free((void *)data_out.ptr);
}

int nrf_cloud_coap_connect(const char * const app_ver)
{
	if (!internal_cc.initialized) {
		LOG_ERR("nRF Cloud CoAP library has not been initialized");
		return -EACCES;
	}

	int err;

	err = nrf_cloud_coap_transport_connect(&internal_cc);
	if (err < 0) {
		return err;
	}

	err = nrf_cloud_coap_transport_authenticate(&internal_cc);
	if (err) {
		return err;
	}

	/* On initial connect, set the control section in the shadow */
	update_control_section();

	/* On initial connect, update the configured info sections in the shadow */
	err = update_configured_info_sections(app_ver);
	if (err != -EIO) {
		return 0;
	}

	nrf_cloud_coap_transport_disconnect(&internal_cc);
	return err;
}

int nrf_cloud_coap_pause(void)
{
	return nrf_cloud_coap_transport_pause(&internal_cc);
}

int nrf_cloud_coap_resume(void)
{
	return nrf_cloud_coap_transport_resume(&internal_cc);
}

static void client_callback(int16_t result_code, size_t offset, const uint8_t *payload, size_t len,
			    bool last_block, void *user_data)
{
	__ASSERT_NO_MSG(user_data != NULL);

	struct cc_xfer_data *xfer = (struct cc_xfer_data *)user_data;

	if (result_code >= 0) {
		LOG_CB_DBG(result_code, offset, len, last_block);
	} else {
		LOG_DBG("Error from CoAP client:%d, offset:0x%X, len:0x%X, last_block:%d",
			result_code, offset, len, last_block);
	}
	if (payload && len) {
		LOG_HEXDUMP_DBG(payload, MIN(len, 96), "payload received");
	}
	if (result_code == COAP_RESPONSE_CODE_UNAUTHORIZED) {
		LOG_ERR("Device not authenticated; reconnection required.");
		xfer->nrfc_cc->authenticated = false;
	} else if ((result_code >= COAP_RESPONSE_CODE_BAD_REQUEST) && len) {
		LOG_ERR("Unexpected response: %*s", len, payload);
	}
	/* Sanitize the xfer struct to ensure callback is valid, in case transfer
	 * was cancelled or timed out.
	 */
	if (atomic_test_bit(&xfer->used, 0)) {
		xfer->result_code = result_code;
		if (xfer->cb) {
			LOG_DBG("Calling user's callback %p", xfer->cb);
			xfer->cb(result_code, offset, payload, len, last_block,
				 xfer->user_data);
		}
	}
	if (last_block || (result_code >= COAP_RESPONSE_CODE_BAD_REQUEST)) {
		LOG_DBG("End of client transfer");
		if (xfer->sem) {
			k_sem_give(xfer->sem);
		}
	}
}

static K_SEM_DEFINE(serial_sem, 1, 1);

static int client_transfer(enum coap_method method,
			   const char *resource, const char *query,
			   const uint8_t *buf, size_t buf_len,
			   enum coap_content_format fmt_out,
			   enum coap_content_format fmt_in,
			   bool response_expected,
			   bool reliable,
			   struct cc_xfer_data *xfer)
{
	if (xfer == NULL) {
		return -ENOBUFS;
	}
	__ASSERT_NO_MSG(resource != NULL);

	/* Use the serial semaphore if this is the internal coap client */
	if (is_internal(xfer->nrfc_cc)) {
		k_sem_take(&serial_sem, K_FOREVER);
	}

	int err;
	int retry;
	char path[MAX_COAP_PATH + 1];
	struct coap_client_option options[1] = {{
		.code = COAP_OPTION_ACCEPT,
		.len = 1,
		.value[0] = fmt_in
	}};
	struct coap_client_request request = {
		.method = method,
		.confirmable = reliable,
		.path = path,
		.fmt = fmt_out,
		.payload = (uint8_t *)buf,
		.len = buf_len,
		.cb = client_callback,
		.user_data = xfer
	};
	struct coap_client *const cc = &xfer->nrfc_cc->cc;

	if (response_expected) {
		request.options = options;
		request.num_options = ARRAY_SIZE(options);
	} else {
		request.options = NULL;
		request.num_options = 0;
	}

	if (!query) {
		strncpy(path, resource, MAX_COAP_PATH);
		path[MAX_COAP_PATH] = '\0';
	} else {
		err = snprintf(path, MAX_COAP_PATH, "%s?%s", resource, query);
		if ((err < 0) || (err >= MAX_COAP_PATH)) {
			LOG_ERR("Could not format string");
			err = -ETXTBSY;
			goto transfer_end;
		}
	}

#if defined(CONFIG_NRF_CLOUD_COAP_LOG_LEVEL_DBG)
	LOG_DBG("%s %s %s Content-Format:%s, %zd bytes out, Accept:%s", reliable ? "CON" : "NON",
		METHOD_NAME(method), path, fmt_name(fmt_out), buf_len,
		response_expected ? fmt_name(fmt_in) : "none");
#endif /* CONFIG_NRF_CLOUD_COAP_LOG_LEVEL_DBG */

	retry = 0;
	k_sem_reset(xfer->sem);
	while ((err = coap_client_req(cc, xfer->nrfc_cc->sock, NULL, &request, NULL)) == -EAGAIN) {
		if (!nrf_cloud_coap_is_connected()) {
			err = -EACCES;
			break;
		}
		/* -EAGAIN means the CoAP client is currently waiting for a response
		 * to a previous request (likely started in a separate thread).
		 */
		if (retry++ > MAX_RETRIES) {
			LOG_ERR("Timeout waiting for CoAP client to be available");
			err = -ETIMEDOUT;
			goto transfer_end;
		}
		LOG_DBG("CoAP client busy");
		k_sleep(K_MSEC(500));
	}

	if (err < 0) {
		LOG_ERR("Error sending CoAP request: %d", err);
	} else {
		if (buf_len) {
			LOG_HEXDUMP_DBG(buf, MIN(64, buf_len), "Sent");
		}
		/* Wait for coap_client to exhaust retries when reliable transfer selected,
		 * otherwise wait a finite time because response might never come.
		 */
		err = k_sem_take(xfer->sem, reliable ? K_FOREVER : K_SECONDS(NON_RESP_WAIT_S));
		if (!err) {
			LOG_DBG("Got callback");
		} else {
			LOG_DBG("Got timeout: %d", err);
			/* Ignore, since caller selected non-reliable transfer. */
			err = 0;
		}
	}

	if (!reliable && !err && (xfer->result_code >= COAP_RESPONSE_CODE_BAD_REQUEST)) {
		/* NON transfers usually do not use a callback,
		 * so make sure a bad result is not ignored.
		 */
		err = xfer->result_code;
	}

transfer_end:
	if (is_internal(xfer->nrfc_cc)) {
		k_sem_give(&serial_sem);
	}
	xfer_ctx_release(xfer);
	return err;
}

int nrf_cloud_coap_get(const char *resource, const char *query,
		       const uint8_t *buf, size_t len,
		       enum coap_content_format fmt_out,
		       enum coap_content_format fmt_in, bool reliable,
		       coap_client_response_cb_t cb, void *user)
{
	void *xfer = xfer_data_init(&internal_cc, cb, user, &cb_sem);

	return client_transfer(COAP_METHOD_GET, resource, query,
			       buf, len, fmt_out, fmt_in, true, reliable, xfer);
}

int nrf_cloud_coap_post(const char *resource, const char *query,
			const uint8_t *buf, size_t len,
			enum coap_content_format fmt, bool reliable,
			coap_client_response_cb_t cb, void *user)
{
	void *xfer = xfer_data_init(&internal_cc, cb, user, &cb_sem);

	return client_transfer(COAP_METHOD_POST, resource, query,
			       buf, len, fmt, fmt, false, reliable, xfer);
}

int nrf_cloud_coap_put(const char *resource, const char *query,
		       const uint8_t *buf, size_t len,
		       enum coap_content_format fmt, bool reliable,
		       coap_client_response_cb_t cb, void *user)
{
	void *xfer = xfer_data_init(&internal_cc, cb, user, &cb_sem);

	return client_transfer(COAP_METHOD_PUT, resource, query,
			       buf, len, fmt, fmt, false, reliable, xfer);
}

int nrf_cloud_coap_delete(const char *resource, const char *query,
			  const uint8_t *buf, size_t len,
			  enum coap_content_format fmt, bool reliable,
			  coap_client_response_cb_t cb, void *user)
{
	void *xfer = xfer_data_init(&internal_cc, cb, user, &cb_sem);

	return client_transfer(COAP_METHOD_DELETE, resource, query,
			       buf, len, fmt, fmt, false, reliable, xfer);
}

int nrf_cloud_coap_fetch(const char *resource, const char *query,
			 const uint8_t *buf, size_t len,
			 enum coap_content_format fmt_out,
			 enum coap_content_format fmt_in, bool reliable,
			 coap_client_response_cb_t cb, void *user)
{
	void *xfer = xfer_data_init(&internal_cc, cb, user, &cb_sem);

	return client_transfer(COAP_METHOD_FETCH, resource, query,
			       buf, len, fmt_out, fmt_in, true, reliable, xfer);
}

int nrf_cloud_coap_patch(const char *resource, const char *query,
			 const uint8_t *buf, size_t len,
			 enum coap_content_format fmt, bool reliable,
			 coap_client_response_cb_t cb, void *user)
{
	void *xfer = xfer_data_init(&internal_cc, cb, user, &cb_sem);

	return client_transfer(COAP_METHOD_PATCH, resource, query,
			       buf, len, fmt, fmt, false, reliable, xfer);
}

static void auth_cb(int16_t result_code, size_t offset, const uint8_t *payload, size_t len,
		    bool last_block, void *user_data)
{
	struct nrf_cloud_coap_client *client = (struct nrf_cloud_coap_client *)user_data;

	LOG_RESULT_CODE_INF("Authorization result_code:", result_code);
	if (result_code == COAP_RESPONSE_CODE_CREATED) {
		client->authenticated = true;
	}
}

static int nrf_cloud_coap_auth_post(struct nrf_cloud_coap_client *const client,
			     char const *const ver_string,
			     const uint8_t *jwt, size_t jwt_len)
{
	/* Use the nrf_cloud_coap_client as the user data so the auth flag can be set */
	void *xfer = xfer_data_init(client, auth_cb, client,
				    is_internal(client) ? &cb_sem : &ext_cc_sem);

	return client_transfer(COAP_METHOD_POST, NRF_CLOUD_COAP_AUTH_RSC,
			       ver_string, jwt, jwt_len,
			       COAP_CONTENT_FORMAT_TEXT_PLAIN, COAP_CONTENT_FORMAT_TEXT_PLAIN,
			       false, true, xfer);
}

int nrf_cloud_coap_disconnect(void)
{
	return nrf_cloud_coap_transport_disconnect(&internal_cc);
}

int nrf_cloud_coap_transport_init(struct nrf_cloud_coap_client *const client)
{
	if (client->initialized) {
		return 0;
	}

	/* Only initialize one time; not idempotent. */
	LOG_DBG("Initializing %s async CoAP client",
		is_internal(client) ? "internal" : "external");

	k_mutex_init(&client->mutex);

	k_mutex_lock(&client->mutex, K_FOREVER);
	client->cid_saved = false;
	client->authenticated = false;
	client->paused = false;
	client->sock =  -1;

	client->cc.fd = client->sock;

	int err = coap_client_init(&client->cc, NULL);

	if (err) {
		LOG_ERR("Failed to initialize CoAP client: %d", err);
	} else {
		client->initialized = true;
	}
	k_mutex_unlock(&client->mutex);

	return err;
}

int nrf_cloud_coap_transport_connect(struct nrf_cloud_coap_client *const client)
{
	int err;
	int tmp;

	if (client->sock != -1) {
		LOG_DBG("Socket already open: sock = %d", client->sock);
		/* Connection was paused, so resume it. */
		err = nrf_cloud_coap_transport_resume(client);
		if (!err) {
			return 0;
		}
		/* Could not resume. Try with a full handshake. */
		tmp = client->sock;
		client->sock = client->cc.fd = -1;
		close(tmp);
	}

	const char *const host_name = CONFIG_NRF_CLOUD_COAP_SERVER_HOSTNAME;
	uint16_t port = htons(CONFIG_NRF_CLOUD_COAP_SERVER_PORT);
	struct addrinfo *info;
	struct addrinfo hints = {
		.ai_family = AF_UNSPEC, /* Both IPv4 and IPv6 addresses accepted. */
		.ai_socktype = SOCK_DGRAM
	};

	err = getaddrinfo(host_name, NULL, &hints, &info);
	if (err) {
		LOG_ERR("ERROR: getaddrinfo for %s failed: %d, errno: %d", host_name, err, errno);
		return -EADDRNOTAVAIL;
	}

	/* Try to connect with whatever IP addresses we get.
	 * Not all carriers support both IPv4 and IPv6, though DNS lookups sometimes return both.
	 */
	for (; info != NULL; info = info->ai_next) {
		char ip[INET6_ADDRSTRLEN] = { 0 };
		struct sockaddr *const sa = info->ai_addr;

		switch (sa->sa_family) {
		case AF_INET6:
			((struct sockaddr_in6 *)sa)->sin6_port = port;
			break;
		case AF_INET:
			((struct sockaddr_in *)sa)->sin_port = port;
			break;
		}

		inet_ntop(sa->sa_family, (void *)&((struct sockaddr_in *)sa)->sin_addr,
			  ip, sizeof(ip));

		LOG_DBG("Server %s IP address: %s, port: %u",
			host_name, ip, CONFIG_NRF_CLOUD_COAP_SERVER_PORT);

		if (client->sock != -1) {
			close(client->sock);
		}

		client->authenticated = false;

		LOG_DBG("Creating socket type IPPROTO_DTLS_1_2");
		client->sock = socket(sa->sa_family, SOCK_DGRAM, IPPROTO_DTLS_1_2);

		if (client->sock < 0) {
			LOG_INF("Failed to create CoAP socket, errno: %d", errno);
			err = -ENOTSOCK;
			continue;
		}

		LOG_DBG("sock = %d", client->sock);

		err = nrfc_dtls_setup(client->sock);
		if (err < 0) {
			LOG_INF("Failed to initialize the DTLS client: %d", err);
			err = -EPROTO;
			continue;
		}

		err = connect(client->sock, sa, info->ai_addrlen);
		if (err == 0) {
			break;
		}

		LOG_INF("Connect failed, errno: %d", errno);
		err = -ECONNREFUSED;
	}

	if (err) {
		nrf_cloud_coap_transport_disconnect(client);
	}

	return err;
}

int nrf_cloud_coap_transport_disconnect(struct nrf_cloud_coap_client *const client)
{
	if (!client) {
		return -EINVAL;
	}

	if (client->sock < 0) {
		return -ENOTCONN;
	}

	coap_client_cancel_requests(&client->cc);
	LOG_DBG("Cancelled requests");

	int tmp;
	int err;

	k_mutex_lock(&client->mutex, K_FOREVER);
	client->cid_saved = false;
	client->authenticated = false;
	client->paused = false;
	tmp = client->sock;
	client->sock = client->cc.fd = -1;
	err = close(tmp);
	k_mutex_unlock(&client->mutex);

	return err;
}

int nrf_cloud_coap_transport_authenticate(struct nrf_cloud_coap_client *const client)
{
	if (!client) {
		return -EINVAL;
	} else if (client->sock < 0) {
		return -ENOTCONN;
	}

	if (client->authenticated) {
		LOG_DBG("Already authenticated");
		return 0;
	}

	int err;
	char *jwt;

#if defined(CONFIG_MODEM_INFO)
	char mfw_string[MODEM_INFO_FWVER_SIZE];
	char ver_string[strlen(VER_STRING_FMT) +
			MODEM_INFO_FWVER_SIZE +
			strlen(BUILD_VERSION_STR) +
			strlen(CDDL_VERSION)];

	err = modem_info_get_fw_version(mfw_string, sizeof(mfw_string));
	if (!err) {
		err = snprintf(ver_string, sizeof(ver_string), VER_STRING_FMT,
			       mfw_string, BUILD_VERSION_STR, CDDL_VERSION);
		if ((err < 0) || (err >= sizeof(ver_string))) {
			LOG_ERR("Could not format string");
			err = -ETXTBSY;
			goto fail;
		}
	} else {
		LOG_ERR("Unable to obtain the modem firmware version: %d", err);
	}
#else
	char *ver_string = VER_STRING_FMT2;
#endif

	LOG_DBG("Generate JWT");
	jwt = nrf_cloud_malloc(JWT_BUF_SZ);
	if (!jwt) {
		err = -ENOMEM;
		goto fail;
	}
	err = nrf_cloud_jwt_generate(NRF_CLOUD_JWT_VALID_TIME_S_MAX, jwt, JWT_BUF_SZ);
	if (err) {
		LOG_ERR("Error generating JWT with modem: %d", err);
		nrf_cloud_free(jwt);
		goto fail;
	}

	LOG_INF("Request authorization with JWT");
	err = nrf_cloud_coap_auth_post(client, ver_string, (uint8_t *)jwt, strlen(jwt));

	nrf_cloud_free(jwt);

	if (!client->authenticated) {
		err = -EACCES;
	}

	if (err) {
		goto fail;
	}

	LOG_INF("Authorized");

	if (nrfc_dtls_cid_is_active(client->sock)) {
		LOG_INF("DTLS CID is active");
	}

	return 0;

fail:
	nrf_cloud_coap_transport_disconnect(client);
	return err;
}

int nrf_cloud_coap_transport_pause(struct nrf_cloud_coap_client *const client)
{
	int err = 0;
	int tmp;

	if (!client) {
		return -EINVAL;
	}
	if (client->sock == -1) {
		return -EBADF; /* Device is disconnected. */
	}

	if (nrfc_dtls_cid_is_active(client->sock) && client->authenticated) {
		LOG_DBG("Cancelling requests");
		coap_client_cancel_requests(&client->cc);

		k_mutex_lock(&client->mutex, K_FOREVER);
		client->cid_saved = false;
		client->paused = true;

		LOG_DBG("CID active and authenticated; pausing...");
		err = nrfc_dtls_session_save(client->sock);
		if (!err) {
			LOG_DBG("DTLS CID session saved.");
			client->cid_saved = true;
		} else {
			LOG_ERR("Unable to save DTLS CID session: %d", err);
			client->authenticated = false;
			client->paused = false;
			tmp = client->sock;
			client->sock = client->cc.fd = -1;
			close(tmp);
			LOG_DBG("Closed socket and marked as unauthenticated.");
		}
	} else {
		k_mutex_lock(&client->mutex, K_FOREVER);
		client->cid_saved = false;
		client->paused = false;
		LOG_WRN("Unable to pause CoAP connection.");
		err = -EACCES;
	}
	k_mutex_unlock(&client->mutex);
	return err;
}

int nrf_cloud_coap_transport_resume(struct nrf_cloud_coap_client *const client)
{
	if (!client) {
		return -EINVAL;
	}

	int err = 0;

	k_mutex_lock(&client->mutex, K_FOREVER);
	if (client->cid_saved) {
		err = nrfc_dtls_session_load(client->sock);
		client->cid_saved = false;
		client->paused = false;
		if (!err) {
			LOG_DBG("Loaded DTLS CID session");
			client->authenticated = true;
			k_mutex_unlock(&client->mutex);
			return 0;
		}
		/* We cannot reuse the CID so we just re-authenticate. */
		client->authenticated = false;
		if (err == -EAGAIN) {
			LOG_INF("Failed to load DTLS CID session");
		} else if (err == -EINVAL) {
			LOG_INF("DLTS CID sessions not supported with current modem firmware");
		} else {
			LOG_ERR("Error on DTLS CID session load: %d", err);
		}
	} else if (!client->paused) {
		LOG_WRN("Unable to resume CoAP connection.");
		err = -EACCES; /* Cannot resume. */
	}
	k_mutex_unlock(&client->mutex);

	return err;
}

#define PROXY_URI_DL_HTTPS		"https://"
#define PROXY_URI_DL_HTTPS_LEN		(sizeof(PROXY_URI_DL_HTTPS) - 1)
#define PROXY_URI_DL_SEP		"/"
#define PROXY_URI_DL_SEP_LEN		(sizeof(PROXY_URI_DL_SEP) - 1)
#define PROXY_URI_ADDED_LEN		(PROXY_URI_DL_HTTPS_LEN + PROXY_URI_DL_SEP_LEN)

int nrf_cloud_coap_transport_proxy_dl_opts_get(struct coap_client_option *const opt_accept,
					       struct coap_client_option *const opt_proxy_uri,
					       char const *const host, char const *const path)
{
	__ASSERT_NO_MSG(opt_accept != NULL);
	__ASSERT_NO_MSG(opt_proxy_uri != NULL);
	__ASSERT_NO_MSG(host != NULL);
	__ASSERT_NO_MSG(path != NULL);

	size_t uri_idx = 0;
	size_t host_len = strlen(host);
	size_t path_len = strlen(path);

	opt_accept->code = COAP_OPTION_ACCEPT;
	opt_accept->len = 1;
	opt_accept->value[0] = COAP_CONTENT_FORMAT_TEXT_PLAIN;

	opt_proxy_uri->code = COAP_OPTION_PROXY_URI;
	opt_proxy_uri->len = host_len + path_len + PROXY_URI_ADDED_LEN;

	if (opt_proxy_uri->len > CONFIG_COAP_EXTENDED_OPTIONS_LEN_VALUE) {
		LOG_ERR("Host and path for CoAP proxy GET is too large: %u bytes",
			opt_proxy_uri->len);
		LOG_INF("Increase CONFIG_COAP_EXTENDED_OPTIONS_LEN_VALUE, current value: %d",
			CONFIG_COAP_EXTENDED_OPTIONS_LEN_VALUE);
		return -E2BIG;
	}

	/* We don't want a NULL terminated string, so just copy the data to create the full URI */
	memcpy(&opt_proxy_uri->value[uri_idx], PROXY_URI_DL_HTTPS, PROXY_URI_DL_HTTPS_LEN);
	uri_idx += PROXY_URI_DL_HTTPS_LEN;

	memcpy(&opt_proxy_uri->value[uri_idx], host, host_len);
	uri_idx += host_len;

	memcpy(&opt_proxy_uri->value[uri_idx], PROXY_URI_DL_SEP, PROXY_URI_DL_SEP_LEN);
	uri_idx += PROXY_URI_DL_SEP_LEN;

	memcpy(&opt_proxy_uri->value[uri_idx], path, path_len);

	LOG_DBG("Proxy URI: %.*s", opt_proxy_uri->len, opt_proxy_uri->value);

	return 0;
}
