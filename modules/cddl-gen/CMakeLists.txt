#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(NOT CONFIG_CDDL_GEN)
  return()
endif()

zephyr_library()
zephyr_library_sources(
  ${ZEPHYR_CDDL_GEN_MODULE_DIR}/src/cbor_decode.c
  ${ZEPHYR_CDDL_GEN_MODULE_DIR}/src/cbor_encode.c
  ${ZEPHYR_CDDL_GEN_MODULE_DIR}/src/cbor_common.c
  )

zephyr_include_directories(
  ${ZEPHYR_CDDL_GEN_MODULE_DIR}/include
  )
