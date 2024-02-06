/*
 *  Copyright Oberon microsystems AG, Switzerland
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

#ifndef PSA_CRYPTO_DRIVER_CONFIG_H
#define PSA_CRYPTO_DRIVER_CONFIG_H


#if defined(MBEDTLS_PSA_CRYPTO_CONFIG_FILE)
#include MBEDTLS_PSA_CRYPTO_CONFIG_FILE
#else
#include "psa/crypto_config.h"
#endif

#if defined(PSA_USE_DEMO_ENTROPY_DRIVER) || \
    defined(PSA_USE_DEMO_HARDWARE_DRIVER) || \
    defined(PSA_USE_DEMO_OPAQUE_DRIVER)
#include "demo_driver_config.h"
#endif

#endif /* PSA_CRYPTO_DRIVER_CONFIG_H */
