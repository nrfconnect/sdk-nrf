#
#  Copyright (c) 2019 Nordic Semiconductor ASA
#
#  SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(bh1749.c)
zephyr_library_sources_ifdef(CONFIG_BH1749_TRIGGER bh1749_trigger.c)
