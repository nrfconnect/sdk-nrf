#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

set(fw_base ${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf71_wifi/bins)
set(fw_dir ${fw_base}/${SB_CONFIG_WIFI_NRF71_PATCH_VERSION})
set(manifest_file ${fw_dir}/manifest.json)
set(generated_overlay ${CMAKE_BINARY_DIR}/nrf71_wifi_patch.overlay)

if(NOT EXISTS ${manifest_file})
  message(WARNING
    "nRF7120 Wi-Fi patch manifest not found: ${manifest_file}"
  )
  return()
endif()

file(READ ${manifest_file} manifest_json)
string(JSON patch_count LENGTH "${manifest_json}" patches)
if(patch_count EQUAL 0)
  message(WARNING
    "nRF7120 Wi-Fi patch manifest contains no patches: ${manifest_file}"
  )
  return()
endif()

set(OVERLAY_NODES)
set(OVERLAY_WICR)

math(EXPR last_idx "${patch_count} - 1")
foreach(idx RANGE ${last_idx})
  string(JSON name GET "${manifest_json}" patches ${idx} name)
  string(JSON origin GET "${manifest_json}" patches ${idx} origin)
  string(JSON size GET "${manifest_json}" patches ${idx} size)

  # Devicetree unit addresses are hexadecimal without the 0x prefix.
  string(REGEX REPLACE "^0[xX]" "" unit_address "${origin}")
  string(APPEND OVERLAY_NODES
    "\t\t${name}_rom_patch_addr: ${name}-rom-patch@${unit_address} {\n"
    "\t\t\treg = <${origin} ${size}>;\n"
    "\t\t};\n"
  )

  if(name STREQUAL "lmac" OR name STREQUAL "umac")
    string(APPEND OVERLAY_WICR
      "\tfirmware-${name}rompatchaddr = <&${name}_rom_patch_addr>;\n"
    )
  endif()
endforeach()

# Trim the trailing newline left by the last APPEND.
string(REGEX REPLACE "\n$" "" OVERLAY_NODES "${OVERLAY_NODES}")
string(REGEX REPLACE "\n$" "" OVERLAY_WICR "${OVERLAY_WICR}")

configure_file(
  ${CMAKE_CURRENT_LIST_DIR}/patch_overlay.in
  ${generated_overlay}
  @ONLY
)
message(STATUS "Generated nRF7120 Wi-Fi patch DT overlay: ${generated_overlay}")

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${manifest_file})

add_overlay_dts(${DEFAULT_IMAGE} ${generated_overlay})
set_config_bool(${DEFAULT_IMAGE} CONFIG_WIFI_NRF71_PATCH y)
set_config_string(${DEFAULT_IMAGE} CONFIG_WIFI_NRF71_PATCH_VERSION
  "${SB_CONFIG_WIFI_NRF71_PATCH_VERSION}"
)
