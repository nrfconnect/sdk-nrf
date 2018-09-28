#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/board.c)

target_sources_ifdef(CONFIG_HAS_DTS_LIS3DH app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/accel_lis3dh.c)

target_sources_ifdef(CONFIG_HAS_DTS_BMA400 app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/accel_bma400.c)

target_sources_ifdef(CONFIG_HAS_DTS_QDEC app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/wheel.c)

target_sources_ifdef(CONFIG_HAS_DTS_TOUCHPAD app PRIVATE
                     ${CMAKE_CURRENT_SOURCE_DIR}/motion_touchpad.c)

target_sources_ifdef(CONFIG_HAS_DTS_MOTION app PRIVATE
                     ${CMAKE_CURRENT_SOURCE_DIR}/motion_pca20041.c
                     ${CMAKE_CURRENT_SOURCE_DIR}/motion_pca20041_priv.c)

target_sources_ifdef(CONFIG_BOARD_NRF52_PCA63519 app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/buttons_pca63519.c)

target_sources_ifdef(CONFIG_DESKTOP_LED_ENABLE app
		     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/leds.c)

target_sources_ifdef(CONFIG_BOARD_NRF52840_PCA20041 app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/buttons_pca20041.c)

target_sources_ifdef(CONFIG_BOARD_NRF52840_PCA10056 app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/motion_pca10056.c)
