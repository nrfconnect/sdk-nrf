#
#  Copyright (c) 2019 Nordic Semiconductor ASA
#
#  SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_sources(bh1749.c)
zephyr_sources_ifdef(CONFIG_BH1749_LOCAL_TRIGGER bh1749_trigger.c)
