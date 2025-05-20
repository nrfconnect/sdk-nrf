/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 */
#include <mbedtls/rsa.h>

/* Missing symbol when introducing Mbed TLS 3.6.0 and RSA_ALT*/

/*
 * Get length in bits of RSA modulus
 */
size_t mbedtls_rsa_get_bitlen(const mbedtls_rsa_context *ctx)
{
    return mbedtls_mpi_bitlen(&ctx->N);
}
