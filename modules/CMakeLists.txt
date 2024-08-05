#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_subdirectory_ifdef(CONFIG_BUILD_WITH_TFM trusted-firmware-m)
add_subdirectory_ifdef(CONFIG_MEMFAULT memfault-firmware-sdk)
add_subdirectory_ifdef(CONFIG_MCUBOOT mcuboot/hooks)
add_subdirectory_ifdef(CONFIG_COREMARK coremark)
add_subdirectory(wfa-qt)
