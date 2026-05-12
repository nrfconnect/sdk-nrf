#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_PSA_CORE_OBERON)
  add_subdirectory(nrf_oberon)
elseif(CONFIG_PSA_CORE_LITE)
  add_subdirectory(lite)
else()
  message(FATAL_ERROR "CONFIG_MBEDTLS_PSA_CRYPTO_C enabled without PSA core")
endif()
