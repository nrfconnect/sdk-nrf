#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONF_FILE)
    get_filename_component(CONFIG_FILE_NAME ${CONF_FILE} NAME)
endif()

# Set the paritions configuration
if(NOT "${CONFIG_FILE_NAME}" STREQUAL "prj_no_dfu.conf")
    set(PM_STATIC_YML_FILE ${ZEPHYR_NRF_MODULE_DIR}/samples/matter/light_switch/configuration/${BOARD}/pm_static_dfu.yml PARENT_SCOPE)
endif()
