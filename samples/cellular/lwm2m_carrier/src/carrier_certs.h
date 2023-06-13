/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CARRIER_CERTS_H__
#define CARRIER_CERTS_H__

/**
 * @brief Provision CA certificates to modem if needed.
 *
 * @retval  0 in case of success
 * @retval -1 in case of failure
 */
int carrier_cert_provision(void);

/**
 * @brief Provision a PSK to modem.
 *
 * @retval  0 in case of success
 * @retval -1 in case of failure
 */
int carrier_psk_provision(void);

#endif /* CARRIER_CERTS_H__ */
