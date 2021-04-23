#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if (CONFIG_BUILD_S1_VARIANT AND
    ((${CONFIG_S1_VARIANT_IMAGE_NAME} STREQUAL "app" AND NOT IMAGE_NAME)
    OR
    ("${CONFIG_S1_VARIANT_IMAGE_NAME}_" STREQUAL "${IMAGE_NAME}")))
# Create second executable for the second slot of the second stage
# bootloader. This is done inside this file since it is non-trivial to add
# executable targets outside the root CMakeLists.txt. The problem is that
# most of the variables required for compiling/linking is only available
# in the scope of this file.

  set(link_variant_name s1)
  set(link_variant ${link_variant_name}_)

  configure_linker_script(
    ${link_variant}linker_prebuilt.cmd
    "-DLINK_INTO_${link_variant_name}"
    ${PRIV_STACK_DEP}
    ${CODE_RELOCATION_DEP}
    ${OFFSETS_H_TARGET}
    )

  add_custom_target(
    ${link_variant}linker_prebuilt_target
    DEPENDS
    ${link_variant}linker_prebuilt.cmd
    )

  # Create prebuilt executable, which is used as input for ISR generation.
  set(${link_variant}prebuilt ${link_variant}image_prebuilt)
  add_executable(${${link_variant}prebuilt} ${ZEPHYR_BASE}/misc/empty_file.c)
  toolchain_ld_link_elf(
    TARGET_ELF            ${${link_variant}prebuilt}
    OUTPUT_MAP            ${PROJECT_BINARY_DIR}/${${link_variant}prebuilt}.map
    LIBRARIES_PRE_SCRIPT  ""
    LINKER_SCRIPT         ${PROJECT_BINARY_DIR}/${link_variant}linker_prebuilt.cmd
    LIBRARIES_POST_SCRIPT ""
    DEPENDENCIES          ""
    )
  set_property(TARGET ${${link_variant}prebuilt} PROPERTY LINK_DEPENDS ${PROJECT_BINARY_DIR}/${link_variant}linker_prebuilt.cmd)
  add_dependencies(   ${${link_variant}prebuilt} ${link_variant}linker_prebuilt_target ${PRIV_STACK_DEP} ${LINKER_SCRIPT_TARGET} ${OFFSETS_LIB})

  if(CONFIG_GEN_ISR_TABLES)
    # ${link_variant}isr_tables.c is generated from ${ZEPHYR_PREBUILT_EXECUTABLE} by
    # gen_isr_tables.py
    add_custom_command(
      OUTPUT ${link_variant}isr_tables.c
      COMMAND $<TARGET_PROPERTY:bintools,elfconvert_command>
              $<TARGET_PROPERTY:bintools,elfconvert_flag>
              $<TARGET_PROPERTY:bintools,elfconvert_flag_intarget>${OUTPUT_FORMAT}
              $<TARGET_PROPERTY:bintools,elfconvert_flag_outtarget>binary
              $<TARGET_PROPERTY:bintools,elfconvert_flag_section_only>.intList
              $<TARGET_PROPERTY:bintools,elfconvert_flag_infile>$<TARGET_FILE:${${link_variant}prebuilt}>
              $<TARGET_PROPERTY:bintools,elfconvert_flag_outfile>${link_variant}isrList.bin
              $<TARGET_PROPERTY:bintools,elfconvert_flag_final>
      COMMAND ${PYTHON_EXECUTABLE}
      ${ZEPHYR_BASE}/arch/common/gen_isr_tables.py
      --output-source ${link_variant}isr_tables.c
      --kernel $<TARGET_FILE:${${link_variant}prebuilt}>
      --intlist ${link_variant}isrList.bin
      $<$<BOOL:${CONFIG_BIG_ENDIAN}>:--big-endian>
      $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--debug>
      ${GEN_ISR_TABLE_EXTRA_ARG}
      DEPENDS ${${link_variant}prebuilt}
      )

    set(${link_variant}generated_kernel_files ${link_variant}isr_tables.c)
  endif()

  add_custom_command(
    OUTPUT ${link_variant}dev_handles.c
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/scripts/gen_handles.py
    --output-source ${link_variant}dev_handles.c
    --kernel $<TARGET_FILE:${${link_variant}prebuilt}>
    --zephyr-base ${ZEPHYR_BASE}
    DEPENDS $<TARGET_FILE:${${link_variant}prebuilt}>
    )
  set(${link_variant}generated_kernel_files ${link_variant}dev_handles.c)

  configure_linker_script(
    ${link_variant}linker.cmd
    "-DLINK_INTO_${link_variant_name};-DLINKER_ZEPHYR_FINAL;-DLINKER_PASS2"
    ${PRIV_STACK_DEP}
    ${CODE_RELOCATION_DEP}
    ${${link_variant}prebuilt}
    ${OFFSETS_H_TARGET}
    )

  add_custom_target(
    ${link_variant}linker_target
    DEPENDS
    ${link_variant}linker.cmd
    )

  # Link against linker script with updated offset.
  set(exe ${link_variant}image)
  add_executable(${exe} ${ZEPHYR_BASE}/misc/empty_file.c ${${link_variant}generated_kernel_files})
  toolchain_ld_link_elf(
    TARGET_ELF            ${exe}
    OUTPUT_MAP            ${PROJECT_BINARY_DIR}/${exe}.map
    LIBRARIES_PRE_SCRIPT  ""
    LINKER_SCRIPT         ${PROJECT_BINARY_DIR}/${link_variant}linker.cmd
    LIBRARIES_POST_SCRIPT ""
    DEPENDENCIES          ""
    )
  add_dependencies(${exe} ${link_variant}linker_target)

  set(${link_variant}hex_cmd "")
  set(${link_variant}hex_byprod "")

  if (IMAGE_NAME)
    set(output ${PROJECT_BINARY_DIR}/../../zephyr/${exe}.hex)
  else()
    set(output ${PROJECT_BINARY_DIR}/${exe}.hex)
  endif()

  # Rule to generate hex file of .elf
  add_custom_command(${${link_variant}hex_cmd}
    COMMAND $<TARGET_PROPERTY:bintools,elfconvert_command>
            $<TARGET_PROPERTY:bintools,elfconvert_flag>
            $<TARGET_PROPERTY:bintools,elfconvert_flag_gapfill>0xff
            $<TARGET_PROPERTY:bintools,elfconvert_flag_outtarget>ihex
            $<TARGET_PROPERTY:bintools,elfconvert_flag_infile>${exe}.elf
            $<TARGET_PROPERTY:bintools,elfconvert_flag_outfile>${output}
            $<TARGET_PROPERTY:bintools,elfconvert_flag_final>
    DEPENDS ${exe}
    OUTPUT ${output}
    COMMAND_EXPAND_LISTS
  )

  add_custom_target(
    ${link_variant}image_hex
    ALL
    DEPENDS
    ${output}
    )

  if (IMAGE_NAME)
  # Register in the parent image that this child image will have
  # ${link_variant}image.hex as a byproduct, this allows the parent image to know
  # where the hex file comes from and create custom commands that
  # depend on it.
  set_property(
    TARGET         zephyr_property_target
    APPEND_STRING
    PROPERTY       shared_vars
    "list(APPEND ${IMAGE_NAME}BUILD_BYPRODUCTS ${output})\n"
    )
  endif()
endif()
