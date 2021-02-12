# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

foreach(file ${TOOLS_VERSION_FILES})
  file(STRINGS ${file} VERSIONS_ALL REGEX "^[a-zA-Z0-9_-]*=.*")
  get_filename_component(file_name ${file} NAME_WE)
  string(REPLACE "tools-versions-" "" os ${file_name})
  string(TOUPPER ${os} OS_UPPER)
  foreach(program ${VERSIONS_ALL})
    string(REGEX REPLACE "(^[^=]*)=([^\;]*)\;*.*" "\\1" program_name "${program}")
    string(TOUPPER ${program_name} PROGRAM_NAME_UPPER)
    set(${PROGRAM_NAME_UPPER}_VERSION_${OS_UPPER} ${CMAKE_MATCH_2})
  endforeach()
endforeach()

foreach(pip_file ${PIP_REQUIREMENTS_FILES})
  file(STRINGS ${pip_file} PACKAGE_VERSIONS REGEX "^[^#].*")
  foreach(package ${PACKAGE_VERSIONS})
    string(REGEX REPLACE "(^[^>=\;]*)([\;]|([>=][^\;]*))\;*.*" "\\1" package_name "${package}")
    string(TOUPPER ${package_name} PACKAGE_NAME_UPPER)
    set(${PACKAGE_NAME_UPPER}_VERSION ${CMAKE_MATCH_3})
    if("${${PACKAGE_NAME_UPPER}_VERSION}" STREQUAL "")
      set(${PACKAGE_NAME_UPPER}_VERSION "\\ ")
    endif()
  endforeach()
endforeach()

configure_file(${VERSION_IN} ${VERSION_OUT} @ONLY)
