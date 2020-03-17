#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

define_property(GLOBAL PROPERTY PM_IMAGES
  BRIEF_DOCS "A list of all images that should be processed by the Partition Manager."
  FULL_DOCS "A list of all images that should be processed by the Partition Manager.
Each image's directory will be searched for a pm.yml, and will receive a pm_config.h header file with the result.
Also, the each image's hex file will be automatically associated with its partition.")

macro(add_region name size base placement_strategy)
  list(APPEND regions ${name})
  list(APPEND region_arguments "--${name}-size;${size}")
  list(APPEND region_arguments "--${name}-base-address;${base}")
  list(APPEND region_arguments "--${name}-placement-strategy;${placement_strategy}")
endmacro()

macro(add_region_with_dev name size base placement_strategy device)
  add_region(${name} ${size} ${base} ${placement_strategy})
  list(APPEND region_arguments "--${name}-device;${device}")
endmacro()

get_property(PM_IMAGES GLOBAL PROPERTY PM_IMAGES)
get_property(PM_SUBSYS_PREPROCESSED GLOBAL PROPERTY PM_SUBSYS_PREPROCESSED)

if(EXISTS "${PM_STATIC_YML_FILE}" AND NOT IS_DIRECTORY "${PM_STATIC_YML_FILE}")
  set(static_configuration_file ${PM_STATIC_YML_FILE})
else()
  set(static_configuration_file ${APPLICATION_SOURCE_DIR}/pm_static.yml)
endif()

if(PM_IMAGES OR (EXISTS ${static_configuration_file}))
  # Partition manager is enabled because we have populated PM_IMAGES,
  # or because the application has specified a static configuration.
  if (EXISTS ${static_configuration_file})
    set(static_configuration --static-config ${static_configuration_file})
  endif()

  set(generated_path zephyr/include/generated)

  # Add the "app" as an image partition.
  set_property(GLOBAL PROPERTY app_PM_HEX_FILE ${PROJECT_BINARY_DIR}/${KERNEL_HEX_NAME})
  set_property(GLOBAL PROPERTY app_PM_TARGET ${logical_target_for_zephyr_elf})

  # Prepare the input_files, header_files, and images lists
  foreach (image_name ${PM_IMAGES})
    list(APPEND images ${image_name})
    list(APPEND input_files ${CMAKE_BINARY_DIR}/${image_name}/${generated_path}/pm.yml)
    list(APPEND header_files ${CMAKE_BINARY_DIR}/${image_name}/${generated_path}/pm_config.h)
  endforeach()

  # Special treatmen of the app image.
  list(APPEND images "app")
  list(APPEND input_files ${CMAKE_BINARY_DIR}/${generated_path}/pm.yml)
  list(APPEND header_files ${CMAKE_BINARY_DIR}/${generated_path}/pm_config.h)

  # Add subsys defined pm.yml to the input_files
  list(APPEND input_files ${PM_SUBSYS_PREPROCESSED})


  if (DEFINED CONFIG_SOC_NRF9160)
    # See nRF9160 Product Specification, chapter "UICR"
    set(otp_start_addr "0xff8108")
    set(otp_size 756) # 189 * 4
  elseif (DEFINED CONFIG_SOC_NRF5340_CPUAPP)
    # See nRF5340 Product Specification, chapter Application Core -> ... "UICR"
    set(otp_start_addr "0xff8100")
    set(otp_size 764)  # 191 * 4
  endif()

  math(EXPR flash_size "${CONFIG_FLASH_SIZE} * 1024" OUTPUT_FORMAT HEXADECIMAL)

  if (CONFIG_SOC_NRF9160 OR CONFIG_SOC_NRF5340_CPUAPP)
    add_region(
      otp
      ${otp_size}
      ${otp_start_addr}
      start_to_end
      )
  endif()
  add_region_with_dev(
    flash_primary
    ${flash_size}
    ${CONFIG_FLASH_BASE_ADDRESS}
    complex
    NRF_FLASH_DRV_NAME
    )

  if (CONFIG_PM_EXTERNAL_FLASH)
    add_region_with_dev(
      external_flash
      ${CONFIG_PM_EXTERNAL_FLASH_SIZE}
      ${CONFIG_PM_EXTERNAL_FLASH_BASE}
      start_to_end
      ${CONFIG_PM_EXTERNAL_FLASH_DEV_NAME}
      )
  endif()

  set(pm_cmd
    ${PYTHON_EXECUTABLE}
    ${NRF_DIR}/scripts/partition_manager.py
    --input-files ${input_files}
    --regions ${regions}
    --output-partitions ${CMAKE_BINARY_DIR}/partitions.yml
    --output-regions ${CMAKE_BINARY_DIR}/regions.yml
    ${static_configuration}
    ${region_arguments}
    )

  set(pm_output_cmd
    ${PYTHON_EXECUTABLE}
    ${NRF_DIR}/scripts/partition_manager_output.py
    --input-partitions ${CMAKE_BINARY_DIR}/partitions.yml
    --input-regions ${CMAKE_BINARY_DIR}/regions.yml
    --config-file ${CMAKE_BINARY_DIR}/pm.config
    --input-names ${images}
    --header-files
    ${header_files}
    )

  # Run the partition manager algorithm.
  execute_process(
    COMMAND
    ${pm_cmd}
    RESULT_VARIABLE ret
    )

  if(NOT ${ret} EQUAL "0")
    message(FATAL_ERROR "Partition Manager failed, aborting. Command: ${pm_cmd}")
  endif()

  # Produce header files and config file.
  execute_process(
    COMMAND
    ${pm_output_cmd}
    RESULT_VARIABLE ret
    )

  if(NOT ${ret} EQUAL "0")
    message(FATAL_ERROR "Partition Manager output generation failed, aborting. Command: ${pm_output_cmd}")
  endif()

  # Create a dummy target that we can add properties to for
  # extraction in generator expressions.
  add_custom_target(partition_manager)

  # Make Partition Manager configuration available in CMake
  import_kconfig(PM_ ${CMAKE_BINARY_DIR}/pm.config pm_var_names)

  foreach(name ${pm_var_names})
    set_property(
      TARGET partition_manager
      PROPERTY ${name}
      ${${name}}
      )
  endforeach()

  # Turn the space-separated list into a Cmake list.
  string(REPLACE " " ";" PM_ALL_BY_SIZE ${PM_ALL_BY_SIZE})

  # Iterate over every partition, from smallest to largest.
  foreach(part ${PM_ALL_BY_SIZE})
    string(TOUPPER ${part} PART)
    get_property(${part}_PM_HEX_FILE GLOBAL PROPERTY ${part}_PM_HEX_FILE)
    get_property(${part}_PM_ELF_FILE GLOBAL PROPERTY ${part}_PM_ELF_FILE)

    # Process container partitions (if it has a SPAN list it is a container partition).
    if(DEFINED PM_${PART}_SPAN)
      string(REPLACE " " ";" PM_${PART}_SPAN ${PM_${PART}_SPAN})
      list(APPEND containers ${part})
    endif()

    # Include the partition in the merge operation if it has a hex file.
    if(DEFINED ${part}_PM_HEX_FILE)
      get_property(${part}_PM_TARGET GLOBAL PROPERTY ${part}_PM_TARGET)
      list(APPEND explicitly_assigned ${part})
    else()
      if(${part} IN_LIST images)
        set(${part}_PM_HEX_FILE ${CMAKE_BINARY_DIR}/${part}/zephyr/${${part}_KERNEL_HEX_NAME})
        set(${part}_PM_ELF_FILE ${CMAKE_BINARY_DIR}/${part}/zephyr/${${part}_KERNEL_ELF_NAME})
        set(${part}_PM_TARGET ${part}_subimage)
      elseif(${part} IN_LIST containers)
        set(${part}_PM_HEX_FILE ${PROJECT_BINARY_DIR}/${part}.hex)
        set(${part}_PM_TARGET ${part}_hex)
      endif()
      list(APPEND implicitly_assigned ${part})
    endif()
  endforeach()

  set(PM_MERGED_SPAN ${implicitly_assigned} ${explicitly_assigned})
  set(merged_overlap TRUE) # Enable overlapping for the merged hex file.

  # Iterate over all container partitions, plus the "fake" merged paritition.
  # The loop will create a hex file for each iteration.
  foreach(container ${containers} merged)
    string(TOUPPER ${container} CONTAINER)

    # Prepare the list of hex files and list of dependencies for the merge command.
    foreach(part ${PM_${CONTAINER}_SPAN})
      string(TOUPPER ${part} PART)
      list(APPEND ${container}hex_files ${${part}_PM_HEX_FILE})
      list(APPEND ${container}elf_files ${${part}_PM_ELF_FILE})
      list(APPEND ${container}targets ${${part}_PM_TARGET})
    endforeach()

    # If overlapping is enabled, add the appropriate argument.
    if(${${container}_overlap})
      set(${container}overlap_arg --overlap=replace)
    endif()

    # Add command to merge files.
    add_custom_command(
      OUTPUT ${PROJECT_BINARY_DIR}/${container}.hex
      COMMAND
      ${PYTHON_EXECUTABLE}
      ${ZEPHYR_BASE}/scripts/mergehex.py
      -o ${PROJECT_BINARY_DIR}/${container}.hex
      ${${container}overlap_arg}
      ${${container}hex_files}
      DEPENDS
      ${${container}targets}
      ${${container}hex_files}
      # SES will load symbols from all elf files listed as dependencies to ${PROJECT_BINARY_DIR}/merged.hex.
      # Therefore we add ${${container}elf_files} as dependency to ensure they are loaded by SES even though
      # it is unnecessary for building the application.
      ${${container}elf_files}
      )

    # Wrapper target for the merge command.
    add_custom_target(${container}_hex ALL DEPENDS ${PROJECT_BINARY_DIR}/${container}.hex)
  endforeach()

  # Add merged.hex as the representative hex file for flashing this app.
  if(TARGET flash)
    add_dependencies(flash merged_hex)
  endif()
  set(ZEPHYR_RUNNER_CONFIG_KERNEL_HEX "${PROJECT_BINARY_DIR}/merged.hex"
    CACHE STRING "Path to merged image in Intel Hex format" FORCE)
endif()

if (CONFIG_SECURE_BOOT AND CONFIG_BOOTLOADER_MCUBOOT)
  # Create symbols for the offsets required for moving test update hex files
  # to MCUBoots secondary slot. This is needed because objcopy does not
  # support arithmetic expressions as argument (e.g. '0x100+0x200'), and all
  # of the symbols used to generate the offset is only available as a
  # generator expression when MCUBoots cmake code exectues. This because
  # partition manager is performed as the last step in the configuration stage.
  math(EXPR s0_offset "${PM_MCUBOOT_SECONDARY_ADDRESS} - ${PM_S0_ADDRESS}")
  math(EXPR s1_offset "${PM_MCUBOOT_SECONDARY_ADDRESS} - ${PM_S1_ADDRESS}")

  set_property(
    TARGET partition_manager
    PROPERTY s0_TO_SECONDARY
    ${s0_offset}
    )
  set_property(
    TARGET partition_manager
    PROPERTY s1_TO_SECONDARY
    ${s1_offset}
    )
endif()
