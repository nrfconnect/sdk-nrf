#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_auraconfig)

add_subdirectory(${ZEPHYR_BASE}/tests/bsim/babblekit babblekit)
target_link_libraries(app PRIVATE babblekit)

# Include application events and configuration headers
zephyr_library_include_directories(app PRIVATE
    ${ZEPHYR_NRF_MODULE_DIR}/samples/bluetooth/nrf_auraconfig/include
    ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf5340_audio/include
    ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf5340_audio/src/bluetooth
    ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf5340_audio/src/modules
    ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf5340_audio/src/utils/macros
)

add_subdirectory(${ZEPHYR_NRF_MODULE_DIR}/applications/nrf5340_audio/src/bluetooth bluetooth_build)

target_sources_ifdef(CONFIG_NRF5340_AUDIO_SD_CARD_MODULE app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf5340_audio/src/modules/sd_card.c)
target_sources_ifdef(CONFIG_NRF5340_AUDIO_SD_CARD_LC3_FILE app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf5340_audio/src/modules/lc3_file.c)
target_sources_ifdef(CONFIG_NRF5340_AUDIO_SD_CARD_LC3_STREAMER app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf5340_audio/src/modules/lc3_streamer.c)

target_sources(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/samples/bluetooth/nrf_auraconfig/src/nrf_auraconfig.c)

target_sources(app PRIVATE src/led_mock.c)
target_sources(app PRIVATE src/audio_sync_timer_mock.c)
target_sources(app PRIVATE src/main.c)

zephyr_include_directories(
  ${BSIM_COMPONENTS_PATH}/libUtilv1/src/
  ${BSIM_COMPONENTS_PATH}/libPhyComv1/src/
  )
