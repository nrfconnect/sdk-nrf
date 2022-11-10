.. _wifi_mgmt_ext:

Wi-Fi management extension
##########################

.. contents::
   :local:
   :depth: 2

The Wi-Fi management extension library adds an automatic connection feature to the Wi-Fi stack.

Overview
********

The automatic connection feature is implemented by adding a ``NET_REQUEST_WIFI_CONNECT_STORED`` command to Zephyr's Wi-Fi L2 Layer.
Credentials are pulled from the :ref:`lib_wifi_credentials` library.

Configuration
*************

To use this library, enable the :kconfig:option:`CONFIG_WIFI_MGMT_EXT` Kconfig option.

For development purposes, you can set static Wi-Fi credentials configuration using the following Kconfig options:

* :kconfig:option:`CONFIG_WIFI_CREDENTIALS_STATIC_SSID`
* :kconfig:option:`CONFIG_WIFI_CREDENTIALS_STATIC_PASSWORD`
* :kconfig:option:`CONFIG_WIFI_CREDENTIALS_STATIC_TYPE`

Usage
*****

The following code snippet shows how to use the :ref:`zephyr:net_mgmt_interface` command:

.. code-block:: c

   int ret;

   struct net_if *iface = net_if_get_default();
   int rc = net_mgmt(NET_REQUEST_WIFI_CONNECT_STORED, iface, NULL, 0);

   if (rc) {
      printk("an error occurred when trying to auto-connect to a network. err: %d", rc);
   }


Limitations
***********

The library has the following limitations:

* It can only be used with Nordic's ``hostap``-based Wi-Fi stack.
  The Wi-Fi configuration is highly vendor-specific.
* The commands ``NET_REQUEST_WIFI_CONNECT`` and ``NET_REQUEST_WIFI_CONNECT_STORED`` clear the list of configured Wi-Fi networks in RAM.
  Automatic connection has to be requested again after directly requesting connection to a specific network.

API documentation
*****************

| Header file: :file:`include/net/wifi_mgmt_ext.h`
| Source files: :file:`subsys/net/lib/wifi_mgmt_ext`

.. doxygengroup:: wifi_mgmt_ext
   :project: nrf
   :members:
