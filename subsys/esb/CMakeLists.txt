#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

if(CONFIG_ESB)
  zephyr_library_sources(esb.c)
  zephyr_library_sources_ifdef(CONFIG_HAS_HW_NRF_PPI esb_ppi.c)
  zephyr_library_sources_ifndef(CONFIG_HAS_HW_NRF_PPI esb_dppi.c)
endif()

if(CONFIG_ESB_LITE)
  zephyr_library_sources(lite/esb_common.c)
  zephyr_library_sources(lite/esb_queues.c)
  zephyr_library_sources(lite/esb_graph.c)
  zephyr_library_sources_ifdef(CONFIG_ESB_PTX lite/esb_ptx.c)
  zephyr_library_sources_ifdef(CONFIG_ESB_PRX lite/esb_prx.c)
endif()
