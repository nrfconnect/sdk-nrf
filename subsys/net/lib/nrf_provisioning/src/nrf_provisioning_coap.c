/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

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

#define MODEM_INFO_FWVER_SIZE 41
#define CLIENT_VERSION "1"

#define AUTH_MVER "mver=%s"
#define AUTH_CVER "cver=%s"
#define AUTH_PATH "p/auth"
#define AUTH_API_TEMPLATE (AUTH_PATH "?" AUTH_MVER "&" AUTH_CVER)

#define CMDS_PATH "p/cmd"
#define CMDS_AFTER "after=%s"
#define CMDS_MAX_RX_SZ "rxMaxSize=%s"
#define CMDS_MAX_TX_SZ "txMaxSize=%s"
#define CMDS_API_TEMPLATE (CMDS_PATH "?" CMDS_AFTER "&" CMDS_MAX_RX_SZ "&" CMDS_MAX_TX_SZ)

#define RETRY_AMOUNT 10
static const char *resp_path = "p/rsp";

static struct addrinfo *address;
static struct coap_client client;

static K_SEM_DEFINE(coap_response, 0, 1);

int nrf_provisioning_coap_init(struct nrf_provisioning_mm_change *mmode)
{
	static bool initialized;
	int ret;

	nrf_provisioning_codec_init(mmode);

	if (initialized) {
		return 0;
	}

	if (IS_ENABLED(CONFIG_NRF_PROVISIONING_SYS_INIT)) {
		ret = nrf_modem_lib_init();
		if (ret < 0) {
			LOG_ERR("Failed to initialize modem library, error: %d", ret);
			return ret;
		}
	}

	/* Modem info library is used to obtain the modem FW version */
	ret = modem_info_init();
	if (ret) {
		LOG_ERR("Modem info initialization failed, error: %d", ret);
		return ret;
	}

	LOG_INF("Init CoAP client");
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

	LOG_INF("TLS setup");

	/* Set up TLS peer verification */
	enum {
		NONE = 0,
		OPTIONAL = 1,
		REQUIRED = 2,
	};

	verify = REQUIRED;

	err = setsockopt(fd, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(verify));
	if (err) {
		LOG_ERR("Failed to setup peer verification, err %d", errno);
		return err;
	}

	/* Associate the socket with the security tag
	 * we have provisioned the certificate with.
	 */
	err = setsockopt(fd, SOL_TLS, TLS_SEC_TAG_LIST, tls_sec_tag, sizeof(tls_sec_tag));
	if (err) {
		LOG_ERR("Failed to setup TLS sec tag, err %d", errno);
		return err;
	}

	if (IS_ENABLED(CONFIG_NRF_PROVISIONING_COAP_TLS_SESSION_CACHE)) {
		session_cache = TLS_SESSION_CACHE_ENABLED;
	} else {
		session_cache = TLS_SESSION_CACHE_DISABLED;
	}

	err = setsockopt(fd, SOL_TLS, TLS_SESSION_CACHE,
					&session_cache, sizeof(session_cache));
	if (err) {
		LOG_ERR("Failed to set TLS_SESSION_CACHE option: %d", errno);
		return err;
	}

	err = setsockopt(fd, SOL_TLS, TLS_HOSTNAME, COAP_HOST, strlen(COAP_HOST));
	if (err) {
		LOG_ERR("Failed to setup TLS hostname, err %d", errno);
		return err;
	}

	return 0;
}

static int socket_connect(int *const fd)
{
	static struct addrinfo hints;
	int st;
	int ret = 0;
	struct sockaddr *sa;
	char peer_addr[INET6_ADDRSTRLEN];

#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_IPV4)
	hints.ai_family = AF_UNSPEC;
#elif defined(CONFIG_NET_IPV6)
	hints.ai_family = AF_INET6;
#elif defined(CONFIG_NET_IPV4)
	hints.ai_family = AF_INET;
#else
	hints.ai_family = AF_UNSPEC;
#endif /* defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_IPV4) */

	hints.ai_socktype = SOCK_DGRAM;
	st = getaddrinfo(COAP_HOST, PEER_PORT, &hints, &address);
	LOG_DBG("getaddrinfo status: %d", st);
	LOG_DBG("fd: %d", *fd);

	if (st != 0) {
		LOG_ERR("Unable to resolve address");
		ret = -EFAULT;
		goto clean_up;
	}

	sa = address->ai_addr;
	inet_ntop(sa->sa_family,
		  (void *)&((struct sockaddr_in *)sa)->sin_addr,
		  peer_addr,
		  INET6_ADDRSTRLEN);
	LOG_DBG("getaddrinfo() %s", peer_addr);

	*fd = socket(address->ai_family, address->ai_socktype, IPPROTO_DTLS_1_2);
	if (*fd < 0) {
		LOG_ERR("Failed to create UDP socket %d", errno);
		ret = -ENOTCONN;
		goto clean_up;
	}

	/* Setup DTLS socket options */
	ret = dtls_setup(*fd);
	if (ret) {
		LOG_ERR("Failed to setup TLS socket option");
		ret = -EACCES;
		goto clean_up;
	}

	if (connect(*fd, address->ai_addr, address->ai_addrlen) < 0) {
		LOG_ERR("Failed to connect UDP socket %d", errno);
		if (errno == ETIMEDOUT) {
			ret = -ETIMEDOUT;
		} else {
			ret = -ECONNREFUSED;
		}
		goto clean_up;
	}
	LOG_INF("Connected");

clean_up:

	if (ret) {
		if (*fd > -1) {
			(void)close(*fd);
			*fd = -1;
		}
	}

	return ret;
}

static int socket_close(int *const fd)
{
	int ret;

	if (*fd > -1) {
		ret = close(*fd);
		if (ret) {
			LOG_WRN("Failed to close socket, error: %d", errno);
		}
		*fd = -1;
	}

	return 0;
}

static void coap_callback(int16_t code, size_t offset, const uint8_t *payload, size_t len,
			  bool last_block, void *user_data)
{
	struct nrf_provisioning_coap_context *coap_ctx = NULL;

	if (user_data) {
		coap_ctx = (struct nrf_provisioning_coap_context *)user_data;
	}
	LOG_DBG("Callback code %d", code);
	if (!coap_ctx) {
		LOG_WRN("CoAP context not provided");
		k_sem_give(&coap_response);
		return;
	}

	coap_ctx->code = code;

	if (code == COAP_RESPONSE_CODE_CONTENT || code == COAP_RESPONSE_CODE_CHANGED ||
	    code == COAP_RESPONSE_CODE_CREATED) {
		if (len) {
			LOG_DBG("Response received, offset %d len %d", offset, len);
			if (offset == 0) {
				coap_ctx->response_len = 0;
			}

			if (coap_ctx->response_len + len > coap_ctx->rx_buf_len) {
				LOG_ERR("RX buffer too small");
				coap_ctx->code = -ENOMEM;
				k_sem_give(&coap_response);
				return;
			}

			memcpy(coap_ctx->rx_buf + offset, payload, len);
			coap_ctx->response = coap_ctx->rx_buf;
			coap_ctx->response_len += len;
		} else {
			LOG_DBG("Operation successful");
			coap_ctx->response_len = 0;
		}
	}

	if (last_block) {
		LOG_DBG("Last packet received");
		k_sem_give(&coap_response);
	}
}

static int send_coap_request(struct coap_client *client, uint8_t method, const char *path,
			     const uint8_t *payload, size_t len,
			     struct nrf_provisioning_coap_context *const coap_ctx)
{
	int retries = 0;
	struct coap_client_request client_request = {
		.method = method,
		.confirmable = true,
		.path = path,
		.fmt = COAP_CONTENT_FORMAT_APP_CBOR,
		.payload = NULL,
		.len = 0,
		.cb = coap_callback,
		.user_data = coap_ctx
	};

	if (payload != NULL) {
		client_request.payload = (uint8_t *)payload;
		client_request.len = len;
	}

	while (coap_client_req(client, coap_ctx->connect_socket, NULL, &client_request, -1) ==
	       -EAGAIN) {
		if (retries > RETRY_AMOUNT) {
			break;
		}
		LOG_DBG("CoAP client busy");
		k_sleep(K_MSEC(500));
		retries++;
	}
	k_sem_take(&coap_response, K_FOREVER);

	return 0;
}

static int max_token_len(void)
{
	int token_len = 0;

#if defined(CONFIG_NRF_PROVISIONING_AT_ATTESTTOKEN_MAX_LEN)
	token_len = MAX(token_len, CONFIG_NRF_PROVISIONING_AT_ATTESTTOKEN_MAX_LEN);
#endif

#if defined(CONFIG_MODEM_JWT_MAX_LEN)
	token_len = MAX(token_len, CONFIG_MODEM_JWT_MAX_LEN);
#endif

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

	if (IS_ENABLED(CONFIG_NRF_PROVISIONING_ATTESTTOKEN)) {
		ret = nrf_provisioning_at_attest_token_get(tok_ptr, tok_len);

		if (ret != 0) {
			LOG_ERR("Failed to generate attestation token, error: %d", ret);
			if (IS_ENABLED(CONFIG_NRF_PROVISIONING_JWT)) {
				goto a_alt_tok;
			}
			goto fail;
		}
	}

a_alt_tok:
	if (IS_ENABLED(CONFIG_NRF_PROVISIONING_JWT)) {
		ret = nrf_provisioning_jwt_generate(0, tok_ptr, tok_len + 1);

		if (ret < 0) {
			LOG_ERR("Failed to generate JWT, error: %d", ret);
			goto fail;
		}
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
	char mver[CONFIG_MODEM_INFO_BUFFER_SIZE];
	char *mvernmb;
	int cnt;

	if (!buffer) {
		LOG_ERR("Cannot generate auth path, no output pointer given");
		__ASSERT_NO_MSG(false);
		return -EINVAL;
	}

	ret = modem_info_string_get(MODEM_INFO_FW_VERSION, mver, sizeof(mver));

	if (ret <= 0) {
		LOG_ERR("Failed to get modem FW version");
		return ret ? ret : -ENODATA;
	}

	mvernmb = strtok(mver, "_-");
	cnt = 1;

	/* mfw_nrf9160_1.3.2-FOTA-TEST - for example */
	while (cnt++ < 3) {
		mvernmb = strtok(NULL, "_-");
	}

	if (len < (sizeof(AUTH_API_TEMPLATE) + strlen(mvernmb) + strlen(CLIENT_VERSION))) {
		LOG_ERR("Cannot generate auth path, buffer too small");
		return -ENOMEM;
	}

	ret = snprintk(buffer, len, AUTH_API_TEMPLATE, mvernmb, CLIENT_VERSION);

	if ((ret < 0) || (ret >= len)) {
		LOG_ERR("Could not format URL");
		return -ETXTBSY;
	}

	return 0;
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
				coap_ctx);
	if (ret < 0) {
		LOG_ERR("Failed to send CoAP request");
		return ret;
	}

	LOG_DBG("Response code %d", coap_ctx->code);
	if (coap_ctx->code != COAP_RESPONSE_CODE_CREATED) {
		if (coap_ctx->code == COAP_RESPONSE_CODE_INTERNAL_ERROR ||
		    coap_ctx->code == COAP_RESPONSE_CODE_SERVICE_UNAVAILABLE) {
			return -EBUSY;
		} else if (coap_ctx->code == COAP_RESPONSE_CODE_UNAUTHORIZED ||
			   coap_ctx->code == COAP_RESPONSE_CODE_FORBIDDEN) {
			LOG_ERR("Unauthorized, code %d", coap_ctx->code);
			return -EACCES;
		}
		LOG_ERR("Unknown result code %d", coap_ctx->code);
		return -ENOTSUP;
	}

	return 0;
}

static int request_commands(struct coap_client *client,
			    struct nrf_provisioning_coap_context *const coap_ctx)
{
	int ret;
	char after[NRF_PROVISIONING_CORRELATION_ID_SIZE];
	char *rx_buf_sz = STRINGIFY(CONFIG_NRF_PROVISIONING_COAP_RX_BUF_SZ);
	char *tx_buf_sz = STRINGIFY(CONFIG_NRF_PROVISIONING_COAP_TX_BUF_SZ);
	char cmd[sizeof(CMDS_API_TEMPLATE) + NRF_PROVISIONING_CORRELATION_ID_SIZE +
		 strlen(rx_buf_sz) + strlen(tx_buf_sz)];

	LOG_DBG("Get commands");

	memcpy(after, nrf_provisioning_codec_get_latest_cmd_id(),
	NRF_PROVISIONING_CORRELATION_ID_SIZE);

	ret = snprintk(cmd, sizeof(cmd), CMDS_API_TEMPLATE, after, rx_buf_sz, tx_buf_sz);

	if ((ret < 0) || (ret >= sizeof(cmd))) {
		LOG_ERR("Could not format URL");
		return -ETXTBSY;
	}

	LOG_DBG("Path: %s", cmd);
	ret = send_coap_request(client, COAP_METHOD_GET, cmd, NULL, 0, coap_ctx);
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
				cdc_ctx->opkt_sz, coap_ctx);
	if (ret < 0) {
		LOG_ERR("Failed to send CoAP request");
		return ret;
	}

	LOG_DBG("Response code %d", coap_ctx->code);
	if (coap_ctx->code != COAP_RESPONSE_CODE_CHANGED) {
		if (coap_ctx->code == COAP_RESPONSE_CODE_INTERNAL_ERROR ||
		    coap_ctx->code == COAP_RESPONSE_CODE_SERVICE_UNAVAILABLE) {
			return -EBUSY;
		} else if (coap_ctx->code == COAP_RESPONSE_CODE_BAD_REQUEST) {
			LOG_ERR("Bad request");
			return -EINVAL;
		}
		LOG_ERR("Unknown result code %d", coap_ctx->code);
		return -ENOTSUP;
	}

	return 0;
}

int nrf_provisioning_coap_req(struct nrf_provisioning_coap_context *const coap_ctx)
{
	__ASSERT_NO_MSG(coap_ctx != NULL);

	/* Only one provisioning ongoing at a time*/
	static union {
		char coap[CONFIG_NRF_PROVISIONING_COAP_TX_BUF_SZ];
		char at[CONFIG_NRF_PROVISIONING_CODEC_AT_CMD_LEN];
	} tx_buf;
	static char rx_buf[CONFIG_NRF_PROVISIONING_COAP_RX_BUF_SZ];

	int ret;
	char *auth_token = NULL;
	struct cdc_context cdc_ctx;
	bool finished = false;

	coap_ctx->rx_buf = rx_buf;
	coap_ctx->rx_buf_len = sizeof(rx_buf);

	ret = socket_connect(&coap_ctx->connect_socket);
	if (ret) {
		LOG_ERR("Failed to connect socket");
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

		ret = request_commands(&client, coap_ctx);
		if (ret < 0) {
			break;
		}

		LOG_DBG("Response code %d", coap_ctx->code);
		socket_close(&coap_ctx->connect_socket);

		if (coap_ctx->code == COAP_RESPONSE_CODE_CONTENT) {
			if (!coap_ctx->response_len) {
				LOG_INF("No more commands to process on server side");
				ret = 0;
				break;
			}
			nrf_provisioning_codec_setup(&cdc_ctx, tx_buf.at, sizeof(tx_buf));

			/* Codec input and output buffers */
			cdc_ctx.ipkt = coap_ctx->response;
			cdc_ctx.ipkt_sz = coap_ctx->response_len;
			cdc_ctx.opkt = tx_buf.coap;
			cdc_ctx.opkt_sz = sizeof(tx_buf);

			ret = nrf_provisioning_codec_process_commands();
			if (ret < 0) {
				LOG_ERR("ret %d", ret);
				break;
			} else if (ret > 0) {
				/* Need to send responses before we are done */
				LOG_INF("Finished");
				finished = true;
			}

			ret = socket_connect(&coap_ctx->connect_socket);
			if (ret < 0) {
				LOG_ERR("Failed to connect socket, error: %d", ret);
				break;
			}

			ret = authenticate(&client, auth_token, coap_ctx);
			if (ret < 0) {
				break;
			}

			ret = send_response(&client, coap_ctx, &cdc_ctx);
			if (ret < 0) {
				break;
			}
			/* Provisioning finished */
			if (finished) {
				ret = NRF_PROVISIONING_FINISHED;
				break;
			}
			nrf_provisioning_codec_teardown();
			continue;
		} else if (coap_ctx->code == COAP_RESPONSE_CODE_UNAUTHORIZED) {
			LOG_ERR("Unauthorized");
			ret = -EACCES;
		} else if (coap_ctx->code == COAP_RESPONSE_CODE_INTERNAL_ERROR) {
			LOG_ERR("Internal error");
			ret = -EBUSY;
		} else if (coap_ctx->code == COAP_RESPONSE_CODE_NOT_ACCEPTABLE) {
			LOG_ERR("Not acceptable");
			ret = -EINVAL;
		} else if (coap_ctx->code == COAP_RESPONSE_CODE_FORBIDDEN) {
			LOG_ERR("Forbidden");
			ret = -EACCES;
		} else if (coap_ctx->code == COAP_RESPONSE_CODE_BAD_REQUEST) {
			LOG_ERR("Bad request");
			ret = -EINVAL;
		} else {
			LOG_ERR("Unknown response code %d", coap_ctx->code);
			ret = -ENOTSUP;
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
