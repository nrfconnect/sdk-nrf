#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

# The application uses the conf/<board> scheme for configuration files.
#set(APPLICATION_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/conf/\${BOARD}")

set(PM_STATIC_YML_FILE
  #"${CMAKE_CURRENT_SOURCE_DIR}/conf/\${BOARD}/pm_static_${BOARD}.yml"
   ${CMAKE_CURRENT_SOURCE_DIR}/boards/pm_static_${BOARD}.yml
  )

list(APPEND hci_rpmsg_b0n_OVERLAY_CONFIG overlay-minimal-size.conf)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(central_esl)

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
)

# Add ESLS Client source
zephyr_library_sources_ifdef(
  CONFIG_BT_ESL_CLIENT
  ../service/esl_client.c
  ../service/esl_client_settings.c
  ../service/esl_common.c)

zephyr_library_sources_ifdef(
  CONFIG_ESL_CLIENT_SHELL
  ../service/esl_client_shell.c
  ../service/esl_dummy_cmd.c)
zephyr_library_sources_ifdef(CONFIG_BT_ESL_TAG_STORAGE  ../service/esl_client_tag_storage.c)

zephyr_library_include_directories_ifdef(CONFIG_BT_ESL_CLIENT ../service)
target_include_directories(app PRIVATE   ${ZEPHYR_BASE}/subsys/bluetooth/host)

# NORDIC SDK APP END

zephyr_library_include_directories(.)
