#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# nRF54L needs extra sysbuild settings (Pure signature mode, KMU keyfile
# generation, sample PSA cert reference); nRF5340 and nRF91x get the right
# defaults from Kconfig.
if("${BOARD}" MATCHES "^nrf54l")
  list(APPEND SB_EXTRA_CONF_FILE
    "${CMAKE_CURRENT_LIST_DIR}/nrf54l.conf")

  # nrf54l.conf points SB_CONFIG_SECURE_BOOT_SIGNING_KEY_FILE at a built-in
  # sample key. Warn unless the user overrides it with their own key.
  if(NOT DEFINED SB_CONFIG_SECURE_BOOT_SIGNING_KEY_FILE)
    message(WARNING
      "Building with the built-in sample NSIB signing key "
      "(samples/tfm/common/keys/nrf54l_nsib_sample_private.pem). This is a "
      "publicly known dummy key and is NOT secure: do NOT use it in "
      "production. Set SB_CONFIG_SECURE_BOOT_SIGNING_KEY_FILE to your own key "
      "for any non-development build."
    )
  endif()
endif()

find_package(Sysbuild REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(sysbuild LANGUAGES)
