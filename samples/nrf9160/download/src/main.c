/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <string.h>
#include <zephyr.h>
#include <stdlib.h>
#include <modem/bsdlib.h>
#include <modem/lte_lc.h>
#include <modem/at_cmd.h>
#include <modem/at_notif.h>
#include <modem/modem_key_mgmt.h>
#include <net/download_client.h>

#define URL CONFIG_SAMPLE_FILE_URL
#define SEC_TAG CONFIG_SAMPLE_SEC_TAG

#define PROGRESS_WIDTH 50

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

static int64_t ref_time;

/* Initialize AT communications */
static int at_comms_init(void)
{
	int err;

	err = at_cmd_init();
	if (err) {
		printk("Failed to initialize AT commands, err %d\n", err);
		return err;
	}

	err = at_notif_init();
	if (err) {
		printk("Failed to initialize AT notifications, err %d\n", err);
		return err;
	}

	return 0;
}

#if CONFIG_SAMPLE_SECURE_SOCKET
/* Provision certificate to modem */
static int cert_provision(void)
{
	int err;
	bool exists;
	uint8_t unused;

	err = modem_key_mgmt_exists(SEC_TAG,
				    MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				    &exists, &unused);
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
	}

	switch (event->id) {
	case DOWNLOAD_CLIENT_EVT_FRAGMENT:
		downloaded += event->fragment.len;
		if (file_size) {
			progress_print(downloaded, file_size);
		} else {
			printk("\r[ %d bytes ] ", downloaded);
		}
		return 0;

	case DOWNLOAD_CLIENT_EVT_DONE:
		ms_elapsed = k_uptime_delta(&ref_time);
		speed = ((float)file_size / ms_elapsed) * MSEC_PER_SEC;

		printk("\nDownload completed in %lld ms @ %d bytes per sec, total %d bytes\n",
		       ms_elapsed, speed, downloaded);
		printk("Bye\n");
		downloaded = 0;
		return 0;

	case DOWNLOAD_CLIENT_EVT_ERROR:
		printk("Error %d during download\n", event->error);
		downloaded = 0;
		/* Stop download */
		return -1;
	}

	return 0;
}

void main(void)
{
	int err;

	printk("Download client sample started\n");

	err = bsdlib_init();
	if (err) {
		printk("Failed to initialize bsdlib!");
		return;
	}

	/* Initialize AT comms in order to provision the certificate */
	err = at_comms_init();
	if (err) {
		return;
	}

#if CONFIG_SAMPLE_SECURE_SOCKET
	/* Provision certificates before connecting to the network */
	err = cert_provision();
	if (err) {
		return;
	}
#endif

	printk("Waiting for network.. ");
	err = lte_lc_init_and_connect();
	if (err) {
		printk("Failed to connect to the LTE network, err %d\n", err);
		return;
	}

	printk("OK\n");

	err = download_client_init(&downloader, callback);
	if (err) {
		printk("Failed to initialize the client, err %d", err);
		return;
	}

	err = download_client_connect(&downloader, URL, &config);
	if (err) {
		printk("Failed to connect, err %d", err);
		return;
	}

	ref_time = k_uptime_get();

	err = download_client_start(&downloader, URL, 0);
	if (err) {
		printk("Failed to start the downloader, err %d", err);
		return;
	}

	printk("Downloading %s\n", URL);
}
