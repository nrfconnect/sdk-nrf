#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(NOT CONFIG_CJSON_LIB)
  return()
endif()

zephyr_library_named(cjson)
zephyr_include_directories(
	${ZEPHYR_CJSON_MODULE_DIR}
	include
)

zephyr_library_sources(
	${ZEPHYR_CJSON_MODULE_DIR}/cJSON.c
	${ZEPHYR_CJSON_MODULE_DIR}/cJSON_Utils.c
	os/cJSON_os.c
)
