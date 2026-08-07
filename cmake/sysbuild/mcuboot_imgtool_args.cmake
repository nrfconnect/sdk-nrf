# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

function(ncs_mcuboot_imgtool_pad_value_args variable)
  if(NOT DEFINED CONFIG_NCS_MCUBOOT_IMGTOOL_PAD_VALUE)
    set(${variable} "" PARENT_SCOPE)
    return()
  endif()

  if(CONFIG_NCS_MCUBOOT_IMGTOOL_PAD_VALUE STREQUAL "")
    set(${variable} "" PARENT_SCOPE)
    return()
  endif()

  if(NOT CONFIG_NCS_MCUBOOT_IMGTOOL_PAD_VALUE MATCHES "^(0|0xff)$")
    message(FATAL_ERROR
      "CONFIG_NCS_MCUBOOT_IMGTOOL_PAD_VALUE must be empty, 0, or 0xff "
      "(got \"${CONFIG_NCS_MCUBOOT_IMGTOOL_PAD_VALUE}\")")
  endif()

  set(${variable} --pad-value ${CONFIG_NCS_MCUBOOT_IMGTOOL_PAD_VALUE} PARENT_SCOPE)
endfunction()

function(ncs_mcuboot_imgtool_append_pad_value_args args_variable)
  ncs_mcuboot_imgtool_pad_value_args(pad_value_args)
  if(pad_value_args)
    set(${args_variable} ${${args_variable}} ${pad_value_args} PARENT_SCOPE)
  endif()
endfunction()
