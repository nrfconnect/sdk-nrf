#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

include_guard(GLOBAL)

if(CONFIG_NCS_IS_VARIANT_IMAGE)
  set(AUTOCONF_H ${PROJECT_BINARY_DIR}/include/generated/zephyr/autoconf.h)
  set(DOTCONFIG  ${PROJECT_BINARY_DIR}/.config)

  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${AUTOCONF_H})
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${DOTCONFIG})

  if(SB_CONFIG_PARTITION_MANAGER)
    # A variant build should reuse same .config and thus autoconf.h, therefore
    # copy files from original and bypass Kconfig invocation.
    set(preload_autoconf_h ${PRELOAD_BINARY_DIR}/zephyr/include/generated/zephyr/autoconf.h)
    set(preload_dotconfig  ${PRELOAD_BINARY_DIR}/zephyr/.config)

    file(COPY ${preload_dotconfig}  DESTINATION ${PROJECT_BINARY_DIR})
    file(COPY ${preload_autoconf_h} DESTINATION ${PROJECT_BINARY_DIR}/include/generated/zephyr)

    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${preload_autoconf_h})
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${preload_dotconfig})

    import_kconfig("CONFIG" ${DOTCONFIG})
  else()
    include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/bootloader_dts_utils.cmake)

    dt_chosen(code_partition PROPERTY "zephyr,code-partition")
    dt_partition_addr(code_partition_offset PATH "${code_partition}" REQUIRED)
    dt_reg_size(code_partition_size PATH "${code_partition}" REQUIRED)

    set(preload_autoconf_h ${PRELOAD_BINARY_DIR}/zephyr/include/generated/zephyr/autoconf.h)
    set(preload_dotconfig  ${PRELOAD_BINARY_DIR}/zephyr/.config)

    file(STRINGS ${preload_autoconf_h} autoconf_content)
    file(STRINGS ${preload_dotconfig} dotconfig_content)


    set(soc_nrf54h20_cpurad 0)
    set(soc_series_nrf54l 0)
    if("${dotconfig_content}" MATCHES "CONFIG_SOC_NRF54H20_CPURAD=y")
      # Needed for the correct CONFIG_BUILD_OUTPUT_ADJUST_LMA calculation in the variant image
      # .config and autoconf.h files for nRF54H20 CPURAD.
      dt_partition_addr(code_partition_abs_addr PATH "${code_partition}" REQUIRED ABSOLUTE)
      dt_chosen(sram_property PROPERTY "zephyr,sram")
      dt_reg_addr(sram_addr PATH "${sram_property}" REQUIRED)

      set(soc_nrf54h20_cpurad 1)
    elseif("${dotconfig_content}" MATCHES "CONFIG_SOC_SERIES_NRF54L=y")
      set(soc_series_nrf54l 1)
    endif()

    # Modify the CONFIG_FLASH_LOAD_OFFSET and CONFIG_FLASH_LOAD_SIZE for both the .config and autoconf.h files.
    # If partition manager is not used, these values should be taken from the device tree.
    # Additionally, convert primary slot dependencies to secondary slot dependencies.
    set(dotconfig_variant_content)

    if(soc_series_nrf54l EQUAL 0)
      foreach(line IN LISTS dotconfig_content)
	if("${line}" MATCHES "^CONFIG_FLASH_LOAD_OFFSET=.*$")
	  string(REGEX REPLACE "CONFIG_FLASH_LOAD_OFFSET=(.*)" "CONFIG_FLASH_LOAD_OFFSET=${code_partition_offset}" line ${line})
	endif()

	if("${line}" MATCHES "^CONFIG_FLASH_LOAD_SIZE=.*$")
	  string(REGEX REPLACE "CONFIG_FLASH_LOAD_SIZE=(.*)" "CONFIG_FLASH_LOAD_SIZE=${code_partition_size}" line ${line})
	endif()

	if("${line}" MATCHES "(--dependencies|-d).*\([0-9, ]+primary[0-9., ]+\)")
	  string(REGEX REPLACE "primary" "secondary" line ${line})
	endif()

	if(soc_nrf54h20_cpurad AND "${line}" MATCHES "^CONFIG_BUILD_OUTPUT_ADJUST_LMA=.*$")
	  string(REGEX REPLACE "CONFIG_BUILD_OUTPUT_ADJUST_LMA=(.*)" "CONFIG_BUILD_OUTPUT_ADJUST_LMA=\"${code_partition_abs_addr}-${sram_addr}\"" line ${line})
	endif()

	list(APPEND dotconfig_variant_content "${line}\n")
      endforeach()

      set(autoconf_variant_content)
      foreach(line IN LISTS autoconf_content)
	if("${line}" MATCHES "^#define CONFIG_FLASH_LOAD_OFFSET .*$")
	  string(REGEX REPLACE "#define CONFIG_FLASH_LOAD_OFFSET (.*)" "#define CONFIG_FLASH_LOAD_OFFSET ${code_partition_offset}" line ${line})
	endif()

	if("${line}" MATCHES "^#define CONFIG_FLASH_LOAD_SIZE .*$")
	  string(REGEX REPLACE "#define CONFIG_FLASH_LOAD_SIZE (.*)" "#define CONFIG_FLASH_LOAD_SIZE ${code_partition_size}" line ${line})
	endif()

	if("${line}" MATCHES "(--dependencies|-d).*\([0-9, ]+primary[0-9., ]+\)")
	  string(REGEX REPLACE "primary" "secondary" line ${line})
	endif()

	if(soc_nrf54h20_cpurad AND "${line}" MATCHES "^#define CONFIG_BUILD_OUTPUT_ADJUST_LMA .*$")
	  string(REGEX REPLACE "#define CONFIG_BUILD_OUTPUT_ADJUST_LMA (.*)" "#define CONFIG_BUILD_OUTPUT_ADJUST_LMA \"${code_partition_abs_addr}-${sram_addr}\"" line ${line})
	endif()

	list(APPEND autoconf_variant_content "${line}\n")
      endforeach()
    elseif(soc_series_nrf54l EQUAL 1)
      # This is very similar to what happens with the nrf54h20 above, except it is done with Secure
      # Boot awareness. In Secure Boot configuration we expect MCUboot, or S0 image, to run from
      # partition labeled s0_slot, and s1_image to run from partition labeled s1_slot.
      # Due how to entire "variant" configuration works, here, even though S1 image of MCUboot is
      # built, we will see configuration for MCUboot in S0, this also included code partition from
      # DTS. Here, we only know that we are supposed to process MCUboot configured for s0_slot
      # partition into s1_slot partition.

      # First we will check if zephyr,code-partition is the same as s0_slot, to confirm that
      # we even have good config to start with. We will also get the s1_slot partition parameters.
      dt_partition_addr(s0_slot_addr LABEL "s0_slot" REQUIRED)
      if(code_partition_offset EQUAL s0_slot_addr)
        dt_partition_addr(s1_slot_addr LABEL "s1_slot" REQUIRED)
        dt_partition_addr(s1_slot_size LABEL "s1_slot" REQUIRED)
	foreach(line IN LISTS dotconfig_content)
	  if("${line}" MATCHES "^CONFIG_FLASH_LOAD_OFFSET=.*$")
	    string(REGEX REPLACE "CONFIG_FLASH_LOAD_OFFSET=(.*)" "CONFIG_FLASH_LOAD_OFFSET=${s1_slot_addr}" line ${line})
	  endif()

	  if("${line}" MATCHES "^CONFIG_FLASH_LOAD_SIZE=.*$")
	    string(REGEX REPLACE "CONFIG_FLASH_LOAD_SIZE=(.*)" "CONFIG_FLASH_LOAD_SIZE=${s1_slot_size}" line ${line})
	  endif()

	  list(APPEND dotconfig_variant_content "${line}\n")
	endforeach()

	set(autoconf_variant_content)
	foreach(line IN LISTS autoconf_content)
	  if("${line}" MATCHES "^#define CONFIG_FLASH_LOAD_OFFSET .*$")
	    string(REGEX REPLACE "#define CONFIG_FLASH_LOAD_OFFSET (.*)" "#define CONFIG_FLASH_LOAD_OFFSET ${s1_slot_addr}" line ${line})
	  endif()

	  if("${line}" MATCHES "^#define CONFIG_FLASH_LOAD_SIZE .*$")
	    string(REGEX REPLACE "#define CONFIG_FLASH_LOAD_SIZE (.*)" "#define CONFIG_FLASH_LOAD_SIZE ${s1_slot_size}" line ${line})
	  endif()

	  list(APPEND autoconf_variant_content "${line}\n")
	endforeach()
      else()
        message(FATAL_ERROR "Expected zephyr,code-partition to match s0_slot partition for MCUboot build")
      endif()
    else()
      message(FATAL_ERROR "Unexpected path")
    endif()

    set(dest_autoconf_h ${PROJECT_BINARY_DIR}/include/generated/zephyr/autoconf.h)
    set(dest_dotconfig  ${PROJECT_BINARY_DIR}/.config)

    file(WRITE ${dest_autoconf_h} ${autoconf_variant_content})
    file(WRITE ${dest_dotconfig} ${dotconfig_variant_content})

    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${preload_autoconf_h})
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${preload_dotconfig})

    import_kconfig("CONFIG" ${DOTCONFIG})

  endif()

  file(APPEND ${AUTOCONF_H} "#define CONFIG_NCS_IS_VARIANT_IMAGE 1")
else()
  include(${ZEPHYR_BASE}/cmake/modules/kconfig.cmake)
endif()
