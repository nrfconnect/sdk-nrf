/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <net/downloader.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(download, LOG_LEVEL_INF);


#if CONFIG_MODEM_KEY_MGMT
#include <modem/modem_key_mgmt.h>
#else
#include <zephyr/net/tls_credentials.h>
#endif

#define URL CONFIG_SAMPLE_FILE_URL
#if defined(CONFIG_SAMPLE_SEC_TAG)
#define SEC_TAG CONFIG_SAMPLE_SEC_TAG
#else
#define SEC_TAG 0
#endif

#define CONN_LAYER_EVENT_MASK (NET_EVENT_CONN_IF_FATAL_ERROR)
#define L4_EVENT_MASK (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)

#define PROGRESS_WIDTH 50
#define STARTING_OFFSET 0
#define RETRY_BACKOFF_INITIAL_MS 1000
#define RETRY_BACKOFF_MAX_MS 30000
#define MAX_ATTEMPTS 8
#define NETWORK_POLL_MS 5000

static struct net_mgmt_event_callback conn_cb;

static K_SEM_DEFINE(download_result_sem, 0, 1);
static bool download_succeeded;
static int download_error;

static int sec_tag_list[] = { SEC_TAG };
static const char cert[] = {
#if defined(CONFIG_SAMPLE_PROVISION_CERT)
	#include "sample_cert.inc"

	/* Null terminate certificate if running Mbed TLS on the application core.
	 * Required by TLS credentials API.
	 */
	IF_ENABLED(CONFIG_TLS_CREDENTIALS, (0x00))
#else
	0x00
#endif
};
BUILD_ASSERT(sizeof(cert) < KB(4), "Certificate too large");

static const char client_cert[] = {
#if defined(CONFIG_SAMPLE_PROVISION_CLIENT_CERT)
	#include "sample_client_cert.inc"
	IF_ENABLED(CONFIG_TLS_CREDENTIALS, (0x00))
#else
	0x00
#endif
};
BUILD_ASSERT(!IS_ENABLED(CONFIG_SAMPLE_PROVISION_CLIENT_CERT) || sizeof(client_cert) < KB(4),
	     "Client certificate too large");
static const char client_key[] = {
#if defined(CONFIG_SAMPLE_PROVISION_CLIENT_CERT)
	#include "sample_client_key.inc"
	IF_ENABLED(CONFIG_TLS_CREDENTIALS, (0x00))
#else
	0x00
#endif
};
BUILD_ASSERT(!IS_ENABLED(CONFIG_SAMPLE_PROVISION_CLIENT_CERT) || sizeof(client_key) < KB(4),
	     "Client private key too large");

static char dl_buf[2048];

static int callback(const struct downloader_evt *event);

static struct downloader downloader;
static struct downloader_cfg dl_cfg = {
	.callback = callback,
	.buf = dl_buf,
	.buf_size = sizeof(dl_buf),
};
static struct downloader_host_cfg host_dl_cfg = {
	.sec_tag_list = IS_ENABLED(CONFIG_SAMPLE_SECURE_SOCKET) ? sec_tag_list : 0,
	.sec_tag_count = IS_ENABLED(CONFIG_SAMPLE_SECURE_SOCKET) ? ARRAY_SIZE(sec_tag_list) : 0,
	.range_override = 0,
};

#if CONFIG_SAMPLE_COMPUTE_HASH
#include <psa/crypto.h>
static psa_hash_operation_t hash_ctx;

#if defined(CONFIG_SAMPLE_COMPARE_HASH)
BUILD_ASSERT(sizeof(CONFIG_SAMPLE_SHA256_HASH) == 65,
	     "CONFIG_SAMPLE_SHA256_HASH is not set or wrong length");
#else
#define CONFIG_SAMPLE_SHA256_HASH 0
#endif /* CONFIG_SAMPLE_COMPARE_HASH */
#endif

static int64_t ref_time;

static int cert_provision(void)
{
	int err;

	LOG_INF("Provisioning certificate");

#if CONFIG_MODEM_KEY_MGMT
	bool exists;

	err = modem_key_mgmt_exists(SEC_TAG,
				    MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				    &exists);
	if (err) {
		LOG_ERR("Failed to check for certificates err %d", err);
		return err;
	}

	if (exists) {
		/* Let's compare the existing credential */
		err = modem_key_mgmt_cmp(SEC_TAG,
					 MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
					 cert, sizeof(cert));

		LOG_INF("Certificate %s", err ? "mismatch" : "match");

		if (!err) {
			return 0;
		}
	}

	/*  Provision certificate to the modem */
	err = modem_key_mgmt_write(SEC_TAG,
				   MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				   cert, sizeof(cert));
	if (err) {
		LOG_ERR("Failed to provision certificate, err %d", err);
		return err;
	}

	/* NB: Client certificate/key provisioning (CONFIG_SAMPLE_PROVISION_CLIENT_CERT) is not
	 * implemented for modem-managed credentials yet.
	 */
#else /* CONFIG_MODEM_KEY_MGMT */
	err = tls_credential_add(SEC_TAG,
				 TLS_CREDENTIAL_CA_CERTIFICATE,
				 cert,
				 sizeof(cert));
	if (err == -EEXIST) {
		LOG_INF("CA certificate already exists, sec tag: %d", SEC_TAG);
	} else if (err < 0) {
		LOG_ERR("Failed to register CA certificate: %d", err);
		return err;
	}

	if (IS_ENABLED(CONFIG_SAMPLE_PROVISION_CLIENT_CERT)) {
		err = tls_credential_add(SEC_TAG,
					 TLS_CREDENTIAL_PUBLIC_CERTIFICATE,
					 client_cert,
					 sizeof(client_cert));
		if (err == -EEXIST) {
			LOG_INF("Client certificate already exists, sec tag: %d", SEC_TAG);
		} else if (err < 0) {
			LOG_ERR("Failed to register client certificate: %d", err);
			return err;
		}

		err = tls_credential_add(SEC_TAG,
					 TLS_CREDENTIAL_PRIVATE_KEY,
					 client_key,
					 sizeof(client_key));
		if (err == -EEXIST) {
			LOG_INF("Client private key already exists, sec tag: %d", SEC_TAG);
		} else if (err < 0) {
			LOG_ERR("Failed to register client private key: %d", err);
			return err;
		}
	}
#endif /* !CONFIG_MODEM_KEY_MGMT */

	return 0;
}

static void l4_event_handler(uint64_t event, struct net_if *iface, void *info,
			     size_t info_length, void *user_data)
{
	LOG_INF("Network %s", event == NET_EVENT_L4_CONNECTED ? "connected" : "disconnected");
}

NET_MGMT_REGISTER_EVENT_HANDLER(l4_events, L4_EVENT_MASK, l4_event_handler, NULL);

static void connectivity_event_handler(struct net_mgmt_event_callback *cb,
				       uint64_t event,
				       struct net_if *iface)
{
	int err;
	if (event == NET_EVENT_CONN_IF_FATAL_ERROR) {
		LOG_ERR("Fatal error received from the connectivity layer, "
			"rebooting network interface");

		(void)conn_mgr_if_disconnect(iface);
		(void)conn_mgr_all_if_down(true);

		err = conn_mgr_all_if_up(true);
		if (err) {
			LOG_ERR("conn_mgr_all_if_up, error: %d", err);
			return;
		}

		LOG_INF("Reconnecting to network");

		err = conn_mgr_all_if_connect(true);
		if (err) {
			LOG_ERR("conn_mgr_all_if_connect, error: %d", err);
			return;
		}

		return;
	}
}

static int callback(const struct downloader_evt *event)
{
	static size_t downloaded;
	static size_t file_size;
	uint32_t speed;
	int64_t ms_elapsed;
#if CONFIG_SAMPLE_COMPUTE_HASH
	psa_status_t status;
#endif
	if (downloaded == 0) {
		downloader_file_size_get(&downloader, &file_size);
		downloaded += STARTING_OFFSET;
	}

	switch (event->id) {
	case DOWNLOADER_EVT_FRAGMENT:
		downloaded += event->fragment.len;

#if CONFIG_SAMPLE_COMPUTE_HASH
		status = psa_hash_update(&hash_ctx, event->fragment.buf, event->fragment.len);
		if (status != PSA_SUCCESS) {
			LOG_ERR("Error during hash update: %d", status);
			return status;
		}
#endif
		return 0;

	case DOWNLOADER_EVT_DONE:
		ms_elapsed = k_uptime_delta(&ref_time);
		speed = ((float)file_size / ms_elapsed) * MSEC_PER_SEC;
		LOG_INF("Download completed in %lld ms @ %u bytes per sec, total %zu bytes",
			ms_elapsed, speed, downloaded);

#if CONFIG_SAMPLE_COMPUTE_HASH
		uint8_t hash[32];
		char hash_str[64 + 1];
		size_t hash_length;

		status = psa_hash_finish(&hash_ctx, hash, sizeof(hash), &hash_length);
		if (status != PSA_SUCCESS) {
			LOG_ERR("Error during hash finish: %d", status);
			k_sem_give(&download_result_sem);
			return status;
		}

		bin2hex(hash, sizeof(hash), hash_str, sizeof(hash_str));

		LOG_INF("SHA256: %s", hash_str);

		if (IS_ENABLED(CONFIG_SAMPLE_COMPARE_HASH) &&
		    strcmp(hash_str, CONFIG_SAMPLE_SHA256_HASH)) {
			LOG_ERR("Expect: %s", CONFIG_SAMPLE_SHA256_HASH);
			LOG_ERR("SHA256 mismatch!");
		}
#endif /* CONFIG_SAMPLE_COMPUTE_HASH */

		LOG_INF("Done");
		download_succeeded = true;
		k_sem_give(&download_result_sem);
		return 0;

	case DOWNLOADER_EVT_ERROR:
		if (event->error == -ECONNRESET) {
			/* With ECONNRESET, allow library to attempt a reconnect by returning 0 */
		} else {
			download_error = event->error;
			if (event->error == -ENETDOWN) {
				LOG_INF("Network not ready, waiting");
			} else {
				LOG_ERR("Error %d during download", event->error);
			}
			/* Stop download */
			return -1;
		}
		break;
	case DOWNLOADER_EVT_STOPPED:
		LOG_WRN("Download canceled");
		downloaded = 0;
		file_size = 0;
		download_succeeded = false;
		k_sem_give(&download_result_sem);
		break;
	case DOWNLOADER_EVT_DEINITIALIZED:
		LOG_INF("Client deinitialized");
		break;
	}

	return 0;
}

int main(void)
{
	int err;

	LOG_INF("Download client sample started");

	/* Setup handler for Zephyr NET Connection Manager Connectivity layer. */
	net_mgmt_init_event_callback(&conn_cb, connectivity_event_handler, CONN_LAYER_EVENT_MASK);
	net_mgmt_add_event_callback(&conn_cb);

	err = conn_mgr_all_if_up(true);
	if (err) {
		LOG_ERR("conn_mgr_all_if_up, error: %d", err);
		return err;
	}

	if (IS_ENABLED(CONFIG_SAMPLE_PROVISION_CERT)) {
		/* Provision certificates before connecting to the network */
		err = cert_provision();
		if (err) {
			return 0;
		}
	}
	LOG_INF("Connecting to network");

	err = conn_mgr_all_if_connect(true);
	if (err) {
		LOG_ERR("conn_mgr_all_if_connect, error: %d", err);
		return err;
	}

	err = downloader_init(&downloader, &dl_cfg);
	if (err) {
		LOG_ERR("Failed to initialize the client, err %d", err);
		return 0;
	}

	/* The downloader picks the IP family itself, but does not wait for the network.
	 * Poll until the network is up, and retry failed downloads with an increasing backoff.
	 */
	int32_t retry_backoff_ms = 0;
	int attempt = 0;

	do {
		if (download_error == -ENETDOWN) {
			/* Waiting for the network does not count as an attempt */
			k_sleep(K_MSEC(NETWORK_POLL_MS));
		} else {
			attempt++;
			if (attempt > MAX_ATTEMPTS) {
				break;
			}

			/* Back off before retries. */
			if (retry_backoff_ms > 0) {
				k_sleep(K_MSEC(retry_backoff_ms));
				retry_backoff_ms = MIN(retry_backoff_ms * 2, RETRY_BACKOFF_MAX_MS);
			} else {
				retry_backoff_ms = RETRY_BACKOFF_INITIAL_MS;
			}
		}
		download_error = 0;

#if CONFIG_SAMPLE_COMPUTE_HASH
		psa_status_t status;

		/* Abort any previous (possibly incomplete) operation before starting over. */
		(void)psa_hash_abort(&hash_ctx);

		status = psa_hash_setup(&hash_ctx, PSA_ALG_SHA_256);
		if (status != PSA_SUCCESS) {
			LOG_ERR("psa_hash_setup, error: %d", status);
			break;
		}
#endif

		ref_time = k_uptime_get();

		err = downloader_get(&downloader, &host_dl_cfg, URL, STARTING_OFFSET);
		if (err) {
			LOG_ERR("Failed to start the downloader, err %d", err);
			continue;
		}

		LOG_INF("Downloading %s", URL);

		k_sem_take(&download_result_sem, K_FOREVER);
	} while (!download_succeeded);

	if (!download_succeeded) {
		LOG_ERR("Giving up after %d failed attempts", attempt);
	}

	LOG_INF("Download finished, bringing network interfaces down");
	(void)conn_mgr_all_if_disconnect(true);
	(void)conn_mgr_all_if_down(true);

	return 0;
}
