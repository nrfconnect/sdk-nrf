.. _snippet-wifi-p2p:

Wi-Fi P2P Snippet (wifi-p2p)
##########################################

.. code-block:: console

   west build  -S wifi-p2p [...]

Overview
********

This snippet enables Wi-Fi P2P support in supported networking samples.

Requirements
************

Hardware support for:

- :kconfig:option:`CONFIG_WIFI`
- :kconfig:option:`CONFIG_WIFI_USE_NATIVE_NETWORKING`
- :kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT`
