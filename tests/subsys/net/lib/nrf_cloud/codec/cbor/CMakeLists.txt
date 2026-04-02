#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_cloud_codec_cbor_test)
set(NRFXLIB_MODEM_DIR ${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem)

# Test sources: the CoAP CBOR codec under test, its generated zcbor
# encoder/decoder files, and stubs for the internal helpers it calls.
target_sources(app PRIVATE
  src/main.c
  src/fakes.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/coap/src/nrf_cloud_coap_codec.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/coap/generated/src/msg_encode.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/coap/generated/src/agnss_encode.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/coap/generated/src/pgps_encode.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/coap/generated/src/pgps_decode.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/coap/generated/src/ground_fix_encode.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/coap/generated/src/ground_fix_decode.c
)

target_include_directories(app PRIVATE
  src
  ${NRFXLIB_MODEM_DIR}/include
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/include
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/src
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/coap/include
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/coap/generated/include
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/mqtt/include
  ${ZEPHYR_BASE}/subsys/testsuite/include
  ${ZEPHYR_CJSON_MODULE_DIR}
)

# Block heavy implementations not needed for CBOR codec unit tests.
# nrf_cloud_coap_codec.c calls only a small subset of the symbols declared
# in these headers; the actual implementations are provided by src/fakes.c.
target_compile_options(app PRIVATE
  # Log level symbols are defined via Kconfig.template.log_config and have no
  # prompt, so they cannot be set in prj.conf without their parent modules
  # enabled. Define them directly as compile options instead.
  -DCONFIG_NRF_CLOUD_LOG_LEVEL=4
  -DCONFIG_NRF_CLOUD_COAP_LOG_LEVEL=4
  # The three options below cannot be set via prj.conf in this test context:
  # - NRF_CLOUD_AGNSS / NRF_CLOUD_PGPS: Kconfig silently ignores them because
  #   their parent dependencies (CONFIG_NRF_CLOUD, etc.) are not satisfied on
  #   native_sim, so the guarded functions would not be compiled in.
  # - NRF_MODEM: platform-level symbol defined in nrfxlib with hardware deps,
  #   not user-configurable on native_sim.
  -DCONFIG_NRF_CLOUD_AGNSS
  -DCONFIG_NRF_CLOUD_PGPS
  -DCONFIG_NRF_MODEM
)

set_source_files_properties(
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/src/nrf_cloud_codec_internal.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/src/nrf_cloud_mem.c
  PROPERTIES HEADER_FILE_ONLY ON
)
