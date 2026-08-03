#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(NOT CONFIG_WIFI_NRF71_PATCH)
  return()
endif()

if(CONFIG_WIFI_NRF71_PATCH_DIR STREQUAL "")
  message(WARNING "No WIFI_NRF71_PATCH_DIR default for the selected WIFI_NRF71_PATCH_VERSION")
  return()
endif()

# Paths and tools
set(fw_dir ${CONFIG_WIFI_NRF71_PATCH_DIR})
set(manifest_file ${fw_dir}/manifest.json)
set(merge_script ${ZEPHYR_BASE}/scripts/build/mergehex.py)
set(zephyr_dir ${APPLICATION_BINARY_DIR}/zephyr)

# Patch geometry
dt_chosen(code_partition PROPERTY "zephyr,code-partition")
dt_reg_addr(patch_region_start PATH ${code_partition})
dt_reg_size(patch_region_size PATH ${code_partition})
math(EXPR patch_region_end "${patch_region_start} + ${patch_region_size}")

# WICR node
dt_nodelabel(wicr_node NODELABEL wicr REQUIRED)
dt_node_has_status(wicr_enabled PATH ${wicr_node} STATUS okay)
if(NOT wicr_enabled)
  message(WARNING
    "CONFIG_WIFI_NRF71_PATCH requires the wicr devicetree node to be enabled"
  )
  return()
endif()

# Manifest parsing
if(NOT EXISTS ${manifest_file})
  message(WARNING "Missing nRF7120 Wi-Fi patch manifest: ${manifest_file}")
  return()
endif()

file(READ ${manifest_file} manifest)
string(JSON patch_count LENGTH "${manifest}" "patches")
if(patch_count EQUAL 0)
  message(WARNING "Manifest does not define a Wi-Fi patch")
  return()
endif()

# Patch validation
set(patches)
math(EXPR last_idx "${patch_count} - 1")
foreach(idx RANGE ${last_idx})
  string(JSON name GET "${manifest}" "patches" ${idx} "name")
  string(JSON file GET "${manifest}" "patches" ${idx} "file")
  string(JSON manifest_origin GET "${manifest}" "patches" ${idx} "origin")
  string(JSON manifest_size GET "${manifest}" "patches" ${idx} "size")

  dt_prop(patch_node PATH ${wicr_node} PROPERTY "firmware-${name}rompatchaddr")
  dt_reg_addr(origin PATH "${patch_node}")
  dt_reg_size(size PATH "${patch_node}")

  if(origin STREQUAL "" OR origin STREQUAL "NONE"
     OR size STREQUAL "" OR size STREQUAL "NONE")
    message(WARNING
      "${name} patch devicetree entry does not provide a patch region. "
      "Please include:\n"
      "${fw_dir}/nrf7120_wifi_patch.dtsi"
    )
    return()
  endif()

  math(EXPR addr "${origin}")
  math(EXPR end "${addr} + ${size}")
  math(EXPR manifest_addr "${manifest_origin}")

  if(NOT addr EQUAL manifest_addr OR NOT size EQUAL manifest_size)
    message(WARNING
      "${name} patch address mismatch: devicetree has origin=${origin} "
      "size=${size}, manifest expects origin=${manifest_origin} "
      "size=${manifest_size}. Please include:\n"
      "${fw_dir}/nrf7120_wifi_patch.dtsi"
    )
    return()
  endif()

  if(size LESS_EQUAL 0)
    message(WARNING "${name} patch size from devicetree must be greater than zero")
    return()
  endif()

  set(bin ${fw_dir}/${file})
  if(NOT EXISTS ${bin})
    message(WARNING "${name} patch binary not found: ${bin}")
    return()
  endif()

  file(SIZE ${bin} bin_size)
  if(NOT bin_size EQUAL size)
    message(WARNING
      "${name} patch binary size mismatch: devicetree=${size}, "
      "file=${bin_size}"
    )
    return()
  endif()

  set(${name}_bin ${bin})
  set(${name}_origin ${origin})
  set(${name}_addr ${addr})
  set(${name}_size ${size})

  if(addr LESS patch_region_start OR end GREATER patch_region_end)
    message(WARNING
      "${name} patch [${origin}, ${end}) is outside code partition "
      "[${patch_region_start}, ${patch_region_end})"
    )
    return()
  endif()

  list(APPEND patches ${name})
endforeach()

# Linker reservations
#
# Each patch gets a NOLOAD section at its target address. A content-hash symbol
# is added to the section to force a relink when the patch binary changes.
#
# The LD template inserts these snippets before the 4-byte .last_section.
# This moves .last_section past the highest patch, so _flash_used includes the
# full span through the patch plus alignment and four bytes. The CMake linker
# generator keeps fixed NOLOAD reservations out of the FLASH usage total.
if(NOT CONFIG_CMAKE_LINKER_GENERATOR)
  set(linker_dir ${CMAKE_CURRENT_BINARY_DIR}/linker)
  file(MAKE_DIRECTORY ${linker_dir})
endif()

foreach(name IN LISTS patches)
  file(SHA256 ${${name}_bin} sha256)

  if(CONFIG_CMAKE_LINKER_GENERATOR)
    zephyr_linker_section(
      NAME .wifi_${name}_patch
      VMA FLASH
      ADDRESS ${${name}_addr}
      TYPE NOLOAD
      NOINPUT
      MIN_SIZE ${${name}_size}
    )

    # Content hash in symbol table forces relink on patch update.
    zephyr_linker_symbol(SYMBOL __wifi_${name}_patch_${sha256} EXPR 0)
  else()
    # Variable names must match the placeholders in the linker template.
    set(section_name wifi_${name}_patch)
    set(patch_origin ${${name}_addr})
    set(patch_size ${${name}_size})
    set(_patch_sha256 ${sha256})
    configure_file(
      ${CMAKE_CURRENT_LIST_DIR}/wifi_patch_section.ld.in
      ${linker_dir}/wifi_${name}_patch.ld
      @ONLY
    )
    zephyr_linker_sources(SECTIONS ${linker_dir}/wifi_${name}_patch.ld)
  endif()
endforeach()

# Post-build merge
#
# TF-M provides secure and non-secure HEX files separately. The merge script
# combines them with the Wi-Fi patch(es) into a single HEX file for flashing.

# Keep the KERNEL_NAME-based prefix aligned with image_signing.cmake.
set(merged_prefix ${zephyr_dir}/${KERNEL_NAME}.nrf7120)
set(merged_hex ${merged_prefix}.hex)
set(merged_bin ${merged_prefix}.bin)
set(merge_byproducts ${merged_hex} ${merged_bin})

set(merge_inputs ${zephyr_dir}/${KERNEL_HEX_NAME})
if(CONFIG_BUILD_WITH_TFM)
  set(merge_inputs "$<TARGET_PROPERTY:tfm,TFM_S_HEX_FILE>")
  if(CONFIG_TFM_USE_NS_APP)
    list(APPEND merge_inputs "$<TARGET_PROPERTY:tfm,TFM_NS_HEX_FILE>")
  else()
    list(APPEND merge_inputs ${zephyr_dir}/${KERNEL_HEX_NAME})
  endif()
endif()

set(merge_command)
set(merge_depends ${merge_inputs})
set(patch_hexes)
foreach(name IN LISTS patches)
  set(patch_hex ${zephyr_dir}/${name}.hex)
  list(APPEND merge_command
    COMMAND ${CMAKE_OBJCOPY}
      --input-target=binary
      --output-target=ihex
      --change-addresses ${${name}_origin}
      ${${name}_bin}
      ${patch_hex}
  )
  list(APPEND patch_hexes ${patch_hex})
  list(APPEND merge_byproducts ${patch_hex})
  list(APPEND merge_depends ${${name}_bin})
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${${name}_bin})
endforeach()

list(APPEND merge_command
  COMMAND ${PYTHON_EXECUTABLE}
    ${merge_script}
    -o ${merged_hex}
    ${merge_inputs}
    ${patch_hexes}
  COMMAND ${CMAKE_OBJCOPY}
    --input-target=ihex
    --output-target=binary
    --gap-fill 0xFF
    ${merged_hex}
    ${merged_bin}
)

add_custom_command(
  OUTPUT ${merge_byproducts}
  DEPENDS ${merge_depends}
  ${merge_command}
  COMMENT "Generating nRF7120 merged HEX: ${merged_hex}"
  COMMAND_EXPAND_LISTS
  VERBATIM
)
add_custom_target(nrf71_wifi_patch_merge ALL DEPENDS ${merge_byproducts})

# Flash runner override
if(NOT CONFIG_BOOTLOADER_MCUBOOT)
  set_target_properties(runners_yaml_props_target PROPERTIES
    hex_file ${merged_hex}
    bin_file ${merged_bin}
  )
endif()

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  ${manifest_file}
  ${merge_script}
)
