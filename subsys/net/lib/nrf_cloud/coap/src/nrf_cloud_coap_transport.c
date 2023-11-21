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
#include <zephyr/random/rand32.h>
#include <nrf_socket.h>
#include <nrf_modem_at.h>
#include <date_time.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_coap.h>
#include <cJSON.h>
#include <version.h>
#include "nrf_cloud_codec_internal.h"
#include "nrfc_dtls.h"
#include "coap_codec.h"
#include "nrf_cloud_coap_transport.h"
#include "nrf_cloud_mem.h"

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

static int sock = -1;
static bool authenticated;
static bool cid_saved;

static struct coap_client coap_client;
static int nrf_cloud_coap_authenticate(void);

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

bool nrf_cloud_coap_is_connected(void)
{
	return authenticated;
}

/**@brief Resolves the configured hostname. */
static int server_resolve(struct sockaddr_in *server4)
{
	int err;
	struct addrinfo *result;
	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_DGRAM
	};
	char ipv4_addr[NET_IPV4_ADDR_LEN];

	LOG_DBG("Looking up server %s", CONFIG_NRF_CLOUD_COAP_SERVER_HOSTNAME);
	err = getaddrinfo(CONFIG_NRF_CLOUD_COAP_SERVER_HOSTNAME, NULL, &hints, &result);
	if (err != 0) {
		LOG_ERR("ERROR: getaddrinfo for %s failed: %d, errno:%d",
			CONFIG_NRF_CLOUD_COAP_SERVER_HOSTNAME, err, -errno);
		return -EIO;
	}

	if (result == NULL) {
		LOG_ERR("ERROR: Address not found");
		return -ENOENT;
	}

	/* IPv4 Address. */
	server4->sin_addr.s_addr = ((struct sockaddr_in *)result->ai_addr)->sin_addr.s_addr;
	server4->sin_family = AF_INET;
	server4->sin_port = htons(CONFIG_NRF_CLOUD_COAP_SERVER_PORT);

	inet_ntop(AF_INET, &server4->sin_addr.s_addr, ipv4_addr,
		  sizeof(ipv4_addr));
	LOG_DBG("Server %s IP address: %s, port: %u",
		CONFIG_NRF_CLOUD_COAP_SERVER_HOSTNAME, ipv4_addr,
		CONFIG_NRF_CLOUD_COAP_SERVER_PORT);

	/* Free the address. */
	freeaddrinfo(result);

	return 0;
}

/**@brief Initialize the CoAP client */
int nrf_cloud_coap_init(void)
{
	static bool initialized;
	int err;

	authenticated = false;

	if (!initialized) {
		/* Only initialize one time; not idempotent. */
		LOG_DBG("Initializing async CoAP client");
		err = coap_client_init(&coap_client, NULL);
		if (err) {
			LOG_ERR("Failed to initialize CoAP client: %d", err);
			return err;
		}
		(void)nrf_cloud_codec_init(NULL);
#if defined(CONFIG_MODEM_INFO)
		err = modem_info_init();
		if (err) {
			return err;
		}
#endif
		initialized = true;
	}

	return 0;
}

static int update_device_status(const char * const app_ver)
{
	int err;

#if defined(CONFIG_NRF_CLOUD_SEND_DEVICE_STATUS)
	struct nrf_cloud_modem_info mdm_inf = {
		.device = NRF_CLOUD_INFO_SET,
		.application_version = app_ver,
		.mpi = NULL
	};
	struct nrf_cloud_device_status dev_status = {
		.modem = &mdm_inf,
		.svc = NULL
	};

	mdm_inf.network = IS_ENABLED(CONFIG_NRF_CLOUD_SEND_DEVICE_STATUS_NETWORK) ?
					NRF_CLOUD_INFO_SET : NRF_CLOUD_INFO_CLEAR;

	mdm_inf.sim = IS_ENABLED(CONFIG_NRF_CLOUD_SEND_DEVICE_STATUS_SIM) ?
					NRF_CLOUD_INFO_SET : NRF_CLOUD_INFO_CLEAR;

	dev_status.conn_inf = IS_ENABLED(CONFIG_NRF_CLOUD_SEND_DEVICE_STATUS_CONN_INF) ?
					NRF_CLOUD_INFO_SET : NRF_CLOUD_INFO_CLEAR;

	err = nrf_cloud_coap_shadow_device_status_update(&dev_status);
#else
	ARG_UNUSED(app_ver);
	err = 0;
#endif
	return err;
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
	struct sockaddr_storage server;
	int err;

	err = server_resolve((struct sockaddr_in *)&server);
	if (err) {
		LOG_ERR("Failed to resolve server name: %d", err);
		return err;
	}

	LOG_DBG("Creating socket type IPPROTO_DTLS_1_2");
	if (sock == -1) {
		sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_DTLS_1_2);
	}
	LOG_DBG("sock = %d", sock);
	if (sock < 0) {
		LOG_ERR("Failed to create CoAP socket: %d.", -errno);
		return -errno;
	}

	err = nrfc_dtls_setup(sock);
	if (err < 0) {
		LOG_ERR("Failed to initialize the DTLS client: %d", err);
		goto fail;
	}
	authenticated = false;

	err = connect(sock, (struct sockaddr *)&server,
		      sizeof(struct sockaddr_in));
	if (err < 0) {
		LOG_ERR("Connect failed : %d", -errno);
		goto fail;
	}

	err = nrf_cloud_coap_authenticate();
	if (err < 0) {
		goto fail;
	}

	/* On initial connect, set the control section in the shadow */
	update_control_section();

	/* On each connect, update the device status in the shadow */
	return update_device_status(app_ver);

fail:
	(void)nrf_cloud_coap_disconnect();
	return err;
}

static void auth_cb(int16_t result_code, size_t offset, const uint8_t *payload, size_t len,
		    bool last_block, void *user_data)
{
	LOG_RESULT_CODE_INF("Authorization result_code:", result_code);
	if (result_code < COAP_RESPONSE_CODE_BAD_REQUEST) {
		authenticated = true;
	}
}

static int nrf_cloud_coap_authenticate(void)
{
	int err;
	char *jwt;

	if (authenticated) {
		LOG_DBG("Already authenticated");
		return 0;
	}

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
			return -ETXTBSY;
		}
	} else {
		LOG_ERR("Unable to obtain the modem firmware version: %d", err);
	}
#else
	char *ver_string = VER_STRING_FMT2;
#endif

	LOG_DBG("Generate JWT");
	jwt = k_malloc(JWT_BUF_SZ);
	if (!jwt) {
		return -ENOMEM;
	}
	err = nrf_cloud_jwt_generate(NRF_CLOUD_JWT_VALID_TIME_S_MAX, jwt, JWT_BUF_SZ);
	if (err) {
		LOG_ERR("Error generating JWT with modem: %d", err);
		k_free(jwt);
		return err;
	}

	LOG_INF("Request authorization with JWT");
	err = nrf_cloud_coap_post("auth/jwt", err ? NULL : ver_string,
				 (uint8_t *)jwt, strlen(jwt),
				 COAP_CONTENT_FORMAT_TEXT_PLAIN, true, auth_cb, NULL);
	k_free(jwt);

	if (err) {
		return err;
	}
	if (!authenticated) {
		return -EACCES;
	}

	LOG_INF("Authorized");

	if (nrfc_dtls_cid_is_active(sock)) {
		LOG_INF("DTLS CID is active");
	}
	return err;
}

int nrf_cloud_coap_pause(void)
{
	int err = 0;

	cid_saved = false;

	if (nrfc_dtls_cid_is_active(sock) && authenticated) {
		err = nrfc_dtls_session_save(sock);
		if (!err) {
			LOG_DBG("DTLS CID session saved.");
			cid_saved = true;
		} else {
			LOG_ERR("Unable to save DTLS CID session: %d", err);
		}
	} else {
		LOG_WRN("Unable to pause CoAP connection.");
		err = -EACCES;
	}
	return err;
}

int nrf_cloud_coap_resume(void)
{
	int err = 0;

	if (cid_saved) {
		err = nrfc_dtls_session_load(sock);
		cid_saved = false;
		if (!err) {
			LOG_DBG("Loaded DTLS CID session");
			authenticated = true;
		} else if (err == -EAGAIN) {
			LOG_INF("Failed to load DTLS CID session");
		} else if (err == -EINVAL) {
			LOG_INF("DLTS CID sessions not supported with current modem firmware");
		} else {
			LOG_ERR("Error on DTLS CID session load: %d", err);
		}
	} else {
		LOG_WRN("Unable to resume CoAP connection.");
		err = -EACCES; /* Cannot resume. */
	}

	return err;
}

static K_SEM_DEFINE(serial_sem, 1, 1);
static K_SEM_DEFINE(cb_sem, 0, 1);

struct user_cb {
	coap_client_response_cb_t cb;
	void *user_data;
	int result_code;
};

static void client_callback(int16_t result_code, size_t offset, const uint8_t *payload, size_t len,
			    bool last_block, void *user_data)
{
	struct user_cb *user_cb = (struct user_cb *)user_data;

	user_cb->result_code = result_code;
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
		authenticated = false; /* Lost authorization; need to reconnect. */
	} else if ((result_code >= COAP_RESPONSE_CODE_BAD_REQUEST) && len) {
		LOG_ERR("Unexpected response: %*s", len, payload);
	}
	if ((user_cb != NULL) && (user_cb->cb != NULL)) {
		LOG_DBG("Calling user's callback %p", user_cb->cb);
		user_cb->cb(result_code, offset, payload, len, last_block, user_cb->user_data);
	}
	if (last_block || (result_code >= COAP_RESPONSE_CODE_BAD_REQUEST)) {
		LOG_DBG("End of client transfer");
		k_sem_give(&cb_sem);
	}
}

static int client_transfer(enum coap_method method,
			   const char *resource, const char *query,
			   const uint8_t *buf, size_t buf_len,
			   enum coap_content_format fmt_out,
			   enum coap_content_format fmt_in,
			   bool response_expected,
			   bool reliable,
			   coap_client_response_cb_t cb, void *user)
{
	__ASSERT_NO_MSG(resource != NULL);

	k_sem_take(&serial_sem, K_FOREVER);

	int err;
	int retry;
	char path[MAX_COAP_PATH + 1];
	struct user_cb user_cb = {
		.cb = cb,
		.user_data = user
	};
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
		.user_data = &user_cb
	};

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
			return -ETXTBSY;
		}
	}

#if defined(CONFIG_NRF_CLOUD_COAP_LOG_LEVEL_DBG)
	LOG_DBG("%s %s %s Content-Format:%s, %zd bytes out, Accept:%s", reliable ? "CON" : "NON",
		METHOD_NAME(method), path, fmt_name(fmt_out), buf_len,
		response_expected ? fmt_name(fmt_in) : "none");
#endif /* CONFIG_NRF_CLOUD_COAP_LOG_LEVEL_DBG */

	retry = 0;
	k_sem_reset(&cb_sem);
	while ((err = coap_client_req(&coap_client, sock, NULL, &request,
				      reliable ? -1 : CONFIG_NON_RESP_RETRIES)) == -EAGAIN) {
		/* -EAGAIN means the CoAP client is currently waiting for a response
		 * to a previous request (likely started in a separate thread).
		 */
		if (retry++ > MAX_RETRIES) {
			LOG_ERR("Timeout waiting for CoAP client to be available");
			return -ETIMEDOUT;
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
		(void)k_sem_take(&cb_sem, K_FOREVER); /* Wait for coap_client to exhaust retries */
	}

	if (!reliable && !err && (user_cb.result_code >= COAP_RESPONSE_CODE_BAD_REQUEST)) {
		/* NON transfers usually do not use a callback,
		 * so make sure a bad result is not ignored.
		 */
		err = user_cb.result_code;
	}

	k_sem_give(&serial_sem);
	return err;
}

int nrf_cloud_coap_get(const char *resource, const char *query,
		       const uint8_t *buf, size_t len,
		       enum coap_content_format fmt_out,
		       enum coap_content_format fmt_in, bool reliable,
		       coap_client_response_cb_t cb, void *user)
{
	return client_transfer(COAP_METHOD_GET, resource, query,
			   buf, len, fmt_out, fmt_in, true, reliable, cb, user);
}

int nrf_cloud_coap_post(const char *resource, const char *query,
			const uint8_t *buf, size_t len,
			enum coap_content_format fmt, bool reliable,
			coap_client_response_cb_t cb, void *user)
{
	return client_transfer(COAP_METHOD_POST, resource, query,
			   buf, len, fmt, fmt, false, reliable, cb, user);
}

int nrf_cloud_coap_put(const char *resource, const char *query,
		       const uint8_t *buf, size_t len,
		       enum coap_content_format fmt, bool reliable,
		       coap_client_response_cb_t cb, void *user)
{
	return client_transfer(COAP_METHOD_PUT, resource, query,
			   buf, len, fmt, fmt, false, reliable, cb, user);
}

int nrf_cloud_coap_delete(const char *resource, const char *query,
			  const uint8_t *buf, size_t len,
			  enum coap_content_format fmt, bool reliable,
			  coap_client_response_cb_t cb, void *user)
{
	return client_transfer(COAP_METHOD_DELETE, resource, query,
			   buf, len, fmt, fmt, false, reliable, cb, user);
}

int nrf_cloud_coap_fetch(const char *resource, const char *query,
			 const uint8_t *buf, size_t len,
			 enum coap_content_format fmt_out,
			 enum coap_content_format fmt_in, bool reliable,
			 coap_client_response_cb_t cb, void *user)
{
	return client_transfer(COAP_METHOD_FETCH, resource, query,
			   buf, len, fmt_out, fmt_in, true, reliable, cb, user);
}

int nrf_cloud_coap_patch(const char *resource, const char *query,
			 const uint8_t *buf, size_t len,
			 enum coap_content_format fmt, bool reliable,
			 coap_client_response_cb_t cb, void *user)
{
	return client_transfer(COAP_METHOD_PATCH, resource, query,
			   buf, len, fmt, fmt, false, reliable, cb, user);
}

int nrf_cloud_coap_disconnect(void)
{
	int err;

	if (sock < 0) {
		return -ENOTCONN;
	}

	authenticated = false;
	err = close(sock);
	sock = -1;

	return err;
}
