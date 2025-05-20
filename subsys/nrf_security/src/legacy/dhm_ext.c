/*
 *  Diffie-Hellman-Merkle key exchange
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*
 *  The following sources were referenced in the design of this implementation
 *  of the Diffie-Hellman-Merkle algorithm:
 *
 *  [1] Handbook of Applied Cryptography - 1997, Chapter 12
 *      Menezes, van Oorschot and Vanstone
 *
 */

/* Copied from mbed TLS, missing in CryptoCell runtime library */

#include "common.h"

#if defined(MBEDTLS_DHM_C) && (defined(CONFIG_CC3XX_BACKEND) || defined(CONFIG_PSA_CRYPTO_DRIVER_CC3XX))

#include "mbedtls/dhm.h"
#include "mbedtls/platform_util.h"
#include "mbedtls/error.h"

#include <string.h>

#if defined(MBEDTLS_PEM_PARSE_C)
#include "mbedtls/pem.h"
#endif

#if defined(MBEDTLS_ASN1_PARSE_C)
#include "mbedtls/asn1.h"
#endif

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdlib.h>
#include <stdio.h>
#define mbedtls_printf     printf
#define mbedtls_calloc    calloc
#define mbedtls_free       free
#endif

size_t mbedtls_dhm_get_bitlen( const mbedtls_dhm_context *ctx )
{
    return( mbedtls_mpi_bitlen( &ctx->P ) );
}

size_t mbedtls_dhm_get_len( const mbedtls_dhm_context *ctx )
{
    return( mbedtls_mpi_size( &ctx->P ) );
}


#endif /* defined(MBEDTLS_DHM_C) && defined(CONFIG_CC3XX_BACKEND) */
