#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Add a dependency so that the remote image will be built and flashed first
add_dependencies(${DEFAULT_IMAGE} remote)
# Add dependency so that the remote image is flashed first.
sysbuild_add_dependencies(FLASH ${DEFAULT_IMAGE} remote)
