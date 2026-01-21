/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#if defined(CONFIG_FOTA_DOWNLOAD)
#include <net/fota_download.h>
#endif
#include "nrf_cloud_coap_transport.h"
#include <zephyr/net/coap.h>
#include <zephyr/net/socket.h>
#include <zephyr/random/random.h>
#include <errno.h>
#include "nrf_cloud_mem.h"
#if defined(CONFIG_NRF_CLOUD_FOTA_SMP)
#include <mcumgr_smp_client.h>
#include <fota_download_util.h>
#endif
#include "nrf_cloud_download.h"

LOG_MODULE_REGISTER(nrf_cloud_download_coap, CONFIG_NRF_CLOUD_LOG_LEVEL);

#if defined(CONFIG_NRF_CLOUD_PGPS)
BUILD_ASSERT(
	CONFIG_COAP_EXTENDED_OPTIONS_LEN_VALUE >= 80,
	"CONFIG_COAP_EXTENDED_OPTIONS_LEN_VALUE should be at least 80 for CoAP P-GPS downloads");
#endif

#if defined(CONFIG_NRF_CLOUD_FOTA_POLL)
BUILD_ASSERT(
	CONFIG_COAP_EXTENDED_OPTIONS_LEN_VALUE >= 192,
	"CONFIG_COAP_EXTENDED_OPTIONS_LEN_VALUE should be at least 192 for CoAP FOTA downloads");
#endif

const uint8_t coap_auth_request_template[] = {
	0x48, 0x02,					/* CoAP POST */
	0xFF, 0xFF,					/* Message ID */
	0x90, 0x4A, 0x50, 0x61, 0xBF, 0x40, 0x33, 0x40, /* Token */
	0xB4, 0x61, 0x75, 0x74, 0x68,			/* URI: "auth" */
	0x03, 0x6A, 0x77, 0x74,				/* URI "jwt" */
	0x10,						/* Content-Type Text */
	0xFF,						/* Payload-Marker */
};
#define JWT_BUF_SZ		   600
#define COAP_AUTH_REQ_HEADER_SIZE  (sizeof(coap_auth_request_template))
#define COAP_AUTH_REQ_BUF_SIZE	   (JWT_BUF_SZ + COAP_AUTH_REQ_HEADER_SIZE)
#define COAP_AUTH_REQ_MID_OFFSET   2
#define COAP_AUTH_REQ_TOKEN_OFFSET 4
#define COAP_REQ_WAIT_TIME_MS	   300

static int handle_coap_auth(int socket)
{
	int32_t err = 0;
	struct coap_packet reply;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t *packet_buf = nrf_cloud_malloc(COAP_AUTH_REQ_BUF_SIZE);
	uint8_t response_token[COAP_TOKEN_MAX_LEN];
	uint16_t mid = sys_cpu_to_be16(coap_next_id());
	size_t request_size;

	if (!packet_buf) {
		LOG_ERR("Failed to allocate memory for CoAP auth request buffer");
		err = -ENOMEM;
		goto end;
	}

	/* Make new random token */
	sys_rand_get(token, COAP_TOKEN_MAX_LEN);

	/* Fill in CoAP header */
	memcpy(packet_buf, coap_auth_request_template, COAP_AUTH_REQ_HEADER_SIZE);
	memcpy(packet_buf + COAP_AUTH_REQ_MID_OFFSET, &mid, sizeof(mid));
	memcpy(packet_buf + COAP_AUTH_REQ_TOKEN_OFFSET, token, COAP_TOKEN_MAX_LEN);

	/* Generate JWT */
	err = nrf_cloud_jwt_generate(NRF_CLOUD_JWT_VALID_TIME_S_MAX,
				     packet_buf + COAP_AUTH_REQ_HEADER_SIZE, JWT_BUF_SZ);
	if (err) {
		LOG_ERR("Error generating JWT with modem: %d", err);
		goto end;
	}

	request_size = COAP_AUTH_REQ_HEADER_SIZE +
		       strnlen((char *)packet_buf + COAP_AUTH_REQ_HEADER_SIZE, JWT_BUF_SZ);

	/* Send the request */
	LOG_DBG("Sending CoAP auth request, size %zu", request_size);
	err = zsock_send(socket, packet_buf, request_size, 0);
	if (err < 0) {
		LOG_ERR("Failed to send CoAP request, errno %d", errno);
		goto end;
	}

	for (size_t i = 0; i < 10; ++i) {
		k_sleep(K_MSEC(COAP_REQ_WAIT_TIME_MS));
		/* Poll for response */
		err = zsock_recv(socket, packet_buf, COAP_AUTH_REQ_BUF_SIZE, ZSOCK_MSG_DONTWAIT);
		if (err < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				continue;
			} else {
				err = -errno;
				LOG_ERR("Error receiving response: %d", err);
				goto end;
			}
		}
		LOG_DBG("Received CoAP auth response, size %d", err);
		/* Parse response */
		err = coap_packet_parse(&reply, packet_buf, err, NULL, 0);
		if (err < 0) {
			LOG_ERR("Error parsing response: %d", err);
			continue;
		}
		/* Match token */
		coap_header_get_token(&reply, response_token);
		if (memcmp(response_token, token, COAP_TOKEN_MAX_LEN) != 0) {
			continue;
		}
		/* Check response code */
		if (coap_header_get_code(&reply) != COAP_RESPONSE_CODE_CREATED) {
			LOG_ERR("Error in response: %d", coap_header_get_code(&reply));
			continue;
		} else {
			err = 0;
			goto end;
		}
	}
	err = -ETIMEDOUT;

end:
	nrf_cloud_free(packet_buf);
	return err;
}

int nrf_cloud_download_start_internal(struct nrf_cloud_download_data *const dl)
{
	static char proxy_uri[CONFIG_FOTA_DOWNLOAD_RESOURCE_LOCATOR_LENGTH];
#if defined(CONFIG_FOTA_DOWNLOAD) && defined(CONFIG_NRF_CLOUD_FOTA_TYPE_BOOT_SUPPORTED)
	char buf[CONFIG_FOTA_DOWNLOAD_RESOURCE_LOCATOR_LENGTH];
#endif
	const char *file_path = dl->path;
	int ret = 0;
	struct downloader_host_cfg host_cfg = {
		.cid = true,
		.auth_cb = handle_coap_auth,
		.proxy_uri = proxy_uri,
	};

	if (ret) {
		LOG_ERR("Failed to initialize CoAP download, error: %d", ret);
		return ret;
	}

#if defined(CONFIG_FOTA_DOWNLOAD) && defined(CONFIG_NRF_CLOUD_FOTA_TYPE_BOOT_SUPPORTED)
	if (dl->type == NRF_CLOUD_DL_TYPE_FOTA) {
		/* Copy file path to modifiable buffer and check for a bootloader file */
		memcpy(buf, dl->path, strlen(dl->path) + 1);
		ret = fota_download_b1_file_parse(buf);

		if (ret == 0) {
			/* A bootloader file has been found */
			file_path = buf;
		}
	}
#endif
	ret = nrf_cloud_coap_transport_proxy_dl_uri_get(proxy_uri, ARRAY_SIZE(proxy_uri), dl->host,
							file_path);
	if (ret) {
		LOG_ERR("Failed to get CoAP proxy URL, error: %d", ret);
		return ret;
	}

	const char *host = "coaps://" CONFIG_NRF_CLOUD_COAP_SERVER_HOSTNAME;
	const char *file = "proxy";

	if (dl->type == NRF_CLOUD_DL_TYPE_FOTA) {
		return fota_download_with_host_cfg(
			host, file,
			dl->dl_host_conf.sec_tag_count ? dl->dl_host_conf.sec_tag_list[0] : -1,
			dl->dl_host_conf.pdn_id, dl->dl_host_conf.range_override,
			dl->fota.expected_type, &host_cfg);
	} else {
		/* If not downloading FOTA, use downloader directly */
		dl->dl_host_conf.cid = true;
		dl->dl_host_conf.auth_cb = handle_coap_auth;
		dl->dl_host_conf.proxy_uri = proxy_uri;

		return downloader_get_with_host_and_file(dl->dl, &dl->dl_host_conf, host, file, 0);
	}
}
