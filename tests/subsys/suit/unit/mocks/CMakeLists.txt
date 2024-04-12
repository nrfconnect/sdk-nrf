#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_include_directories(testbinary PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/include
)

if (CONFIG_MOCK_SUIT_PROCESSOR)
  # Include suit-processor directory, so manifest and cose libraries get generated.
  add_subdirectory(${ZEPHYR_SUIT_PROCESSOR_MODULE_DIR} suit_processor)
  target_compile_options(test_interface INTERFACE -DCONFIG_SUIT_PROCESSOR)
  target_include_directories(testbinary PRIVATE
    ${ZEPHYR_SUIT_PROCESSOR_MODULE_DIR}/include
  )
endif()

target_include_directories(testbinary PRIVATE
  ${SUIT_SUBSYS_DIR}/plat_err/include
)

if (CONFIG_MOCK_SUIT_MCI)
  target_compile_options(test_interface INTERFACE -DCONFIG_SUIT_MCI)
  target_include_directories(testbinary PRIVATE
    ${SUIT_SUBSYS_DIR}/mci/include
  )

  if (NOT CONFIG_MOCK_SUIT_MCI_UTILS)
    target_sources(testbinary PRIVATE
      ${SUIT_SUBSYS_DIR}/mci/src/utils.c
    )
  endif()
endif()

if (CONFIG_MOCK_SUIT_METADATA)
  target_compile_options(test_interface INTERFACE -DCONFIG_SUIT_METADATA)
  target_include_directories(testbinary PRIVATE
    ${SUIT_SUBSYS_DIR}/metadata/include
  )
endif()

if (CONFIG_MOCK_SUIT_STORAGE)
  target_compile_options(test_interface INTERFACE -DCONFIG_SUIT_STORAGE)
  target_include_directories(testbinary PRIVATE
    ${SUIT_SUBSYS_DIR}/storage/include
  )
endif()

if (CONFIG_MOCK_SUIT_UTILS)
  target_compile_options(test_interface INTERFACE -DCONFIG_SUIT_UTILS)
  target_include_directories(testbinary PRIVATE ${SUIT_SUBSYS_DIR}/utils/include)
  target_include_directories(testbinary PRIVATE ${ZEPHYR_ZCBOR_MODULE_DIR}/include)

  if (NOT CONFIG_MOCK_SUIT_PLAT_DECODE_UTIL)
    target_sources(testbinary PRIVATE
      ${SUIT_SUBSYS_DIR}/utils/src/suit_plat_decode_util.c)
  endif()
endif()

if (CONFIG_MOCK_SUIT_LOG)
  target_compile_options(test_interface INTERFACE -DCONFIG_SUIT_LOG_LEVEL)
  target_compile_options(test_interface INTERFACE -DCONFIG_SUIT_LOG_LEVEL_DBG)
  target_compile_options(test_interface INTERFACE -include ztest.h)
  target_sources(testbinary PRIVATE $ENV{ZEPHYR_BASE}/subsys/logging/log_minimal.c)
endif()

if (CONFIG_MOCK_SUIT_CRYPTO)
  target_compile_options(test_interface INTERFACE -DCONFIG_SUIT_CRYPTO)
  target_include_directories(testbinary PRIVATE ${ZEPHYR_MBEDTLS_MODULE_DIR}/include)
endif()

if (CONFIG_MOCK_SUIT_PLAT_CHECK_COMPONENT_COMPATIBILITY)
  target_compile_options(test_interface INTERFACE -DCONFIG_SUIT_PLAT_CHECK_COMPONENT_ID)
  target_include_directories(testbinary PRIVATE ${SUIT_SUBSYS_DIR}/platform/include)
endif()

if (CONFIG_MOCK_SUIT_MEMPTR_STORAGE)
  target_compile_options(test_interface INTERFACE -DCONFIG_SUIT_MEMPTR_STORAGE)
  target_include_directories(testbinary PRIVATE ${SUIT_SUBSYS_DIR}/memptr_storage/include)
endif()

if (CONFIG_MOCK_SUIT_PLATFORM_INTERNAL)
  target_compile_options(test_interface INTERFACE -DCONFIG_SUIT_PLATFORM)
  target_include_directories(testbinary PRIVATE ${SUIT_SUBSYS_DIR}/platform/include)
endif()

if (CONFIG_MOCK_SUIT_PLATFORM)
  target_compile_options(test_interface INTERFACE -DCONFIG_SUIT_PLATFORM)
  target_include_directories(testbinary PRIVATE ${SUIT_SUBSYS_DIR}/platform/include)
endif()

if (CONFIG_MOCK_DIGEST_SINK)
  target_compile_options(test_interface INTERFACE -DCONFIG_SUIT_STREAM)
  target_include_directories(testbinary PRIVATE ${SUIT_SUBSYS_DIR}/stream/stream_sinks/include)
endif()

if (CONFIG_MOCK_GENERIC_ADDRESS_STREAMER)
  target_compile_options(test_interface INTERFACE -DCONFIG_SUIT_STREAM)
  target_include_directories(testbinary PRIVATE ${SUIT_SUBSYS_DIR}/stream/stream_sources/include)
endif()

if (CONFIG_MOCK_DFU_CACHE_STREAMER)
  target_compile_options(test_interface INTERFACE -DCONFIG_SUIT_STREAM -DCONFIG_SUIT_CACHE_RW)
  target_include_directories(testbinary PRIVATE ${SUIT_SUBSYS_DIR}/stream/stream_sources/include)
endif()

if (CONFIG_MOCK_DFU_CACHE_SINK)
  target_compile_options(test_interface INTERFACE -DCONFIG_SUIT_STREAM)
  target_include_directories(testbinary PRIVATE ${SUIT_SUBSYS_DIR}/stream/stream_sinks/include)
endif()

if (CONFIG_MOCK_FETCH_SOURCE_STREAMER)
  target_compile_options(test_interface INTERFACE -DCONFIG_SUIT_STREAM -DCONFIG_SUIT_STREAM_FETCH_SOURCE_MGR)
  target_include_directories(testbinary PRIVATE ${SUIT_SUBSYS_DIR}/stream/stream_sources/include)
endif()

if (CONFIG_MOCK_SDFW_BUILTIN_KEYS)
  target_compile_options(test_interface INTERFACE -DCONFIG_SDFW_BUILTIN_KEYS)
  target_include_directories(testbinary PRIVATE ${ZEPHYR_MBEDTLS_MODULE_DIR}/include)
  target_include_directories(testbinary PRIVATE ${ZEPHYR_SECDOM_MODULE_DIR}/include)
endif()
