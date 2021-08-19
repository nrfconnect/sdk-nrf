#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_include_directories(app PRIVATE .)
target_include_directories(app PRIVATE certs)
target_sources(app PRIVATE carrier_certs.c)
