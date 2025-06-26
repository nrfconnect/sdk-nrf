#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

set(SUIT_GENERATOR_CLI_SCRIPT "${ZEPHYR_SUIT_GENERATOR_MODULE_DIR}/suit_generator/cli.py")
sysbuild_get(DEFAULT_BINARY_DIR IMAGE ${DEFAULT_IMAGE} VAR APPLICATION_BINARY_DIR CACHE)
set(MPI_BINARY_DIR ${DEFAULT_BINARY_DIR}/zephyr)

# Usage:
#   configure_storage_address_cache()
#
# Store the absolute address of the SUIT storage inside CMake cache.
#
function(configure_storage_address_cache)
  dt_nodelabel(
    suit_storage_dev
    TARGET
    ${DEFAULT_IMAGE}
    NODELABEL
    "suit_storage_partition"
    )

  if(NOT DEFINED suit_storage_dev)
    message(WARNING "Unable to read SUIT storage address from DTS - nodes undefined")
    return()
  endif()

  # Calculate SUIT storage address, based on the DTS
  dt_reg_addr(
    suit_storage_address
    TARGET
    ${DEFAULT_IMAGE}
    PATH
    "${suit_storage_dev}"
    )

  set(SUIT_STORAGE_ADDRESS ${suit_storage_address} CACHE STRING "SUIT storage address")
endfunction()

# Usage:
#   generate_mpi_hex(<manifest_role>)
#
# Will generate HEX file for a single manifest role, based on Kconfigs defined by the template.
#
# <manifest_role>: The role that was used to generate a set of Kconfigs, defined by the
#                  Kconfig.template.manifest_config.
#
# Required Kconfigs:
#  - SUIT_MPI_<manifest_role>_VENDOR_NAME
#  - SUIT_MPI_<manifest_role>_CLASS_NAME
#  - SUIT_MPI_<manifest_role>_OFFSET
#  - SUIT_MPI_<manifest_role>_PATH
#  - SUIT_MPI_SLOT_SIZE
#
function(generate_mpi_hex manifest_role)
  set(generate_args "")
  set(descr_args "")

  if(NOT DEFINED SB_CONFIG_SUIT_MPI_${manifest_role})
    return()
  endif()

  if((NOT DEFINED SB_CONFIG_SUIT_MPI_${manifest_role}_VENDOR_NAME) OR
      (NOT DEFINED SB_CONFIG_SUIT_MPI_${manifest_role}_CLASS_NAME) OR
      (NOT DEFINED SB_CONFIG_SUIT_MPI_${manifest_role}_OFFSET) OR
      (NOT DEFINED SB_CONFIG_SUIT_MPI_${manifest_role}_PATH) OR
      (NOT DEFINED SB_CONFIG_SUIT_MPI_SLOT_SIZE)
      )
    message(FATAL_ERROR "Malformed configuration for: ${manifest_role}")
    return()
  endif()

  if(SB_CONFIG_SUIT_MPI_${manifest_role}_VENDOR_NAME STREQUAL "nordicsemi.com")
    message(WARNING "
      -----------------------------------------------------------
      --- WARNING: Using default Vendor ID (nordicsemi.com).  ---
      --- It should not be used for production.               ---
      --- See SB_CONFIG_SUIT_MPI_${manifest_role}_VENDOR_NAME     \t---
      --- and SB_CONFIG_SUIT_MPI_${manifest_role}_CLASS_NAME      \t---
      -----------------------------------------------------------
      \n"
    )
  endif()

  MATH(
    EXPR
    SUIT_MPI_${manifest_role}_ADDRESS
    "${SUIT_STORAGE_ADDRESS}
      + ${SB_CONFIG_SUIT_MPI_${manifest_role}_OFFSET}"
    OUTPUT_FORMAT HEXADECIMAL
  )

  list(APPEND generate_args
    --vendor-name ${SB_CONFIG_SUIT_MPI_${manifest_role}_VENDOR_NAME}
    --class-name ${SB_CONFIG_SUIT_MPI_${manifest_role}_CLASS_NAME}
    --address ${SUIT_MPI_${manifest_role}_ADDRESS}
    --size ${SB_CONFIG_SUIT_MPI_SLOT_SIZE}
    --output-file ${SB_CONFIG_SUIT_MPI_${manifest_role}_PATH}
    )

  list(APPEND descr_args
    ${SB_CONFIG_SUIT_MPI_${manifest_role}_PATH}
    ${SB_CONFIG_SUIT_MPI_${manifest_role}_VENDOR_NAME}
    ${SB_CONFIG_SUIT_MPI_${manifest_role}_CLASS_NAME}
    "address=${SUIT_MPI_${manifest_role}_ADDRESS}"
    )

  if(DEFINED SB_CONFIG_SUIT_MPI_${manifest_role}_DOWNGRADE_PREVENTION)
    list(APPEND generate_args
      --downgrade-prevention-enabled
      )
    list(APPEND descr_args
      downgrade-prevention-enabled
      )
  endif()

  if(DEFINED SB_CONFIG_SUIT_MPI_${manifest_role}_INDEPENDENT_UPDATE)
    list(APPEND generate_args
      --independent-updates
      )
    list(APPEND descr_args
      "independent updates"
      )
  endif()

  if(DEFINED SB_CONFIG_SUIT_MPI_${manifest_role}_SIGNATURE_CHECK_ENABLED_ON_UPDATE)
    list(APPEND generate_args
      --signature-verification update
      )
     list(APPEND descr_args
       "signed updates"
       )
  elseif(DEFINED SB_CONFIG_SUIT_MPI_${manifest_role}_SIGNATURE_CHECK_ENABLED_ON_UPDATE_AND_BOOT)
    list(APPEND generate_args
      --signature-verification update-and-boot
      )
     list(APPEND descr_args
       "signed updates"
       "signed boot"
       )
  endif()

  add_custom_target(
    create_suit_mpi_${manifest_role}
    COMMAND ${PYTHON_EXECUTABLE} ${SUIT_GENERATOR_CLI_SCRIPT}
      mpi
      generate
      ${generate_args}
    BYPRODUCTS ${SB_CONFIG_SUIT_MPI_${manifest_role}_PATH}
    COMMENT "Create SUIT artifacts"
  )
  message(INFO " Generate MPI for ${manifest_role} manifest (${descr_args})")
endfunction()

# Usage:
#   generate_mpi_area(<area_name> [<manifest_role>...])
#
# Will generate HEX file for an MPI area attached to a single domain.
# At the end of the merging process, a digest of the resulting binary will be
# calculated and appended at the end of it.
#
# <area_name>: Name of the area. A valid area must define output file path, address and size.
# <manifest_role>: The role that was used to generate a set of Kconfigs, defined by the
#                  Kconfig.template.manifest_config.
#                  Used to extract the HEX file path containing the MPI configuration.
#
# Required Kconfigs:
#  - SUIT_MPI_<area_name>_PATH
#  - SUIT_MPI_<area_name>_OFFSET
#  - SUIT_MPI_<area_name>_SIZE
#  - SUIT_MPI_<manifest_role>_PATH
#
function (generate_mpi_area area)
  set(merge_args "")
  set(dependencies "")

  if((NOT DEFINED SB_CONFIG_SUIT_MPI_${area}_PATH) OR
      (NOT DEFINED SB_CONFIG_SUIT_MPI_${area}_OFFSET) OR
      (NOT DEFINED SB_CONFIG_SUIT_MPI_${area}_SIZE)
      )
    message(FATAL_ERROR "Malformed configuration for: ${area}")
    return()
  endif()

  # Calculate the absolute address of the MPI area.
  MATH(
    EXPR
    SUIT_MPI_${area}_ADDRESS
    "${SUIT_STORAGE_ADDRESS}
      + ${SB_CONFIG_SUIT_MPI_${area}_OFFSET}"
    OUTPUT_FORMAT HEXADECIMAL
  )

  set(output ${MPI_BINARY_DIR}/${SB_CONFIG_SUIT_MPI_${area}_PATH})
  set(address ${SUIT_MPI_${area}_ADDRESS})
  set(size ${SB_CONFIG_SUIT_MPI_${area}_SIZE})

  list(APPEND merge_args
    --output-file ${output}
    --address ${address}
    --size ${size}
    )

  foreach(role ${ARGN})
    if(NOT DEFINED SB_CONFIG_SUIT_MPI_${role}_PATH)
      continue()
    endif()

    list(APPEND merge_args "--file" "${SB_CONFIG_SUIT_MPI_${role}_PATH}")
    list(APPEND dependencies "${SB_CONFIG_SUIT_MPI_${role}_PATH}")
  endforeach()

  add_custom_target(
    create_suit_mpi_area_${area}
    ALL
    COMMAND ${PYTHON_EXECUTABLE} ${SUIT_GENERATOR_CLI_SCRIPT}
      mpi
      merge
      ${merge_args}
    DEPENDS ${dependencies}
    BYPRODUCTS ${output}
    COMMENT "Create SUIT artifacts"
  )

  message(INFO " Generate merged MPI for ${area} (${output})")
endfunction()

if(DEFINED SB_CONFIG_SOC_SERIES_NRF92X)
  configure_storage_address_cache()
endif() # SB_CONFIG_SOC_SERIES_NRF92X

if(DEFINED SB_CONFIG_SUIT_MPI_GENERATE)
  include(${CMAKE_CURRENT_LIST_DIR}/suit_provisioning_${SB_CONFIG_SOC}.cmake)
endif() # SB_CONFIG_SUIT_MPI_GENERATE
