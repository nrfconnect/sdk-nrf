/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/toolchain.h>
#include <net/downloader.h>
#include <net/downloader_transport.h>

#include "dl_parse.h"
#include "dl_socket.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(downloader, CONFIG_DOWNLOADER_LOG_LEVEL);

static int stopped_evt_send(struct downloader *dl);

#define FALLBACK_HTTP  "http://"
#define FALLBACK_HTTPS "https://"

#define STATE_ALLOW_ANY 0xffffffff

#if defined(CONFIG_DOWNLOADER_LOG_LEVEL_WRN)
char *state_to_str(int state)
{
	switch (state) {
	case DOWNLOADER_IDLE:
		return "IDLE";
	case DOWNLOADER_CONNECTING:
		return "CONNECTING";
	case DOWNLOADER_CONNECTED:
		return "CONNECTED";
	case DOWNLOADER_DOWNLOADING:
		return "DOWNLOADING";
	case DOWNLOADER_STOPPING:
		return "STOPPING";
	case DOWNLOADER_DEINITIALIZING:
		return "DEINITIALIZING";
	case DOWNLOADER_DEINITIALIZED:
		return "DEINITIALIZED";
	}

	return "unknown";
}
#else
char *state_to_str(int state)
{
	return "";
}
#endif

static void state_set(struct downloader *dl, unsigned int before_state, unsigned int new_state)
{
	k_mutex_lock(&dl->mutex, K_FOREVER);
	if ((dl->state != before_state) && (before_state != STATE_ALLOW_ANY)) {
		k_mutex_unlock(&dl->mutex);
		return;
	}

	dl->state = new_state;
	k_mutex_unlock(&dl->mutex);
	LOG_DBG("state = %d (%s)", new_state, state_to_str(new_state));
}

static bool is_state(struct downloader *dl, enum downloader_state state)
{
	bool ret;

	k_mutex_lock(&dl->mutex, K_FOREVER);
	ret = dl->state == state;
	k_mutex_unlock(&dl->mutex);
	return ret;
}

static bool is_valid_state(struct downloader *dl, enum downloader_state state)
{
	k_mutex_lock(&dl->mutex, K_FOREVER);
	switch (state) {
	case DOWNLOADER_DEINITIALIZED:
	case DOWNLOADER_IDLE:
	case DOWNLOADER_CONNECTING:
	case DOWNLOADER_CONNECTED:
	case DOWNLOADER_DOWNLOADING:
	case DOWNLOADER_STOPPING:
	case DOWNLOADER_DEINITIALIZING:
		k_mutex_unlock(&dl->mutex);
		return true;
	default:
		break;
	}

	k_mutex_unlock(&dl->mutex);
	return false;
}

static int transport_init(struct downloader *dl, struct downloader_host_cfg *dl_host_cfg,
			  const char *url)
{
	__ASSERT_NO_MSG(dl && dl->transport);

	return dl->transport->init(dl, dl_host_cfg, url);
}

static int transport_deinit(struct downloader *dl)
{
	int err;

	__ASSERT_NO_MSG(dl && dl->transport);

	err = dl->transport->deinit(dl);
	dl->transport = NULL;

	return err;
}

static int transport_connect(struct downloader *dl)
{
	__ASSERT_NO_MSG(dl && dl->transport);

	return dl->transport->connect(dl);
}

static int transport_close(struct downloader *dl)
{
	__ASSERT_NO_MSG(dl && dl->transport);

	return dl->transport->close(dl);
}

static int transport_download(struct downloader *dl)
{
	__ASSERT_NO_MSG(dl && dl->transport);

	return dl->transport->download(dl);
}

static int reconnect(struct downloader *dl)
{
	int err = 0;

	LOG_DBG("Reconnecting...");

	err = transport_close(dl);
	if (err) {
		LOG_DBG("disconnect failed, %d", err);
	}

	return  transport_connect(dl);
}

static void restart_and_suspend(struct downloader *dl)
{
	if (!is_state(dl, DOWNLOADER_DOWNLOADING)) {
		return;
	}

	if (!dl->host_cfg.keep_connection) {
		transport_close(dl);
		if (!dl->complete) {
			stopped_evt_send(dl);
		}

		state_set(dl, DOWNLOADER_DOWNLOADING, DOWNLOADER_IDLE);
		return;
	}

	if (!dl->complete) {
		stopped_evt_send(dl);
	}
	state_set(dl, DOWNLOADER_DOWNLOADING, DOWNLOADER_CONNECTED);
}

static int data_evt_send(const struct downloader *dl, void *data, size_t len)
{
	const struct downloader_evt evt = {.id = DOWNLOADER_EVT_FRAGMENT,
					   .fragment = {
						   .buf = data,
						   .len = len,
					   }};

	return dl->cfg.callback(&evt);
}

static int download_complete_evt_send(const struct downloader *dl)
{
	const struct downloader_evt evt = {
		.id = DOWNLOADER_EVT_DONE,
	};

	return dl->cfg.callback(&evt);
}

static int stopped_evt_send(struct downloader *dl)
{
	const struct downloader_evt evt = {
		.id = DOWNLOADER_EVT_STOPPED,
	};

	return dl->cfg.callback(&evt);
}

static int error_evt_send(const struct downloader *dl, int error)
{
	/* Error will be sent as negative. */
	__ASSERT_NO_MSG(error < 0);

	const struct downloader_evt evt = {.id = DOWNLOADER_EVT_ERROR, .error = error};

	return dl->cfg.callback(&evt);
}

static int deinit_evt_send(const struct downloader *dl)
{
	const struct downloader_evt evt = {
		.id = DOWNLOADER_EVT_DEINITIALIZED,
	};

	return dl->cfg.callback(&evt);
}

/* Events from the transport */
int dl_transport_evt_data(struct downloader *dl, void *data, size_t len)
{
	int err;

	LOG_DBG("Read %d bytes from transport", len);

	if (dl->file_size) {
		LOG_INF("Downloaded %u/%u bytes (%d%%)", dl->progress, dl->file_size,
			(dl->progress * 100) / dl->file_size);
	} else {
		LOG_INF("Downloaded %u bytes", dl->progress);
	}

	err = data_evt_send(dl, data, len);
	if (err) {
		/* Application refused data, suspend */
		restart_and_suspend(dl);
	}

	return 0;
}

void download_thread(void *cli, void *a, void *b)
{
	int rc, rc2;
	struct downloader *const dl = cli;

	while (true) {
		rc = 0;

		if (is_state(dl, DOWNLOADER_IDLE)) {
			/* Client idle, wait for action */
			k_sem_take(&dl->event_sem, K_FOREVER);
		}

		/* Connect to the target host */
		if (is_state(dl, DOWNLOADER_CONNECTING)) {
			/* Client */
			rc = transport_connect(dl);
			if (rc) {
				rc = error_evt_send(dl, rc);
				if (rc) {
					stopped_evt_send(dl);
					state_set(dl, DOWNLOADER_CONNECTING, DOWNLOADER_IDLE);
					continue;
				}
				continue;
			}

			/* Connection successful */
			state_set(dl, DOWNLOADER_CONNECTING, DOWNLOADER_DOWNLOADING);
		}

		if (is_state(dl, DOWNLOADER_CONNECTED)) {
			/* Client connected, wait for action */
			k_sem_take(&dl->event_sem, K_FOREVER);
		}

		if (is_state(dl, DOWNLOADER_DOWNLOADING)) {
			/* Download until transport returns an error or the download is complete
			 * (separate event).
			 */
			rc = transport_download(dl);
			if (rc) {
				if (rc == -ECONNRESET) {
					goto reconnect;
				}

				rc = error_evt_send(dl, rc);
				if (rc) {
					restart_and_suspend(dl);
					continue;
				}

reconnect:
				rc2 = reconnect(dl);
				if (rc2 == 0) {
					continue;
				}

				LOG_ERR("Failed to reconnect, err %d", rc2);

				rc2 = error_evt_send(dl, rc2);
				if (rc2 == 0 && is_state(dl, DOWNLOADER_DOWNLOADING)) {
					goto reconnect;
				}

				transport_close(dl);
				stopped_evt_send(dl);
				state_set(dl, DOWNLOADER_DOWNLOADING, DOWNLOADER_IDLE);
				continue;
			}

			if (dl->complete) {
				LOG_INF("Download complete");
				restart_and_suspend(dl);
				download_complete_evt_send(dl);
			}
		}

		if (is_state(dl, DOWNLOADER_STOPPING)) {
			if (!dl->host_cfg.keep_connection) {
				transport_close(dl);
				state_set(dl, DOWNLOADER_STOPPING, DOWNLOADER_IDLE);
			} else {
				state_set(dl, DOWNLOADER_STOPPING, DOWNLOADER_CONNECTED);
			}

			stopped_evt_send(dl);
		}

		if (is_state(dl, DOWNLOADER_DEINITIALIZING)) {
			if (dl->transport) {
				transport_close(dl);
				transport_deinit(dl);
			}

			state_set(dl, DOWNLOADER_DEINITIALIZING, DOWNLOADER_DEINITIALIZED);
			deinit_evt_send(dl);
			return;
		}
	}
}

int downloader_init(struct downloader *const dl, struct downloader_cfg *dl_cfg)
{
	if (!dl || !dl_cfg || !dl_cfg->callback || !dl_cfg->buf || dl_cfg->buf_size == 0) {
		return -EINVAL;
	}

	memset(dl, 0, sizeof(*dl));
	dl->cfg = *dl_cfg;
	k_sem_init(&dl->event_sem, 0, 1);
	k_mutex_init(&dl->mutex);

	k_mutex_lock(&dl->mutex, K_FOREVER);

	/* The thread is spawned now, but it will suspend itself;
	 * it is resumed when the download is started via the API.
	 */
	dl->tid = k_thread_create(&dl->thread, dl->thread_stack,
				  K_THREAD_STACK_SIZEOF(dl->thread_stack), download_thread, dl,
				  NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);

	k_thread_name_set(dl->tid, "downloader");

	dl->state = DOWNLOADER_IDLE;

	k_mutex_unlock(&dl->mutex);

	return 0;
}

int downloader_deinit(struct downloader *const dl)
{
	if (!dl) {
		return -EINVAL;
	}

	if (!is_valid_state(dl, dl->state) || is_state(dl, DOWNLOADER_DEINITIALIZED)) {
		return -EPERM;
	}

	if (is_state(dl, DOWNLOADER_CONNECTING) ||
	    (is_state(dl, DOWNLOADER_DOWNLOADING) && (dl->file_size != dl->progress))) {
		error_evt_send(dl, -ECANCELED);
		stopped_evt_send(dl);
	}

	state_set(dl, STATE_ALLOW_ANY, DOWNLOADER_DEINITIALIZING);
	k_sem_give(&dl->event_sem);

	k_thread_join(&dl->thread, K_FOREVER);

	return 0;
}

static int downloader_start(struct downloader *dl, const struct downloader_host_cfg *dl_host_cfg,
			    const char *url, size_t from)
{
	__ASSERT_NO_MSG(dl != NULL);
	__ASSERT_NO_MSG(dl_host_cfg != NULL);
	__ASSERT_NO_MSG(url != NULL);

	int err;
	const struct dl_transport *transport_connected = dl->transport;
	bool host_connected = false;

	LOG_DBG("URL: %s", url);

	k_mutex_lock(&dl->mutex, K_FOREVER);

	if (!is_state(dl, DOWNLOADER_IDLE) && !is_state(dl, DOWNLOADER_CONNECTED)) {
		LOG_ERR("Invalid start state: %d", dl->state);
		k_mutex_unlock(&dl->mutex);
		return -EPERM;
	}

	/* Check if we are already connected to the correct host */
	if (is_state(dl, DOWNLOADER_CONNECTED)) {
		char hostname[CONFIG_DOWNLOADER_MAX_HOSTNAME_SIZE];

		err = dl_parse_url_host(url, hostname, sizeof(hostname));
		if (err) {
			LOG_ERR("Failed to parse hostname");
			k_mutex_unlock(&dl->mutex);
			return -EINVAL;
		}
		if (strncmp(hostname, dl->hostname, sizeof(hostname)) == 0) {
			host_connected = true;
		}
	}

	/* Extract the hostname, without protocol or port */
	err = dl_parse_url_host(url, dl->hostname, sizeof(dl->hostname));
	if (err) {
		LOG_ERR("Failed to parse hostname, err %d", err);
		k_mutex_unlock(&dl->mutex);
		return -EINVAL;
	}

	/* Extract the filename, without protocol or port */
	err = dl_parse_url_file(url, dl->file, sizeof(dl->file));
	if (err) {
		LOG_ERR("Failed to parse filename, err %d, url %s", err, url);
		k_mutex_unlock(&dl->mutex);
		return err;
	}

	dl->host_cfg = *dl_host_cfg;
	dl->file_size = 0;
	dl->progress = from;
	dl->buf_offset = 0;
	dl->complete = false;

	if (dl->host_cfg.redirects_max == 0) {
		dl->host_cfg.redirects_max = CONFIG_DOWNLOADER_MAX_REDIRECTS;
	}

	dl->transport = NULL;
	STRUCT_SECTION_FOREACH(dl_transport_entry, entry)
	{
		if (entry->transport->proto_supported(dl, url)) {
			dl->transport = entry->transport;
			break;
		}
	}

	if (!dl->transport) {
		if (strstr(url, "://") == NULL) {
			char *fallback = FALLBACK_HTTP;

			if (dl_host_cfg->sec_tag_list && dl_host_cfg->sec_tag_count) {
				fallback = FALLBACK_HTTPS;
			}

			LOG_WRN("Protocol not specified for %s, attempting %s", url, fallback);
			STRUCT_SECTION_FOREACH(dl_transport_entry, entry)
			{
				if (entry->transport->proto_supported(dl, fallback)) {
					dl->transport = entry->transport;
					break;
				}
			}
		}

		if (!dl->transport) {
			LOG_ERR("Protocol not found for %s", url);
			k_mutex_unlock(&dl->mutex);
			return -EPROTONOSUPPORT;
		}
	};

	if (is_state(dl, DOWNLOADER_CONNECTED)) {
		if (host_connected) {
			state_set(dl, DOWNLOADER_CONNECTED, DOWNLOADER_DOWNLOADING);
			goto out;
		} else if (transport_connected) {
			/* We are connected to the wrong host */
			LOG_DBG("Closing connection to connect different host or protocol");
			transport_connected->close(dl);
			transport_connected->deinit(dl);
			state_set(dl, DOWNLOADER_CONNECTED, DOWNLOADER_IDLE);
		}
	}

	state_set(dl, DOWNLOADER_IDLE, DOWNLOADER_CONNECTING);

	err = transport_init(dl, &dl->host_cfg, url);
	if (err) {
		state_set(dl, DOWNLOADER_CONNECTING, DOWNLOADER_IDLE);
		k_mutex_unlock(&dl->mutex);
		LOG_ERR("Failed to initialize transport, err %d", err);
		return err;
	}

out:
	k_mutex_unlock(&dl->mutex);

	/* Let the thread run */
	k_sem_give(&dl->event_sem);
	return 0;
}

int downloader_cancel(struct downloader *const dl)
{
	if (!dl) {
		return -EINVAL;
	}

	k_mutex_lock(&dl->mutex, K_FOREVER);
	if (!is_state(dl, DOWNLOADER_CONNECTING) && !is_state(dl, DOWNLOADER_DOWNLOADING)) {
		k_mutex_unlock(&dl->mutex);
		return -EPERM;
	}

	state_set(dl, STATE_ALLOW_ANY, DOWNLOADER_STOPPING);
	k_mutex_unlock(&dl->mutex);
	return 0;
}

int downloader_get(struct downloader *dl, const struct downloader_host_cfg *dl_host_cfg,
		   const char *url, size_t from)
{
	int rc;

	if (!dl || !dl_host_cfg || !url) {
		return -EINVAL;
	}

	rc = downloader_start(dl, dl_host_cfg, url, from);

	return rc;
}

int downloader_get_with_host_and_file(struct downloader *dl,
				      const struct downloader_host_cfg *dl_host_cfg,
				      const char *host, const char *file, size_t from)
{
	int rc;

	if (!dl || !dl_host_cfg || !host || !file) {
		return -EINVAL;
	}

	k_mutex_lock(&dl->mutex, K_FOREVER);

	if (!is_state(dl, DOWNLOADER_IDLE) && !is_state(dl, DOWNLOADER_CONNECTED)) {
		LOG_ERR("Invalid start state: %d", dl->state);
		k_mutex_unlock(&dl->mutex);
		return -EPERM;
	}

	/* We use the download client buffer to parse the url */
	if (strlen(host) + strlen(file) + 2 > dl->cfg.buf_size) {
		LOG_ERR("Download client buffer is not large enough to parse url");
		k_mutex_unlock(&dl->mutex);
		return -EINVAL;
	}

	snprintf(dl->cfg.buf, dl->cfg.buf_size, "%s/%s", host, file);

	rc = downloader_start(dl, dl_host_cfg, dl->cfg.buf, from);

	k_mutex_unlock(&dl->mutex);

	return rc;
}

int downloader_file_size_get(struct downloader *dl, size_t *size)
{
	if (!dl || !size) {
		return -EINVAL;
	}

	if (is_state(dl, DOWNLOADER_DEINITIALIZED)) {
		return -EPERM;
	}

	k_mutex_lock(&dl->mutex, K_FOREVER);
	*size = dl->file_size;
	k_mutex_unlock(&dl->mutex);

	return 0;
}

int downloader_downloaded_size_get(struct downloader *dl, size_t *size)
{
	if (!dl || !size) {
		return -EINVAL;
	}

	if (is_state(dl, DOWNLOADER_DEINITIALIZED)) {
		return -EPERM;
	}

	k_mutex_lock(&dl->mutex, K_FOREVER);
	*size = dl->progress;
	k_mutex_unlock(&dl->mutex);

	return 0;
}
