#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

define_property(GLOBAL PROPERTY PM_IMAGES
  BRIEF_DOCS "A list of all images (${IMAGE} instances) that should be processed by the Partition Manager."
  FULL_DOCS "A list of all images (${IMAGE} instances) that should be processed by the Partition Manager.
Each image's directory will be searched for a pm.yml, and will receive a pm_config.h header file with the result.
Also, the each image's hex file will be automatically associated with its partition.")

if(FIRST_BOILERPLATE_EXECUTION)
  get_property(PM_IMAGES GLOBAL PROPERTY PM_IMAGES)

  set(static_configuration_file ${APPLICATION_SOURCE_DIR}/pm_static.yml)

  if(PM_IMAGES OR (EXISTS ${static_configuration_file}))
    # Partition manager is enabled because we have populated PM_IMAGES,
    # or because the application has specified a static configuration.
    if (EXISTS ${static_configuration_file})
      set(static_configuration --static-config ${static_configuration_file})
    endif()

    set(generated_path include/generated)

    # Add the "app" as an image partition.
    set_property(GLOBAL PROPERTY app_PROJECT_BINARY_DIR ${PROJECT_BINARY_DIR})
    set_property(GLOBAL PROPERTY app_PM_HEX_FILE ${PROJECT_BINARY_DIR}/${KERNEL_HEX_NAME})
    set_property(GLOBAL PROPERTY app_PM_TARGET ${logical_target_for_zephyr_elf})
    list(APPEND PM_IMAGES app_)

    # Prepare the input_files, header_files, and images lists
    foreach (IMAGE ${PM_IMAGES})
      get_image_name(${IMAGE} image_name) # Removes the trailing '_'
      list(APPEND images ${image_name})
      get_property(${IMAGE}PROJECT_BINARY_DIR GLOBAL PROPERTY ${IMAGE}PROJECT_BINARY_DIR)
      list(APPEND input_files ${${IMAGE}PROJECT_BINARY_DIR}/${generated_path}/pm.yml)
      list(APPEND header_files ${${IMAGE}PROJECT_BINARY_DIR}/${generated_path}/pm_config.h)
    endforeach()

    math(EXPR flash_size "${CONFIG_FLASH_SIZE} * 1024")

    set(pm_cmd
      ${PYTHON_EXECUTABLE}
      ${NRF_DIR}/scripts/partition_manager.py
      --input-names ${images}
      --input-files ${input_files}
      --flash-size ${flash_size}
      --output ${CMAKE_BINARY_DIR}/partitions.yml
      ${static_configuration}
      )

    set(pm_output_cmd
      ${PYTHON_EXECUTABLE}
      ${NRF_DIR}/scripts/partition_manager_output.py
      --input ${CMAKE_BINARY_DIR}/partitions.yml
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

      # Process phony partitions (if it has a SPAN list it is a phony partition).
      if(DEFINED PM_${PART}_SPAN)
        string(REPLACE " " ";" PM_${PART}_SPAN ${PM_${PART}_SPAN})
        list(APPEND phonies ${part})
      endif()

      # Include the partition in the merge operation if it has a hex file.
      if(DEFINED ${part}_PM_HEX_FILE)
        get_property(${part}_PM_TARGET GLOBAL PROPERTY ${part}_PM_TARGET)
        list(APPEND explicitly_assigned ${part})
      else()
        if(${part} IN_LIST images)
          get_property(${part}_KERNEL_NAME GLOBAL PROPERTY ${part}_KERNEL_NAME)
          set(${part}_PM_HEX_FILE ${${part}_PROJECT_BINARY_DIR}/${${part}_KERNEL_NAME}.hex)
          set(${part}_PM_TARGET ${part}_zephyr_final)
        elseif(${part} IN_LIST phonies)
          set(${part}_PM_HEX_FILE ${PROJECT_BINARY_DIR}/${part}.hex)
          set(${part}_PM_TARGET ${part}_hex)
        endif()
        list(APPEND implicitly_assigned ${part})
      endif()
    endforeach()

    set(PM_MERGED_SPAN ${implicitly_assigned} ${explicitly_assigned})
    set(merged_overlap TRUE) # Enable overlapping for the merged hex file.

    # Iterate over all phony partitions, plus the "fake" merged paritition.
    # The loop will create a hex file for each iteration.
    foreach(phony ${phonies} merged)
      string(TOUPPER ${phony} PHONY)

      # Prepare the list of hex files and list of dependencies for the merge command.
      foreach(part ${PM_${PHONY}_SPAN})
        string(TOUPPER ${part} PART)
        list(APPEND ${phony}hex_files ${${part}_PM_HEX_FILE})
        list(APPEND ${phony}targets ${${part}_PM_TARGET})
      endforeach()

      # If overlapping is enabled, add the appropriate argument.
      if(${${phony}_overlap})
        set(${phony}overlap_arg --overlap=replace)
      endif()

      # Add command to merge files.
      add_custom_command(
        OUTPUT ${PROJECT_BINARY_DIR}/${phony}.hex
        COMMAND
        ${PYTHON_EXECUTABLE}
        ${ZEPHYR_BASE}/scripts/mergehex.py
        -o ${PROJECT_BINARY_DIR}/${phony}.hex
        ${${phony}overlap_arg}
        ${${phony}hex_files}
        DEPENDS
        ${${phony}targets}
        )

      # Wrapper target for the merge command.
      add_custom_target(${phony}_hex ALL DEPENDS ${PROJECT_BINARY_DIR}/${phony}.hex)
    endforeach()

    # Add merged.hex as the representative hex file for flashing this app.
    if(TARGET flash)
      add_dependencies(flash merged_hex)
    endif()
    set(ZEPHYR_RUNNER_CONFIG_KERNEL_HEX "${PROJECT_BINARY_DIR}/merged.hex"
      CACHE STRING "Path to merged image in Intel Hex format" FORCE)
  endif()
endif()
