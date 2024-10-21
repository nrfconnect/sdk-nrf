/*
 * Copyright (c) 2019-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/toolchain.h>
#include <net/download_client.h>
#include <net/download_client_transport.h>

#include "download_client_internal.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(download_client, CONFIG_DOWNLOAD_CLIENT_LOG_LEVEL);

#define HOSTNAME_SIZE CONFIG_DOWNLOAD_CLIENT_MAX_HOSTNAME_SIZE

static int close_evt_send(struct download_client *dlc);

#if CONFIG_DOWNLOAD_CLIENT_LOG_LEVEL_DBG
char *state_to_str(int state)
{
	switch (state) {
	case DOWNLOAD_CLIENT_IDLE:
		return "IDLE";
	case DOWNLOAD_CLIENT_CONNECTING:
		return "CONNECTING";
	case DOWNLOAD_CLIENT_CONNECTED:
		return "CONNECTED";
	case DOWNLOAD_CLIENT_DOWNLOADING:
		return "DOWNLOADING";
	case DOWNLOAD_CLIENT_DEINITIALIZING:
		return "DEINITIALIZING";
	case DOWNLOAD_CLIENT_DEINITIALIZED:
		return "DEINITIALIZED";
	}

	return "unknown";
}
#else
char *state_to_str(int state) {
	return "";
}
#endif

static void state_set(struct download_client *dlc, int state)
{
	k_mutex_lock(&dlc->mutex, K_FOREVER);
	dlc->state = state;
	k_mutex_unlock(&dlc->mutex);
	LOG_DBG("state = %d (%s)", state, state_to_str(state));
}

static bool is_state(struct download_client *dlc, enum download_client_state state)
{
	bool ret;

	k_mutex_lock(&dlc->mutex, K_FOREVER);
	ret = dlc->state == state;
	k_mutex_unlock(&dlc->mutex);
	return ret;
}

static int transport_init(struct download_client *dlc,
			  struct download_client_host_cfg *host_config, const char *uri) {
	int err;

	if (!dlc || !dlc->transport) {
		return -EINVAL;
	}

	err = ((struct dlc_transport *)dlc->transport)->init(dlc, host_config, uri);

	return err;
}

static int transport_deinit(struct download_client *dlc) {
	int err;

	if (!dlc || !dlc->transport) {
		return -EINVAL;
	}

	err = ((struct dlc_transport *)dlc->transport)->deinit(dlc);

	return err;
}

static int transport_connect(struct download_client *dlc) {
	int err;

	if (!dlc || !dlc->transport) {
		return -EINVAL;
	}

	err = ((struct dlc_transport *)dlc->transport)->connect(dlc);

	return err;
}

static int transport_close(struct download_client *dlc) {
	int err;

	if (!dlc || !dlc->transport) {
		return -EINVAL;
	}

	err = ((struct dlc_transport *)dlc->transport)->close(dlc);
	close_evt_send(dlc);

	return err;
}

static int transport_download(struct download_client *dlc) {
	int err;

	if (!dlc || !dlc->transport) {
		return -EINVAL;
	}

	err = ((struct dlc_transport *)dlc->transport)->download(dlc);

	return err;
}


static int reconnect(struct download_client *dlc)
{
	int err = 0;

	LOG_INF("Reconnecting...");

	err = transport_close(dlc);
	if (err) {
		LOG_DBG("disconnect failed, %d", err);
	}

	err = transport_connect(dlc);

	state_set(dlc, DOWNLOAD_CLIENT_CONNECTING);

	return err;
}

static void restart_and_suspend(struct download_client *dlc) {

	if (!is_state(dlc, DOWNLOAD_CLIENT_DOWNLOADING)) {
		return;
	}

	if (!dlc->host_config.keep_connection) {
		transport_close(dlc);
		state_set(dlc, DOWNLOAD_CLIENT_IDLE);
		return;
	}
	state_set(dlc, DOWNLOAD_CLIENT_CONNECTED);
}

static int data_evt_send(const struct download_client *dlc, void *data, size_t len)
{
	const struct download_client_evt evt = {
		.id = DOWNLOAD_CLIENT_EVT_FRAGMENT,
		.fragment = {
			.buf = data,
			.len = len,
		}
	};

	return dlc->config.callback(&evt);
}

static int download_complete_evt_send(const struct download_client *dlc)
{
	const struct download_client_evt evt = {
		.id = DOWNLOAD_CLIENT_EVT_DONE,
	};

	return dlc->config.callback(&evt);
}

static int close_evt_send(struct download_client *dlc)
{
	const struct download_client_evt evt = {
		.id = DOWNLOAD_CLIENT_EVT_CLOSED,
	};

	return dlc->config.callback(&evt);
}

static int error_evt_send(const struct download_client *dlc, int error)
{
	/* Error will be sent as negative. */
	__ASSERT_NO_MSG(error < 0);

	const struct download_client_evt evt = {
		.id = DOWNLOAD_CLIENT_EVT_ERROR,
		.error = error
	};

	return dlc->config.callback(&evt);
}

static int deinit_evt_send(const struct download_client *dlc)
{
	const struct download_client_evt evt = {
		.id = DOWNLOAD_CLIENT_EVT_DEINITIALIZED,
	};

	return dlc->config.callback(&evt);
}

/* Events from the transport */
int dlc_transport_evt_connected(struct download_client *dlc)
{
	k_mutex_lock(&dlc->mutex, K_FOREVER);
	if (dlc->state == DOWNLOAD_CLIENT_CONNECTING) {
		state_set(dlc, DOWNLOAD_CLIENT_CONNECTED);
	}
	k_mutex_unlock(&dlc->mutex);

	return 0;
}

int dlc_transport_evt_disconnected(struct download_client *dlc)
{
	state_set(dlc, DOWNLOAD_CLIENT_IDLE);

	return 0;
}

int dlc_transport_evt_data(struct download_client *dlc, void *data, size_t len)
{
	int err;

	LOG_DBG("Read %d bytes from transport", len);

	if (dlc->file_size) {
		LOG_INF("Downloaded %u/%u bytes (%d%%)",
			dlc->progress, dlc->file_size,
			(dlc->progress * 100) / dlc->file_size);
	} else {
		LOG_INF("Downloaded %u bytes", dlc->progress);
	}

	err = data_evt_send(dlc, data, len);
	if (err) {
		/* Application refused data, suspend */
		restart_and_suspend(dlc);
	}

	return 0;
}

int dlc_transport_evt_download_complete(struct download_client *dlc)
{
	LOG_INF("Download complete");
	download_complete_evt_send(dlc);
	restart_and_suspend(dlc);

	return 0;
}

void download_thread(void *cli, void *a, void *b)
{
	int rc;
	struct download_client *const dlc = cli;

	while (true) {
		rc = 0;

		if (is_state(dlc, DOWNLOAD_CLIENT_IDLE)) {
			/* Client idle, wait for action */
			k_sem_take(&dlc->event_sem, K_FOREVER);
		}

		/* Connect to the target host */
		if (is_state(dlc, DOWNLOAD_CLIENT_CONNECTING)) {
			/* Client */
			rc = transport_connect(dlc);
			if (rc) {
				state_set(dlc, DOWNLOAD_CLIENT_IDLE);
				error_evt_send(dlc, rc);
				continue;
			}

			/* Connection successful */
			state_set(dlc, DOWNLOAD_CLIENT_DOWNLOADING);
		}

		if (is_state(dlc, DOWNLOAD_CLIENT_CONNECTED)) {
			/* Client connected, wait for action */
			k_sem_take(&dlc->event_sem, K_FOREVER);
		}

		if (is_state(dlc, DOWNLOAD_CLIENT_DOWNLOADING)) {
			/* Download untill transport returns an error or the download is complete
			 * (separate event).
			 */
			rc = transport_download(dlc);
			if (rc) {
				rc = error_evt_send(dlc, rc);
				if (rc) {
					restart_and_suspend(dlc);
				}

				rc = reconnect(dlc);
				if (rc) {
					restart_and_suspend(dlc);
				}
			}
		}

		if (is_state(dlc, DOWNLOAD_CLIENT_DEINITIALIZING)) {
			printk("Deinitializing download client");
			transport_close(dlc);
			transport_deinit(dlc);
			state_set(dlc, DOWNLOAD_CLIENT_DEINITIALIZED);
			deinit_evt_send(dlc);
			return;
		}
	}
}

int download_client_init(struct download_client *const dlc,
			 struct download_client_cfg *config)
{
	if (dlc == NULL || config == NULL || config->callback == NULL ||
	    config->buf == NULL || config->buf_size == 0) {
		return -EINVAL;
	}

	if (dlc->state != DOWNLOAD_CLIENT_DEINITIALIZED) {
		return -EALREADY;
	}

	memset(dlc, 0, sizeof(*dlc));
	dlc->config = *config;
	k_sem_init(&dlc->event_sem, 0, 1);
	k_mutex_init(&dlc->mutex);

	k_mutex_lock(&dlc->mutex, K_FOREVER);

	/* The thread is spawned now, but it will suspend itself;
	 * it is resumed when the download is started via the API.
	 */
	dlc->tid =
		k_thread_create(&dlc->thread, dlc->thread_stack,
				K_THREAD_STACK_SIZEOF(dlc->thread_stack),
				download_thread, dlc, NULL, NULL,
				K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);

	k_thread_name_set(dlc->tid, "download_client");

	state_set(dlc, DOWNLOAD_CLIENT_IDLE);

	k_mutex_unlock(&dlc->mutex);

	return 0;
}

int download_client_deinit(struct download_client *const dlc)
{
	if (!dlc) {
		return -EINVAL;
	}

	state_set(dlc, DOWNLOAD_CLIENT_DEINITIALIZING);
	k_sem_give(&dlc->event_sem);

	return 0;
}

int download_client_start(struct download_client *dlc,
			  const struct download_client_host_cfg *host_config,
			  const char *uri, size_t from)
{
	int err;
	struct dlc_transport *transport_connected = NULL;

	if (dlc == NULL || host_config == NULL || uri == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&dlc->mutex, K_FOREVER);

	if (!is_state(dlc, DOWNLOAD_CLIENT_IDLE) &&
	    !is_state(dlc, DOWNLOAD_CLIENT_CONNECTED)) {
		LOG_ERR("Invalid start state: %d", dlc->state);
		k_mutex_unlock(&dlc->mutex);
		return -EPERM;
	}

	/* Check if we are already connected to the correct host */
	if (is_state(dlc, DOWNLOAD_CLIENT_CONNECTED)) {
		char hostname[CONFIG_DOWNLOAD_CLIENT_MAX_HOSTNAME_SIZE];

		err = url_parse_host(uri, hostname, sizeof(hostname));
		if (err) {
			LOG_ERR("Failed to parse hostname");
			return EINVAL;
		}
		if (strncmp(hostname, dlc->hostname, sizeof(hostname)) == 0) {
			transport_connected = dlc->transport;
		}
	}

	/* Extract the hostname, without protocol or port */
	err = url_parse_host(uri, dlc->hostname, sizeof(dlc->hostname));
	if (err) {
		LOG_ERR("Failed to parse hostname");
		return EINVAL;
	}

	/* Extract the filename, without protocol or port */
	err = url_parse_file(uri, dlc->file, sizeof(dlc->file));
	if (err) {
		LOG_ERR("Failed to parse filename");
		return EINVAL;
	}

	dlc->host_config = *host_config;
	dlc->file_size = 0;
	dlc->progress = from;
	dlc->buf_offset = 0;

	dlc->transport = NULL;

	printk("Finding transport for %s\n", uri);

	STRUCT_SECTION_FOREACH(dlc_transport_entry, entry) {
		if (entry->transport->proto_supported(dlc, uri)) {
			dlc->transport = entry->transport;
			break;
		}
	}

	if (!dlc->transport) {
		LOG_ERR("Protocol not specified for %s", uri);
		return -EINVAL;
	};

	printk("Found transport, state is %d", dlc->state);

	if (is_state(dlc, DOWNLOAD_CLIENT_CONNECTED)) {
		if (dlc->transport == transport_connected) {
			state_set(dlc, DOWNLOAD_CLIENT_DOWNLOADING);
			goto out;
		} else {
			/* We are connected to the wrong host */
			LOG_DBG("Closing connection to connect different host or protocol");
			transport_connected->close(dlc);
			transport_connected->deinit(dlc);
			state_set(dlc, DOWNLOAD_CLIENT_CONNECTING);
		}
	} else {
		/* IDLE */
		state_set(dlc, DOWNLOAD_CLIENT_CONNECTING);
	}

	memset(dlc->transport_internal, 0, sizeof(dlc->transport_internal));
	err = transport_init(dlc, &dlc->host_config, uri);
	if (err) {
		state_set(dlc, DOWNLOAD_CLIENT_IDLE);
		k_mutex_unlock(&dlc->mutex);
		LOG_ERR("Failed to initialize transport, err %d", err);
		return err;
	}

	printk("Transport initialized\n");
	k_sleep(K_MSEC(1000));

out:
	k_mutex_unlock(&dlc->mutex);

	/* Let the thread run */
	k_sem_give(&dlc->event_sem);
	return 0;
}

int download_client_stop(struct download_client *const dlc)
{
	if (dlc == NULL || is_state(dlc, DOWNLOAD_CLIENT_IDLE)) {
		return -EINVAL;
	}

	transport_close(dlc);
	close_evt_send(dlc);
	state_set(dlc, DOWNLOAD_CLIENT_IDLE);

	return 0;
}

int download_client_get(struct download_client *dlc,
			const struct download_client_host_cfg *host_config,
			const char *uri, size_t from)
{
	int rc;

	if (dlc == NULL || uri == NULL) {
		return -EINVAL;
	}

	printk("Get file\n");
	k_sleep(K_SECONDS(1));

	k_mutex_lock(&dlc->mutex, K_FOREVER);

	rc = download_client_start(dlc, host_config, uri, from);

	k_mutex_unlock(&dlc->mutex);

	return rc;
}

int download_client_file_size_get(struct download_client *dlc, size_t *size)
{
	if (!dlc || !size) {
		return -EINVAL;
	}

	k_mutex_lock(&dlc->mutex, K_FOREVER);
	*size = dlc->file_size;
	k_mutex_unlock(&dlc->mutex);

	return 0;
}

int download_client_downloaded_size_get(struct download_client *dlc, size_t *size)
{
	if (!dlc || !size) {
		return -EINVAL;
	}

	k_mutex_lock(&dlc->mutex, K_FOREVER);
	*size = dlc->progress;
	k_mutex_unlock(&dlc->mutex);

	return 0;
}
