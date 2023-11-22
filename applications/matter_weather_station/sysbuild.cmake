#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if("${OVERLAY_CONFIG}" STREQUAL "overlay-factory_data.conf")
    set(PM_STATIC_YML_FILE ${APPLICATION_CONFIG_DIR}/pm_static_factory_data.yml PARENT_SCOPE)
endif()
