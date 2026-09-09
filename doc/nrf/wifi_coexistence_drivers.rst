.. _wifi_coexistence_drivers:
.. _wifi_drivers:

Wi-Fi and coexistence drivers
#############################

The |NCS| provides host Wi-Fi® drivers that implement the IEEE 802.11 protocol and integrate the Nordic Semiconductor Wi-Fi devices with the Zephyr networking stack.
These devices are FullMAC devices, which means that the complete IEEE 802.11 MAC layer, including the :abbr:`MLME (MAC Layer Management Entity)`, runs on the device firmware rather than on the host.
The host driver configures the device and transfers frames between the device and the networking stack.

The nRF70 Series devices are Wi-Fi companion ICs that add Wi-Fi connectivity to Nordic Semiconductor System-on-Chips (SoCs), such as the nRF53, nRF54L, nRF54H, and nRF91 Series.

The nRF71 Series is a standalone, highly integrated, low-power Wi-Fi 6 and Bluetooth® Low Energy combo System-on-Chip (SoC).
Unlike the nRF70 Series companion IC, nRF71 Series devices integrate the Wi-Fi radio and the application processor in a single SoC.
The host Wi-Fi driver runs on the nRF71 Series application processor.

The following pages describe the Wi-Fi drivers and the nRF71 short-range coexistence driver.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   drivers/wifi/nrf70_native
   drivers/wifi/nrf70_portable
   drivers/wifi/low_level_api
   drivers/wifi/nrf71_fw_if
   drivers/nrf71_sr_coex
