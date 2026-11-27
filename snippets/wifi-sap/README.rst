.. _snippet-wifi-sap:

Wi-Fi Software Access Point snippet (wifi-sap)
##############################################

.. contents::
   :local:
   :depth: 2

The ``wifi-sap`` snippet configures the device to act as a Wi-Fi® Software Access Point (SAP), so that Wi-Fi stations can connect to it directly.

Overview
********

The snippet applies the following configuration:

* Enables the SAP mode in the driver (:kconfig:option:`CONFIG_NRF70_AP_MODE` on the nRF70 Series and :kconfig:option:`CONFIG_NRF71_AP_MODE` on the nRF71 Series) and in the WPA supplicant (:kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT_AP`).
* Enables the zperf tool and the zperf server (:kconfig:option:`CONFIG_NET_ZPERF` and :kconfig:option:`CONFIG_NET_ZPERF_SERVER`) so that you can measure the throughput of the connected stations.
* Assigns the static IPv4 address ``192.168.1.1`` to the access point and enables the DHCPv4 server (:kconfig:option:`CONFIG_NET_DHCPV4_SERVER`) so that it can assign addresses to the connected stations.
* Tunes the packet and buffer counts and the driver control and data heap sizes for the access point traffic.
* Disables the Wi-Fi credentials storage (:kconfig:option:`CONFIG_WIFI_CREDENTIALS`), the flash driver, NVS, and the settings subsystem, which an access point build does not need.

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

Apply the snippet when building, for example for the :ref:`wifi_shell_sample` sample:

.. code-block:: console

   west build -p -b <board target> -S wifi-sap
