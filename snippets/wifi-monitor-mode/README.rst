.. _snippet-wifi-monitor-mode:

Wi-Fi Monitor mode snippet (wifi-monitor-mode)
##############################################

.. contents::
   :local:
   :depth: 2

The ``wifi-monitor-mode`` snippet configures the Wi-Fi® driver to operate in Monitor mode, in which the device receives raw 802.11 frames instead of acting as a station.

Overview
********

The snippet enables raw data reception (:kconfig:option:`CONFIG_NRF70_RAW_DATA_RX` on the nRF70 Series and :kconfig:option:`CONFIG_NRF71_RAW_DATA_RX` on the nRF71 Series) and packet sockets (:kconfig:option:`CONFIG_NET_SOCKETS_PACKET`), which the application uses to read the received frames.
Because Monitor mode does not use a Wi-Fi connection, the snippet also disables the WPA supplicant (:kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT`), the station mode, and the connection manager (:kconfig:option:`CONFIG_NET_CONNECTION_MANAGER`).

Supported board targets
***********************

The snippet applies the nRF70 Series configuration to the following board targets:

* ``nrf7002dk/nrf5340/cpuapp``, including the ``nrf7001`` and ``ns`` variants
* ``nrf5340dk/nrf5340/cpuapp``
* ``nrf54l15dk/nrf54l15/cpuapp``
* ``nrf54lm20dk/nrf54lm20a/cpuapp`` and ``nrf54lm20dk/nrf54lm20b/cpuapp``

The snippet applies the nRF71 Series configuration to the following board targets:

* ``nrf7120dk/nrf7120/cpuapp``
* ``nrf7120dk/nrf7120/cpuapp/ns``

Usage
*****

Apply the snippet when building, for example:

.. code-block:: console

   west build -S wifi-monitor-mode [...]
