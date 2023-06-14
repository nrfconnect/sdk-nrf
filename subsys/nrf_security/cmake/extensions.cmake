#
# Copyright (c) 2021-2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

#
# This macro creates a variable for a base input if CONFIG_<base> is set and
# sets it to True
#
macro(kconfig_check_and_set_base base)
  if (CONFIG_${base})
    nrf_security_debug("Setting ${base} to True")
    set(${base} TRUE)
  endif()
endmacro()


#
# This macro creates a variable with a value for a base input if CONFIG_<base>
# is set
#
macro(kconfig_check_and_set_base_val base val)
  if (CONFIG_${base})
    nrf_security_debug("Setting ${base} ${val}")
    set(${base} ${val})
  endif()
endmacro()

#
# This macro sets the value of base input to 1 if CONFIG_<base> is set
#
macro(kconfig_check_and_set_base_to_one base)
  kconfig_check_and_set_base_val(${base} 1)
endmacro()

#
# This macro expects the CONFIG_<base> to be a int value and creates a variable
# named base with the value of CONFIG_<base>
#
macro(kconfig_check_And_set_base_int base)
  kconfig_check_and_set_base_val(${base} "${CONFIG_${base}}")
endmacro()

#
# Internal macro which will create a variable base if it doesn't exist
# if all Kconfig variables in the ARGN list are true (stripped for CONFIG_ is given)
#
macro(kconfig_check_and_set_base_depends base)
  if(NOT ${base})
    set(${base} TRUE)
    foreach(arg ${ARGN})
      if (NOT CONFIG_${arg})
        nrf_security_debug("Unsetting ${base} because ${arg} is not set")
        unset(${base})
        break()
      endif()
    endforeach()
  endif()
endmacro()

#
# Internal macro which will create a variable base and set to 1 if dependent
# Kconfig variables in the ARGN list (stripped for CONFIG_ is given)
#
macro(kconfig_check_and_set_base_to_one_depends base)
  set(${base} 1)
  foreach(arg ${ARGN})
    if (NOT CONFIG_${arg})
      nrf_security_debug("Unsetting ${base} because ${arg} is not set")
      unset(${base})
      break()
    endif()
  endforeach()
endmacro()

#
# Internal macro to configure file if Kconfig config is set
#
# This needs some work
#
macro(nrf_security_configure_file config location file)
  if (${mbedtls_config})
    nrf_security_debug("Configure file: ${file}")
    get_filename_component(file_name ${file} NAME)
    configure_file(${file} ${location}/${file_name} COPYONLY)
  endif()
endmacro()

#
# Internal macro to append all unnamed parameters with 'prefix' if condition
# is met
#
macro(append_with_prefix_ifdef cond var prefix)
  if (${${cond}})
    append_with_prefix(${var} ${prefix} ${ARGN})
  endif()
endmacro()

function(append_with_prefix var prefix)
  set(listVar ${${var}})
  foreach(f ${ARGN})
    list(APPEND listVar "${prefix}/${f}")
  endforeach(f)
  set(${var} ${listVar} PARENT_SCOPE)
endfunction(append_with_prefix)

#
# Add common configurations/options from the zephyr interface libraries
#
# This includes
# Compile options
# Standard includes
# C flags/Linker flags
#
macro(nrf_security_add_zephyr_options lib_name)
  if(TARGET zephyr_interface)
    # Add compile options and includes from zephyr
    target_compile_options(${lib_name} PRIVATE $<TARGET_PROPERTY:zephyr_interface,INTERFACE_COMPILE_OPTIONS>)
    target_compile_definitions(${lib_name} PRIVATE $<TARGET_PROPERTY:zephyr_interface,INTERFACE_COMPILE_DEFINITIONS>)
    target_include_directories(${lib_name} PRIVATE $<TARGET_PROPERTY:zephyr_interface,INTERFACE_INCLUDE_DIRECTORIES>)
    target_include_directories(${lib_name} PRIVATE $<TARGET_PROPERTY:zephyr_interface,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>)
    add_dependencies(${lib_name} zephyr_interface)

    # Unsure if these are needed any more
    target_compile_options(${lib_name} PRIVATE ${TOOLCHAIN_C_FLAGS})
    target_ld_options(${lib_name} PRIVATE ${TOOLCHAIN_LD_FLAGS})
  else()
    target_compile_options(${lib_name} PRIVATE "SHELL: -imacros ${ZEPHYR_AUTOCONF}")
    target_include_directories(${lib_name} PRIVATE
      $<$<TARGET_EXISTS:nrf_cc3xx_platform>:$<TARGET_PROPERTY:nrf_cc3xx_platform,INTERFACE_INCLUDE_DIRECTORIES>>
    )
  endif()
endmacro()

# Include debugging
include(${CMAKE_CURRENT_LIST_DIR}/nrf_security_debug.cmake)
