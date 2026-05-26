#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_TLS_CREDENTIALS_BACKEND_NRF_MODEM)
  zephyr_library()
  zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/lib/tls_credentials)
  zephyr_library_sources(tls_credentials_nrf_modem.c)
endif()
