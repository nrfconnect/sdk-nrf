.. _snippet-wifi-p2p:

Wi-Fi Direct snippet (wifi-p2p)
###############################

.. contents::
   :local:
   :depth: 2

The ``wifi-p2p`` snippet enables Wi-Fi Direct®, also known as Wi-Fi peer-to-peer (P2P), which lets two devices connect to each other without an access point.

Overview
********

The snippet enables the P2P mode and the Software Access Point (SAP) mode in the driver (:kconfig:option:`CONFIG_NRF70_P2P_MODE` and :kconfig:option:`CONFIG_NRF70_AP_MODE` on the nRF70 Series, :kconfig:option:`CONFIG_NRF71_P2P_MODE` and :kconfig:option:`CONFIG_NRF71_AP_MODE` on the nRF71 Series), and the P2P support in the WPA supplicant (:kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT_P2P`).

On the nRF70 Series, the snippet enables link time optimization (:kconfig:option:`CONFIG_LTO`) and local ISR table declaration (:kconfig:option:`CONFIG_ISR_TABLES_LOCAL_DECLARATION`) to reduce the code size.
On the nRF71 Series, it increases the number of receive buffers (:kconfig:option:`CONFIG_NRF71_RX_NUM_BUFS`) and the driver data heap size (:kconfig:option:`CONFIG_NRF_WIFI_DATA_HEAP_SIZE`).

Supported board targets
***********************

The snippet applies the nRF70 Series configuration to the following board targets:

* ``nrf7002dk/nrf5340/cpuapp``, including the ``nrf7001`` and ``ns`` variants
* ``nrf5340dk/nrf5340/cpuapp``
* ``nrf54lm20dk/nrf54lm20a/cpuapp`` and ``nrf54lm20dk/nrf54lm20b/cpuapp``

The snippet applies the nRF71 Series configuration to the following board targets:

* ``nrf7120dk/nrf7120/cpuapp``
* ``nrf7120dk/nrf7120/cpuapp/ns``

Usage
*****

To build an application with Wi-Fi Direct support, use the :ref:`wifi_shell_sample` sample with the ``wifi-p2p`` snippet and external flash for firmware patches:

.. code-block:: console

   west build --pristine --board nrf7002dk/nrf5340/cpuapp -S wifi-p2p -S nrf70-fw-patch-ext-flash

See also
********

* :ref:`ug_wifi_direct` for the Wi-Fi Direct feature description and testing instructions.
* :ref:`snippet-nrf70-fw-patch-ext-flash` for storing the firmware patches in external memory.
