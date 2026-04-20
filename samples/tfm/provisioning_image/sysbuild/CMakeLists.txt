#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

find_package(Sysbuild REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(sysbuild LANGUAGES)

# On nRF54L, b0 looks up its public key in CRACEN KMU instead of OTP PROVISION.
# Generate keyfile.json so that "west flash --recover" provisions the NSIB
# Ed25519 public key to KMU via "nrfutil device x-provision-keys".
#
# Override the default key with -DNSIB_KEY_FILE=<pem>. The default matches
# the key used by tfm_psa_template so the two samples agree out of the box.
set(_default_nsib_key
    "${CMAKE_CURRENT_LIST_DIR}/../../common/keys/nrf54l_nsib_sample_private.pem")

if("${BOARD}" MATCHES "^nrf54l")
  if("${NSIB_KEY_FILE}" STREQUAL "")
    if(EXISTS "${_default_nsib_key}")
      set(NSIB_KEY_FILE "${_default_nsib_key}")
      message(STATUS "NSIB_KEY_FILE not set -- using sample key: ${NSIB_KEY_FILE}")
    else()
      message(WARNING
        "NSIB_KEY_FILE is not set and the default sample key is missing. "
        "keyfile.json will not be generated, and b0 will fail to validate "
        "firmware (-102). Pass -DNSIB_KEY_FILE=<pem> to fix."
      )
    endif()
  endif()

  if(NOT "${NSIB_KEY_FILE}" STREQUAL "")
    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    add_custom_command(
      OUTPUT  ${CMAKE_BINARY_DIR}/keyfile.json
      COMMAND ${Python3_EXECUTABLE} -m west ncs-provision upload
              --keyname BL_PUBKEY
              --key     ${NSIB_KEY_FILE}
              --build-dir ${CMAKE_BINARY_DIR}
              --soc     ${SB_CONFIG_SOC}
              --dry-run
      DEPENDS ${NSIB_KEY_FILE}
      COMMENT "Generating KMU keyfile for NSIB b0 key provisioning (keyfile.json)"
      VERBATIM
    )

    add_custom_target(generate_kmu_keyfile_json ALL
      DEPENDS ${CMAKE_BINARY_DIR}/keyfile.json
    )
  endif()
endif()

unset(_default_nsib_key)
