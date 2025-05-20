#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_subdirectory_ifdef(CONFIG_MOCK_NRFX nrfx)
add_subdirectory_ifdef(CONFIG_MOCK_SDFW sdfw)
