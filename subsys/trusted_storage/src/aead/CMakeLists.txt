#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_sources_ifdef(CONFIG_TRUSTED_STORAGE_BACKEND_AEAD
	trusted_backend_aead.c
)
zephyr_sources_ifdef(CONFIG_TRUSTED_STORAGE_BACKEND_AEAD_CRYPTO_PSA_CHACHAPOLY
	aead_crypt_psa_chachapoly.c
)
zephyr_sources_ifdef(CONFIG_TRUSTED_STORAGE_BACKEND_AEAD_NONCE_PSA_SEED_COUNTER
	aead_ctr_nonce.c
)
zephyr_sources_ifdef(CONFIG_TRUSTED_STORAGE_BACKEND_AEAD_KEY_HASH_UID
	aead_key_hash.c
)
zephyr_sources_ifdef(CONFIG_TRUSTED_STORAGE_BACKEND_AEAD_KEY_DERIVE_FROM_HUK
	aead_key_huk.c
)
