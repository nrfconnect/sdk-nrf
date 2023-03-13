/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>
#include <modem/modem_key_mgmt.h>

#define HTTPS_PORT 443

#define HTTPS_HOSTNAME "example.com"

#define TLS_SEC_TAG 42

/* Certificate for `example.com` */
static const char cert[] = {
#include "../cert/DigiCertGlobalRootCA.pem"
};

BUILD_ASSERT(sizeof(cert) < KB(4), "Certificate too large");

struct lookup_ciphersuite {
	char *name;
	uint16_t value;
	bool supported;
};

/* Taken from the IANA register */
static struct lookup_ciphersuite ciphersuites[] = {
	{ "TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384", 0xC024 },
	{ "TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA", 0xC00A },
	{ "TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256", 0xC023 },
	{ "TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA", 0xC009 },
	{ "TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA", 0xC014 },
	{ "TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256", 0xC027 },
	{ "TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA", 0xC013 },
	{ "TLS_PSK_WITH_AES_256_CBC_SHA", 0x008D },
	{ "TLS_PSK_WITH_AES_128_CBC_SHA256", 0x00AE },
	{ "TLS_PSK_WITH_AES_128_CBC_SHA", 0x008C },
	{ "TLS_PSK_WITH_AES_128_CCM_8", 0xC0A8 },
	{ "TLS_EMPTY_RENEGOTIATIONINFO_SCSV", 0x00FF },
#if defined(CONFIG_EXTENDED_CIPHERSUITE_LIST)
	{ "TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256", 0xC02B },
	{ "TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384", 0xC030 },
	{ "TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256", 0xC02F },
#endif
};

/* Provision certificate to modem */
int cert_provision(void)
{
	int err;
	bool exists;
	int mismatch;

	/* It may be sufficient for the application to check whether the correct
	 * certificate is provisioned with a given tag directly using modem_key_mgmt_cmp().
	 * Here, for the sake of the completeness, we check that a certificate exists
	 * before comparing it with what we expect it to be.
	 */
	err = modem_key_mgmt_exists(TLS_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, &exists);
	if (err) {
		printk("Failed to check for certificates, err %d\n", err);
		return err;
	}

	if (exists) {
		mismatch = modem_key_mgmt_cmp(TLS_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, cert,
					      strlen(cert));
		if (!mismatch) {
			printk("Certificate match\n");
			return 0;
		}

		printk("Certificate mismatch\n");
		err = modem_key_mgmt_delete(TLS_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);
		if (err) {
			printk("Failed to delete existing certificate, err %d\n", err);
		}
	}

	printk("Provisioning certificate\n");

	/*  Provision certificate to the modem */
	err = modem_key_mgmt_write(TLS_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, cert,
				   sizeof(cert) - 1);
	if (err) {
		printk("Failed to provision certificate, err %d\n", err);
		return err;
	}

	return 0;
}

/* Setup TLS options on a given socket */
int tls_setup(int fd)
{
	int err;
	int verify;

	/* Security tag that we have provisioned the certificate with */
	const sec_tag_t tls_sec_tag[] = {
		TLS_SEC_TAG,
	};

	/* Set up TLS peer verification */
	verify = TLS_PEER_VERIFY_REQUIRED;

	err = setsockopt(fd, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(verify));
	if (err) {
		printk("Failed to setup peer verification, err %d, %s\n", errno, strerror(errno));
		return err;
	}

	/* Associate the socket with the security tag
	 * we have provisioned the certificate with.
	 */
	err = setsockopt(fd, SOL_TLS, TLS_SEC_TAG_LIST, tls_sec_tag, sizeof(tls_sec_tag));
	if (err) {
		printk("Failed to setup TLS sec tag, err %d, %s\n", errno, strerror(errno));
		return err;
	}

	err = setsockopt(fd, SOL_TLS, TLS_HOSTNAME, HTTPS_HOSTNAME, sizeof(HTTPS_HOSTNAME) - 1);
	if (err) {
		printk("Failed to setup TLS hostname, err %d, %s\n", errno, strerror(errno));
		return err;
	}
	return 0;
}

void main(void)
{
	int err;
	int fd;
	struct addrinfo *res;
	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
	};
	nrf_sec_cipher_t ciphersuite_list[] = { 0 };

	printk("TLS Ciphersuites sample started\n\r");

	err = nrf_modem_lib_init();
	if (err) {
		printk("Modem library initialization failed, error: %d\n", err);
		return;
	}

	/* Provision certificates before connecting to the LTE network */
	err = cert_provision();
	if (err) {
		return;
	}

	printk("Waiting for network.. ");
	err = lte_lc_init_and_connect();
	if (err) {
		printk("Failed to connect to the LTE network, err %d\n", err);
		return;
	}
	printk("OK\n");

	err = getaddrinfo(HTTPS_HOSTNAME, NULL, &hints, &res);
	if (err) {
		printk("getaddrinfo() failed, err %d, %s\n", errno, strerror(errno));
		return;
	}

	((struct sockaddr_in *)res->ai_addr)->sin_port = htons(HTTPS_PORT);

	printk("Trying all ciphersuites to find which ones are supported...\n");

	for (int i = 0; i < ARRAY_SIZE(ciphersuites); i++) {
		fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TLS_1_2);
		if (fd == -1) {
			printk("Failed to open socket!\n");
			goto clean_up;
		}

		/* Setup TLS socket options */
		err = tls_setup(fd);
		if (err) {
			goto clean_up;
		}

		printk("Trying ciphersuite: %s\n", ciphersuites[i].name);

		ciphersuite_list[0] = ciphersuites[i].value;

		err = setsockopt(fd, SOL_TLS, TLS_CIPHERSUITE_LIST, ciphersuite_list,
				 sizeof(ciphersuite_list));
		if (err) {
			printk("Failed to setup ciphersuite list, err %d, %s\n", errno,
			       strerror(errno));
			goto clean_up;
		}

		printk("Connecting to %s... ", HTTPS_HOSTNAME);
		err = connect(fd, res->ai_addr, sizeof(struct sockaddr_in));
		if (err) {
			printk("connect() failed, err: %d, %s\n", errno, strerror(errno));
			ciphersuites[i].supported = false;
		} else {
			printk("Connected.\n");
			ciphersuites[i].supported = true;
		}

		close(fd);
	}

	fd = -1;

	printk("\nCiphersuite support summary for host `%s`:\n", HTTPS_HOSTNAME);

	for (int i = 0; i < ARRAY_SIZE(ciphersuites); i++) {
		printk("%s: %s\n", ciphersuites[i].name,
		       ciphersuites[i].supported ? "Yes" : "No");
	}

	printk("\nFinished.\n");

clean_up:
	freeaddrinfo(res);
	(void)close(fd);

	lte_lc_power_off();
}
