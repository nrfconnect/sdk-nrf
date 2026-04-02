#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_cloud_codec_json_test)

# Test sources: the codec under test plus fakes for its internal helpers
target_sources(app PRIVATE
  src/main.c
  src/fakes.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/src/nrf_cloud_codec.c
)

target_include_directories(app PRIVATE
  src
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/include
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/src
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/mqtt/include
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/mqtt/src
  ${ZEPHYR_BASE}/subsys/testsuite/include
  ${ZEPHYR_CJSON_MODULE_DIR}
)

# Prevent the library versions of these files from being compiled.
# nrf_cloud_codec.c is added above as an explicit source; it depends
# only on the three helper functions and the memory wrappers provided
# in src/fakes.c. nrf_cloud_codec_internal.c and nrf_cloud_mem.c
# bring in heavy transitive dependencies (modem, FOTA, BT) that are
# not needed for these JSON codec unit tests.
#
# NOTE: CBOR codec tests are not yet implemented. They will live in
#       codec/cbor/ and require CONFIG_NRF_CLOUD_COAP=y plus their
#       own prj.conf and CMakeLists.txt.
set_source_files_properties(
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/src/nrf_cloud_codec_internal.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/src/nrf_cloud_mem.c
  PROPERTIES HEADER_FILE_ONLY ON
)

# CONFIG_NRF_CLOUD_LOG_LEVEL is a generated Kconfig symbol that only exists
# when a transport (MQTT/CoAP/REST) is enabled. In this minimal test config
# it must be supplied as a raw preprocessor define.
target_compile_options(app PRIVATE
  -DCONFIG_NRF_CLOUD_LOG_LEVEL=4
)
