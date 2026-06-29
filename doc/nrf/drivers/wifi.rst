.. _wifi_drivers:

Wi-Fi drivers
#############

The |NCS| provides Wi-Fi® drivers that implement the IEEE 802.11 protocol and integrate the Nordic Semiconductor Wi-Fi devices with the Zephyr networking stack.
These devices are FullMAC devices that implement the Wi-Fi protocol in the chipset.

The following sections describe the drivers available for each device series.

nRF70 Series
************

The nRF70 Series devices are Wi-Fi companion ICs that add Wi-Fi connectivity to Nordic Semiconductor System-on-Chips (SoCs), such as the nRF53, nRF54L, nRF54H, and nRF91 Series.
The following pages describe the nRF70 Series Wi-Fi driver, its OS-agnostic portable implementation, and the low-level API.

.. toctree::
   :maxdepth: 1

   wifi/nrf70_native
   wifi/nrf70_portable
   wifi/low_level_api

nRF71 Series
************

The nRF71 Series is the next generation of Nordic Semiconductor Wi-Fi devices.
Unlike the nRF70 Series companion ICs, it is a standalone, highly integrated, low-power Wi-Fi 6 and Bluetooth® Low Energy combo System-on-Chip (SoC) with an integrated application processor, and therefore does not require a separate host SoC.
The following page describes the nRF71 Series driver and firmware interface.

.. toctree::
   :maxdepth: 1

   wifi/nrf71_fw_if
