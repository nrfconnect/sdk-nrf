#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
# The purpose of this file is to provice a set of debug commands to nrf security
# module.


#
# Internal macro that shows debug output in for nrf_security related
# configurations when Cmake is run with -DDEBUG_NRF_SECURITY=1
#
macro(nrf_security_debug)
  if(DEBUG_NRF_SECURITY)
    message("${ARGN}")
  endif()
endmacro()

#
# Internal macro that shows source files in a mbed TLS library (lib_name)
# when CMake is run with -DDEBUG_NRF_SECURITY=1
# Note that inspecting the zephyr.map file is safer
#
macro(nrf_security_debug_list_target_files lib_name)
  if(DEBUG_NRF_SECURITY)
    message("--- Sources in ${lib_name} ---")
    get_target_property(file_list ${lib_name} SOURCES)
    foreach(line IN LISTS file_list)
      message("${line}")
    endforeach()
    message("--- End sources in ${lib_name} ---")
  endif()
endmacro()

macro(nrf_security_debug_if_defined variable)
  if(DEBUG_NRF_SECURITY AND DEFINED variable)
    message("${variable}: ${ARGN}")
  endif()
endmacro()
