#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

# Do not add source using zephyr cmake extensions.
# This is because our library should not be added to the 'ZEPHYR_LIBS' list,
# since it should not be linked to the zephyr elf itself.

add_library(bootloader bootloader.c)

target_link_libraries(bootloader PRIVATE bl_crypto)

add_subdirectory(c_runtime_setup)
target_link_libraries(bootloader PRIVATE c_runtime_setup)

add_subdirectory(provision)
target_link_libraries(bootloader PRIVATE provision)

add_subdirectory(libc)
target_link_libraries(bootloader PRIVATE libc)

add_subdirectory(debug)
target_link_libraries(bootloader PRIVATE debuglib)

add_subdirectory_ifdef(CONFIG_SB_FLASH_PROTECT fprotect)
target_link_libraries_ifdef(CONFIG_SB_FLASH_PROTECT bootloader PRIVATE fprotect)

add_subdirectory(fw_metadata)
target_link_libraries(bootloader PRIVATE fw_metadata)

add_dependencies(bootloader offsets_h)
target_link_libraries(bootloader PRIVATE zephyr_interface)

target_include_directories(
  bootloader
  PRIVATE
  include
  )

target_include_directories_ifdef(CONFIG_SECURE_BOOT_DEBUG
  bootloader
  PRIVATE
  include/debug
  )

set(NRF_BOOTLOADER_SCRIPTS ${NRF_DIR}/scripts/bootloader)

include(${NRF_DIR}/subsys/bootloader/cmake/fw_metadata.cmake)

# Create linker flags for zephyr bootloader
set(bl_zephyr_lnk
  ${LINKERFLAGPREFIX},-Map=${PROJECT_BINARY_DIR}/bootloader.map
  -u_OffsetAbsSyms
  -u_ConfigAbsSyms
  # We need all symbols defined in the bootloader library,
  # Even those that are not used anywhere.
  # Acheive this by using the '--whole-archive' flag.
  ${LINKERFLAGPREFIX},--whole-archive
  bootloader # include bootloader library
  ${LINKERFLAGPREFIX},--no-whole-archive
  ${OFFSETS_O_PATH}
  ${LIB_INCLUDE_DIR}
  ${TOOLCHAIN_LIBS}
  )

zephyr_get_include_directories_for_lang(C ZEPHYR_INCLUDES)

# Set our custom linker script to be called when linking the bootloader.
set(LINKER_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/include/linker.ld)
construct_add_custom_command_for_linker_pass(bootloader custom_command)
unset(LINKER_SCRIPT)
# For some reason, we need to have ${PROJECT_BINARY_DIR} before the
# in the OUTPUT argument. The file ends up in the same place, but for
# cmake to see that the bootloader_script target points to the correct
# file, it is needed. Modify the custom command by appending
# ${PROJECT_BINARY_DIR/ to the OUTPUT argument.
STRING(REGEX REPLACE "(OUTPUT;)" "\\1${PROJECT_BINARY_DIR}\/" bl_custom_command "${custom_command}")
add_custom_command(
  ${bl_custom_command}
  )

# Create custom target for generated linker script 'bootloader.cmd'
# This is done since CMake does not handle dependencies to files very
# well, so this is done to be able to represent a dependency to the
# generated 'bootloader.cmd' file.
add_custom_target(
  bootloader_script
  DEPENDS
  ${ALIGN_SIZING_DEP}
  ${PROJECT_BINARY_DIR}/bootloader.cmd
  offsets_h
  )

# Give the 'bootloader_script' target all of the include directories so
# that cmake can successfully find the bootloader_script's header
# dependencies.
zephyr_get_include_directories_for_lang(C
  ZEPHYR_INCLUDE_DIRS
  STRIP_PREFIX # Don't use a -I prefix
  )

# Associate include directories to linker script target.
# This needs to be done since preprocessing is done in linker script.
set_property(TARGET
  bootloader_script
  PROPERTY INCLUDE_DIRECTORIES
  ${ZEPHYR_INCLUDE_DIRS}
  )

# Create the executable target with a dummy file.
add_executable(bootloader_elf ${ZEPHYR_BASE}/misc/empty_file.c)

# Ensure that we are compiled with the same flags.
target_link_libraries(bootloader_elf zephyr_interface)

# Link executable target with custom bootloader linker script 'bootloader.cmd' and the linker flags in 'bl_zephyr_lnk'
target_link_libraries(bootloader_elf ${TOPT} ${PROJECT_BINARY_DIR}/bootloader.cmd ${bl_zephyr_lnk})
target_link_libraries(bootloader_elf zephyr_interface)

# Create dependency between elf target and linker script.
set_property(TARGET bootloader_elf PROPERTY LINK_DEPENDS ${PROJECT_BINARY_DIR}/bootloader.cmd)

# Since CMake does not handle file dependencies very well, create dependency to 'bootloader_script'
# target, as well as standard stuff.
add_dependencies(bootloader_elf ${ALIGN_SIZING_DEP} ${PRIV_STACK_DEP} bootloader_script offsets)

set(BOOTLOADER_HEX_NAME bootloader.hex)
set(BOOTLOADER_HEX ${PROJECT_BINARY_DIR}/${BOOTLOADER_HEX_NAME})
set(KERNEL_HEX ${PROJECT_BINARY_DIR}/${KERNEL_HEX_NAME})
set(SIGNED_KERNEL_HEX_NAME signed_${KERNEL_HEX_NAME})
set(SIGNED_KERNEL_HEX ${PROJECT_BINARY_DIR}/${SIGNED_KERNEL_HEX_NAME})
set(PROVISION_HEX_NAME provision_data.hex)
set(PROVISION_HEX ${PROJECT_BINARY_DIR}/${PROVISION_HEX_NAME})

message(INFO ${BOOTLOADER_HEX})

# Create custom command for creating hex file from bootloader elf file.
add_custom_command(
  OUTPUT ${BOOTLOADER_HEX}
  COMMAND ${CMAKE_OBJCOPY}
  -S
  -Oihex
  -R .comment
  -R COMMON
  -R .eh_frame
  bootloader_elf.elf
  ${BOOTLOADER_HEX}
  DEPENDS bootloader_elf
  )
# Create target with the generated custom command.
# This is needed since CMake does not represent denpendencies to files in a good manner.
add_custom_target(bootloader_hex DEPENDS ${BOOTLOADER_HEX})

# Append all hex files to merge to list.
list(APPEND to_merge ${BOOTLOADER_HEX})
list(APPEND to_merge ${SIGNED_KERNEL_HEX})
list(APPEND to_merge ${PROVISION_HEX})
set_property(GLOBAL APPEND PROPERTY HEX_FILES_TO_MERGE ${to_merge})

include (${CMAKE_CURRENT_LIST_DIR}/cmake/debug_keys.cmake)
include (${CMAKE_CURRENT_LIST_DIR}/cmake/sign.cmake)

add_custom_target(
  sign_target
  DEPENDS
  ${PROJECT_BINARY_DIR}/${SIGNED_KERNEL_HEX_NAME}
  offsets_h
  )

if(DEFINED DT_FLASH_AREA_S0_OFFSET)
	set(S0_ADDR ${DT_FLASH_AREA_S0_OFFSET})
else()
	if(CONFIG_SOC_NRF9160)
		set(S0_ADDR 0x8000)
	else()
		set(S0_ADDR 0x4000)
	endif()
endif()

if(DEFINED DT_FLASH_AREA_S1_OFFSET)
	set(S1_ADDR ${DT_FLASH_AREA_S1_OFFSET})
else()
	if(CONFIG_SOC_NRF9160)
		set(S1_ADDR 0xc0000)
	else()
		set(S1_ADDR 0x10000)
	endif()
endif()

if(DEFINED DT_FLASH_AREA_PROVISION_OFFSET)
	set(PROVISION_ADDR ${DT_FLASH_AREA_PROVISION_OFFSET})
else()
	if(CONFIG_SOC_NRF52832)
		set(PROVISION_ADDR 0x7f000)
	else()
		set(PROVISION_ADDR 0xff000)
	endif()
endif()

add_custom_command(
  OUTPUT
  ${PROVISION_HEX}
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${NRF_BOOTLOADER_SCRIPTS}/provision.py
  --s0-addr ${S0_ADDR}
  --s1-addr ${S1_ADDR}
  --provision-addr ${PROVISION_ADDR}
  --public-key-files "${SIGNATURE_PUBLIC_KEY_FILE},${PUBLIC_KEY_FILES}"
  --output ${PROVISION_HEX}
  DEPENDS ${PROVISION_DEPENDS}
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  COMMENT
  "Creating provision data for Bootloader, storing to ${PROVISION_HEX_NAME}"
  USES_TERMINAL
  )
add_custom_target(
  provision_target
  DEPENDS
  ${PROJECT_BINARY_DIR}/${PROVISION_HEX_NAME}
  offsets_h
  )


if(CONFIG_OUTPUT_PRINT_MEMORY_USAGE)
  set(option ${LINKERFLAGPREFIX},--print-memory-usage)
  string(MAKE_C_IDENTIFIER check${option} check)

  set(SAVED_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
  set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${option}")
  zephyr_check_compiler_flag(C "" ${check})
  set(CMAKE_REQUIRED_FLAGS ${SAVED_CMAKE_REQUIRED_FLAGS})

  target_link_libraries_ifdef(${check} bootloader_elf ${option})
endif()
