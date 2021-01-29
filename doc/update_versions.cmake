# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

if(NOT DEFINED NRF_BASE)
  message(FATAL_ERROR "No NRF_BASE argument supplied")
endif()

if(NOT DEFINED ZEPHYR_BASE)
  message(FATAL_ERROR "No ZEPHYR_BASE argument supplied")
endif()

if(NOT DEFINED MCUBOOT_BASE)
  message(FATAL_ERROR "No MCUBOOT_BASE argument supplied")
endif()

if(NOT DEFINED NRF_DOC_DIR)
  message(FATAL_ERROR "No NRF_DOC_DIR argument supplied")
endif()

if(NOT DEFINED NRF_RST_OUT)
  message(FATAL_ERROR "No NRF_RST_OUT argument supplied")
endif()

file(STRINGS ${NRF_BASE}/scripts/tools-versions-minimum.txt MINIMUM_VERSIONS_ALL REGEX "^[a-zA-Z0-9_-]*=.*")
file(STRINGS ${NRF_BASE}/scripts/tools-versions-darwin.txt DARWIN_VERSIONS_ALL REGEX "^[a-zA-Z0-9_-]*=.*")
file(STRINGS ${NRF_BASE}/scripts/tools-versions-win10.txt WIN10_VERSIONS_ALL REGEX "^[a-zA-Z0-9_-]*=.*")
file(STRINGS ${NRF_BASE}/scripts/tools-versions-linux.txt LINUX_VERSIONS_ALL REGEX "^[a-zA-Z0-9_-]*=.*")

foreach(os MINIMUM DARWIN WIN10 LINUX)
  foreach(program ${${os}_VERSIONS_ALL})
    string(REGEX REPLACE "(^[^=]*)=([^\;]*)\;*.*" "\\1" program_name "${program}")
    string(TOUPPER ${program_name} PROGRAM_NAME_UPPER)
    set(${PROGRAM_NAME_UPPER}_VERSION_${os} ${CMAKE_MATCH_2})
  endforeach()
endforeach()

# Loading Zephyr requirements-base.txt first, then mcuboot, and afterwards NCS
# specific ensures that any packages present in multiple files will present the
# version defined by NCS.
file(STRINGS ${ZEPHYR_BASE}/scripts/requirements-base.txt ZEPHYR_BASE_PACKAGE_VERSIONS REGEX "^[^#].*")
file(STRINGS ${ZEPHYR_BASE}/scripts/requirements-doc.txt ZEPHYR_DOCS_PACKAGE_VERSIONS REGEX "^[^#].*")
file(STRINGS ${MCUBOOT_BASE}/scripts/requirements.txt MCUBOOT_PACKAGE_VERSIONS REGEX "^[^#].*")
file(STRINGS ${NRF_BASE}/scripts/requirements-base.txt NRF_BASE_PACKAGE_VERSIONS REGEX "^[^#].*")
file(STRINGS ${NRF_BASE}/scripts/requirements-doc.txt NRF_DOC_PACKAGE_VERSIONS REGEX "^[^#].*")
file(STRINGS ${NRF_BASE}/scripts/requirements-build.txt NRF_BUILD_PACKAGE_VERSIONS REGEX "^[^#].*")

foreach(package ${ZEPHYR_BASE_PACKAGE_VERSIONS}
                ${ZEPHYR_DOCS_PACKAGE_VERSIONS}
                ${MCUBOOT_PACKAGE_VERSIONS}
                ${NRF_BASE_PACKAGE_VERSIONS}
                ${NRF_DOC_PACKAGE_VERSIONS}
                ${NRF_BUILD_PACKAGE_VERSIONS}
)
  string(REGEX REPLACE "(^[^>=\;]*)([\;]|([>=][^\;]*))\;*.*" "\\1" package_name "${package}")
  string(TOUPPER ${package_name} PACKAGE_NAME_UPPER)
  set(${PACKAGE_NAME_UPPER}_VERSION ${CMAKE_MATCH_3})
  if("${${PACKAGE_NAME_UPPER}_VERSION}" STREQUAL "")
    set(${PACKAGE_NAME_UPPER}_VERSION "\\ ")
  endif()
endforeach()

set(NRF_VERSION_IN ${NRF_DOC_DIR}/versions.txt.in)
set(NRF_RST_VERSION_OUT ${NRF_RST_OUT}/doc/nrf/versions.txt)
configure_file(${NRF_VERSION_IN} ${NRF_RST_VERSION_OUT} @ONLY)
