#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

function(forward_bl_storage_addr image)
  dt_nodelabel(bl_storage_path NODELABEL bl_storage TARGET b0)

  if(NOT DEFINED bl_storage_path)
    return()
  endif()

  dt_reg_addr(bl_storage_addr PATH "${bl_storage_path}" TARGET b0)

  if(NOT DEFINED bl_storage_addr)
    return()
  endif()

  set(${image}_TFM_BL_STORAGE_ADDR "${bl_storage_addr}" CACHE INTERNAL "")
endfunction()
