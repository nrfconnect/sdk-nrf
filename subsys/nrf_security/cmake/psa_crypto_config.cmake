#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
# Convert all standard Kconfig variables for mbed TLS (strip CONFIG_)

# PSA Core implementation
kconfig_check_and_set_base_to_one(PSA_CORE_OBERON)

# Convert CRACEN driver configuration
kconfig_check_and_set_base_to_one(PSA_CRYPTO_DRIVER_CRACEN)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_CCM_AES)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_GCM_AES)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_CHACHA20_POLY1305)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_AEAD_DRIVER)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_CTR_AES)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_CBC_PKCS7_AES)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_CBC_NO_PADDING_AES)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECB_NO_PADDING_AES)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_STREAM_CIPHER_CHACHA20)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_CIPHER_DRIVER)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDH_BRAINPOOL_P_R1_192)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDH_BRAINPOOL_P_R1_224)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDH_BRAINPOOL_P_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDH_BRAINPOOL_P_R1_320)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDH_BRAINPOOL_P_R1_384)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDH_BRAINPOOL_P_R1_512)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDH_BRAINPOOL_P_R1)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDH_SECP_R1_192)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDH_SECP_R1_224)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDH_SECP_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDH_SECP_R1_384)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDH_SECP_R1_521)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDH_SECP_R1)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDH_MONTGOMERY_255)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDH_MONTGOMERY_448)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDH_MONTGOMERY)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDH_SECP_K1_192)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDH_SECP_K1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDH_SECP_K1)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_AGREEMENT_DRIVER)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDSA_BRAINPOOL_P_R1_192)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDSA_BRAINPOOL_P_R1_224)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDSA_BRAINPOOL_P_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDSA_BRAINPOOL_P_R1_320)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDSA_BRAINPOOL_P_R1_384)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDSA_BRAINPOOL_P_R1_512)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDSA_BRAINPOOL_P_R1)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDSA_SECP_R1_192)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDSA_SECP_R1_224)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDSA_SECP_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDSA_SECP_R1_384)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDSA_SECP_R1_521)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDSA_SECP_R1)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDSA_SECP_K1_192)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDSA_SECP_K1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDSA_SECP_K1)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_DETERMINISTIC_ECDSA)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECDSA)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_PURE_EDDSA_TWISTED_EDWARDS_255)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_PURE_EDDSA_TWISTED_EDWARDS_448)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_PURE_EDDSA_TWISTED_EDWARDS)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_RSA_PKCS1V15_SIGN)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ED25519PH)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_RSA_PSS)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_ANY_ECC)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_ANY_RSA)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_DRIVER)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_RSA_OAEP)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_RSA_PKCS1V15_CRYPT)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ASYMMETRIC_ENCRYPTION_DRIVER)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_SHA_1)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_SHA_224)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_SHA_256)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_SHA_384)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_SHA_512)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_SHA3_224)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_SHA3_256)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_SHA3_384)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_SHA3_512)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_HASH_DRIVER)

# Key management driver configurations
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_PUBLIC_KEY_SECP_R1_192)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP_R1_192)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECP_R1_192)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECP_R1_192)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_DERIVE_SECP_R1_192)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_R1_192)

kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_PUBLIC_KEY_SECP_R1_224)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP_R1_224)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECP_R1_224)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECP_R1_224)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_DERIVE_SECP_R1_224)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_R1_224)

kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_PUBLIC_KEY_SECP_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECP_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECP_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_DERIVE_SECP_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_R1_256)

kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_PUBLIC_KEY_SECP_R1_384)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP_R1_384)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECP_R1_384)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECP_R1_384)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_DERIVE_SECP_R1_384)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_R1_384)

kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_PUBLIC_KEY_SECP_R1_521)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP_R1_521)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECP_R1_521)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECP_R1_521)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_DERIVE_SECP_R1_521)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_R1_521)

kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_R1)

kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_PUBLIC_KEY_SECP_K1_192)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP_K1_192)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECP_K1_192)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECP_K1_192)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_DERIVE_SECP_K1_192)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_K1_192)

kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_PUBLIC_KEY_SECP_K1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP_K1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECP_K1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECP_K1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_DERIVE_SECP_K1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_K1_256)

kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_K1)

kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_PUBLIC_KEY_MONTGOMERY_255)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_IMPORT_MONTGOMERY_255)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_EXPORT_MONTGOMERY_255)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_GENERATE_MONTGOMERY_255)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_DERIVE_MONTGOMERY_255)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_MONTGOMERY_255)

kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_PUBLIC_KEY_MONTGOMERY_448)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_IMPORT_MONTGOMERY_448)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_EXPORT_MONTGOMERY_448)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_GENERATE_MONTGOMERY_448)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_DERIVE_MONTGOMERY_448)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_MONTGOMERY_448)

kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_MONTGOMERY)


kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_PUBLIC_KEY_TWISTED_EDWARDS_255)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_IMPORT_TWISTED_EDWARDS_255)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_EXPORT_TWISTED_EDWARDS_255)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_GENERATE_TWISTED_EDWARDS_255)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_DERIVE_TWISTED_EDWARDS_255)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_TWISTED_EDWARDS_255)


kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_PUBLIC_KEY_TWISTED_EDWARDS_448)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_IMPORT_TWISTED_EDWARDS_448)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_EXPORT_TWISTED_EDWARDS_448)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_GENERATE_TWISTED_EDWARDS_448)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_DERIVE_TWISTED_EDWARDS_448)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_TWISTED_EDWARDS_448)

kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_TWISTED_EDWARDS)

kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_PUBLIC_KEY_BRAINPOOL_P_R1_192)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_IMPORT_BRAINPOOL_P_R1_192)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_EXPORT_BRAINPOOL_P_R1_192)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_GENERATE_BRAINPOOL_P_R1_192)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_DERIVE_BRAINPOOL_P_R1_192)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_BRAINPOOL_P_R1_192)

kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_PUBLIC_KEY_BRAINPOOL_P_R1_224)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_IMPORT_BRAINPOOL_P_R1_224)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_EXPORT_BRAINPOOL_P_R1_224)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_GENERATE_BRAINPOOL_P_R1_224)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_DERIVE_BRAINPOOL_P_R1_224)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_BRAINPOOL_P_R1_224)

kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_PUBLIC_KEY_BRAINPOOL_P_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_IMPORT_BRAINPOOL_P_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_EXPORT_BRAINPOOL_P_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_GENERATE_BRAINPOOL_P_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_DERIVE_BRAINPOOL_P_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_BRAINPOOL_P_R1_256)

kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_PUBLIC_KEY_BRAINPOOL_P_R1_320)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_IMPORT_BRAINPOOL_P_R1_320)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_EXPORT_BRAINPOOL_P_R1_320)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_GENERATE_BRAINPOOL_P_R1_320)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_DERIVE_BRAINPOOL_P_R1_320)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_BRAINPOOL_P_R1_320)

kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_PUBLIC_KEY_BRAINPOOL_P_R1_384)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_IMPORT_BRAINPOOL_P_R1_384)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_EXPORT_BRAINPOOL_P_R1_384)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_GENERATE_BRAINPOOL_P_R1_384)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_DERIVE_BRAINPOOL_P_R1_384)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_BRAINPOOL_P_R1_384)

kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_PUBLIC_KEY_BRAINPOOL_P_R1_512)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_IMPORT_BRAINPOOL_P_R1_512)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_EXPORT_BRAINPOOL_P_R1_512)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_GENERATE_BRAINPOOL_P_R1_512)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_DERIVE_BRAINPOOL_P_R1_512)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_BRAINPOOL_P_R1_512)

kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_BRAINPOOL_P_R1)

kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_PUBLIC_KEY)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_IMPORT)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_EXPORT)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_GENERATE)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_DERIVE)

kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_SPAKE2P_PUBLIC_KEY_SECP_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_SPAKE2P_KEY_PAIR_IMPORT_SECP_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_SPAKE2P_KEY_PAIR_EXPORT_SECP_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_SPAKE2P_KEY_PAIR_DERIVE_SECP_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_SPAKE2P_SECP_R1_256)

kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_SRP_6_PUBLIC_KEY_3072)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_SRP_6_KEY_PAIR_IMPORT_3072)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_SRP_6_KEY_PAIR_EXPORT_3072)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_SRP_6_3072)

kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_RSA_PUBLIC_KEY)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_RSA_KEY_PAIR_IMPORT)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_RSA_KEY_PAIR_EXPORT)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_TYPE_RSA_KEY_PAIR_GENERATE)

kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_MANAGEMENT_DRIVER)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KMU_ENCRYPTED_KEYS)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KMU_DRIVER)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_PLATFORM_KEYS)

# MAC driver configurations
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_HMAC)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_CMAC)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_MAC_DRIVER)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_SRP_6)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_SRP_PASSWORD_HASH)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_CTR_DRBG_DRIVER)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECJPAKE_SECP_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_ECJPAKE)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_SPAKE2P)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_PAKE_DRIVER)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_HKDF)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_SP800_108_COUNTER_CMAC)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_TLS12_ECJPAKE_TO_PMS)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_TLS12_PRF)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_TLS12_PSK_TO_MS)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_PBKDF2_HMAC)
kconfig_check_and_set_base_to_one(PSA_NEED_CRACEN_KEY_DERIVATION_DRIVER)

# Convert nrf_cc3xx_platform driver configurations
kconfig_check_and_set_base_to_one(PSA_NEED_CC3XX_CTR_DRBG_DRIVER)
kconfig_check_and_set_base_to_one(PSA_NEED_CC3XX_HMAC_DRBG_DRIVER)

# Convert nrf_cc3xx driver configurations
kconfig_check_and_set_base_to_one(PSA_NEED_CC3XX_AEAD_DRIVER)
kconfig_check_and_set_base_to_one(PSA_NEED_CC3XX_ASYMMETRIC_ENCRYPTION_DRIVER)
kconfig_check_and_set_base_to_one(PSA_NEED_CC3XX_CIPHER_DRIVER)
kconfig_check_and_set_base_to_one(PSA_NEED_CC3XX_KEY_AGREEMENT_DRIVER)
kconfig_check_and_set_base_to_one(PSA_NEED_CC3XX_HASH_DRIVER)
kconfig_check_and_set_base_to_one(PSA_NEED_CC3XX_KEY_MANAGEMENT_DRIVER)
kconfig_check_and_set_base_to_one(PSA_NEED_CC3XX_MAC_DRIVER)
kconfig_check_and_set_base_to_one(PSA_NEED_CC3XX_ASYMMETRIC_SIGNATURE_DRIVER)


# Convert nrf_oberon driver configurations
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_AEAD_DRIVER)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ANY_RSA_KEY_SIZE)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ASYMMETRIC_ENCRYPTION_DRIVER)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ASYMMETRIC_SIGNATURE_DRIVER)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_CBC_NO_PADDING_AES)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_CBC_PKCS7_AES)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_CCM_AES)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_CCM_STAR_NO_TAG_AES)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_CHACHA20_POLY1305)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_CIPHER_DRIVER)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_CMAC)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_CTR_AES)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_CTR_DRBG_DRIVER)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ECB_NO_PADDING_AES)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ECDH)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ECDH_MONTGOMERY_255)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ECDH_MONTGOMERY_448)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ECDH_SECP_R1_224)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ECDH_SECP_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ECDH_SECP_R1_384)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ECDH_SECP_R1_521)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ECDSA_DETERMINISTIC)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ECDSA_RANDOMIZED)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ECDSA_SECP_R1_224)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ECDSA_SECP_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ECDSA_SECP_R1_384)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ECDSA_SECP_R1_521)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ECDSA_SIGN)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ECDSA_VERIFY)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ECJPAKE_SECP_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ED25519PH)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ED448PH)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_GCM_AES)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_HASH_DRIVER)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_HKDF)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_HKDF_EXPAND)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_HKDF_EXTRACT)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_HMAC)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_HMAC_DRBG_DRIVER)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_JPAKE)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_AGREEMENT_DRIVER)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_DERIVATION_DRIVER)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_MANAGEMENT_DRIVER)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_MONTGOMERY)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_MONTGOMERY_255)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_MONTGOMERY_448)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_SECP)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_SECP_R1_224)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_SECP_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_SECP_R1_384)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_SECP_R1_521)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_TWISTED_EDWARDS)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_TWISTED_EDWARDS_255)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_TWISTED_EDWARDS_448)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_MONTGOMERY)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_MONTGOMERY_255)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_MONTGOMERY_448)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECP)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECP_R1_224)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECP_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECP_R1_384)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECP_R1_521)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_TWISTED_EDWARDS)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_TWISTED_EDWARDS_255)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_TWISTED_EDWARDS_448)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_MONTGOMERY)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_MONTGOMERY_255)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_MONTGOMERY_448)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECP)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECP_R1_224)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECP_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECP_R1_384)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECP_R1_521)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_TWISTED_EDWARDS)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_TWISTED_EDWARDS_255)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_TWISTED_EDWARDS_448)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_MONTGOMERY)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_MONTGOMERY_255)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_MONTGOMERY_448)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP_R1_224)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP_R1_384)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP_R1_521)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_TWISTED_EDWARDS)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_TWISTED_EDWARDS_255)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_TWISTED_EDWARDS_448)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_MONTGOMERY)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_MONTGOMERY_255)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_MONTGOMERY_448)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_SECP)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_SECP_R1_224)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_SECP_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_SECP_R1_384)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_SECP_R1_521)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_TWISTED_EDWARDS)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_TWISTED_EDWARDS_255)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_TWISTED_EDWARDS_448)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_RSA_KEY_PAIR_EXPORT)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_RSA_KEY_PAIR_IMPORT)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_RSA_PUBLIC_KEY)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_SPAKE2P_KEY_PAIR_DERIVE)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_SPAKE2P_KEY_PAIR_DERIVE_SECP)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_SPAKE2P_KEY_PAIR_DERIVE_SECP_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_SPAKE2P_KEY_PAIR_EXPORT)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_SPAKE2P_KEY_PAIR_EXPORT_SECP)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_SPAKE2P_KEY_PAIR_EXPORT_SECP_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_SPAKE2P_KEY_PAIR_IMPORT)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_SPAKE2P_KEY_PAIR_IMPORT_SECP)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_SPAKE2P_KEY_PAIR_IMPORT_SECP_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_SPAKE2P_PUBLIC_KEY)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_SPAKE2P_PUBLIC_KEY_SECP)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_SPAKE2P_PUBLIC_KEY_SECP_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_SRP_6_KEY_PAIR_EXPORT)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_SRP_6_KEY_PAIR_EXPORT_3072)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_SRP_6_KEY_PAIR_IMPORT)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_SRP_6_KEY_PAIR_IMPORT_3072)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_SRP_6_PUBLIC_KEY)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_SRP_6_PUBLIC_KEY_3072)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_MAC_DRIVER)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_PAKE_DRIVER)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_PBKDF2_AES_CMAC_PRF_128)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_PBKDF2_HMAC)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_PURE_EDDSA_TWISTED_EDWARDS_255)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_PURE_EDDSA_TWISTED_EDWARDS_448)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_RSA_ANY_CRYPT)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_RSA_ANY_SIGN)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_RSA_ANY_VERIFY)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_RSA_KEY_SIZE_1024)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_RSA_KEY_SIZE_1536)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_RSA_KEY_SIZE_2048)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_RSA_KEY_SIZE_3072)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_RSA_KEY_SIZE_4096)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_RSA_KEY_SIZE_6144)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_RSA_KEY_SIZE_8192)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_RSA_OAEP)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_RSA_PKCS1V15_CRYPT)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_RSA_PKCS1V15_SIGN)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_RSA_PSS)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_SHA3)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_SHA3_224)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_SHA3_256)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_SHA3_384)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_SHA3_512)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_SHAKE)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_SHAKE256_512)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_SHA_1)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_SHA_224)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_SHA_256)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_SHA_384)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_SHA_512)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_SPAKE2P)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_SPAKE2P_CMAC_SECP_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_SPAKE2P_HMAC_SECP_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_SPAKE2P_MATTER)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_SRP_6)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_SRP_6_3072)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_SRP_PASSWORD_HASH)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_STREAM_CIPHER_CHACHA20)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_TLS12_ECJPAKE_TO_PMS)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_TLS12_PRF)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_TLS12_PSK_TO_MS)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_AES_KW)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_AES_KWP)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_WRAP_DRIVER)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_WPA3_SAE_PT)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_WPA3_SAE_PT_SECP)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_WPA3_SAE_PT_SECP_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_WPA3_SAE)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_WPA3_SAE_SECP_R1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_SP800_108_COUNTER_CMAC)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_SP800_108_COUNTER_HMAC)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ECDH_SECP_K1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ECDSA_SECP_K1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_HSS_VERIFY)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_ENCAPSULATION_DRIVER)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_SECP_K1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECP_K1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECP_K1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP_K1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_SECP_K1_256)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_HSS_PUBLIC_KEY)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_LMS_PUBLIC_KEY)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ML_DSA_KEY_PAIR_EXPORT)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ML_DSA_KEY_PAIR_IMPORT)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ML_DSA_PUBLIC_KEY)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ML_KEM_KEY_PAIR_EXPORT)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ML_KEM_KEY_PAIR_IMPORT)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_ML_KEM_PUBLIC_KEY)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_XMSS_MT_PUBLIC_KEY)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_KEY_TYPE_XMSS_PUBLIC_KEY)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_LMS_VERIFY)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ML_DSA)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ML_DSA_44)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ML_DSA_65)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ML_DSA_87)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ML_DSA_SIGN)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ML_DSA_VERIFY)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ML_KEM)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ML_KEM_1024)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ML_KEM_512)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_ML_KEM_768)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_SHAKE128_256)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_SHAKE256_192)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_SHAKE256_256)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_SHA_256_192)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_STREAM_CIPHER_XCHACHA20)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_WPA3_SAE_H2E)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_XCHACHA20_POLY1305)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_XMSS_MT_VERIFY)
kconfig_check_and_set_base_to_one(PSA_NEED_OBERON_XMSS_VERIFY)

# Convert NRF_RNG driver configuration
kconfig_check_and_set_base_to_one(PSA_NEED_NRF_RNG_ENTROPY_DRIVER)

# Nordic specific
kconfig_check_and_set_base_to_one(PSA_CRYPTO_DRIVER_ALG_PRNG_TEST)

# PSA and Drivers
kconfig_check_and_set_base_to_one(MBEDTLS_PSA_CRYPTO_STORAGE_C)
kconfig_check_and_set_base_to_one(MBEDTLS_PSA_CRYPTO_DRIVERS)
kconfig_check_and_set_base_int(MBEDTLS_PSA_KEY_SLOT_COUNT)
kconfig_check_and_set_base_to_one(MBEDTLS_PSA_STATIC_KEY_SLOTS)
kconfig_check_and_set_base_int(MBEDTLS_PSA_STATIC_KEY_SLOT_BUFFER_SIZE)

# Generate the PSA config file (default nrf-psa-crypto-config.h)
configure_file(${NRF_SECURITY_ROOT}/configs/psa_crypto_config.h.template
  ${generated_include_path}/${CONFIG_MBEDTLS_PSA_CRYPTO_USER_CONFIG_FILE}
)
