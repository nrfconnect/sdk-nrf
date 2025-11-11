#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_lc3_streamer)

# lc3_streamer source must be added manually as kconfigs and CMakeLists in nRF5340 audio application
# is not available from here.
target_sources(app PRIVATE
  src/main.c
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf5340_audio/src/modules/lc3_streamer.c
  ${ZEPHYR_NRF_MODULE_DIR}/tests/nrf5340_audio/fakes/lc3_file/lc3_file_fake.c
  ${ZEPHYR_NRF_MODULE_DIR}/tests/nrf5340_audio/fakes/lc3_file/lc3_file_fake_data.c
  ${ZEPHYR_NRF_MODULE_DIR}/tests/nrf5340_audio/fakes/k_work/k_work_fake.c
)

target_compile_definitions(app PRIVATE CONFIG_MODULE_SD_CARD_LC3_STREAMER_LOG_LEVEL=3)
target_compile_definitions(app PRIVATE CONFIG_SD_CARD_LC3_STREAMER_STACK_SIZE=500)
target_compile_definitions(app PRIVATE CONFIG_SD_CARD_LC3_STREAMER_THREAD_PRIO=4)
target_compile_definitions(app PRIVATE CONFIG_SD_CARD_LC3_STREAMER_MAX_NUM_STREAMS=3)
target_compile_definitions(app PRIVATE CONFIG_SD_CARD_LC3_STREAMER_MAX_FRAME_SIZE=251)
target_compile_definitions(app PRIVATE CONFIG_FS_FATFS_MAX_LFN=40)

target_include_directories(app PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf5340_audio/src
  ${ZEPHYR_NRF_MODULE_DIR}/tests/nrf5340_audio/fakes
)
