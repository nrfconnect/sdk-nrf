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
#include "nrf_cloud_dns.h"
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
/* Mutex to be used when using the internal coap_client */
static K_MUTEX_DEFINE(internal_transfer_mut);

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
	int err = 0;

	(void)nrf_cloud_print_details();
	k_mutex_lock(&internal_transfer_mut, K_FOREVER);

	internal_cc.authenticated = false;

	if (!internal_cc.initialized) {
		err = add_creds();
		if (err) {
			goto exit;
		}

		(void)nrf_cloud_codec_init(NULL);

#if defined(CONFIG_MODEM_INFO)
		err = modem_info_init();
		if (err) {
			goto exit;
		}
#endif
		err = nrf_cloud_coap_transport_init(&internal_cc);
		if (err) {
			goto exit;
		}
	}

exit:
	k_mutex_unlock(&internal_transfer_mut);
	return err;
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
	int err = 0;

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
	int err = 0;

	if (!internal_cc.initialized) {
		LOG_ERR("nRF Cloud CoAP library has not been initialized");
		return -EACCES;
	}

	k_mutex_lock(&internal_transfer_mut, K_FOREVER);

	err = nrf_cloud_coap_transport_connect(&internal_cc);
	if (err < 0) {
		goto exit;
	}

	err = nrf_cloud_coap_transport_authenticate(&internal_cc);
	if (err) {
		goto exit;
	}

	/* On initial connect, set the control section in the shadow */
	update_control_section();

	/* On initial connect, update the configured info sections in the shadow */
	err = update_configured_info_sections(app_ver);
	if (err != -EIO) {
		err = 0;
		goto exit;
	}

	nrf_cloud_coap_transport_disconnect(&internal_cc);

exit:
	k_mutex_unlock(&internal_transfer_mut);
	return err;
}

int nrf_cloud_coap_pause(void)
{
	int err = 0;

	k_mutex_lock(&internal_transfer_mut, K_FOREVER);
	err = nrf_cloud_coap_transport_pause(&internal_cc);
	k_mutex_unlock(&internal_transfer_mut);

	return err;
}

int nrf_cloud_coap_resume(void)
{
	int err = 0;

	k_mutex_lock(&internal_transfer_mut, K_FOREVER);
	err = nrf_cloud_coap_transport_resume(&internal_cc);
	k_mutex_unlock(&internal_transfer_mut);

	return err;
}

static void client_callback(const struct coap_client_response_data *data, void *user_data)
{
	__ASSERT_NO_MSG(user_data != NULL);

	struct cc_xfer_data *xfer = (struct cc_xfer_data *)user_data;

	if (data->result_code >= 0) {
		LOG_CB_DBG(data->result_code, data->offset, data->payload_len, data->last_block);
	} else {
		LOG_DBG("Error from CoAP client:%d, offset:0x%X, len:0x%X, last_block:%d",
			data->result_code, data->offset, data->payload_len, data->last_block);
	}
	if (data->payload && data->payload_len) {
		LOG_HEXDUMP_DBG(data->payload, MIN(data->payload_len, 96), "payload received");
	}
	if (data->result_code == COAP_RESPONSE_CODE_UNAUTHORIZED) {
		LOG_ERR("Device not authenticated; reconnection required.");
		xfer->nrfc_cc->authenticated = false;
	} else if ((data->result_code >= COAP_RESPONSE_CODE_BAD_REQUEST) && data->payload_len) {
		LOG_ERR("Unexpected response: %*s", data->payload_len, data->payload);
	}
	/* Sanitize the xfer struct to ensure callback is valid, in case transfer
	 * was cancelled or timed out.
	 */
	if (atomic_test_bit(&xfer->used, 0)) {
		xfer->result_code = data->result_code;
		if (xfer->cb) {
			LOG_DBG("Calling user's callback %p", xfer->cb);
			xfer->cb(data, xfer->user_data);
		}
	}
	if (data->result_code || (data->result_code >= COAP_RESPONSE_CODE_BAD_REQUEST)) {
		LOG_DBG("End of client transfer");
		if (xfer->sem) {
			k_sem_give(xfer->sem);
		}
	}
}


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

	int err = 0;
	int retry;
	struct coap_client_request request = {
		.method = method,
		.confirmable = reliable,
		.fmt = fmt_out,
		.payload = (uint8_t *)buf,
		.len = buf_len,
		.cb = client_callback,
		.user_data = xfer
	};
	struct coap_client *const cc = &xfer->nrfc_cc->cc;

	if (response_expected) {
		request.options[0].code = COAP_OPTION_ACCEPT;
		request.options[0].len = 1;
		request.options[0].value[0] = fmt_in;
		request.num_options = 1;
	} else {
		request.num_options = 0;
	}

	if (!query) {
		strncpy(request.path, resource, MAX_PATH_SIZE);
		request.path[MAX_PATH_SIZE - 1] = '\0';
	} else {
		err = snprintk(request.path, sizeof(request.path), "%s?%s", resource, query);
		if ((err <= 0) || (err >= sizeof(request.path))) {
			LOG_ERR("Could not format string");
			err = -ETXTBSY;
			goto transfer_end;
		}
	}

#if defined(CONFIG_NRF_CLOUD_COAP_LOG_LEVEL_DBG)
	LOG_DBG("%s %s %s Content-Format:%s, %zd bytes out, Accept:%s", reliable ? "CON" : "NON",
		METHOD_NAME(method), request.path, fmt_name(fmt_out), buf_len,
		response_expected ? fmt_name(fmt_in) : "none");
#endif /* CONFIG_NRF_CLOUD_COAP_LOG_LEVEL_DBG */

	retry = 0;
	k_sem_reset(xfer->sem);
	while ((xfer->nrfc_cc->sock >= 0) &&
	       (err = coap_client_req(cc, xfer->nrfc_cc->sock, NULL, &request, NULL)) == -EAGAIN) {
		if (!nrf_cloud_coap_is_connected()) {
			err = -EACCES;
			break;
		}
		/* -EAGAIN means the CoAP client is currently waiting for a response
		 * to a previous request (likely started in a separate thread).
		 */
		if (retry++ > CONFIG_NRF_CLOUD_COAP_MAX_RETRIES) {
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

		if (xfer->nrfc_cc->sock < 0) {
			LOG_ERR("Socket closed during CoAP request");
			err = -ESHUTDOWN;
			goto transfer_end;
		}

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
	xfer_ctx_release(xfer);
	coap_client_cancel_request(cc, &request);
	if (err == -ETIMEDOUT && IS_ENABLED(CONFIG_NRF_CLOUD_COAP_DISCONNECT_ON_FAILED_REQUEST)) {
		nrf_cloud_coap_disconnect();
	}
	return err;
}

int nrf_cloud_coap_get(const char *resource, const char *query,
		       const uint8_t *buf, size_t len,
		       enum coap_content_format fmt_out,
		       enum coap_content_format fmt_in, bool reliable,
		       coap_client_response_cb_t cb, void *user)
{
	int err = 0;

	k_mutex_lock(&internal_transfer_mut, K_FOREVER);
	void *xfer = xfer_data_init(&internal_cc, cb, user, &cb_sem);

	err = client_transfer(COAP_METHOD_GET, resource, query,
			      buf, len, fmt_out, fmt_in, true, reliable, xfer);
	k_mutex_unlock(&internal_transfer_mut);

	return err;
}

int nrf_cloud_coap_post(const char *resource, const char *query,
			const uint8_t *buf, size_t len,
			enum coap_content_format fmt, bool reliable,
			coap_client_response_cb_t cb, void *user)
{
	int err = 0;

	k_mutex_lock(&internal_transfer_mut, K_FOREVER);
	void *xfer = xfer_data_init(&internal_cc, cb, user, &cb_sem);

	err = client_transfer(COAP_METHOD_POST, resource, query,
			      buf, len, fmt, fmt, false, reliable, xfer);
	k_mutex_unlock(&internal_transfer_mut);

	return err;
}

int nrf_cloud_coap_put(const char *resource, const char *query,
		       const uint8_t *buf, size_t len,
		       enum coap_content_format fmt, bool reliable,
		       coap_client_response_cb_t cb, void *user)
{
	int err = 0;

	k_mutex_lock(&internal_transfer_mut, K_FOREVER);
	void *xfer = xfer_data_init(&internal_cc, cb, user, &cb_sem);

	err = client_transfer(COAP_METHOD_PUT, resource, query,
			      buf, len, fmt, fmt, false, reliable, xfer);
	k_mutex_unlock(&internal_transfer_mut);

	return err;
}

int nrf_cloud_coap_delete(const char *resource, const char *query,
			  const uint8_t *buf, size_t len,
			  enum coap_content_format fmt, bool reliable,
			  coap_client_response_cb_t cb, void *user)
{
	int err = 0;

	k_mutex_lock(&internal_transfer_mut, K_FOREVER);
	void *xfer = xfer_data_init(&internal_cc, cb, user, &cb_sem);

	err = client_transfer(COAP_METHOD_DELETE, resource, query,
			      buf, len, fmt, fmt, false, reliable, xfer);
	k_mutex_unlock(&internal_transfer_mut);

	return err;
}

int nrf_cloud_coap_fetch(const char *resource, const char *query,
			 const uint8_t *buf, size_t len,
			 enum coap_content_format fmt_out,
			 enum coap_content_format fmt_in, bool reliable,
			 coap_client_response_cb_t cb, void *user)
{
	int err = 0;

	k_mutex_lock(&internal_transfer_mut, K_FOREVER);
	void *xfer = xfer_data_init(&internal_cc, cb, user, &cb_sem);

	err = client_transfer(COAP_METHOD_FETCH, resource, query,
			      buf, len, fmt_out, fmt_in, true, reliable, xfer);
	k_mutex_unlock(&internal_transfer_mut);

	return err;
}

int nrf_cloud_coap_patch(const char *resource, const char *query,
			 const uint8_t *buf, size_t len,
			 enum coap_content_format fmt, bool reliable,
			 coap_client_response_cb_t cb, void *user)
{
	int err = 0;

	k_mutex_lock(&internal_transfer_mut, K_FOREVER);
	void *xfer = xfer_data_init(&internal_cc, cb, user, &cb_sem);

	err = client_transfer(COAP_METHOD_PATCH, resource, query,
			      buf, len, fmt, fmt, false, reliable, xfer);
	k_mutex_unlock(&internal_transfer_mut);

	return err;
}

static void auth_cb(const struct coap_client_response_data *data, void *user_data)
{
	struct nrf_cloud_coap_client *client = (struct nrf_cloud_coap_client *)user_data;

	LOG_RESULT_CODE_INF("Authorization result_code:", data->result_code);
	if (data->result_code == COAP_RESPONSE_CODE_CREATED) {
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
	int err = 0;

	k_mutex_lock(&internal_transfer_mut, K_FOREVER);
	err = nrf_cloud_coap_transport_disconnect(&internal_cc);
	k_mutex_unlock(&internal_transfer_mut);

	return err;
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

static int nrf_cloud_coap_connect_host_cb(struct sockaddr *const addr)
{
	int err = 0;
	int sock;
	size_t addr_size;

	LOG_DBG("Creating socket type IPPROTO_DTLS_1_2");
	sock = zsock_socket(addr->sa_family, SOCK_DGRAM, IPPROTO_DTLS_1_2);

	if (sock < 0) {
		LOG_DBG("Failed to create CoAP socket, errno: %d", errno);
		err = -ENOTSOCK;
		goto out;
	}

	LOG_DBG("sock = %d", sock);

	err = nrfc_dtls_setup(sock);
	if (err < 0) {
		LOG_DBG("Failed to initialize the DTLS client: %d", err);
		err = -EPROTO;
		goto out;
	}

	addr_size = sizeof(struct sockaddr_in6);
	if (addr->sa_family == AF_INET) {
		addr_size = sizeof(struct sockaddr_in);
	}

	err = zsock_connect(sock, addr, addr_size);
	if (err) {
		LOG_DBG("Connect failed, errno: %d", errno);
		err = -ECONNREFUSED;
		goto out;
	}

out:
	if (err) {
		if (sock >= 0) {
			zsock_close(sock);
		}
		return err;
	}

	return sock;
}

int nrf_cloud_coap_transport_connect(struct nrf_cloud_coap_client *const client)
{
	int err = 0;
	int tmp;
	int sock;

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
		zsock_close(tmp);
	}

	client->authenticated = false;

	const char *const host_name = CONFIG_NRF_CLOUD_COAP_SERVER_HOSTNAME;
	uint16_t port = CONFIG_NRF_CLOUD_COAP_SERVER_PORT;

	struct zsock_addrinfo hints = {
		.ai_socktype = SOCK_DGRAM
	};
	sock = nrf_cloud_connect_host(host_name, port, &hints, &nrf_cloud_coap_connect_host_cb);

	if (sock < 0) {
		LOG_ERR("Could not connect to nRF Cloud CoAP server %s, port: %d. err: %d",
			host_name, port, sock);
		nrf_cloud_coap_transport_disconnect(client);
		return -ECONNREFUSED;
	}

	client->sock = sock;

	return 0;
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
	int err = 0;

	k_mutex_lock(&client->mutex, K_FOREVER);
	client->cid_saved = false;
	client->authenticated = false;
	client->paused = false;
	tmp = client->sock;
	client->sock = client->cc.fd = -1;
	err = zsock_close(tmp);
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

	int err = 0;
	char *jwt;

#if defined(CONFIG_MODEM_INFO)
	char mfw_string[MODEM_INFO_FWVER_SIZE];
	char ver_string[strlen(VER_STRING_FMT) +
			MODEM_INFO_FWVER_SIZE +
			strlen(BUILD_VERSION_STR) +
			strlen(CDDL_VERSION)];

	err = modem_info_get_fw_version(mfw_string, sizeof(mfw_string));
	if (!err) {
		err = snprintk(ver_string, sizeof(ver_string), VER_STRING_FMT,
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
			zsock_close(tmp);
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

int nrf_cloud_coap_transport_proxy_dl_uri_get(char *const uri, size_t uri_len,
					      char const *const host, char const *const path)
{
	__ASSERT_NO_MSG(uri != NULL);
	__ASSERT_NO_MSG(host != NULL);
	__ASSERT_NO_MSG(path != NULL);

	size_t host_len = strlen(host);
	size_t path_len = strlen(path);

	const size_t needed_len =
		PROXY_URI_DL_HTTPS_LEN + host_len + PROXY_URI_DL_SEP_LEN +  path_len;

	if (needed_len > uri_len) {
		LOG_ERR("Host and path for CoAP proxy GET is too large: %u bytes",
			needed_len);
		return -E2BIG;
	}

	/* We don't want a NULL terminated string, so just copy the data to create the full URI */
	memcpy(uri, PROXY_URI_DL_HTTPS, PROXY_URI_DL_HTTPS_LEN);
	memcpy(&uri[PROXY_URI_DL_HTTPS_LEN], host, host_len);
	memcpy(&uri[PROXY_URI_DL_HTTPS_LEN + host_len], PROXY_URI_DL_SEP, PROXY_URI_DL_SEP_LEN);
	memcpy(&uri[PROXY_URI_DL_HTTPS_LEN + host_len + PROXY_URI_DL_SEP_LEN], path, path_len);

	LOG_DBG("Proxy URI: %.*s", needed_len, uri);

	return 0;
}
