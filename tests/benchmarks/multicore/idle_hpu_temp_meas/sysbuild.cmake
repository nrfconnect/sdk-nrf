#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Add a dependency so that the remote image will be built and flashed first
add_dependencies(idle_hpu_temp_meas remote_sleep_forever)
# Add dependency so that the remote image is flashed first.
sysbuild_add_dependencies(FLASH idle_hpu_temp_meas remote_sleep_forever)
