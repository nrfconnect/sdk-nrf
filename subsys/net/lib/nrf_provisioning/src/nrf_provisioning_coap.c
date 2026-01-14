/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif
/* Define _POSIX_C_SOURCE before including <string.h> in order to use `strtok_r`. */
#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <ncs_version.h>

#include <modem/modem_info.h>
#include <modem/nrf_modem_lib.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/coap_client.h>

#include <net/nrf_provisioning.h>
#include "nrf_provisioning_at.h"
#include "nrf_provisioning_codec.h"
#include "nrf_provisioning_coap.h"
#include "nrf_provisioning_jwt.h"

LOG_MODULE_REGISTER(nrf_provisioning_coap, CONFIG_NRF_PROVISIONING_LOG_LEVEL);

#if !defined(CONFIG_NRF_PROVISIONING_ROOT_CA_SEC_TAG)
#error "CONFIG_NRF_PROVISIONING_ROOT_CA_SEC_TAG not given"
#endif

#define TLS_SEC_TAG CONFIG_NRF_PROVISIONING_ROOT_CA_SEC_TAG
#define COAP_HOST CONFIG_NRF_PROVISIONING_COAP_HOSTNAME
#define PEER_PORT CONFIG_NRF_PROVISIONING_COAP_PORT

#define CLIENT_VERSION NCS_VERSION_STRING

#define AUTH_MVER "mver=%s"
#define AUTH_CVER "cver=%s"
#define AUTH_PATH_JWT "p/auth-jwt"
#define AUTH_API_TEMPLATE (AUTH_PATH_JWT "?" AUTH_MVER "&" AUTH_CVER)

#define CMDS_PATH "p/cmd"
#define CMDS_AFTER "after=%s"
#define CMDS_MAX_RX_SZ "rxMaxSize=%s"
#define CMDS_MAX_TX_SZ "txMaxSize=%s"
#define CMDS_LIMIT "limit=%s"
#define CMDS_API_TEMPLATE (CMDS_PATH "?" \
	CMDS_AFTER "&" CMDS_MAX_RX_SZ "&" CMDS_MAX_TX_SZ "&" CMDS_LIMIT)

#define RETRY_AMOUNT 10
static const char *resp_path = "p/rsp";
static const char *dtls_suspend = "/.dtls/suspend";

static struct coap_client client;
static bool socket_keep_open;

static K_SEM_DEFINE(coap_response, 0, 1);

int nrf_provisioning_coap_init(nrf_provisioning_event_cb_t callback)
{
	static bool initialized;
	int ret;

	nrf_provisioning_codec_init(callback);

	if (initialized) {
		return 0;
	}

	/* Modem info library is used to obtain the modem FW version */
	ret = modem_info_init();
	if (ret) {
		LOG_ERR("Modem info initialization failed, error: %d", ret);
		return ret;
	}

	LOG_INF("Init CoAP client");
	client.fd = -1;
	ret = coap_client_init(&client, NULL);
	if (ret) {
		LOG_ERR("Failed to initialize CoAP client, error: %d", ret);
		return ret;
	}

	initialized = true;

	return 0;
}

/* Setup DTLS options on a given socket */
static int dtls_setup(int fd)
{
	int err;
	int verify;
	int session_cache;

	/* Security tag that we have provisioned the certificate with */
	const sec_tag_t tls_sec_tag[] = {
		TLS_SEC_TAG,
	};

	LOG_INF("DTLS setup");

	/* Set up TLS peer verification */
	verify = ZSOCK_TLS_PEER_VERIFY_REQUIRED;

	err = zsock_setsockopt(fd, ZSOCK_SOL_TLS, ZSOCK_TLS_PEER_VERIFY, &verify, sizeof(verify));
	if (err) {
		LOG_ERR("Failed to setup peer verification, err %d", errno);
		return -errno;
	}

	/* Associate the socket with the security tag
	 * we have provisioned the certificate with.
	 */
	err = zsock_setsockopt(fd, ZSOCK_SOL_TLS, ZSOCK_TLS_SEC_TAG_LIST, tls_sec_tag,
			       sizeof(tls_sec_tag));
	if (err) {
		LOG_ERR("Failed to setup TLS sec tag, err %d", errno);
		return -errno;
	}

	if (IS_ENABLED(CONFIG_NRF_PROVISIONING_COAP_DTLS_SESSION_CACHE)) {
		session_cache = ZSOCK_TLS_SESSION_CACHE_ENABLED;
	} else {
		session_cache = ZSOCK_TLS_SESSION_CACHE_DISABLED;
	}

	err = zsock_setsockopt(fd, ZSOCK_SOL_TLS, ZSOCK_TLS_SESSION_CACHE, &session_cache,
			       sizeof(session_cache));
	if (err) {
		LOG_ERR("Failed to set TLS_SESSION_CACHE option: %d", errno);
		return -errno;
	}

	err = zsock_setsockopt(fd, ZSOCK_SOL_TLS, ZSOCK_TLS_HOSTNAME, COAP_HOST, strlen(COAP_HOST));
	if (err) {
		LOG_ERR("Failed to setup TLS hostname, err %d", errno);
		return -errno;
	}

	/* Enable connection ID */
	uint32_t dtls_cid = ZSOCK_TLS_DTLS_CID_SUPPORTED;

	err = zsock_setsockopt(fd, ZSOCK_SOL_TLS, ZSOCK_TLS_DTLS_CID, &dtls_cid, sizeof(dtls_cid));
	if (err) {
		LOG_ERR("Failed to enable connection ID, err %d", errno);
		return -errno;
	}

	if (IS_ENABLED(CONFIG_SOC_NRF9120)) {
		uint32_t val = 1;

		err = zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, SO_KEEPOPEN, &val, sizeof(val));
		if (err) {
			LOG_ERR("Failed to set socket option SO_KEEPOPEN, err %d", errno);
			socket_keep_open = false;
			return -errno;
		}
		socket_keep_open = true;
	}

	return 0;
}

static int socket_connect(int *const fd)
{
	struct zsock_addrinfo hints = {};
	struct zsock_addrinfo *address = NULL;
	int st;
	int ret = 0;
	struct net_sockaddr *sa;
	char peer_addr[NET_INET6_ADDRSTRLEN];

#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_IPV4)
	hints.ai_family = NET_AF_UNSPEC;
#elif defined(CONFIG_NET_IPV6)
	hints.ai_family = NET_AF_INET6;
#elif defined(CONFIG_NET_IPV4)
	hints.ai_family = NET_AF_INET;
#else
	hints.ai_family = NET_AF_UNSPEC;
#endif /* defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_IPV4) */

	LOG_DBG("CoAP host: %s", COAP_HOST);
	hints.ai_socktype = NET_SOCK_DGRAM;
	st = zsock_getaddrinfo(COAP_HOST, PEER_PORT, &hints, &address);
	LOG_DBG("getaddrinfo status: %d", st);
	LOG_DBG("fd: %d", *fd);

	if (st != 0) {
		LOG_ERR("Unable to resolve address");
		ret = -EFAULT;
		goto clean_up;
	}

	sa = address->ai_addr;
	zsock_inet_ntop(sa->sa_family,
		  (void *)&((struct net_sockaddr_in *)sa)->sin_addr,
		  peer_addr,
		  NET_INET6_ADDRSTRLEN);
	LOG_DBG("getaddrinfo() %s", peer_addr);

	*fd = zsock_socket(address->ai_family, address->ai_socktype, NET_IPPROTO_DTLS_1_2);
	if (*fd < 0) {
		LOG_ERR("Failed to create UDP socket %d", errno);
		ret = -errno;
		goto clean_up;
	}

	/* Setup DTLS socket options */
	ret = dtls_setup(*fd);
	if (ret) {
		goto clean_up;
	}

	if (zsock_connect(*fd, address->ai_addr, address->ai_addrlen) < 0) {
		LOG_ERR("Failed to connect UDP socket %d", errno);
		ret = -errno;
		goto clean_up;
	}
	LOG_INF("Connected");

clean_up:

	if (address) {
		zsock_freeaddrinfo(address);
	}

	if (ret) {
		if (*fd > -1) {
			(void)zsock_close(*fd);
			*fd = -1;
		}
	}

	return ret;
}

static int socket_close(int *const fd)
{
	int ret;

	if (*fd > -1) {
		int temp = *fd;

		*fd = client.fd = -1;
		ret = zsock_close(temp);
		if (ret) {
			LOG_WRN("Failed to close socket, error: %d", errno);
		}
	}

	return 0;
}

static void coap_callback(const struct coap_client_response_data *data, void *user_data)
{
	struct nrf_provisioning_coap_context *coap_ctx = NULL;

	if (user_data) {
		coap_ctx = (struct nrf_provisioning_coap_context *)user_data;
	}

	if (!data) {
		LOG_WRN("CoAP response data is NULL");
		if (coap_ctx) {
			coap_ctx->code = -EINVAL;
		}
		k_sem_give(&coap_response);
		return;
	}

	LOG_DBG("Callback code %d", data->result_code);
	if (!coap_ctx) {
		LOG_WRN("CoAP context not provided");
		k_sem_give(&coap_response);
		return;
	}

	coap_ctx->code = data->result_code;

	if (data->result_code == COAP_RESPONSE_CODE_CONTENT ||
	    data->result_code == COAP_RESPONSE_CODE_CHANGED ||
	    data->result_code == COAP_RESPONSE_CODE_CREATED) {
		if (data->payload_len) {
			LOG_DBG("Response received, offset %d len %d",
				data->offset, data->payload_len);
			if (data->offset == 0) {
				coap_ctx->response_len = 0;
			}

			if (coap_ctx->response_len + data->payload_len > coap_ctx->rx_buf_len) {
				LOG_ERR("RX buffer too small");
				coap_ctx->code = -ENOMEM;
				k_sem_give(&coap_response);
				return;
			}

			memcpy(coap_ctx->rx_buf + data->offset, data->payload, data->payload_len);
			coap_ctx->response = coap_ctx->rx_buf;
			coap_ctx->response_len += data->payload_len;
		} else {
			LOG_DBG("Operation successful");
			coap_ctx->response_len = 0;
		}
	}

	if (data->last_block) {
		LOG_DBG("Last packet received");
		k_sem_give(&coap_response);
	}
}

static int send_coap_request(struct coap_client *client, uint8_t method, const char *path,
			     const uint8_t *payload, size_t len,
			     struct nrf_provisioning_coap_context *const coap_ctx, bool confirmable)
{
	int ret;
	int retries = 0;
	struct coap_transmission_parameters params = coap_get_transmission_parameters();
	static struct coap_client_option block2_option;

	struct coap_client_request client_request = {
		.method = method,
		.confirmable = confirmable,
		.path = "",
		.fmt = COAP_CONTENT_FORMAT_APP_CBOR,
		.payload = NULL,
		.len = 0,
		.cb = coap_callback,
		.user_data = coap_ctx,
		.num_options = 0
	};

	/* Copy path into the request structure */
	strncpy(client_request.path, path, sizeof(client_request.path) - 1);
	client_request.path[sizeof(client_request.path) - 1] = '\0';

	if (payload != NULL) {
		client_request.payload = (uint8_t *)payload;
		client_request.len = len;
	}

	/* Suggest the maximum block size CONFIG_COAP_CLIENT_BLOCK_SIZE to the server */
	if (method == COAP_METHOD_GET) {
		block2_option = coap_client_option_initial_block2();
		client_request.options[0] = block2_option;
		client_request.num_options = 1;
	}

	while ((ret = coap_client_req(client, coap_ctx->connect_socket, NULL, &client_request,
				      NULL)) == -EAGAIN) {
		if (retries > RETRY_AMOUNT) {
			break;
		}
		LOG_DBG("CoAP client busy");
		k_sleep(K_MSEC(params.ack_timeout));
		retries++;
	}

	if (ret == 0) {
		return k_sem_take(&coap_response,
				  K_SECONDS(CONFIG_NRF_PROVISIONING_COAP_TIMEOUT_SECONDS));
	}

	return ret;
}

static int max_token_len(void)
{
	int token_len = 0;

	token_len = MAX(token_len, CONFIG_MODEM_JWT_MAX_LEN);

	return token_len;
}

static int generate_auth_token(char **auth_token)
{
	int tok_len;
	int ret;
	char *tok_ptr;

	tok_len = max_token_len();

	if (!tok_len) {
		LOG_ERR("Authentication token not configured");
		return -EINVAL;
	}

	*auth_token = k_malloc(tok_len);
	if (!*auth_token) {
		return -ENOMEM;
	}

	memset(*auth_token, 0x0, tok_len);
	tok_ptr = *auth_token;

	ret = nrf_provisioning_jwt_generate(0, tok_ptr, tok_len + 1);

	if (ret < 0) {
		LOG_ERR("Failed to generate JWT, error: %d", ret);
		goto fail;
	}

	return 0;

fail:
	k_free(*auth_token);
	*auth_token = NULL;

	return ret;
}

static int generate_auth_path(char *buffer, size_t len)
{
	int ret;
	char mver[MODEM_INFO_FWVER_SIZE];

	if (!buffer) {
		LOG_ERR("Cannot generate auth path, no output pointer given");
		__ASSERT_NO_MSG(false);
		return -EINVAL;
	}

	ret = modem_info_get_fw_version(mver, sizeof(mver));
	if (ret < 0) {
		LOG_ERR("Failed to get modem FW version");
		return ret;
	}

	if (len < (sizeof(AUTH_API_TEMPLATE) + strlen(mver) + strlen(CLIENT_VERSION))) {
		LOG_ERR("Cannot generate auth path, buffer too small");
		return -ENOMEM;
	}

	ret = snprintk(buffer, len, AUTH_API_TEMPLATE, mver, CLIENT_VERSION);
	if ((ret < 0) || (ret >= len)) {
		LOG_ERR("Could not format URL");
		return -ETXTBSY;
	}

	return 0;
}

static int response_code_to_error(int code)
{
	switch (code) {
	case COAP_RESPONSE_CODE_UNAUTHORIZED:
		LOG_ERR("Device not authorized");
		return -EACCES;
	case COAP_RESPONSE_CODE_FORBIDDEN:
		LOG_ERR("Device provided wrong auth credentials");
		return -EPERM;
	case COAP_RESPONSE_CODE_BAD_REQUEST:
		LOG_ERR("Bad request");
		return -EINVAL;
	case COAP_RESPONSE_CODE_NOT_ACCEPTABLE:
		LOG_ERR("Invalid Accept Headers");
		return -EINVAL;
	case COAP_RESPONSE_CODE_NOT_FOUND:
		LOG_ERR("Resource not found");
		return -ENOENT;
	case COAP_RESPONSE_CODE_INTERNAL_ERROR:
		LOG_ERR("Internal server error");
		return -EBUSY;
	case COAP_RESPONSE_CODE_SERVICE_UNAVAILABLE:
		LOG_ERR("Service unavailable");
		return -EBUSY;
	default:
		LOG_ERR("Unknown response code %d", code);
		return -ENOTSUP;
	}
}

static int authenticate(struct coap_client *client, const char *auth_token,
			struct nrf_provisioning_coap_context *const coap_ctx)
{
	int ret;
	char path[sizeof(AUTH_API_TEMPLATE) + MODEM_INFO_FWVER_SIZE + strlen(CLIENT_VERSION)];

	LOG_DBG("Authenticate");

	ret = generate_auth_path(path, sizeof(path));
	if (ret < 0) {
		LOG_ERR("Failed to generate path");
		return ret;
	}

	LOG_DBG("Path: %s", path);
	ret = send_coap_request(client, COAP_METHOD_POST, path, auth_token, strlen(auth_token),
				coap_ctx, true);
	if (ret < 0) {
		LOG_ERR("Failed to send CoAP request");
		return ret;
	}

	LOG_DBG("Response code %d", coap_ctx->code);
	if (coap_ctx->code != COAP_RESPONSE_CODE_CREATED) {
		if (coap_ctx->code < 0) {
			return coap_ctx->code;
		}
		return response_code_to_error(coap_ctx->code);
	}

	return 0;
}

static int request_commands(struct coap_client *client,
			    struct nrf_provisioning_coap_context *const coap_ctx)
{
	int ret;
	char after[NRF_PROVISIONING_CORRELATION_ID_SIZE];
	const char *rx_buf_sz = STRINGIFY(CONFIG_NRF_PROVISIONING_RX_BUF_SZ);
	const char *tx_buf_sz = STRINGIFY(CONFIG_NRF_PROVISIONING_TX_BUF_SZ);
	const char *limit = STRINGIFY(CONFIG_NRF_PROVISIONING_CBOR_RECORDS);
	char cmd[sizeof(CMDS_API_TEMPLATE) + NRF_PROVISIONING_CORRELATION_ID_SIZE +
		 strlen(rx_buf_sz) + strlen(tx_buf_sz) + strlen(limit)];

	LOG_DBG("Get commands");

	memcpy(after, nrf_provisioning_codec_get_latest_cmd_id(),
	       NRF_PROVISIONING_CORRELATION_ID_SIZE);

	ret = snprintk(cmd, sizeof(cmd), CMDS_API_TEMPLATE, after, rx_buf_sz, tx_buf_sz, limit);

	if ((ret < 0) || (ret >= sizeof(cmd))) {
		LOG_ERR("Could not format URL");
		return -ETXTBSY;
	}

	LOG_DBG("Path: %s", cmd);
	ret = send_coap_request(client, COAP_METHOD_GET, cmd, NULL, 0, coap_ctx, true);
	if (ret < 0) {
		LOG_ERR("Failed to send CoAP request");
		return ret;
	}

	return 0;
}

static int send_response(struct coap_client *client,
			 struct nrf_provisioning_coap_context *const coap_ctx,
			 struct cdc_context *cdc_ctx)
{
	int ret;

	LOG_DBG("Response size %d", cdc_ctx->opkt_sz);
	LOG_HEXDUMP_DBG(cdc_ctx->opkt, cdc_ctx->opkt_sz, "Response to server");
	ret = send_coap_request(client, COAP_METHOD_POST, resp_path, cdc_ctx->opkt,
				cdc_ctx->opkt_sz, coap_ctx, true);
	if (ret < 0) {
		LOG_ERR("Failed to send CoAP request");
		return ret;
	}

	LOG_DBG("Response code %d", coap_ctx->code);
	if (coap_ctx->code != COAP_RESPONSE_CODE_CHANGED) {
		if (coap_ctx->code < 0) {
			return coap_ctx->code;
		}
		return response_code_to_error(coap_ctx->code);
	}

	return 0;
}

static void suspend_dtls_session(struct coap_client *client,
				struct nrf_provisioning_coap_context *const coap_ctx)
{
	int ret;

	/* Send a request to suspend DTLS session */
	ret = send_coap_request(client, COAP_METHOD_POST, dtls_suspend, NULL, 0, coap_ctx, false);
	if (ret < 0) {
		LOG_WRN("Failed to send CoAP request");
		socket_keep_open = false;
	}

	LOG_DBG("Response code %d", coap_ctx->code);
	if (coap_ctx->code == COAP_RESPONSE_CODE_NOT_FOUND) {
		LOG_WRN("DTLS session suspension not supported");
		socket_keep_open = false;
	}
}

int nrf_provisioning_coap_req(struct nrf_provisioning_coap_context *const coap_ctx)
{
	__ASSERT_NO_MSG(coap_ctx != NULL);

	/* Only one provisioning ongoing at a time*/
	static union {
		char coap[CONFIG_NRF_PROVISIONING_TX_BUF_SZ];
		char at[CONFIG_NRF_PROVISIONING_CODEC_AT_CMD_LEN];
	} tx_buf;
	static char rx_buf[CONFIG_NRF_PROVISIONING_RX_BUF_SZ];

	int ret;
	char *auth_token = NULL;
	struct cdc_context cdc_ctx;
	bool finished = false;
	int retries = 0;

	coap_ctx->rx_buf = rx_buf;
	coap_ctx->rx_buf_len = sizeof(rx_buf);

	ret = socket_connect(&coap_ctx->connect_socket);
	if (ret < 0) {
		return ret;
	}

	ret = generate_auth_token(&auth_token);
	if (ret) {
		LOG_ERR("Failed to generate authentication token");
		return ret;
	}

	while (true) {
		ret = authenticate(&client, auth_token, coap_ctx);
		if (ret < 0) {
			break;
		}

		LOG_INF("Requesting commands");
		ret = request_commands(&client, coap_ctx);
		if (ret < 0) {
			break;
		}

		LOG_DBG("Response code %d", coap_ctx->code);
		if (!socket_keep_open) {
			socket_close(&coap_ctx->connect_socket);
		}

		if (coap_ctx->code == COAP_RESPONSE_CODE_CONTENT) {
			if (!coap_ctx->response_len) {
				LOG_INF("No commands to process on server side");
				ret = -ENODATA;
				break;
			}
			nrf_provisioning_codec_setup(&cdc_ctx, tx_buf.at, sizeof(tx_buf));

			/* Codec input and output buffers */
			cdc_ctx.ipkt = coap_ctx->response;
			cdc_ctx.ipkt_sz = coap_ctx->response_len;
			cdc_ctx.opkt = tx_buf.coap;
			cdc_ctx.opkt_sz = sizeof(tx_buf);

			if (socket_keep_open) {
				LOG_DBG("Requesting DTLS session suspension");
				suspend_dtls_session(&client, coap_ctx);
			}

			LOG_INF("Processing commands");
			ret = nrf_provisioning_codec_process_commands();
			if (ret < 0) {
				LOG_ERR("ret %d", ret);
				break;
			} else if (ret > 0) {
				/* Need to send responses before we are done */
				LOG_INF("Finished");
				finished = true;
			}
retry_response:
			if (!socket_keep_open) {
				ret = socket_connect(&coap_ctx->connect_socket);
				if (ret < 0) {
					break;
				}

				ret = authenticate(&client, auth_token, coap_ctx);
				if (ret < 0) {
					break;
				}
			}

			LOG_INF("Sending response to server");
			ret = send_response(&client, coap_ctx, &cdc_ctx);
			if (ret < 0) {
				if (socket_keep_open && retries++ == 0) {
					/* Try reconnecting */
					socket_close(&coap_ctx->connect_socket);
					socket_keep_open = false;
					goto retry_response;
				}
				LOG_ERR("Failed to send response, ret %d", ret);
				break;
			}
			/* Provisioning finished */
			if (finished) {
				ret = NRF_PROVISIONING_FINISHED;
				break;
			}
			nrf_provisioning_codec_teardown();
			continue;
		} else {
			ret = response_code_to_error(coap_ctx->code);
		}
		break;
	}

	nrf_provisioning_codec_teardown();

	if (auth_token) {
		k_free(auth_token);
	}

	socket_close(&coap_ctx->connect_socket);

	return ret;
}
