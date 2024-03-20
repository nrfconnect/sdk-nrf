.. _softap_wifi_provision_sample:

Wi-Fi: SoftAP based provision
#############################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to use the :ref:`lib_softap_wifi_provision` library to provision credentials to Nordic Semiconductor's Wi-Fi® device.

.. |wifi| replace:: Wi-Fi

.. include:: /includes/net_connection_manager.txt

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Configuration
*************

|config|

Configuration files
===================

The sample includes the following pre-configured configuration file for the development kits that are supported:

* :file:`prj.conf` - Configuration file for all build targets.

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/provisioning/softap`

.. include:: /includes/build_and_run_ns.txt

Dependencies
************

This sample uses the following |NCS| and Zephyr libraries:

* :ref:`lib_softap_wifi_provision`
* :ref:`net_mgmt_interface`
* :ref:`Connection Manager <zephyr:conn_mgr_overview>`
