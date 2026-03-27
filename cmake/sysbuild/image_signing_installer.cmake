# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/bootloader_dts_utils.cmake)

function(concat_binaries)
  cmake_parse_arguments(ARG "" "OUTPUT" "INPUT;DEPENDS" ${ARGN})

  string(CONCAT py_script
    "import sys; open(sys.argv[1], 'wb').write(b''.join("
    "open(f, 'rb').read() for f in sys.argv[2:]))"
  )
  add_custom_command(
    OUTPUT "${ARG_OUTPUT}"
    COMMAND ${PYTHON_EXECUTABLE} -c "${py_script}" "${ARG_OUTPUT}" ${ARG_INPUT}
    DEPENDS ${ARG_INPUT} ${ARG_DEPENDS}
    COMMENT "Concatenate binary files -> ${ARG_OUTPUT}"
    VERBATIM
  )
endfunction()

set(keyfile "${SB_CONFIG_BOOT_SIGNATURE_KEY_FILE}")
string(CONFIGURE "${keyfile}" keyfile)

if(NOT SB_CONFIG_BOOT_SIGNATURE_TYPE_NONE)
  # Check for misconfiguration.
  if("${keyfile}" STREQUAL "")
    # No signature key file, no signed binaries. No error, though:
    # this is the documented behavior.
    message(WARNING "Neither SB_CONFIG_BOOT_SIGNATURE_TYPE_NONE "
                    "SB_CONFIG_BOOT_SIGNATURE_KEY_FILE are set, the generated"
                    " build will not be bootable by MCUboot unless it is "
                    "signed manually/externally.")
    return()
  endif()
endif()

set(fw_loader_image ${SB_CONFIG_FIRMWARE_LOADER_IMAGE_NAME})

if(NOT SB_CONFIG_MCUBOOT_SIGN_MERGED_BINARY)
  sysbuild_get(binary_dir IMAGE ${fw_loader_image} VAR APPLICATION_BINARY_DIR CACHE)
  sysbuild_get(binary_bin_name IMAGE ${fw_loader_image} VAR CONFIG_KERNEL_BIN_NAME KCONFIG)
  set(fw_loader_image_signed_bin ${binary_dir}/zephyr/${binary_bin_name}.signed.bin)
else()
  set(fw_loader_image_signed_bin ${CMAKE_BINARY_DIR}/firmware_updater.signed.bin)
endif()

set(installer_image ${SB_CONFIG_FIRMWARE_LOADER_INSTALLER_APPLICATION_IMAGE_NAME})
sysbuild_get(binary_dir IMAGE ${installer_image} VAR APPLICATION_BINARY_DIR CACHE)
sysbuild_get(binary_bin_name IMAGE ${installer_image} VAR CONFIG_KERNEL_BIN_NAME KCONFIG)
set(installer_image_bin ${binary_dir}/zephyr/${binary_bin_name}.bin)

set(installer_merged_bin ${CMAKE_BINARY_DIR}/fw_loader_installer.merged.bin)

concat_binaries(
  INPUT ${installer_image_bin} ${fw_loader_image_signed_bin}
  OUTPUT ${installer_merged_bin}
  DEPENDS ${fw_loader_image}_extra_byproducts ${installer_image}_extra_byproducts
)

# Fetch devicetree details for flash and slot information
dt_chosen(flash_node TARGET ${installer_image} PROPERTY "zephyr,flash")
dt_prop(write_block_size TARGET ${installer_image} PATH "${flash_node}" PROPERTY
  "write-block-size"
)
dt_nodelabel(slot0_path TARGET ${installer_image} NODELABEL "slot0_partition" REQUIRED)
dt_partition_addr(slot0_addr PATH "${slot0_path}" TARGET ${installer_image} REQUIRED)
dt_reg_size(slot0_size TARGET ${installer_image} PATH ${slot0_path})

# The header size must be equal to the header size of the main image.
sysbuild_get(CONFIG_ROM_START_OFFSET IMAGE ${installer_image} VAR CONFIG_ROM_START_OFFSET KCONFIG)
sysbuild_get(fw_loader_version IMAGE ${fw_loader_image}
  VAR CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION KCONFIG
)


set(pad_header)
set(header_size ${CONFIG_ROM_START_OFFSET})
if(SB_CONFIG_MCUBOOT_SIGN_MERGED_BINARY)
  dt_chosen(code_flash TARGET ${installer_image} PROPERTY "zephyr,code-partition")
  dt_partition_addr(code_addr PATH "${code_flash}" TARGET ${installer_image} REQUIRED)

  math(EXPR header_size "${code_addr} - ${slot0_addr}")
  set(pad_header "--pad-header")
endif()

set(imgtool_rom_command --rom-fixed ${slot0_addr})
set(imgtool_sign
  ${PYTHON_EXECUTABLE} ${IMGTOOL} sign --version ${fw_loader_version}
  --align ${write_block_size} --slot-size ${slot0_size} ${pad_header}
  --header-size ${header_size} ${imgtool_rom_command})

# Fetch extra arguments to imgtool from the main image Kconfig.
set(CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS)
sysbuild_get(CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS IMAGE ${installer_image} VAR
  CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS KCONFIG
)

if(NOT CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS STREQUAL "")
  separate_arguments(imgtool_extra UNIX_COMMAND
    ${CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS}
  )
else()
  set(imgtool_extra)
endif()

# Set proper hash calculation algorithm for signing
if(SB_CONFIG_BOOT_SIGNATURE_TYPE_PURE)
  set(imgtool_extra --pure ${imgtool_extra})
elseif(SB_CONFIG_BOOT_IMG_HASH_ALG_SHA512)
  set(imgtool_extra --sha 512 ${imgtool_extra})
endif()

if(NOT "${keyfile}" STREQUAL "")
  set(imgtool_extra -k "${keyfile}" ${imgtool_extra})
endif()

set(output ${CMAKE_BINARY_DIR}/fw_loader_installer.signed)

add_custom_command(
  OUTPUT ${output}.bin
  DEPENDS ${installer_merged_bin}
  COMMAND ${imgtool_sign} ${imgtool_extra} ${installer_merged_bin} ${output}.bin
)

add_custom_target(installer_signed
  ALL
  DEPENDS ${output}.bin
)

sysbuild_get(CONFIG_INSTALLER_PACKAGE_BUILD_SIGNED_HEX IMAGE ${installer_image}
  VAR CONFIG_INSTALLER_PACKAGE_BUILD_SIGNED_HEX KCONFIG
)

if(CONFIG_INSTALLER_PACKAGE_BUILD_SIGNED_HEX)
  dt_partition_addr(slot0_addr_absolute PATH "${slot0_path}" TARGET ${installer_image}
    ABSOLUTE REQUIRED
  )
  sysbuild_get(objcopy_command IMAGE ${installer_image} VAR CMAKE_OBJCOPY CACHE)

  add_custom_command(
    OUTPUT ${output}.hex
    DEPENDS ${output}.bin
    COMMAND ${objcopy_command} -I binary -O ihex --change-addresses ${slot0_addr_absolute}
      ${output}.bin ${output}.hex
  )

  add_custom_target(installer_signed_hex
    ALL
    DEPENDS ${output}.hex
  )
endif()

if(SB_CONFIG_DFU_ZIP)
  include(${ZEPHYR_NRF_MODULE_DIR}/cmake/fw_zip.cmake)

  generate_dfu_zip(
    OUTPUT ${CMAKE_BINARY_DIR}/dfu_fw_loader_installer.zip
    BIN_FILES ${output}.bin
    TYPE application
    IMAGE ${installer_image}
    SCRIPT_PARAMS
    "version_INSTALLER=${CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION}"
    "version_FW_LOADER=${fw_loader_version}"
    DEPENDS installer_signed
  )
endif()
