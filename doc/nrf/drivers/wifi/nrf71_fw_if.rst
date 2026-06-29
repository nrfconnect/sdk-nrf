.. _nrf71_wifi_fw_if:

nRF71 Series firmware interface
###############################

.. contents::
   :local:
   :depth: 2

The nRF71 Series Wi-Fi® driver communicates with the firmware running on the Radio Processing Unit (RPU) by exchanging commands and events over the host interface.
This firmware interface defines the message headers, commands, events, and the control and RF parameters used for this host-to-RPU communication.

The interface is split across the following header files:

* :file:`drivers/wifi/nrf71/fw_if/nrf71_wifi_common.h` - Common message headers and system-level commands and events.
* :file:`drivers/wifi/nrf71/fw_if/nrf71_wifi_ctrl.h` - Control path commands and events (UMAC interface).
* :file:`drivers/wifi/nrf71/fw_if/nrf71_wifi_rf.h` - RF and baseband control parameters.
* :file:`drivers/wifi/nrf71/fw_if/nrf71_wifi_debug_stats.h` - Debug and statistics interface.

API documentation
*****************

.. doxygengroup:: nrf71_wifi_fw_if
   :project: nrf
