#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(sms.c)
zephyr_library_sources(sms_deliver.c)
zephyr_library_sources(sms_submit.c)
zephyr_library_sources(parser.c)
zephyr_library_sources(string_conversion.c)
