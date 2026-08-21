#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(NOT CONFIG_WIFI_NRF71_PATCH)
  return()
endif()

if(CONFIG_WIFI_NRF71_PATCH_VERSION STREQUAL "")
  message(WARNING "CONFIG_WIFI_NRF71_PATCH_VERSION must select a patch release")
  return()
endif()

# Paths and tools
set(fw_base ${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf71_wifi/bins)
set(fw_dir ${fw_base}/${CONFIG_WIFI_NRF71_PATCH_VERSION})
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
  string(JSON origin GET "${manifest}" "patches" ${idx} "origin")
  string(JSON size GET "${manifest}" "patches" ${idx} "size")

  math(EXPR addr "${origin}")
  math(EXPR size "${size}")
  math(EXPR end "${addr} + ${size}")

  set(bin ${fw_dir}/${file})
  if(NOT EXISTS ${bin})
    message(WARNING "${name} patch not found: ${bin}")
    return()
  endif()

  file(SIZE ${bin} actual_size)
  if(NOT actual_size STREQUAL "${size}")
    message(WARNING
      "${name} size mismatch: manifest=${size}, file=${actual_size}"
    )
    return()
  endif()
  if(size LESS_EQUAL 0)
    message(WARNING "${name} patch size must be greater than zero")
    return()
  endif()

  set(${name}_bin ${bin})
  set(${name}_origin ${origin})
  set(${name}_addr ${addr})
  set(${name}_end ${end})
  set(${name}_size ${size})

  if(addr LESS patch_region_start OR end GREATER patch_region_end)
    message(WARNING
      "${name} patch [${origin}, ${end}) is outside code partition "
      "[${patch_region_start}, ${patch_region_end})"
    )
    return()
  endif()

  foreach(other IN LISTS patches)
    if(addr LESS ${${other}_end} AND ${${other}_addr} LESS end)
      message(WARNING
        "${name} patch [${origin}, ${end}) overlaps "
        "${other} patch [${${other}_origin}, ${${other}_end})"
      )
      return()
    endif()
  endforeach()

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

# Compile definitions
#
# Expose patch addresses to SoC startup code.
foreach(name IN LISTS patches)
  string(TOUPPER ${name} upper)
  if(NOT CONFIG_BUILD_WITH_TFM)
    target_compile_definitions(soc__nordic PRIVATE
      NRF71_WIFI_${upper}_PATCH_ADDR=${${name}_addr}U
    )
  else()
    set_property(TARGET zephyr_property_target
      APPEND PROPERTY TFM_CMAKE_OPTIONS
      -DNRF71_WIFI_${upper}_PATCH_ADDR=${${name}_addr}
    )
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
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${${name}_bin})
endforeach()

list(APPEND merge_command
  COMMAND ${CMAKE_COMMAND} -E echo "Generating nRF7120 merged HEX: ${merged_hex}"
  COMMAND ${PYTHON_EXECUTABLE}
    ${merge_script}
    -o ${merged_hex}
    ${merge_inputs}
    ${patch_hexes}
  COMMAND ${CMAKE_COMMAND} -E echo "Generating nRF7120 merged BIN: ${merged_bin}"
  COMMAND ${CMAKE_OBJCOPY}
    --input-target=ihex
    --output-target=binary
    --gap-fill 0xFF
    ${merged_hex}
    ${merged_bin}
)

set_property(GLOBAL APPEND PROPERTY extra_post_build_commands ${merge_command})
set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts ${merge_byproducts})

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
