# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

# If it is enabled, include the eGPIO FLPR application in the build
if(SB_CONFIG_EGPIO_FLPR_APPLICATION)
  # Extract soc name from related variables
  string(REPLACE "/" ";" split_board_qualifiers "${BOARD_QUALIFIERS}") # Replace all "/" with ";"
  list(GET split_board_qualifiers 1 target_soc) # Get the second element of the list, which is the soc name
  if((DEFINED ${BOARD_REVISION}) AND NOT ${BOARD_REVISION} STREQUAL "")
    string(CONCAT board_target_flpr ${BOARD} "@" ${BOARD_REVISION} "/" ${target_soc} "/cpuflpr")
  else()
    string(CONCAT board_target_flpr ${BOARD} "/" ${target_soc} "/cpuflpr")
  endif()
  set(target_soc) # Clear the variable that is no longer needed

  ExternalZephyrProject_Add(
    APPLICATION flpr_egpio # Add ExternalZephyrProject for the sdp_gpio application
    SOURCE_DIR ${ZEPHYR_NRF_MODULE_DIR}/applications/sdp/gpio # Specify the path to the application directory
    BOARD ${board_target_flpr} # Specify the board for the application
  )
endif()
