/**
 * \file psa/crypto_adjust_config_key_pair_types.h
 * \brief Adjust PSA configuration for key pair types.
 *
 * See docs/proposed/psa-conditional-inclusion-c.md.
 * - Support non-basic operations in a keypair type implicitly enables basic
 *   support for that keypair type.
 * - Support for a keypair type implicitly enables the corresponding public
 *   key type.
 * - Basic support for a keypair type implicilty enables import/export support
 *   for that keypair type. Warning: this is implementation-specific (mainly
 *   for the benefit of testing) and may change in the future!
 */
/*
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

#ifndef PSA_CRYPTO_ADJUST_KEYPAIR_TYPES_H
#define PSA_CRYPTO_ADJUST_KEYPAIR_TYPES_H

/*****************************************************************
 * ANYTHING -> BASIC
 ****************************************************************/

#if defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT) || \
    defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT) || \
    defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE) || \
    defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE)
#define PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_BASIC 1
#endif

#if defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT) || \
    defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT) || \
    defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE) || \
    defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_DERIVE)
#define PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_BASIC 1
#endif

#if defined(PSA_WANT_KEY_TYPE_DH_KEY_PAIR_IMPORT) || \
    defined(PSA_WANT_KEY_TYPE_DH_KEY_PAIR_EXPORT) || \
    defined(PSA_WANT_KEY_TYPE_DH_KEY_PAIR_GENERATE) || \
    defined(PSA_WANT_KEY_TYPE_DH_KEY_PAIR_DERIVE)
#define PSA_WANT_KEY_TYPE_DH_KEY_PAIR_BASIC 1
#endif

/*****************************************************************
 * BASIC -> corresponding PUBLIC
 ****************************************************************/

#if defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_BASIC)
#define PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY 1
#endif

#if defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_BASIC)
#define PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY 1
#endif

#if defined(PSA_WANT_KEY_TYPE_DH_KEY_PAIR_BASIC)
#define PSA_WANT_KEY_TYPE_DH_PUBLIC_KEY 1
#endif

#endif /* PSA_CRYPTO_ADJUST_KEYPAIR_TYPES_H */
