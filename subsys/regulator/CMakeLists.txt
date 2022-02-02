#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_REGULATOR_PI_REG pi_reg.c)
zephyr_library_sources_ifdef(CONFIG_REGULATOR_PI_REG_BT_MESH pi_reg_bt_mesh.c)
