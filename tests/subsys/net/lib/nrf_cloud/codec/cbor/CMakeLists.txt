#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_cloud_codec_cbor_test)

set(nrfxlib_modem_dir ${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem)
zephyr_include_directories(${nrfxlib_modem_dir}/include)

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
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/include
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/src
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/coap/include
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/coap/generated/include
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/mqtt/include
  ${ZEPHYR_BASE}/subsys/testsuite/include
  ${ZEPHYR_CJSON_MODULE_DIR}
)

# Enabling NRF_CLOUD_AGNSS / NRF_CLOUD_PGPS in Kconfig causes the nRF
# Cloud library CMakeLists to be added and a number of helper sources
# to be compiled.  Those sources reference Kconfig symbols that are
# only generated when the full NRF_CLOUD subsystem is enabled, which
# is incompatible with native_sim / qemu_cortex_m3 builds.
#
# Exclude them via HEADER_FILE_ONLY.  The CBOR codec functions under
# test are supplied directly via target_sources above, and fakes.c
# provides the few internal helpers they reach into.
set_source_files_properties(
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/src/nrf_cloud_codec_internal.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/src/nrf_cloud_log.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/src/nrf_cloud_codec.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/src/nrf_cloud_mem.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/src/nrf_cloud_client_id.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/src/nrf_cloud_sec_tag.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/src/nrf_cloud_info.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/src/nrf_cloud_agnss.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/src/nrf_cloud_agnss_utils.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/src/nrf_cloud_pgps.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/src/nrf_cloud_pgps_utils.c
  DIRECTORY ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/
  PROPERTIES HEADER_FILE_ONLY ON
)
