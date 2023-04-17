/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
#include <modem/modem_key_mgmt.h>
#include <net/download_client.h>

#define URL CONFIG_SAMPLE_FILE_URL
#define SEC_TAG CONFIG_SAMPLE_SEC_TAG

#define PROGRESS_WIDTH 50
#define STARTING_OFFSET 0

#if CONFIG_SAMPLE_SECURE_SOCKET
static const char cert[] = {
	#include CONFIG_SAMPLE_CERT_FILE
};
BUILD_ASSERT(sizeof(cert) < KB(4), "Certificate too large");
#endif

static struct download_client downloader;
static struct download_client_cfg config = {
#if CONFIG_SAMPLE_SECURE_SOCKET
	.sec_tag = SEC_TAG,
#else
	.sec_tag = -1,
#endif
};

#if CONFIG_SAMPLE_COMPUTE_HASH
#include <mbedtls/sha256.h>
static mbedtls_sha256_context sha256_ctx;
#endif

static int64_t ref_time;

#if CONFIG_SAMPLE_SECURE_SOCKET
/* Provision certificate to modem */
static int cert_provision(void)
{
	int err;
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
					 cert, sizeof(cert) - 1);
		printk("%s\n", err ? "mismatch" : "match");
		if (!err) {
			return 0;
		}
	}

	printk("Provisioning certificate\n");

	/*  Provision certificate to the modem */
	err = modem_key_mgmt_write(SEC_TAG,
				   MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				   cert, sizeof(cert) - 1);
	if (err) {
		printk("Failed to provision certificate, err %d\n", err);
		return err;
	}

	return 0;
}
#endif

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

		lte_lc_power_off();
		printk("Bye\n");
		return 0;

	case DOWNLOAD_CLIENT_EVT_ERROR:
		printk("Error %d during download\n", event->error);
		if (event->error == -ECONNRESET) {
			/* With ECONNRESET, allow library to attempt a reconnect by returning 0 */
		} else {
			lte_lc_power_off();
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

	err = nrf_modem_lib_init();
	if (err) {
		printk("Modem library initialization failed, error: %d\n", err);
		return 0;
	}

#if CONFIG_SAMPLE_SECURE_SOCKET
	/* Provision certificates before connecting to the network */
	err = cert_provision();
	if (err) {
		return 0;
	}
#endif

	printk("Waiting for network.. ");
	err = lte_lc_init_and_connect();
	if (err) {
		printk("Failed to connect to the LTE network, err %d\n", err);
		return 0;
	}

	printk("OK\n");
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
