#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Services
add_subdirectory_ifdef(CONFIG_SSF_DEVICE_INFO_SERVICE_ENABLED device_info)
add_subdirectory_ifdef(CONFIG_SSF_ENC_FW_SERVICE_ENABLED enc_fw)
add_subdirectory_ifdef(CONFIG_SSF_EXTMEM_SERVICE_ENABLED extmem)
add_subdirectory_ifdef(CONFIG_SSF_PSA_CRYPTO_SERVICE_ENABLED psa_crypto)
add_subdirectory_ifdef(CONFIG_SSF_SUIT_SERVICE_ENABLED suit_service)
