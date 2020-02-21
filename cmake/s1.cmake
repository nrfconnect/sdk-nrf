#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

if (CONFIG_MCUBOOT AND CONFIG_MCUBOOT_BUILD_S1_VARIANT)
# Create second MCUBoot executable for the second slot of the second stage
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
    set(obj_copy_cmd "")
    bintools_objcopy(
      RESULT_CMD_LIST obj_copy_cmd
      TARGET_INPUT    ${OUTPUT_FORMAT}
      TARGET_OUTPUT   "binary"
      SECTION_ONLY    ".intList"
      FILE_INPUT      $<TARGET_FILE:${${link_variant}prebuilt}>
      FILE_OUTPUT     "${link_variant}isrList.bin"
      )

    add_custom_command(
      OUTPUT ${link_variant}isr_tables.c
      ${obj_copy_cmd}
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

  configure_linker_script(
    ${link_variant}linker.cmd
    "-DLINK_INTO_${link_variant_name};-DLINKER_PASS2"
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
  set(output ${PROJECT_BINARY_DIR}/../../zephyr/${exe}.hex)

  # Rule to generate hex file of .elf
  bintools_objcopy(
    RESULT_CMD_LIST    ${link_variant}hex_cmd
    RESULT_BYPROD_LIST ${link_variant}hex_byprod
    STRIP_ALL
    GAP_FILL           "0xff"
    TARGET_OUTPUT      "ihex"
    SECTION_REMOVE     ${out_hex_sections_remove}
    FILE_INPUT         ${exe}.elf
    FILE_OUTPUT        ${output}
    )

  add_custom_command(${${link_variant}hex_cmd}
    DEPENDS ${exe}
    OUTPUT ${output})

  add_custom_target(
    ${link_variant}image_hex
    ALL
    DEPENDS
    ${output}
    )

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
