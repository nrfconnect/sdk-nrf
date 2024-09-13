#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_amend()

if(CONFIG_GPIO_NRFE)
  if(CONFIG_GPIO_NRFE_EGPIO_BACKEND_ICMSG OR CONFIG_GPIO_NRFE_EGPIO_BACKEND_ICBMSG)
    zephyr_library_sources(gpio_nrfe_icmsg.c)
  elseif(CONFIG_GPIO_NRFE_EGPIO_BACKEND_MBOX)
    zephyr_library_sources(gpio_nrfe_mbox.c)
  endif()

  zephyr_library_sources(gpio_nrfe.c)
endif()
