#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_sources_ifdef(CONFIG_TRUSTED_STORAGE_STORAGE_BACKEND_SETTINGS
	storage_backend_settings.c
)

add_subdirectory_ifdef(CONFIG_PSA_PROTECTED_STORAGE protected_storage)
add_subdirectory_ifdef(CONFIG_PSA_INTERNAL_TRUSTED_STORAGE internal_trusted_storage)
add_subdirectory_ifdef(CONFIG_TRUSTED_STORAGE_BACKEND_AEAD aead)
