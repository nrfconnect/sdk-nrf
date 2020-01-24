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

  # Create linker script which has an offset from S0 to S1.
  configure_linker_script(
    linker_mcuboot_s1.cmd
    "-DLINK_MCUBOOT_INTO_S1;-DLINKER_PASS2"
    ${PRIV_STACK_DEP}
    ${CODE_RELOCATION_DEP}
    ${ZEPHYR_PREBUILT_EXECUTABLE}
    ${OFFSETS_H_TARGET}
    )

  add_custom_target(
    linker_mcuboot_s1_target
    DEPENDS
    linker_mcuboot_s1.cmd
    )

  # Link against linker script with updated offset.
  set(MCUBOOT_S1_EXECUTABLE s1_image)
  add_executable(${MCUBOOT_S1_EXECUTABLE} ${ZEPHYR_BASE}/misc/empty_file.c)
  toolchain_ld_link_elf(
    TARGET_ELF            ${MCUBOOT_S1_EXECUTABLE}
    OUTPUT_MAP            ${PROJECT_BINARY_DIR}/${MCUBOOT_S1_EXECUTABLE}.map
    LIBRARIES_PRE_SCRIPT  ""
    LINKER_SCRIPT         ${PROJECT_BINARY_DIR}/linker_mcuboot_s1.cmd
    LIBRARIES_POST_SCRIPT ""
    DEPENDENCIES          ""
    )
  add_dependencies(${MCUBOOT_S1_EXECUTABLE} linker_mcuboot_s1_target)

  set(s1_hex_cmd "")
  set(s1_hex_byprod "")
  set(output ${PROJECT_BINARY_DIR}/../../zephyr/${MCUBOOT_S1_EXECUTABLE}.hex)

  # Rule to generate hex file of .elf
  bintools_objcopy(
    RESULT_CMD_LIST    s1_hex_cmd
    RESULT_BYPROD_LIST s1_hex_byprod
    STRIP_ALL
    GAP_FILL           "0xff"
    TARGET_OUTPUT      "ihex"
    SECTION_REMOVE     ${out_hex_sections_remove}
    FILE_INPUT         ${MCUBOOT_S1_EXECUTABLE}.elf
    FILE_OUTPUT        ${output}
    )

  add_custom_command(${s1_hex_cmd}
    DEPENDS ${MCUBOOT_S1_EXECUTABLE}
    OUTPUT ${output})

  add_custom_target(
    s1_image_hex
    ALL
    DEPENDS
    ${output}
    )

  # Register in the parent image that this child image will have
  # s1_image.hex as a byproduct, this allows the parent image to know
  # where the hex file comes from and create custom commands that
  # depend on it.
  set_property(
    TARGET         zephyr_property_target
    APPEND_STRING
    PROPERTY       shared_vars
    "list(APPEND ${IMAGE_NAME}BUILD_BYPRODUCTS ${output})\n"
    )
endif()
