#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_BOOTLOADER_MCUBOOT)
  get_target_property(mcuboot_board mcuboot BOARD)

  if(((NOT ${ZCMAKE_APPLICATION}_BOARD) AND (NOT mcuboot_board)) OR
     ("${${ZCMAKE_APPLICATION}_BOARD}" STREQUAL "${mcuboot_board}"))
    set_config_bool(${image} CONFIG_NCS_MCUBOOT_IN_BUILD y)
  endif()
endif()
