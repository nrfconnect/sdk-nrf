#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

if(${CMAKE_MAJOR_VERSION} EQUAL "3" AND ${CMAKE_MINOR_VERSION} EQUAL "25")
message(FATAL_ERROR "CMake versions 3.25.x are not compatible wth the PSA architecture tests.\n"
                    "Use an earlier (<= 3.24.x) or later (>= 3.26.x) version of CMake.")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(tfm_psa_test)

target_sources(app PRIVATE src/main.c)

get_target_property(TFM_BINARY_DIR           tfm TFM_BINARY_DIR)
get_target_property(TFM_NS_BIN_FILE          tfm TFM_NS_BIN_FILE)
get_target_property(TFM_NS_HEX_FILE          tfm TFM_NS_HEX_FILE)
get_target_property(TFM_NS_SIGNED_BIN_FILE   tfm TFM_NS_SIGNED_BIN_FILE)

get_target_property(TFM_TOOLCHAIN_PATH       tfm TFM_TOOLCHAIN_PATH)
get_target_property(TFM_TOOLCHAIN_PREFIX     tfm TFM_TOOLCHAIN_PREFIX)
get_target_property(TFM_TOOLCHAIN_NS_FILE    tfm TFM_TOOLCHAIN_NS_FILE)

set(TFM_TEST_REPO_PATH ${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/../tf-m-tests)
set(TFM_PSA_ARCHTEST_REPO_PATH ${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/../psa-arch-tests)

set(TFM_TEST_DIR         "${TFM_TEST_REPO_PATH}/tests_psa_arch/spe/partitions")
set(PSA_ARCH_TESTS_CONFIG_FILE "${TFM_TEST_REPO_PATH}/tests_psa_arch/spe/config/config_test_psa_api.cmake")

if(CONFIG_TFM_PSA_TEST_CRYPTO)
  set(TFM_PSA_TEST_SUITE CRYPTO)
elseif(CONFIG_TFM_PSA_TEST_PROTECTED_STORAGE)
  set(TFM_PSA_TEST_SUITE PROTECTED_STORAGE)
elseif(CONFIG_TFM_PSA_TEST_INTERNAL_TRUSTED_STORAGE)
  set(TFM_PSA_TEST_SUITE INTERNAL_TRUSTED_STORAGE)
elseif(CONFIG_TFM_PSA_TEST_STORAGE)
  set(TFM_PSA_TEST_SUITE STORAGE)
elseif(CONFIG_TFM_PSA_TEST_INITIAL_ATTESTATION)
  set(TFM_PSA_TEST_SUITE INITIAL_ATTESTATION)
endif()

if(NOT DEFINED TFM_PSA_TEST_SUITE)
  message(FATAL_ERROR "Please define witch test suite to run:
    CONFIG_TFM_PSA_TEST_CRYPTO
    CONFIG_TFM_PSA_TEST_PROTECTED_STORAGE
    CONFIG_TFM_PSA_TEST_INTERNAL_TRUSTED_STORAGE
    CONFIG_TFM_PSA_TEST_STORAGE
    CONFIG_TFM_PSA_TEST_INITIAL_ATTESTATION")
endif()
set(TEST_PSA_API "${TFM_PSA_TEST_SUITE}")

set_property(TARGET zephyr_property_target
             APPEND PROPERTY TFM_CMAKE_OPTIONS
             -DCONFIG_TFM_TEST_DIR=${TFM_TEST_DIR}
             -DTFM_PROFILE=profile_large
)

set_property(TARGET zephyr_property_target
             APPEND PROPERTY TFM_CMAKE_OPTIONS
             -DPSA_ARCH_TESTS_PATH=${TFM_PSA_ARCHTEST_REPO_PATH}
)

set_property(TARGET zephyr_property_target
             APPEND PROPERTY TFM_CMAKE_OPTIONS
             -DCONFIG_PSA_ARCH_TESTS_CONFIG_FILE=${PSA_ARCH_TESTS_CONFIG_FILE}
)

set_property(TARGET zephyr_property_target
             APPEND PROPERTY TFM_CMAKE_OPTIONS
             -DTEST_PSA_API=${TEST_PSA_API}
)

set(INSTALL_INTERFACE_INC_DIR ${CMAKE_BINARY_DIR}/tfm/api_ns/interface/include)

file(
  COPY
    ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include/psa/crypto_compat.h
    ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include/psa/crypto_driver_common.h
    ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include/psa/crypto_extra.h
    ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include/psa/crypto_platform.h
    ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include/psa/crypto_sizes.h
    ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include/psa/crypto_struct.h
    ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include/psa/crypto_types.h
    ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include/psa/crypto_values.h
    ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include/psa/crypto.h
    ${NRF_SECURITY_DIR}/include/psa/crypto_driver_contexts_key_derivation.h
    ${NRF_SECURITY_DIR}/include/psa/crypto_driver_contexts_primitives.h
    ${NRF_SECURITY_DIR}/include/psa/crypto_driver_contexts_composites.h
  DESTINATION
    ${INSTALL_INTERFACE_INC_DIR}/psa
)

include(ExternalProject)

if(CONFIG_DEBUG_OPTIMIZATIONS)
  set(tfm_ns_CMAKE_BUILD_TYPE Debug)
else()
  set(tfm_ns_CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

ExternalProject_Add(tfm_psa_arch_test_app
    SOURCE_DIR ${TFM_TEST_REPO_PATH}/tests_psa_arch
    BINARY_DIR ${PROJECT_BINARY_DIR}/tfm_ns
    CONFIGURE_COMMAND
        ${CMAKE_COMMAND}
                -G ${CMAKE_GENERATOR}
                -S ${TFM_TEST_REPO_PATH}/tests_psa_arch
                -B ${PROJECT_BINARY_DIR}/tfm_ns
                -DCONFIG_SPE_PATH=${TFM_BINARY_DIR}/api_ns
                -DTFM_TOOLCHAIN_FILE=cmake/${TFM_TOOLCHAIN_NS_FILE}
                -DCROSS_COMPILE=${TFM_TOOLCHAIN_PATH}/${TFM_TOOLCHAIN_PREFIX}
                -DPSA_TOOLCHAIN_FILE=${TFM_BINARY_DIR}/api_ns/cmake/${TFM_TOOLCHAIN_NS_FILE}
                -DQCBOR_PATH${QCBOR_PATH_TYPE}=${CONFIG_TFM_QCBOR_PATH}
                -DT_COSE_PATH=${CONFIG_TFM_T_COSE_PATH}
                -DCMAKE_BUILD_TYPE=${tfm_ns_CMAKE_BUILD_TYPE}
                -DTEST_PSA_API=${TEST_PSA_API}
                -DZEPHYR_BASE=${ZEPHYR_BASE}
                -DZEPHYR_NRF_MODULE_DIR=${ZEPHYR_NRF_MODULE_DIR}
                -DTFM_BOARDS_NRF_DIR=${TFM_BOARDS_NRF_DIR}
                -DNRF_CONFIG_CPU_FREQ_MHZ=${CONFIG_TFM_CPU_FREQ_MHZ}
    BUILD_COMMAND ${CMAKE_COMMAND} --build .
    INSTALL_COMMAND ""
    BUILD_ALWAYS True
    USES_TERMINAL_BUILD True
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/tfm_ns
    DEPENDS tfm
    BUILD_BYPRODUCTS
        ${TFM_NS_HEX_FILE}
        ${TFM_NS_BIN_FILE}
        ${TFM_NS_SIGNED_BIN_FILE}
)

add_dependencies(app tfm_psa_arch_test_app)
