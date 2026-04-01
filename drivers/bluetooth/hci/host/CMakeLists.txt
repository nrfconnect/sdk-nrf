# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_BT_H4          h4.c)
zephyr_library_sources_ifdef(CONFIG_BT_HCI_IPC     ipc.c)
zephyr_library_sources_ifdef(CONFIG_BT_SPI_ZEPHYR  spi.c)
zephyr_library_sources_ifdef(CONFIG_SOC_NRF5340_CPUAPP nrf53_support.c)

if(CONFIG_BT_USERCHAN)
    zephyr_library_sources(userchan.c)
    target_sources(native_simulator INTERFACE userchan_bottom.c)
endif()
