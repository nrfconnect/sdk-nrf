#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: BSD-5-Clause-Nordic
#

target_sources_ifdef(CONFIG_HAS_DTS_BMA400 app PRIVATE
                     ${CMAKE_CURRENT_SOURCE_DIR}/accel.c)

target_sources_ifdef(CONFIG_BOARD_NRF52_PCA20041 app PRIVATE
                     ${CMAKE_CURRENT_SOURCE_DIR}/board.c)

target_sources_ifdef(CONFIG_BOARD_NRF52840_PCA10056 app PRIVATE
                     ${CMAKE_CURRENT_SOURCE_DIR}/board_pca10056.c)

target_sources_ifdef(CONFIG_BOARD_NRF52_PCA20041 app PRIVATE
                     ${CMAKE_CURRENT_SOURCE_DIR}/buttons.c)

target_sources_ifdef(CONFIG_HAS_DTS_PMW3360 app PRIVATE
                     ${CMAKE_CURRENT_SOURCE_DIR}/motion.c)

target_sources_ifdef(CONFIG_BOARD_NRF52840_PCA10056 app PRIVATE
                     ${CMAKE_CURRENT_SOURCE_DIR}/btn_emu_motion.c)
