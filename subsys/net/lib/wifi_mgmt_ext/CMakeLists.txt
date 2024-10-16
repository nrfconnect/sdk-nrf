#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(wifi_mgmt_ext.c)
zephyr_library_sources_ifdef(
	CONFIG_WIFI_CREDENTIALS_SHELL_AUTOCONNECT
	wifi_cred_shell_autoconnect.c)

if(DEFINED CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE AND NOT DEFINED CONFIG_NET_L2_WIFI_SHELL)
  # Wi-Fi Enterprise test certificates handling
  set(gen_inc_dir ${ZEPHYR_BINARY_DIR}/misc/generated)
  set(gen_dir ${gen_inc_dir}/wifi_enterprise_test_certs)
  # Create output directory for test certs
  file(MAKE_DIRECTORY ${gen_dir})

  # convert .pem files to array data at build time
  zephyr_include_directories(${gen_inc_dir})

  generate_inc_file_for_target(
      app
      ${ZEPHYR_BASE}/samples/net/wifi/test_certs/client.pem
      ${gen_dir}/client.pem.inc
      )

  generate_inc_file_for_target(
      app
      ${ZEPHYR_BASE}/samples/net/wifi/test_certs/client-key.pem
      ${gen_dir}/client-key.pem.inc
      )

  generate_inc_file_for_target(
      app
      ${ZEPHYR_BASE}/samples/net/wifi/test_certs/ca.pem
      ${gen_dir}/ca.pem.inc
      )
endif()
