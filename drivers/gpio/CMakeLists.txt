#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_amend()

if(CONFIG_GPIO_HPF)
  if(CONFIG_GPIO_HPF_GPIO_BACKEND_ICMSG OR CONFIG_GPIO_HPF_GPIO_BACKEND_ICBMSG)
    zephyr_library_sources(gpio_hpf_icmsg.c)
  elseif(CONFIG_GPIO_HPF_GPIO_BACKEND_MBOX)
    zephyr_library_sources(gpio_hpf_mbox.c)
  endif()

  zephyr_library_sources(gpio_hpf.c)
endif()
