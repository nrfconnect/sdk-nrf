#
# Copyright (c) 2021-2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

#
# This macro creates a variable for a base input if CONFIG_<base> is set and
# sets it to True
#
macro(kconfig_check_and_set_base_val base val)
  if (CONFIG_${base})
    nrf_security_debug("Setting ${base} to ${val}")
    set(${base} ${val})
  endif()
endmacro()

#
# This macro creates a variable for a base input if CONFIG_<base> is set and
# sets it to True
#
macro(kconfig_check_and_set_base base)
  kconfig_check_and_set_base_val(${base} True)
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
macro(kconfig_check_and_set_base_int base)
  kconfig_check_and_set_base_val(${base} "${CONFIG_${base}}")
endmacro()

#
# Internal macro which will create a variable base if it doesn't exist and set it to val
# if all Kconfig variables in the ARGN list are true (stripped for CONFIG_ is given)
#
macro(kconfig_check_and_set_base_to_val_depends base val)
  if(NOT ${base})
  set(_argn_all_true TRUE)
  foreach(arg ${ARGN})
    if (NOT CONFIG_${arg})
      set(_argn_all_true FALSE)
      set(_argn_false_arg ${arg})
      break()
    endif()
  endforeach()

  if(_argn_all_true)
    nrf_security_debug("Setting ${base} to True because all depends are set")
    set(${base} ${val})
    set(CONFIG_${base} ${val})
  else()
    nrf_security_debug("Not setting ${base} because ${_argn_false_arg} is not set")
  endif()

  unset(_argn_all_true)
  unset(_argn_false_arg)
  endif()
endmacro()

#
# Internal macro which will create a variable base if it doesn't exist and set it to true
# if all Kconfig variables in the ARGN list are true (stripped for CONFIG_ is given)
#
macro(kconfig_check_and_set_base_depends base)
  kconfig_check_and_set_base_to_val_depends(${base} True ${ARGN})
endmacro()

#
# Internal macro which will create a variable base if it doesn't exist and set it to 1
# if all Kconfig variables in the ARGN list are true (stripped for CONFIG_ is given)
#
macro(kconfig_check_and_set_base_to_one_depends base)
  kconfig_check_and_set_base_to_val_depends(${base} 1 ${ARGN})
endmacro()


#
# Internal macro that makes a backup of a given configuration (suffixed by _COPY)
# This stores the current state of a Kconfig (CONFIG_ is expected)
#
macro(kconfig_backup_current_config config)
  if(${config})
    message("Backup: ${config}: True")
    set(${config}_COPY True})
  else()
    message("Backup: ${config}: False")
    set(${config}_COPY False)
  endif()
endmacro()

#
# Internal macro that restore a backed up copy of a given configuration (suffixed by _COPY)
# This restores the backed up state of a Kconfig (CONFIG_ is expected)
#
macro(kconfig_restore_backup_config config)
  if(${config}_COPY)
    message("Restore: ${config}: True")
    set(${config} True)
  else()
    message("Restore: ${config}: False")
    set(${config} False)
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

    # Ensure that the PSA crypto interface include folder isn't added in library builds (filtered out)
    target_include_directories(${lib_name} 
      PRIVATE
        $<FILTER:$<TARGET_PROPERTY:zephyr_interface,INTERFACE_INCLUDE_DIRECTORIES>,EXCLUDE,${PSA_CRYPTO_CONFIG_INTERFACE_PATH_REGEX}>
    )

    # Ensure that the PSA crypto interface include folder isn't added in library builds (filtered out)
    target_include_directories(${lib_name} 
      PRIVATE
        $<FILTER:$<TARGET_PROPERTY:zephyr_interface,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>,EXCLUDE,${PSA_CRYPTO_CONFIG_INTERFACE_PATH_REGEX}>
    )
    
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
