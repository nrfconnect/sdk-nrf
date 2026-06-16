#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_subdirectory_ifdef(CONFIG_DFU_MULTI_IMAGE dfu_multi_image)
add_subdirectory_ifdef(CONFIG_DFU_TARGET dfu_target)
add_subdirectory_ifdef(CONFIG_FMFU_FDEV fmfu_fdev)
add_subdirectory_ifdef(CONFIG_FW_LOADER_SETTINGS_ADV_NAME fw_loader_settings)
