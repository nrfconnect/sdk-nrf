#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(NOT DEFINED FP_MODEL_ID AND NOT DEFINED FP_ANTI_SPOOFING_KEY)
  message(WARNING "
  -------------------------------------------------------
  --- WARNING: Using demo Fast Pair Model ID and Fast ---
  --- Pair Anti Spoofing Key, it should not be used   ---
  --- for production.                                 ---
  -------------------------------------------------------
  \n"
  )
  set(FP_MODEL_ID "0x4A436B" PARENT_SCOPE)
  set(FP_ANTI_SPOOFING_KEY "rie10A7ONqwd77VmkxGsblPUbMt384qjDgcEJ/ctT9Y=" PARENT_SCOPE)
endif()

if(SB_CONFIG_APP_DFU)
  set_config_bool(${DEFAULT_IMAGE} CONFIG_APP_DFU y)

  if(DEFINED SB_CONFIG_NETCORE_IMAGE_NAME AND NOT (SB_CONFIG_NETCORE_IMAGE_NAME STREQUAL ""))
    set(dfu_speedup_kconfig_fragment
        ${APP_DIR}/sysbuild/common/dfu_speedup_fragment.conf
    )

    if (EXISTS ${dfu_speedup_kconfig_fragment})
      add_overlay_config(
          ${SB_CONFIG_NETCORE_IMAGE_NAME}
          ${dfu_speedup_kconfig_fragment}
      )
    else()
      message(WARNING "DFU speedup fragment for netcore not found")
    endif()
  endif()
else()
  set_config_bool(${DEFAULT_IMAGE} CONFIG_APP_DFU n)
endif()
