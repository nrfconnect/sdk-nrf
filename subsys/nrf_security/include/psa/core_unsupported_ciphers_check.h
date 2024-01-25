/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * Copyright (c) since 2020 Oberon microsystems AG
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * The following code is imported from Oberon and should not be modified.
 */

/* Currently Unsupported Algorithms */

#if defined(CONFIG_PSA_WANT_ALG_SHA_512_224) && !defined(CONFIG_PSA_ACCEL_SHA_512_224)
#error "No crypto implementation for SHA-512-224"
#endif
#if defined(CONFIG_PSA_WANT_ALG_SHA_512_256) && !defined(CONFIG_PSA_ACCEL_SHA_512_256)
#error "No crypto implementation for SHA-512-256"
#endif
#if defined(CONFIG_PSA_WANT_ALG_MD5) && !defined(CONFIG_PSA_ACCEL_MD5)
#error "No crypto implementation for MD5"
#endif
#if defined(CONFIG_PSA_WANT_ALG_RIPEMD160) && !defined(CONFIG_PSA_ACCEL_RIPEMD160)
#error "No crypto implementation for RIPEMD160"
#endif

#if defined(CONFIG_PSA_WANT_KEY_TYPE_AES) && defined(CONFIG_PSA_WANT_ALG_CFB)
#if defined(CONFIG_PSA_WANT_AES_KEY_SIZE_128) && !defined(CONFIG_PSA_ACCEL_CFB_AES_128)
#error "No crypto implementation for 128 bit AES-CFB"
#endif
#if defined(CONFIG_PSA_WANT_AES_KEY_SIZE_192) && !defined(CONFIG_PSA_ACCEL_CFB_AES_192)
#error "No crypto implementation for 192 bit AES-CFB"
#endif
#if defined(CONFIG_PSA_WANT_AES_KEY_SIZE_256) && !defined(CONFIG_PSA_ACCEL_CFB_AES_256)
#error "No crypto implementation for 256 bit AES-CFB"
#endif
#endif
#if defined(CONFIG_PSA_WANT_KEY_TYPE_AES) && defined(CONFIG_PSA_WANT_ALG_OFB)
#if defined(CONFIG_PSA_WANT_AES_KEY_SIZE_128) && !defined(CONFIG_PSA_ACCEL_OFB_AES_128)
#error "No crypto implementation for 128 bit AES-OFB"
#endif
#if defined(CONFIG_PSA_WANT_AES_KEY_SIZE_192) && !defined(CONFIG_PSA_ACCEL_OFB_AES_192)
#error "No crypto implementation for 192 bit AES-OFB"
#endif
#if defined(CONFIG_PSA_WANT_AES_KEY_SIZE_256) && !defined(CONFIG_PSA_ACCEL_OFB_AES_256)
#error "No crypto implementation for 256 bit AES-OFB"
#endif
#endif
#if defined(CONFIG_PSA_WANT_KEY_TYPE_AES) && defined(CONFIG_PSA_WANT_ALG_XTS)
#if defined(CONFIG_PSA_WANT_AES_KEY_SIZE_128) && !defined(CONFIG_PSA_ACCEL_XTS_AES_128)
#error "No crypto implementation for 128 bit AES-XTS"
#endif
#if defined(CONFIG_PSA_WANT_AES_KEY_SIZE_192) && !defined(CONFIG_PSA_ACCEL_XTS_AES_192)
#error "No crypto implementation for 192 bit AES-XTS"
#endif
#if defined(CONFIG_PSA_WANT_AES_KEY_SIZE_256) && !defined(CONFIG_PSA_ACCEL_XTS_AES_256)
#error "No crypto implementation for 256 bit AES-XTS"
#endif
#endif
#if defined(CONFIG_PSA_WANT_KEY_TYPE_AES) && defined(CONFIG_PSA_WANT_ALG_CBC_MAC)
#if defined(CONFIG_PSA_WANT_AES_KEY_SIZE_128) && !defined(CONFIG_PSA_ACCEL_CBC_MAC_AES_128)
#error "No crypto implementation for 128 bit AES-CBC-MAC"
#endif
#if defined(CONFIG_PSA_WANT_AES_KEY_SIZE_192) && !defined(CONFIG_PSA_ACCEL_CBC_MAC_AES_192)
#error "No crypto implementation for 192 bit AES-CBC-MAC"
#endif
#if defined(CONFIG_PSA_WANT_AES_KEY_SIZE_256) && !defined(CONFIG_PSA_ACCEL_CBC_MAC_AES_256)
#error "No crypto implementation for 256 bit AES-CBC-MAC"
#endif
#endif

#if defined(CONFIG_PSA_WANT_ALG_FFDH)
#if defined(CONFIG_PSA_WANT_DH_KEY_SIZE_2048) && !defined(CONFIG_PSA_ACCEL_FFDH_2048)
#error "No crypto implementation for 2048 bit FFDH"
#endif
#if defined(CONFIG_PSA_WANT_DH_KEY_SIZE_3072) && !defined(CONFIG_PSA_ACCEL_FFDH_3072)
#error "No crypto implementation for 3072 bit FFDH"
#endif
#if defined(CONFIG_PSA_WANT_DH_KEY_SIZE_4096) && !defined(CONFIG_PSA_ACCEL_FFDH_4096)
#error "No crypto implementation for 4096 bit FFDH"
#endif
#if defined(CONFIG_PSA_WANT_DH_KEY_SIZE_6144) && !defined(CONFIG_PSA_ACCEL_FFDH_6144)
#error "No crypto implementation for 6144 bit FFDH"
#endif
#if defined(CONFIG_PSA_WANT_DH_KEY_SIZE_8192) && !defined(CONFIG_PSA_ACCEL_FFDH_8192)
#error "No crypto implementation for 8192 bit FFDH"
#endif
#endif

#if defined(CONFIG_PSA_WANT_ECC_SECP_K1_192) &&                                                    \
	!(defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_PUBLIC_KEY_SECP_K1_192) ||                         \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP_K1_192) ||                    \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECP_K1_192) ||                    \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECP_K1_192))
#error "No crypto implementation for secp-k1-192"
#endif
#if defined(CONFIG_PSA_WANT_ECC_SECP_K1_224) &&                                                    \
	!(defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_PUBLIC_KEY_SECP_K1_224) ||                         \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP_K1_224) ||                    \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECP_K1_224) ||                    \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECP_K1_224))
#error "No crypto implementation for secp-k1-224"
#endif
#if defined(CONFIG_PSA_WANT_ECC_SECP_K1_256) &&                                                    \
	!(defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_PUBLIC_KEY_SECP_K1_256) ||                         \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP_K1_256) ||                    \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECP_K1_256) ||                    \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECP_K1_256))
#error "No crypto implementation for secp-k1-256"
#endif

#if defined(CONFIG_PSA_WANT_ECC_SECP_R1_192) &&                                                    \
	!(defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_PUBLIC_KEY_SECP_R1_192) ||                         \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP_R1_192) ||                    \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECP_R1_192) ||                    \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECP_R1_192))
#error "No crypto implementation for secp-r1-192"
#endif

#if defined(CONFIG_PSA_WANT_ECC_SECT_K1_163) &&                                                    \
	!(defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_PUBLIC_KEY_SECT_K1_163) ||                         \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECT_K1_163) ||                    \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECT_K1_163) ||                    \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECT_K1_163))
#error "No crypto implementation for sect-k1-163"
#endif
#if defined(CONFIG_PSA_WANT_ECC_SECT_K1_233) &&                                                    \
	!(defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_PUBLIC_KEY_SECT_K1_233) ||                         \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECT_K1_233) ||                    \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECT_K1_233) ||                    \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECT_K1_233))
#error "No crypto implementation for sect-k1-233"
#endif
#if defined(CONFIG_PSA_WANT_ECC_SECT_K1_239) &&                                                    \
	!(defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_PUBLIC_KEY_SECT_K1_239) ||                         \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECT_K1_239) ||                    \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECT_K1_239) ||                    \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECT_K1_239))
#error "No crypto implementation for sect-k1-239"
#endif
#if defined(CONFIG_PSA_WANT_ECC_SECT_K1_283) &&                                                    \
	!(defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_PUBLIC_KEY_SECT_K1_283) ||                         \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECT_K1_283) ||                    \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECT_K1_283) ||                    \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECT_K1_283))
#error "No crypto implementation for sect-k1-283"
#endif
#if defined(CONFIG_PSA_WANT_ECC_SECT_K1_409) &&                                                    \
	!(defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_PUBLIC_KEY_SECT_K1_409) ||                         \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECT_K1_409) ||                    \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECT_K1_409) ||                    \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECT_K1_409))
#error "No crypto implementation for sect-k1-409"
#endif
#if defined(CONFIG_PSA_WANT_ECC_SECT_K1_571) &&                                                    \
	!(defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_PUBLIC_KEY_SECT_K1_571) ||                         \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECT_K1_571) ||                    \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECT_K1_571) ||                    \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECT_K1_571))
#error "No crypto implementation for sect-k1-571"
#endif

#if defined(CONFIG_PSA_WANT_ECC_SECT_R1_163) &&                                                    \
	!(defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_PUBLIC_KEY_SECT_R1_163) ||                         \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECT_R1_163) ||                    \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECT_R1_163) ||                    \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECT_R1_163))
#error "No crypto implementation for sect-r1-163"
#endif
#if defined(CONFIG_PSA_WANT_ECC_SECT_R1_233) &&                                                    \
	!(defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_PUBLIC_KEY_SECT_R1_233) ||                         \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECT_R1_233) ||                    \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECT_R1_233) ||                    \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECT_R1_233))
#error "No crypto implementation for sect-r1-233"
#endif
#if defined(CONFIG_PSA_WANT_ECC_SECT_R1_283) &&                                                    \
	!(defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_PUBLIC_KEY_SECT_R1_283) ||                         \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECT_R1_283) ||                    \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECT_R1_283) ||                    \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECT_R1_283))
#error "No crypto implementation for sect-r1-283"
#endif
#if defined(CONFIG_PSA_WANT_ECC_SECT_R1_409) &&                                                    \
	!(defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_PUBLIC_KEY_SECT_R1_409) ||                         \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECT_R1_409) ||                    \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECT_R1_409) ||                    \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECT_R1_409))
#error "No crypto implementation for sect-r1-409"
#endif
#if defined(CONFIG_PSA_WANT_ECC_SECT_R1_571) &&                                                    \
	!(defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_PUBLIC_KEY_SECT_R1_571) ||                         \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECT_R1_571) ||                    \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECT_R1_571) ||                    \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECT_R1_571))
#error "No crypto implementation for sect-r1-571"
#endif

#if defined(CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_160) &&                                             \
	!(defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_PUBLIC_KEY_BRAINPOOL_P_R1_160) ||                  \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_IMPORT_BRAINPOOL_P_R1_160) ||             \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_EXPORT_BRAINPOOL_P_R1_160) ||             \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_GENERATE_BRAINPOOL_P_R1_160))
#error "No crypto implementation for brainpoolP160r1"
#endif
#if defined(CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_192) &&                                             \
	!(defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_PUBLIC_KEY_BRAINPOOL_P_R1_192) ||                  \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_IMPORT_BRAINPOOL_P_R1_192) ||             \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_EXPORT_BRAINPOOL_P_R1_192) ||             \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_GENERATE_BRAINPOOL_P_R1_192))
#error "No crypto implementation for brainpoolP192r1"
#endif
#if defined(CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_224) &&                                             \
	!(defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_PUBLIC_KEY_BRAINPOOL_P_R1_224) ||                  \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_IMPORT_BRAINPOOL_P_R1_224) ||             \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_EXPORT_BRAINPOOL_P_R1_224) ||             \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_GENERATE_BRAINPOOL_P_R1_224))
#error "No crypto implementation for brainpoolP224r1"
#endif
#if defined(CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_256) &&                                             \
	!(defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_PUBLIC_KEY_BRAINPOOL_P_R1_256) ||                  \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_IMPORT_BRAINPOOL_P_R1_256) ||             \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_EXPORT_BRAINPOOL_P_R1_256) ||             \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_GENERATE_BRAINPOOL_P_R1_256))
#error "No crypto implementation for brainpoolP256r1"
#endif
#if defined(CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_320) &&                                             \
	!(defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_PUBLIC_KEY_BRAINPOOL_P_R1_320) ||                  \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_IMPORT_BRAINPOOL_P_R1_320) ||             \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_EXPORT_BRAINPOOL_P_R1_320) ||             \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_GENERATE_BRAINPOOL_P_R1_320))
#error "No crypto implementation for brainpoolP320r1"
#endif
#if defined(CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_384) &&                                             \
	!(defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_PUBLIC_KEY_BRAINPOOL_P_R1_384) ||                  \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_IMPORT_BRAINPOOL_P_R1_384) ||             \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_EXPORT_BRAINPOOL_P_R1_384) ||             \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_GENERATE_BRAINPOOL_P_R1_384))
#error "No crypto implementation for brainpoolP384r1"
#endif
#if defined(CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_512) &&                                             \
	!(defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_PUBLIC_KEY_BRAINPOOL_P_R1_512) ||                  \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_IMPORT_BRAINPOOL_P_R1_512) ||             \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_EXPORT_BRAINPOOL_P_R1_512) ||             \
	  defined(CONFIG_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_GENERATE_BRAINPOOL_P_R1_512))
#error "No crypto implementation for brainpoolP512r1"
#endif

#if defined(CONFIG_PSA_WANT_KEY_TYPE_ARIA) && !defined(CONFIG_PSA_ACCEL_ARIA)
#error "No crypto implementation for ARIA"
#endif
#if defined(CONFIG_PSA_WANT_KEY_TYPE_CAMELLIA) && !defined(CONFIG_PSA_ACCEL_CAMELLIA)
#error "No crypto implementation for CAMELLIA"
#endif
#if defined(CONFIG_PSA_WANT_KEY_TYPE_DES) && !defined(CONFIG_PSA_ACCEL_DES)
#error "No crypto implementation for DES"
#endif
