.. _nrf71_wifi_fw_if:

nRF71 Series Wi-Fi driver
#########################

.. contents::
   :local:
   :depth: 2

The nRF71 Series Wi-Fi® driver runs on the host processor (the Arm® Cortex®-M33 application core) and communicates with the firmware running on the :abbr:`RPU (Radio Processing Unit)` by exchanging commands and events over the host interface.
This firmware interface defines the message headers, commands, events, and control and RF parameters used for host-to-RPU communication.

The interface is split across the following header files:

* :file:`drivers/wifi/nrf71/fw_if/nrf71_wifi_common.h` - Common message headers, system-level commands, and events.
* :file:`drivers/wifi/nrf71/fw_if/nrf71_wifi_ctrl.h` - Control path commands and events (UMAC interface).
* :file:`drivers/wifi/nrf71/fw_if/nrf71_wifi_rf.h` - RF and baseband control parameters.
* :file:`drivers/wifi/nrf71/fw_if/nrf71_wifi_debug_stats.h` - Debug and statistics interface.

API documentation
*****************

.. doxygengroup:: nrf71_wifi_fw_if
   :project: nrf
