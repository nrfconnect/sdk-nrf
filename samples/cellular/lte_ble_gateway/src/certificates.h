/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _CERTIFICATES_H_
#define _CERTIFICATES_H_

#define NRF_CLOUD_CLIENT_ID "my-client-id"

#define NRF_CLOUD_CLIENT_PRIVATE_KEY \
	"-----BEGIN RSA PRIVATE KEY-----\n" \
	"NRF_CLOUD_CLIENT_PRIVATE_KEY\n" \
	"-----END RSA PRIVATE KEY-----\n"

#define NRF_CLOUD_CLIENT_PUBLIC_CERTIFICATE \
	"-----BEGIN CERTIFICATE-----\n" \
	"NRF_CLOUD_CLIENT_PUBLIC_CERTIFICATE\n" \
	"-----END CERTIFICATE-----\n"

#define NRF_CLOUD_CA_CERTIFICATE \
	"-----BEGIN CERTIFICATE-----\n" \
	"NRF_CLOUD_CA_CERTIFICATE\n" \
	"-----END CERTIFICATE-----\n"

#endif /* _CERTIFICATES_H_ */
