.. _lib_wifi_ready:

Wi-Fi ready
###########

.. contents::
   :local:
   :depth: 2

The Wi-Fi ready library manages Wi-Fi® readiness for applications by handling supplicant ready and not ready events.

Overview
********

The Wi-Fi ready library informs applications of Wi-Fi readiness by managing supplicant events, indicating when Wi-Fi is available for use.

Configuration
*************

To use this library, enable the :kconfig:option:`CONFIG_WIFI_READY_EVENT_HANDLING` Kconfig option.

API documentation
*****************

| Header file: :file:`include/net/wifi_ready.h`
| Source files: :file:`subsys/net/lib/wifi_ready`

.. doxygengroup:: wifi_ready
   :project: nrf
   :members:
