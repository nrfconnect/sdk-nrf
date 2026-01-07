# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bsim_test_adv_chain)

add_subdirectory(${ZEPHYR_NRF_MODULE_DIR}/tests/bsim/babblekit babblekit)
target_link_libraries(app PRIVATE babblekit)

target_sources(app PRIVATE
  src/main.c
  src/broadcaster_multiple.c
  src/observer.c
)

zephyr_include_directories(
  ${BSIM_COMPONENTS_PATH}/libUtilv1/src/
  ${BSIM_COMPONENTS_PATH}/libPhyComv1/src/
)
