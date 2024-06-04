#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

set(SUIT_GENERATOR_CLI_SCRIPT "${ZEPHYR_SUIT_GENERATOR_MODULE_DIR}/suit_generator/cli.py")

# Usage:
#   configure_storage_address_cache()
#
# Store the absolute address of the SUIT storage inside CMake cache.
#
function(configure_storage_address_cache)
  # Calculate SUIT storage address, based on the DTS
  dt_nodelabel(suit_storage_dev NODELABEL suit_storage_partition)

  if (NOT DEFINED suit_storage_dev)
    message(WARNING "Unable to read SUIT storage address from DTS - nodes undefined")
    return()
  endif()

  dt_reg_addr(SUIT_STORAGE_ADDRESS PATH ${suit_storage_dev})
  set(SUIT_STORAGE_ADDRESS ${SUIT_STORAGE_ADDRESS} CACHE STRING "SUIT storage address")
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
function(generate_mpi_hex MANIFEST_ROLE)
  set(GENERATE_ARGS "")
  set(DESCR_ARGS "")

  if (NOT DEFINED CONFIG_SUIT_MPI_${MANIFEST_ROLE})
    return()
  endif()

  if ((NOT DEFINED CONFIG_SUIT_MPI_${MANIFEST_ROLE}_VENDOR_NAME) OR
      (NOT DEFINED CONFIG_SUIT_MPI_${MANIFEST_ROLE}_CLASS_NAME) OR
      (NOT DEFINED CONFIG_SUIT_MPI_${MANIFEST_ROLE}_OFFSET) OR
      (NOT DEFINED CONFIG_SUIT_MPI_${MANIFEST_ROLE}_PATH) OR
      (NOT DEFINED CONFIG_SUIT_MPI_SLOT_SIZE)
      )
    message(FATAL_ERROR "Malformed configuration for: ${MANIFEST_ROLE}")
    return()
  endif()

  MATH(
    EXPR
    SUIT_MPI_${MANIFEST_ROLE}_ADDRESS
    "${SUIT_STORAGE_ADDRESS}
      + ${CONFIG_SUIT_MPI_${MANIFEST_ROLE}_OFFSET}"
    OUTPUT_FORMAT HEXADECIMAL
  )

  list(APPEND GENERATE_ARGS
    --vendor-name ${CONFIG_SUIT_MPI_${MANIFEST_ROLE}_VENDOR_NAME}
    --class-name ${CONFIG_SUIT_MPI_${MANIFEST_ROLE}_CLASS_NAME}
    --address ${SUIT_MPI_${MANIFEST_ROLE}_ADDRESS}
    --size ${CONFIG_SUIT_MPI_SLOT_SIZE}
    --output-file ${CONFIG_SUIT_MPI_${MANIFEST_ROLE}_PATH}
    )

  list(APPEND DESCR_ARGS
    ${CONFIG_SUIT_MPI_${MANIFEST_ROLE}_PATH}
    ${CONFIG_SUIT_MPI_${MANIFEST_ROLE}_VENDOR_NAME}
    ${CONFIG_SUIT_MPI_${MANIFEST_ROLE}_CLASS_NAME}
    "address=${SUIT_MPI_${MANIFEST_ROLE}_ADDRESS}"
    )

  if(DEFINED CONFIG_SUIT_MPI_${MANIFEST_ROLE}_DOWNGRADE_PREVENTION)
    list(APPEND GENERATE_ARGS
      --downgrade-prevention-enabled
      )
    list(APPEND DESCR_ARGS
      downgrade-prevention-enabled
      )
  endif()

  if(DEFINED CONFIG_SUIT_MPI_${MANIFEST_ROLE}_INDEPENDENT_UPDATE)
    list(APPEND GENERATE_ARGS
      --independent-updates
      )
    list(APPEND DESCR_ARGS
      "independent updates"
      )
  endif()

  if(DEFINED CONFIG_SUIT_MPI_${MANIFEST_ROLE}_SIGNATURE_CHECK_ENABLED_ON_UPDATE)
    list(APPEND GENERATE_ARGS
      --signature-verification update
      )
     list(APPEND DESCR_ARGS
       "signed updates"
       )
  elseif(DEFINED CONFIG_SUIT_MPI_${MANIFEST_ROLE}_SIGNATURE_CHECK_ENABLED_ON_UPDATE_AND_BOOT)
    list(APPEND GENERATE_ARGS
      --signature-verification update-and-boot
      )
     list(APPEND DESCR_ARGS
       "signed updates"
       "signed boot"
       )
  endif()

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${PYTHON_EXECUTABLE} ${SUIT_GENERATOR_CLI_SCRIPT}
    mpi
    generate
    ${GENERATE_ARGS}
  )

  message(INFO " Generate MPI for ${MANIFEST_ROLE} manifest (${DESCR_ARGS})")
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
  set(MERGE_ARGS "")

  if ((NOT DEFINED CONFIG_SUIT_MPI_${area}_PATH) OR
      (NOT DEFINED CONFIG_SUIT_MPI_${area}_OFFSET) OR
      (NOT DEFINED CONFIG_SUIT_MPI_${area}_SIZE)
      )
    message(FATAL_ERROR "Malformed configuration for: ${area}")
    return()
  endif()

  # Calculate the absolute address of the MPI area.
  MATH(
    EXPR
    SUIT_MPI_${area}_ADDRESS
    "${SUIT_STORAGE_ADDRESS}
      + ${CONFIG_SUIT_MPI_${area}_OFFSET}"
    OUTPUT_FORMAT HEXADECIMAL
  )

  set(output ${CONFIG_SUIT_MPI_${area}_PATH})
  set(address ${SUIT_MPI_${area}_ADDRESS})
  set(size ${CONFIG_SUIT_MPI_${area}_SIZE})

  list(APPEND MERGE_ARGS
    --output-file ${output}
    --address ${address}
    --size ${size}
    )

  foreach(role ${ARGN})
    if(NOT DEFINED CONFIG_SUIT_MPI_${role}_PATH)
      continue()
    endif()

    list(APPEND MERGE_ARGS "--file" "${CONFIG_SUIT_MPI_${role}_PATH}")
  endforeach()

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${PYTHON_EXECUTABLE} ${SUIT_GENERATOR_CLI_SCRIPT}
    mpi
    merge
    ${MERGE_ARGS}
  )

  message(INFO " Generate merged MPI for ${area} (${output})")
endfunction()

if(DEFINED CONFIG_SOC_SERIES_NRF54HX)
configure_storage_address_cache()
endif() # CONFIG_SOC_SERIES_NRF54HX

if(DEFINED CONFIG_SUIT_MPI_GENERATE)

if(DEFINED CONFIG_SUIT_MPI_SOC_NRF54H20)
include(soc/nrf54h20.cmake)
endif() # CONFIG_SUIT_MPI_SOC_NRF54H20

endif() # CONFIG_SUIT_MPI_GENERATE
