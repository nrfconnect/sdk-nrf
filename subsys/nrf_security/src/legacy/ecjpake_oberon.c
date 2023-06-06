/*
 *  Elliptic curve J-PAKE
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
 * References in the code are to the Thread v1.0 Specification,
 * available to members of the Thread Group http://threadgroup.org/
 */

/* Copy-pasted from ecjpake.c in mbed TLS disitribution */


#include "common.h"

#if defined(MBEDTLS_ECJPAKE_C) && defined(MBEDTLS_ECJPAKE_ALT)

#include "mbedtls/ecjpake.h"
#include "mbedtls/platform_util.h"
#include "mbedtls/error.h"

#include <string.h>



int mbedtls_ecjpake_set_point_format( mbedtls_ecjpake_context *ctx,
                                      int point_format )
{
    switch( point_format )
    {
        case MBEDTLS_ECP_PF_UNCOMPRESSED:
        case MBEDTLS_ECP_PF_COMPRESSED:
            ctx->point_format = point_format;
            return( 0 );
        default:
            return( MBEDTLS_ERR_ECP_BAD_INPUT_DATA );
    }
}


#endif
