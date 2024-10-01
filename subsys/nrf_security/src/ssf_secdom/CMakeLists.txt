#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(${mbedcrypto_target}
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/ssf_crypto.c
)
