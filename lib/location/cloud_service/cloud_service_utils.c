/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <assert.h>

#include <zephyr/logging/log.h>
#include <zephyr/net/tls_credentials.h>
#include <modem/location.h>
#include <modem/modem_key_mgmt.h>

LOG_MODULE_DECLARE(location, CONFIG_LOCATION_LOG_LEVEL);

/* Note: This file is currently compiled only when CONFIG_LOCATION_SERVICE_HERE is defined.
 *       If functions related to other services are added, some existing parts of this file
 *       must be flagged with CONFIG_LOCATION_SERVICE_HERE.
 */

/* TLS certificate:
 *	GlobalSign Root CA - R3
 *	CN=GlobalSign
 *	O=GlobalSign
 *	OU=GlobalSign Root CA - R3
 *	Serial number=04:00:00:00:00:01:21:58:53:08:A2
 *	Valid from=18 March 2009
 *	Valid to=18 March 2029
 *	Download url=https://secure.globalsign.com/cacert/root-r3.crt
 */
static const char tls_certificate_here[] =
	"-----BEGIN CERTIFICATE-----\n"
	"MIIDXzCCAkegAwIBAgILBAAAAAABIVhTCKIwDQYJKoZIhvcNAQELBQAwTDEgMB4G\n"
	"A1UECxMXR2xvYmFsU2lnbiBSb290IENBIC0gUjMxEzARBgNVBAoTCkdsb2JhbFNp\n"
	"Z24xEzARBgNVBAMTCkdsb2JhbFNpZ24wHhcNMDkwMzE4MTAwMDAwWhcNMjkwMzE4\n"
	"MTAwMDAwWjBMMSAwHgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMzETMBEG\n"
	"A1UEChMKR2xvYmFsU2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjCCASIwDQYJKoZI\n"
	"hvcNAQEBBQADggEPADCCAQoCggEBAMwldpB5BngiFvXAg7aEyiie/QV2EcWtiHL8\n"
	"RgJDx7KKnQRfJMsuS+FggkbhUqsMgUdwbN1k0ev1LKMPgj0MK66X17YUhhB5uzsT\n"
	"gHeMCOFJ0mpiLx9e+pZo34knlTifBtc+ycsmWQ1z3rDI6SYOgxXG71uL0gRgykmm\n"
	"KPZpO/bLyCiR5Z2KYVc3rHQU3HTgOu5yLy6c+9C7v/U9AOEGM+iCK65TpjoWc4zd\n"
	"QQ4gOsC0p6Hpsk+QLjJg6VfLuQSSaGjlOCZgdbKfd/+RFO+uIEn8rUAVSNECMWEZ\n"
	"XriX7613t2Saer9fwRPvm2L7DWzgVGkWqQPabumDk3F2xmmFghcCAwEAAaNCMEAw\n"
	"DgYDVR0PAQH/BAQDAgEGMA8GA1UdEwEB/wQFMAMBAf8wHQYDVR0OBBYEFI/wS3+o\n"
	"LkUkrk1Q+mOai97i3Ru8MA0GCSqGSIb3DQEBCwUAA4IBAQBLQNvAUKr+yAzv95ZU\n"
	"RUm7lgAJQayzE4aGKAczymvmdLm6AC2upArT9fHxD4q/c2dKg8dEe3jgr25sbwMp\n"
	"jjM5RcOO5LlXbKr8EpbsU8Yt5CRsuZRj+9xTaGdWPoO4zzUhw8lo/s7awlOqzJCK\n"
	"6fBdRoyV3XpYKBovHd7NADdBj+1EbddTKJd+82cEHhXXipa0095MJ6RMG3NzdvQX\n"
	"mcIfeg7jLQitChws/zyrVQ4PkX4268NXSb7hLi18YIvDQVETI53O9zJrlAGomecs\n"
	"Mx86OyXShkDOOyyGeMlhLxS67ttVb9+E7gUJTb0o2HLO02JQZR7rkpeDMdmztcpH\n"
	"WD9f\n"
	"-----END CERTIFICATE-----\n";

static const char *cloud_service_utils_get_ca_certificate_here(void)
{
	return tls_certificate_here;
}

static const char *cloud_service_utils_get_ca_certificate(enum location_service service)
{
	if (service == LOCATION_SERVICE_HERE) {
		return cloud_service_utils_get_ca_certificate_here();
	}

	return NULL;
}

static int cloud_service_utils_provision_certificate(
	int sec_tag,
	const char *certificate)
{
	int err;
	bool exists;

	if (certificate == NULL) {
		LOG_ERR("No certificate was provided by the location service");
		return -EFAULT;
	}

	err = modem_key_mgmt_exists(sec_tag, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, &exists);
	if (err) {
		LOG_ERR("Failed to check for certificates err %d", err);
		return err;
	}

	if (exists) {
		LOG_DBG("A certificate is already provisioned to sec tag %d", sec_tag);
		return 0;
	}

	LOG_INF("Provisioning certificate");

	/*  Provision certificate to the modem */
	err = modem_key_mgmt_write(sec_tag,
				   MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				   certificate,
				   strlen(certificate));
	if (err) {
		LOG_ERR("Failed to provision certificate, err %d", err);
		return err;
	}

	return 0;
}

int cloud_service_utils_provision_ca_certificates(void)
{
	int ret;

	ret = cloud_service_utils_provision_certificate(
		CONFIG_LOCATION_SERVICE_HERE_TLS_SEC_TAG,
		cloud_service_utils_get_ca_certificate(LOCATION_SERVICE_HERE));

	return ret;
}
