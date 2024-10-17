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
#define ACPT_IDX 0
#define PRXY_IDX 1
#define OPT_CNT  2
/* CoAP option array */
static struct coap_client_option cc_opts[OPT_CNT] = {0};
/* CoAP client to be used for file downloads */
static struct nrf_cloud_coap_client coap_client;
/* Workqueue item used to resume a downlaod */
static struct k_work_delayable resume_work;

static void resume_work_fn(struct k_work *unused);

static int coap_dl_init(void)
{
	k_work_init_delayable(&resume_work, resume_work_fn);
	return nrf_cloud_coap_transport_init(&coap_client);
}

static int coap_dl_connect_and_auth(void)
{
	int ret = nrf_cloud_coap_transport_connect(&coap_client);

	if (ret) {
		LOG_ERR("CoAP connect failed, error; %d", ret);
		return -EIO;
	}

	ret = nrf_cloud_coap_transport_authenticate(&coap_client);
	if (ret) {
		LOG_ERR("CoAP authentication failed, error; %d", ret);
		return -EACCES;
	}

	return 0;
}

static int fota_dl_evt_send(const struct download_client_evt *evt)
{
#if defined(CONFIG_FOTA_DOWNLOAD_EXTERNAL_DL)
	return fota_download_external_evt_handle(evt);
#endif
	return -ENOTSUP;
}

static int coap_dl_event_send(struct nrf_cloud_download_data const *const dl,
			      const struct download_client_evt *const evt)
{
	/* Send events as if we are the downoad_client */
	if (dl->type == NRF_CLOUD_DL_TYPE_FOTA) {
		return fota_dl_evt_send(evt);
	} else if (dl->type == NRF_CLOUD_DL_TYPE_DL_CLIENT) {
		return dl->dlc->callback(evt);
	}

	return -EINVAL;
}

static int coap_dl_disconnect(void)
{
	return nrf_cloud_coap_transport_disconnect(&coap_client);
}

static void coap_dl_cb(int16_t result_code, size_t offset, const uint8_t *payload, size_t len,
		       bool last_block, void *user_data)
{
	int ret;
	bool send_closed_evt = false;
	bool send_done_evt = last_block;
	bool stop_on_err = false;
	struct nrf_cloud_download_data *dl = (struct nrf_cloud_download_data *)user_data;
	struct download_client_evt evt = {0};

	LOG_DBG("CoAP result: %d, offset: 0x%X, len: 0x%X, last_block: %d",
		result_code, offset, len, last_block);

	if (result_code == COAP_RESPONSE_CODE_CONTENT) {
		evt.id = DOWNLOAD_CLIENT_EVT_FRAGMENT;
		evt.fragment.buf = payload;
		evt.fragment.len = len;
	} else if (result_code == -ECANCELED) {
		LOG_DBG("CoAP request canceled");
		/* This is not actually an error, just use the error event to indicate that
		 * the transfer has been canceled
		 */
		evt.id = DOWNLOAD_CLIENT_EVT_ERROR;
		evt.error = -ECANCELED;
		(void)coap_dl_event_send(dl, &evt);
		return;
	} else if (result_code != COAP_RESPONSE_CODE_OK) {
		LOG_ERR("Unexpected CoAP result: %d", result_code);
		LOG_DBG("CoAP response: %.*s", len, payload);
		evt.id = DOWNLOAD_CLIENT_EVT_ERROR;
		/* Use -ECONNRESET to trigger retry mechanism used by fota_download and
		 * the P-GPS download event handler
		 */
		evt.error = -ECONNRESET;
		stop_on_err = true;
	}

	ret = coap_dl_event_send(dl, &evt);

	if (evt.id == DOWNLOAD_CLIENT_EVT_FRAGMENT) {
		if (ret == 0) {
			/* Fragment was successfully processed */
			dl->coap_rcvd_bytes += len;
		} else if ((ret == -1) && (dl->type == NRF_CLOUD_DL_TYPE_FOTA)) {
			/* For FOTA, -1 on a fragment event requires a closed event
			 * that will generate a request for restart.
			 */
			send_closed_evt = true;
			send_done_evt = false;
			ret = 0;
		}
	} else if ((ret == 0) && (evt.error == -ECONNRESET)) {
		/* Retry if the event handler returned zero during an error event */
		dl->resume_offset = dl->coap_rcvd_bytes;
		k_work_schedule(&resume_work, K_SECONDS(1));
		return;
	}

	if (ret || stop_on_err) {
		send_closed_evt = true;
		send_done_evt = false;
		stop_on_err = true;
	}

	if (send_done_evt) {
		LOG_INF("Download complete");

		memset(&evt, 0, sizeof(evt));
		evt.id = DOWNLOAD_CLIENT_EVT_DONE;

		ret = coap_dl_event_send(dl, &evt);
		if (ret) {
			/* Send a closed event on error */
			send_closed_evt = true;
			stop_on_err = true;
		}

		ret = coap_dl_disconnect();
		if (ret && (ret != -ENOTCONN)) {
			LOG_WRN("Failed to disconnect CoAP transport, error: %d", ret);
		}

		if (dl->type == NRF_CLOUD_DL_TYPE_FOTA) {
			/* fota_download expects a closed event when the download is done */
			send_closed_evt = true;
		}
	}

	if (send_closed_evt) {
		memset(&evt, 0, sizeof(evt));
		evt.id = DOWNLOAD_CLIENT_EVT_CLOSED;
		(void)coap_dl_event_send(dl, &evt);
	}

	if (stop_on_err) {
		LOG_ERR("CoAP download error, stopping download");
		nrf_cloud_download_end();
	}
}

#define MAX_RETRIES 5
#define PROXY_RSC_OFFSET_TMPLT		NRF_CLOUD_COAP_PROXY_RSC "?offset=%u"
#define PROXY_RSC_BUF_SZ		sizeof(PROXY_RSC_OFFSET_TMPLT) + 10
static int coap_dl_start(struct nrf_cloud_download_data *const dl, const size_t offset)
{
	static char rsc_path_offset[PROXY_RSC_BUF_SZ];

	int err;
	int retry = 0;
	struct coap_client *cc = &coap_client.cc;

	struct coap_client_request request = {
		.method = COAP_METHOD_GET,
		.confirmable = true,
		.path = NRF_CLOUD_COAP_PROXY_RSC,
		.fmt = COAP_CONTENT_FORMAT_APP_OCTET_STREAM,
		.payload = NULL,
		.len = 0,
		.cb = coap_dl_cb,
		.user_data = dl,
		.options = cc_opts,
		.num_options = OPT_CNT
	};

	if (offset > 0) {
		/* Use the offset parameter in the resource path */
		err = snprintk(rsc_path_offset, sizeof(rsc_path_offset),
			       PROXY_RSC_OFFSET_TMPLT, offset);
		if ((err < 0) || (err >= sizeof(rsc_path_offset))) {
			LOG_ERR("Could not format CoAP proxy download resource");
			return -EIO;
		}
		request.path = rsc_path_offset;
	}

	while ((err = coap_client_req(cc, cc->fd, NULL, &request, NULL)) == -EAGAIN) {
		if (retry++ > MAX_RETRIES) {
			LOG_ERR("Timeout waiting for CoAP client to be available");
			return -ETIMEDOUT;
		}
		LOG_DBG("CoAP client busy");
		k_sleep(K_MSEC(500));
	}

	if (err == 0) {
		LOG_DBG("Sent CoAP download request, offset: %u", offset);
	} else {
		LOG_ERR("Error sending CoAP request: %d", err);
	}

	return err;
}

static int coap_dl(struct nrf_cloud_download_data *const dl)
{
#if defined(CONFIG_FOTA_DOWNLOAD) && defined(CONFIG_NRF_CLOUD_FOTA_TYPE_BOOT_SUPPORTED)
	static char buf[CONFIG_FOTA_DOWNLOAD_RESOURCE_LOCATOR_LENGTH];
#endif
	const char *file_path = dl->path;
	int ret = coap_dl_init();

	if (ret) {
		LOG_ERR("Failed to initialize CoAP download, error: %d", ret);
		return ret;
	}

	ret = coap_dl_connect_and_auth();
	if (ret) {
		return ret;
	}

#if defined(CONFIG_FOTA_DOWNLOAD) && defined(CONFIG_NRF_CLOUD_FOTA_TYPE_BOOT_SUPPORTED)
	if (dl->type == NRF_CLOUD_DL_TYPE_FOTA) {
		/* Copy file path to modifiable buffer and check for a bootloader file */
		memcpy(buf, dl->path, strlen(dl->path) + 1);
		ret = fota_download_b1_file_parse(buf);

		if (ret == 0) {
			/* A bootload file has been found */
			file_path = buf;
		}
	}
#endif
	/* Reset the received count */
	dl->coap_rcvd_bytes = 0;
	dl->resume_offset = 0;

	/* Get the options for the proxy download */
	ret = nrf_cloud_coap_transport_proxy_dl_opts_get(&cc_opts[ACPT_IDX],
							 &cc_opts[PRXY_IDX],
							 dl->host, file_path);
	if (ret) {
		LOG_ERR("Failed to set CoAP options, error: %d", ret);
		return ret;
	}

	if (dl->type == NRF_CLOUD_DL_TYPE_FOTA) {
#if defined(CONFIG_FOTA_DOWNLOAD_EXTERNAL_DL)
		ret = fota_download_external_start(dl->host, file_path, dl->fota.expected_type,
						  (size_t)dl->fota.img_sz);
		if (ret) {
			LOG_ERR("Failed to start FOTA download, error: %d", ret);
			return ret;
		}
#else
		return -ENOTSUP;
#endif
	}

	return coap_dl_start(dl, 0);
}

int nrf_cloud_download_coap_offset_resume(const size_t offset)
{
	int err = 0;

	k_mutex_lock(&active_dl_mutex, K_FOREVER);

	if ((active_dl.type == NRF_CLOUD_DL_TYPE_FOTA) ||
	    (active_dl.type == NRF_CLOUD_DL_TYPE_DL_CLIENT)) {
		active_dl.resume_offset = offset;
		k_work_schedule(&resume_work, K_SECONDS(1));
	} else if (active_dl.type == NRF_CLOUD_DL_TYPE_NONE) {
		LOG_ERR("No active download, cannot resume");
		err = -ENODEV;
	} else {
		LOG_ERR("Invalid download type (%d), cannot resume", active_dl.type);
		err = -EINVAL;
	}

	k_mutex_unlock(&active_dl_mutex);

	return err;
}

static void resume_work_fn(struct k_work *unused)
{
	int ret;

	if (active_dl.type == NRF_CLOUD_DL_TYPE_NONE) {
		LOG_ERR("No active download, cannot resume");
		return;
	}

	/* Ensure current request is canceled */
	coap_client_cancel_requests(&coap_client.cc);

	ret = coap_dl_connect_and_auth();
	if (ret == 0) {
		LOG_DBG("Resuming download at offset: %u", active_dl.resume_offset);
		ret = coap_dl_start(&active_dl, active_dl.resume_offset);
	}

	/* On failure, send the events required to generate the error/done status */
	if (ret) {
		struct download_client_evt evt = {0};

		LOG_ERR("Failed to resume CoAP download");

		/* Send a non-recoverable error event (not ECONN) */
		evt.id = DOWNLOAD_CLIENT_EVT_ERROR;
		evt.error = -EIO;
		(void)coap_dl_event_send(&active_dl, &evt);

		/* Send a closed event to ensure the terminal fota_download event is generated */
		if (active_dl.type == NRF_CLOUD_DL_TYPE_FOTA) {
			memset(&evt, 0, sizeof(evt));
			evt.id = DOWNLOAD_CLIENT_EVT_CLOSED;
			(void)coap_dl_event_send(&active_dl, &evt);
		}
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
		dl->dl_cfg.sec_tag_count ? dl->dl_cfg.sec_tag_list[0] : -1,
		dl->dl_cfg.pdn_id, dl->dl_cfg.frag_size_override, dl->fota.expected_type);

#endif /* CONFIG_FOTA_DOWNLOAD */

	return -ENOTSUP;
}

static int dlc_start(struct nrf_cloud_download_data *const dl)
{
	__ASSERT(dl->dlc != NULL, "Download client is NULL");
	__ASSERT(dl->dlc->callback != NULL, "Download client callback is NULL");

#if defined(CONFIG_NRF_CLOUD_COAP_DOWNLOADS)
	return coap_dl(dl);
#endif /* CONFIG_NRF_CLOUD_COAP_DOWNLOADS */

	return download_client_get(dl->dlc, dl->host, &dl->dl_cfg, dl->path, 0);
}

static int dlc_disconnect(struct nrf_cloud_download_data *const dl)
{
#if defined(CONFIG_NRF_CLOUD_COAP_DOWNLOADS)
	return coap_dl_disconnect();
#endif /* CONFIG_NRF_CLOUD_COAP_DOWNLOADS */

	return download_client_disconnect(dl->dlc);
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
		ret = dlc_disconnect(&active_dl);
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
#if defined(CONFIG_NRF_CLOUD_COAP_DOWNLOADS)
	coap_dl_disconnect();
#endif

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

int nrf_cloud_download_start(struct nrf_cloud_download_data *const dl)
{
	if (!dl || !dl->path || (dl->type <= NRF_CLOUD_DL_TYPE_NONE) ||
	    (dl->type >= NRF_CLOUD_DL_TYPE_DL__LAST)) {
		return -EINVAL;
	}

	if (dl->type == NRF_CLOUD_DL_TYPE_FOTA) {
		if (!IS_ENABLED(CONFIG_FOTA_DOWNLOAD)) {
			return -ENOTSUP;
		}
		if (!dl->fota.cb) {
			return -ENOEXEC;
		}
		if ((dl->fota.expected_type == DFU_TARGET_IMAGE_TYPE_SMP) &&
		    !IS_ENABLED(CONFIG_NRF_CLOUD_FOTA_SMP)) {
			return -ENOSYS;
		}
	}

	if (!check_fota_file_path_len(dl->path)) {
		LOG_ERR("FOTA download file path is too long");
		return -E2BIG;
	}

	int ret = 0;

	k_mutex_lock(&active_dl_mutex, K_FOREVER);

	/* FOTA has priority */
	if ((active_dl.type == NRF_CLOUD_DL_TYPE_FOTA) ||
	    ((active_dl.type != NRF_CLOUD_DL_TYPE_NONE) &&
	     (dl->type != NRF_CLOUD_DL_TYPE_FOTA))) {
		k_mutex_unlock(&active_dl_mutex);
		/* A download of equal or higher priority is already active. */
		return -EBUSY;
	}

	/* If a download is active, that means the incoming download request is a FOTA
	 * type, which has priority. Cancel the active download.
	 */
	if (active_dl.type == NRF_CLOUD_DL_TYPE_DL_CLIENT) {
		LOG_INF("Stopping active download, incoming FOTA update download has priority");
		ret = dlc_disconnect(&active_dl);

		if (ret) {
			LOG_ERR("Download disconnect failed, error %d", ret);
		}
	}

	active_dl = *dl;

	if (active_dl.type == NRF_CLOUD_DL_TYPE_FOTA) {
		ret = fota_start(&active_dl);
	} else if (active_dl.type == NRF_CLOUD_DL_TYPE_DL_CLIENT) {
		ret = dlc_start(&active_dl);
		if (ret) {
			(void)dlc_disconnect(&active_dl);
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
