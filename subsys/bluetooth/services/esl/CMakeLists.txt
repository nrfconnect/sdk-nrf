#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
zephyr_library()

zephyr_library_sources_ifdef(
  CONFIG_BT_ESL
  esl.c
  esl_settings.c
  esl_common.c)
zephyr_sources_ifdef(CONFIG_ESL_SHELL		esl_shell.c)

zephyr_library_sources_ifdef(
  CONFIG_BT_ESL_CLIENT
  esl_client.c
  esl_client_settings.c
  esl_common.c)

zephyr_library_sources_ifdef(
  CONFIG_ESL_CLIENT_SHELL
  esl_client_shell.c
  esl_dummy_cmd.c)
zephyr_library_sources_ifdef(CONFIG_BT_ESL_TAG_STORAGE  esl_client_tag_storage.c)

zephyr_library_include_directories(${CMAKE_CURRENT_BINARY_DIR})
