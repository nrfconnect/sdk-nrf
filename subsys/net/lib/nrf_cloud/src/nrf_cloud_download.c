/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#if defined(CONFIG_FOTA_DOWNLOAD)
#include <net/fota_download.h>
#endif
#if defined(CONFIG_NRF_CLOUD_COAP)
#include "../coap/include/nrf_cloud_coap_transport.h"
#endif
#if defined(CONFIG_NRF_CLOUD_FOTA_SMP)
#include <mcumgr_smp_client.h>
#include <fota_download_util.h>
#endif
#include "nrf_cloud_download.h"

LOG_MODULE_REGISTER(nrf_cloud_download, CONFIG_NRF_CLOUD_LOG_LEVEL);

#if defined(CONFIG_NRF_CLOUD_COAP_DOWNLOADS) && defined(CONFIG_NRF_CLOUD_PGPS)
BUILD_ASSERT(CONFIG_COAP_EXTENDED_OPTIONS_LEN_VALUE >= 80,
	"CONFIG_COAP_EXTENDED_OPTIONS_LEN_VALUE should be at least 80 for CoAP P-GPS downloads");
#endif

#if defined(CONFIG_NRF_CLOUD_COAP_DOWNLOADS) && defined(CONFIG_NRF_CLOUD_FOTA_POLL)
BUILD_ASSERT(CONFIG_COAP_EXTENDED_OPTIONS_LEN_VALUE >= 192,
	"CONFIG_COAP_EXTENDED_OPTIONS_LEN_VALUE should be at least 192 for CoAP FOTA downloads");
#endif

static K_MUTEX_DEFINE(active_dl_mutex);
static struct nrf_cloud_download_data active_dl = { .type = NRF_CLOUD_DL_TYPE_NONE };

#if defined(CONFIG_NRF_CLOUD_COAP_DOWNLOADS)
/* CoAP client to be used for file downloads */
static struct nrf_cloud_coap_client coap_client;

int nrf_cloud_download_handle_coap_auth(int socket)
{
	int err = 0;
	struct nrf_cloud_coap_client *client = &coap_client;

	/* Initialize client */
	err = nrf_cloud_coap_transport_init(client);
	if (err) {
		LOG_ERR("Failed to initialize CoAP client, error: %d", err);
		return err;
	}

	/* We are already connected using the given socket */
	k_mutex_lock(&client->mutex, K_FOREVER);
	client->sock = socket;
	client->cc.fd = socket;
	k_mutex_unlock(&client->mutex);

	/* Authenticate */
	err = nrf_cloud_coap_transport_authenticate(client);
	if (err) {
		LOG_ERR("Failed to authenticate CoAP client, error: %d", err);
		return err;
	}

	/* Clean up client */
	k_mutex_lock(&client->mutex, K_FOREVER);
	client->sock = -1;
	client->cc.fd = -1;
	client->authenticated = false;
	k_mutex_unlock(&client->mutex);

	return 0;
}

static int coap_dl(struct nrf_cloud_download_data *const dl)
{
	static char proxy_uri[CONFIG_FOTA_DOWNLOAD_RESOURCE_LOCATOR_LENGTH];
#if defined(CONFIG_FOTA_DOWNLOAD) && defined(CONFIG_NRF_CLOUD_FOTA_TYPE_BOOT_SUPPORTED)
	char buf[CONFIG_FOTA_DOWNLOAD_RESOURCE_LOCATOR_LENGTH];
#endif
	const char *file_path = dl->path;
	int ret = 0;
	struct downloader_host_cfg host_cfg = {
		.cid = true,
		.auth_cb = nrf_cloud_download_handle_coap_auth,
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
	ret = nrf_cloud_coap_transport_proxy_dl_uri_get(
		proxy_uri, ARRAY_SIZE(proxy_uri), dl->host, file_path);
	if (ret) {
		LOG_ERR("Failed to get CoAP proxy URL, error: %d", ret);
		return ret;
	}

	const char *host = "coaps://" CONFIG_NRF_CLOUD_COAP_SERVER_HOSTNAME;
	const char *file = "proxy";

	if (dl->type == NRF_CLOUD_DL_TYPE_FOTA) {
		return fota_download_with_host_cfg(host,
						   file, dl->dl_host_conf.sec_tag_count ?
						   dl->dl_host_conf.sec_tag_list[0] : -1,
						   dl->dl_host_conf.pdn_id,
						   dl->dl_host_conf.range_override,
						   dl->fota.expected_type, &host_cfg);
	} else {
		/* If not downloading FOTA, use downloader directly */
		dl->dl_host_conf.cid = true;
		dl->dl_host_conf.auth_cb = nrf_cloud_download_handle_coap_auth;
		dl->dl_host_conf.proxy_uri = proxy_uri;

		return downloader_get_with_host_and_file(
			dl->dl, &dl->dl_host_conf, host, file, 0
		);
	}

}
#endif /* NRF_CLOUD_COAP_DOWNLOADS */

#if defined(CONFIG_FOTA_DOWNLOAD)
static int smp_fota_dl_cancel(void)
{
#if defined(CONFIG_NRF_CLOUD_FOTA_SMP)
	return mcumgr_smp_client_download_cancel();
#else
	return -ENOTSUP;
#endif /* CONFIG_NRF_CLOUD_FOTA_SMP */
}

static void smp_fota_dl_util_init(const struct nrf_cloud_download_fota *const fota)
{
#if defined(CONFIG_NRF_CLOUD_FOTA_SMP)
	fota_download_util_client_init(fota->cb, true);
#endif
}
#endif /* CONFIG_FOTA_DOWNLOAD */

static int fota_start(struct nrf_cloud_download_data *const dl)
{
#if defined(CONFIG_FOTA_DOWNLOAD)

	if (dl->fota.expected_type == DFU_TARGET_IMAGE_TYPE_SMP) {
		/* This needs to be called to set the SMP flag in fota_download */
		smp_fota_dl_util_init(&dl->fota);
	} else {
		/* This needs to be called to clear the SMP flag */
		(void)fota_download_init(dl->fota.cb);
	}

	/* Download using CoAP if enabled */
#if defined(CONFIG_NRF_CLOUD_COAP_DOWNLOADS)
	return coap_dl(dl);
#endif /* CONFIG_NRF_CLOUD_COAP_DOWNLOADS */

	return fota_download_start_with_image_type(dl->host, dl->path,
		dl->dl_host_conf.sec_tag_count ? dl->dl_host_conf.sec_tag_list[0] : -1,
		dl->dl_host_conf.pdn_id, dl->dl_host_conf.range_override, dl->fota.expected_type);

#endif /* CONFIG_FOTA_DOWNLOAD */

	return -ENOTSUP;
}

static int dl_start(struct nrf_cloud_download_data *const cloud_dl)
{
	__ASSERT(cloud_dl->dl != NULL, "Download client is NULL");

#if defined(CONFIG_NRF_CLOUD_COAP_DOWNLOADS)
	return coap_dl(cloud_dl);
#endif /* CONFIG_NRF_CLOUD_COAP_DOWNLOADS */

	return downloader_get_with_host_and_file(cloud_dl->dl, &cloud_dl->dl_host_conf,
						cloud_dl->host, cloud_dl->path, 0);
}

static int dl_disconnect(struct nrf_cloud_download_data *const dl)
{
	return downloader_cancel(dl->dl);
}

static void active_dl_reset(void)
{
	memset(&active_dl, 0, sizeof(active_dl));
	active_dl.type = NRF_CLOUD_DL_TYPE_NONE;
}

static int fota_dl_cancel(struct nrf_cloud_download_data *const dl)
{
	int ret = -ENOTSUP;

#if defined(CONFIG_FOTA_DOWNLOAD)
	if (dl->fota.expected_type == DFU_TARGET_IMAGE_TYPE_SMP) {
		ret = smp_fota_dl_cancel();
	} else {
		ret = fota_download_cancel();
	}
#endif /* CONFIG_FOTA_DOWNLOAD */
	return ret;
}

void nrf_cloud_download_cancel(void)
{
	int ret = 0;

	k_mutex_lock(&active_dl_mutex, K_FOREVER);

	if (active_dl.type == NRF_CLOUD_DL_TYPE_FOTA) {
		ret = fota_dl_cancel(&active_dl);
	} else if (active_dl.type == NRF_CLOUD_DL_TYPE_DL_CLIENT) {
		ret = dl_disconnect(&active_dl);
	} else {
		LOG_WRN("No active download to cancel");
	}

	if (ret) {
		LOG_WRN("Error canceling download: %d", ret);
	}

	active_dl_reset();
	k_mutex_unlock(&active_dl_mutex);
}

void nrf_cloud_download_end(void)
{
	k_mutex_lock(&active_dl_mutex, K_FOREVER);
	active_dl_reset();
	k_mutex_unlock(&active_dl_mutex);
}

static bool check_fota_file_path_len(char const *const file_path)
{
#if defined(CONFIG_FOTA_DOWNLOAD)
	/* Check that the null-terminator is found in the allowable buffer length */
	return memchr(file_path, '\0', CONFIG_FOTA_DOWNLOAD_RESOURCE_LOCATOR_LENGTH) != NULL;
#endif
	return true;
}

int nrf_cloud_download_start(struct nrf_cloud_download_data *const cloud_dl)
{
	int ret = 0;

	if (!cloud_dl || !cloud_dl->path || (cloud_dl->type <= NRF_CLOUD_DL_TYPE_NONE) ||
	    (cloud_dl->type >= NRF_CLOUD_DL_TYPE_DL__LAST)) {
		return -EINVAL;
	}

	if (cloud_dl->type == NRF_CLOUD_DL_TYPE_FOTA) {
		if (!IS_ENABLED(CONFIG_FOTA_DOWNLOAD)) {
			return -ENOTSUP;
		}
		if (!cloud_dl->fota.cb) {
			return -ENOEXEC;
		}
		if ((cloud_dl->fota.expected_type == DFU_TARGET_IMAGE_TYPE_SMP) &&
		    !IS_ENABLED(CONFIG_NRF_CLOUD_FOTA_SMP)) {
			return -ENOSYS;
		}
	}

	if (!check_fota_file_path_len(cloud_dl->path)) {
		LOG_ERR("FOTA download file path is too long");
		return -E2BIG;
	}

	k_mutex_lock(&active_dl_mutex, K_FOREVER);

	/* FOTA has priority */
	if ((active_dl.type == NRF_CLOUD_DL_TYPE_FOTA) ||
	    ((active_dl.type != NRF_CLOUD_DL_TYPE_NONE) &&
	     (cloud_dl->type != NRF_CLOUD_DL_TYPE_FOTA))) {
		k_mutex_unlock(&active_dl_mutex);
		/* A download of equal or higher priority is already active. */
		return -EBUSY;
	}

	/* If a download is active, that means the incoming download request is a FOTA
	 * type, which has priority. Cancel the active download.
	 */
	if (active_dl.type == NRF_CLOUD_DL_TYPE_DL_CLIENT) {
		LOG_INF("Stopping active download, incoming FOTA update download has priority");
		ret = dl_disconnect(&active_dl);

		if (ret) {
			LOG_ERR("Download disconnect failed, error %d", ret);
		}
	}

	active_dl = *cloud_dl;

	if (active_dl.type == NRF_CLOUD_DL_TYPE_FOTA) {
		ret = fota_start(&active_dl);
	} else if (active_dl.type == NRF_CLOUD_DL_TYPE_DL_CLIENT) {
		ret = dl_start(&active_dl);
		if (ret) {
			(void)dl_disconnect(&active_dl);
		}
	} else {
		LOG_WRN("Unhandled download type: %d", active_dl.type);
		ret = -EFTYPE;
	}

	if (ret != 0) {
		active_dl_reset();
	}

	k_mutex_unlock(&active_dl_mutex);

	return ret;
}
