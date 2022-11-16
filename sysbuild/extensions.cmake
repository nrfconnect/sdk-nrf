#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Extension CMake file for sysbuild integration

# This code provides the possibility of applying Kconfig fragments to images
# when secure boot is enabled. Temporary use the existing nRF Connect SDK scheme
# until a more proper scheme is developed uypstream.

# Add an overlay file to an image.
# This can be used by sysbuild to set overlay of Kconfig configuration or devicetree
# in its images.
#
# Parameters:
#   'image' - image name
#   'overlay_file' - overlay to be added to  image
#   'overlay_type' - 'OVERLAY_CONFIG' or 'DTC_OVERLAY_FILE'
function(add_overlay image overlay_file overlay_type)
  set(old_overlays ${${image}_${overlay_type}})
  string(FIND "${old_overlays}" "${overlay_file}" found)
  if (${found} EQUAL -1)
    set(${image}_${overlay_type} "${old_overlays};${overlay_file}" CACHE STRING
      "Extra config fragments for ${image} image" FORCE
    )
  endif()
endfunction()

# Convenience macro to add configuration overlays to image.
macro(add_overlay_config image overlay_file)
  add_overlay(${image} ${overlay_file} OVERLAY_CONFIG)
endmacro()

# Convenience macro to add device tree overlays to image.
macro(add_overlay_dts image overlay_file)
  add_overlay(${image} ${overlay_file} DTC_OVERLAY_FILE)
endmacro()
