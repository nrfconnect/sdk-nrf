.. _wifi_provision:

Wi-Fi Provision
###############

.. contents::
   :local:
   :depth: 2

.. |wifi| replace:: Wi-Fi

.. include:: /includes/net_connection_manager.txt

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

Configuration
*************

|config|

Configuration options
=====================

.. include:: /includes/wifi_credentials_shell.txt

.. include:: /includes/wifi_credentials_static.txt

Configuration files
===================

The sample includes pre-configured configuration files for the development kits that are supported:

* :file:`prj.conf` - For all devices.
* :file:`boards/nrf7002dk_nrf5340_cpuapp_ns.conf` - For the nRF7002 DK.

Files that are located under the :file:`/boards` folder are automatically merged with the :file:`prj.conf` file when you build for the corresponding target.

Building and running
********************

.. include:: /includes/build_and_run_ns.txt

Testing
=======

|test_sample|

Sample output
=============

Troubleshooting
***************

Dependencies
************

This sample uses the following Zephyr libraries:

* :ref:`net_if_interface`
* :ref:`net_mgmt_interface`
* :ref:`Connection Manager <zephyr:conn_mgr_overview>`
