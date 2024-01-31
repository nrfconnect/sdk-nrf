/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <net/download_client.h>

#if CONFIG_MODEM_KEY_MGMT
#include <modem/modem_key_mgmt.h>
#else
#include <zephyr/net/tls_credentials.h>
#endif

#define URL CONFIG_SAMPLE_FILE_URL
#define SEC_TAG CONFIG_SAMPLE_SEC_TAG

/* Macros used to subscribe to specific Zephyr NET management events. */
#define L4_EVENT_MASK (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)
#define CONN_LAYER_EVENT_MASK (NET_EVENT_CONN_IF_FATAL_ERROR)

#define PROGRESS_WIDTH 50
#define STARTING_OFFSET 0

/* Zephyr NET management event callback structures. */
static struct net_mgmt_event_callback l4_cb;
static struct net_mgmt_event_callback conn_cb;
static struct net_if *net_if;

static K_SEM_DEFINE(network_connected_sem, 0, 1);

#if CONFIG_SAMPLE_SECURE_SOCKET
static const char cert[] = {
	#include SAMPLE_CERT_FILE_INC

	/* Null terminate certificate if running Mbed TLS on the application core.
	 * Required by TLS credentials API.
	 */
	IF_ENABLED(CONFIG_TLS_CREDENTIALS, (0x00))
};
static int sec_tag_list[] = { SEC_TAG };
BUILD_ASSERT(sizeof(cert) < KB(4), "Certificate too large");
#endif

static struct download_client downloader;
static struct download_client_cfg config = {
#if CONFIG_SAMPLE_SECURE_SOCKET
	.sec_tag_list = sec_tag_list,
	.sec_tag_count = ARRAY_SIZE(sec_tag_list),
	.set_tls_hostname = true,
#endif
};

#if CONFIG_SAMPLE_COMPUTE_HASH
#include <mbedtls/sha256.h>
static mbedtls_sha256_context sha256_ctx;
#endif

static int64_t ref_time;

#if CONFIG_SAMPLE_SECURE_SOCKET
static int cert_provision(void)
{
	int err;

	printk("Provisioning certificate\n");

#if CONFIG_MODEM_KEY_MGMT
	bool exists;

	err = modem_key_mgmt_exists(SEC_TAG,
				    MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				    &exists);
	if (err) {
		printk("Failed to check for certificates err %d\n", err);
		return err;
	}

	if (exists) {
		printk("Certificate ");
		/* Let's compare the existing credential */
		err = modem_key_mgmt_cmp(SEC_TAG,
					 MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
					 cert, sizeof(cert));

		printk("%s\n", err ? "mismatch" : "match");

		if (!err) {
			return 0;
		}
	}

	/*  Provision certificate to the modem */
	err = modem_key_mgmt_write(SEC_TAG,
				   MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				   cert, sizeof(cert));
	if (err) {
		printk("Failed to provision certificate, err %d\n", err);
		return err;
	}
#else /* CONFIG_MODEM_KEY_MGMT */
	err = tls_credential_add(SEC_TAG,
				 TLS_CREDENTIAL_CA_CERTIFICATE,
				 cert,
				 sizeof(cert));
	if (err == -EEXIST) {
		printk("CA certificate already exists, sec tag: %d\n", SEC_TAG);
	} else if (err < 0) {
		printk("Failed to register CA certificate: %d\n", err);
		return err;
	}
#endif /* !CONFIG_MODEM_KEY_MGMT */

	return 0;
}
#endif

static void on_net_event_l4_disconnected(void)
{
	printk("Disconnected from network\n");
}

static void on_net_event_l4_connected(void)
{
	k_sem_give(&network_connected_sem);
}

static void l4_event_handler(struct net_mgmt_event_callback *cb,
			     uint32_t event,
			     struct net_if *iface)
{
	switch (event) {
	case NET_EVENT_L4_CONNECTED:
		printk("IP Up\n");
		on_net_event_l4_connected();
		break;
	case NET_EVENT_L4_DISCONNECTED:
		printk("IP down\n");
		on_net_event_l4_disconnected();
		break;
	default:
		break;
	}
}

static void connectivity_event_handler(struct net_mgmt_event_callback *cb,
				       uint32_t event,
				       struct net_if *iface)
{
	if (event == NET_EVENT_CONN_IF_FATAL_ERROR) {
		printk("Fatal error received from the connectivity layer, rebooting\n");
		return;
	}
}

static void progress_print(size_t downloaded, size_t file_size)
{
	const int percent = (downloaded * 100) / file_size;
	size_t lpad = (percent * PROGRESS_WIDTH) / 100;
	size_t rpad = PROGRESS_WIDTH - lpad;

	printk("\r[ %3d%% ] |", percent);
	for (size_t i = 0; i < lpad; i++) {
		printk("=");
	}
	for (size_t i = 0; i < rpad; i++) {
		printk(" ");
	}
	printk("| (%d/%d bytes)", downloaded, file_size);
}

static int callback(const struct download_client_evt *event)
{
	static size_t downloaded;
	static size_t file_size;
	uint32_t speed;
	int64_t ms_elapsed;

	if (downloaded == 0) {
		download_client_file_size_get(&downloader, &file_size);
		downloaded += STARTING_OFFSET;
	}

	switch (event->id) {
	case DOWNLOAD_CLIENT_EVT_FRAGMENT:
		downloaded += event->fragment.len;
		if (file_size) {
			progress_print(downloaded, file_size);
		} else {
			printk("\r[ %d bytes ] ", downloaded);
		}

#if CONFIG_SAMPLE_COMPUTE_HASH
		mbedtls_sha256_update(&sha256_ctx,
			event->fragment.buf, event->fragment.len);
#endif
		return 0;

	case DOWNLOAD_CLIENT_EVT_DONE:
		ms_elapsed = k_uptime_delta(&ref_time);
		speed = ((float)file_size / ms_elapsed) * MSEC_PER_SEC;
		printk("\nDownload completed in %lld ms @ %d bytes per sec, total %d bytes\n",
		       ms_elapsed, speed, downloaded);

#if CONFIG_SAMPLE_COMPUTE_HASH
		uint8_t hash[32];
		uint8_t hash_str[64 + 1];

		mbedtls_sha256_finish(&sha256_ctx, hash);
		mbedtls_sha256_free(&sha256_ctx);

		bin2hex(hash, sizeof(hash), hash_str, sizeof(hash_str));

		printk("SHA256: %s\n", hash_str);

#if CONFIG_SAMPLE_COMPARE_HASH
		if (strcmp(hash_str, CONFIG_SAMPLE_SHA256_HASH)) {
			printk("Expect: %s\n", CONFIG_SAMPLE_SHA256_HASH);
			printk("SHA256 mismatch!\n");
		}
#endif /* CONFIG_SAMPLE_COMPARE_HASH */
#endif /* CONFIG_SAMPLE_COMPUTE_HASH */

		(void)conn_mgr_if_disconnect(net_if);
		(void)conn_mgr_all_if_down(true);
		printk("Bye\n");
		return 0;

	case DOWNLOAD_CLIENT_EVT_ERROR:
		printk("Error %d during download\n", event->error);
		if (event->error == -ECONNRESET) {
			/* With ECONNRESET, allow library to attempt a reconnect by returning 0 */
		} else {
			(void)conn_mgr_if_disconnect(net_if);
			(void)conn_mgr_all_if_down(true);
			/* Stop download */
			return -1;
		}
		break;
	case DOWNLOAD_CLIENT_EVT_CLOSED:
		printk("Socket closed\n");
		break;
	}

	return 0;
}

int main(void)
{
	int err;

	printk("Download client sample started\n");

	/* Setup handler for Zephyr NET Connection Manager events. */
	net_mgmt_init_event_callback(&l4_cb, l4_event_handler, L4_EVENT_MASK);
	net_mgmt_add_event_callback(&l4_cb);

	/* Setup handler for Zephyr NET Connection Manager Connectivity layer. */
	net_mgmt_init_event_callback(&conn_cb, connectivity_event_handler, CONN_LAYER_EVENT_MASK);
	net_mgmt_add_event_callback(&conn_cb);

	err = conn_mgr_all_if_up(true);
	if (err) {
		printk("conn_mgr_all_if_up, error: %d\n", err);
		return err;
	}

#if CONFIG_SAMPLE_SECURE_SOCKET
	/* Provision certificates before connecting to the network */
	err = cert_provision();
	if (err) {
		return 0;
	}
#endif

	printk("Connecting to network\n");

	err = conn_mgr_all_if_connect(true);
	if (err) {
		printk("conn_mgr_all_if_connect, error: %d\n", err);
		return err;
	}

	k_sem_take(&network_connected_sem, K_FOREVER);

	printk("Network connected\n");

	err = download_client_init(&downloader, callback);
	if (err) {
		printk("Failed to initialize the client, err %d", err);
		return 0;
	}

#if CONFIG_SAMPLE_COMPUTE_HASH
	mbedtls_sha256_init(&sha256_ctx);
	mbedtls_sha256_starts(&sha256_ctx, false);
#endif

	ref_time = k_uptime_get();

	err = download_client_get(&downloader, URL, &config, URL, STARTING_OFFSET);
	if (err) {
		printk("Failed to start the downloader, err %d", err);
		return 0;
	}

	printk("Downloading %s\n", URL);

	return 0;
}
