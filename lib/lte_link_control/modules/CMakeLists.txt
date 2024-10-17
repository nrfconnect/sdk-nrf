#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_sources(cereg.c)
zephyr_library_sources(cfun.c)
zephyr_library_sources(cscon.c)
zephyr_library_sources(mdmev.c)
zephyr_library_sources(redmob.c)
zephyr_library_sources(xfactoryreset.c)
zephyr_library_sources(xsystemmode.c)
zephyr_library_sources_ifdef(CONFIG_LTE_LC_CONN_EVAL coneval.c)
zephyr_library_sources_ifdef(CONFIG_LTE_LC_EDRX edrx.c)
zephyr_library_sources_ifdef(CONFIG_LTE_LC_NEIGHBOR_CELL_MEAS ncellmeas.c)
zephyr_library_sources_ifdef(CONFIG_LTE_LC_PSM psm.c)
zephyr_library_sources_ifdef(CONFIG_LTE_LC_RAI rai.c)
zephyr_library_sources_ifdef(CONFIG_LTE_LC_PERIODIC_SEARCH periodicsearchconf.c)
zephyr_library_sources_ifdef(CONFIG_LTE_LC_MODEM_SLEEP xmodemsleep.c)
zephyr_library_sources_ifdef(CONFIG_LTE_LC_TAU_PRE_WARNING xt3412.c)
