#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

if(NOT BOARD)
        set(BOARD $ENV{BOARD})
endif()

# Check if selected board is supported.
if(DEFINED NRF_SUPPORTED_BOARDS)
        if(NOT BOARD IN_LIST NRF_SUPPORTED_BOARDS)
                message(FATAL_ERROR "board ${BOARD} is not supported")
        endif()
endif()

# Point to NCS root directory.
set(NRF_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

# Set BOARD_ROOT if board definition exists out of tree.
find_path(_BOARD_DIR NAMES "${BOARD}_defconfig" PATHS ${NRF_DIR}/boards/*/*
          NO_DEFAULT_PATH)

if (_BOARD_DIR)
        set(BOARD_ROOT ${NRF_DIR})
endif()
