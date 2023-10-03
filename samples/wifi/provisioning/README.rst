.. _wifi_provisioning:

Wi-Fi: Provisioning Service
###########################

.. contents::
   :local:
   :depth: 2

The Provisioning Service sample demonstrates how to provision a Wi-Fi® device over a Bluetooth® Low Energy link.

.. _wifi_provisioning_app:

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

The sample requires a smartphone (configurator) with Nordic Semiconductor's nRF Wi-Fi Provisioner app installed in one of the following versions:

* `nRF Wi-Fi Provisioner mobile app for Android`_

   * `Source code for nRF Wi-Fi Provisioner mobile app for Android`_

* `nRF Wi-Fi Provisioner mobile app for iOS`_

   * `Source code for nRF Wi-Fi Provisioner mobile app for iOS`_

Overview
********

With this sample, you can provision a Wi-Fi device that lacks input or output capability, using the :ref:`wifi_prov_readme` library.
The sample is divided into three parts:

* Task and event handling component: Handles provisioning-related tasks and events.
* Transport layer: Based on Bluetooth Low Energy and exchanges information between the device and the nRF Wi-Fi Provisioner app.
* Configuration management component: Manages the provisioning data (in RAM and flash) accessed by multiple threads.

Configuration
*************

|config|

Configuration options
=====================

The following sample-specific Kconfig options are used in this sample (located in :file:`samples/wifi/provisioning/Kconfig`):

.. _CONFIG_WIFI_PROV_ADV_DATA_UPDATE:

CONFIG_WIFI_PROV_ADV_DATA_UPDATE
   This option enables periodic updates of advertisement data.

.. _CONFIG_WIFI_PROV_ADV_DATA_UPDATE_INTERVAL:

CONFIG_WIFI_PROV_ADV_DATA_UPDATE_INTERVAL
   This option specifies the update interval of the advertisement data.

To get live update of the Wi-Fi status without setting up a Bluetooth connection, enable the :ref:`CONFIG_WIFI_PROV_ADV_DATA_UPDATE <CONFIG_WIFI_PROV_ADV_DATA_UPDATE>` Kconfig option.
Set the :ref:`CONFIG_WIFI_PROV_ADV_DATA_UPDATE_INTERVAL <CONFIG_WIFI_PROV_ADV_DATA_UPDATE_INTERVAL>` Kconfig option, to control the update interval.

Features
********

The sample implements advertising over Bluetooth LE.

After powerup, the device checks whether it is provisioned. If it is, it loads saved credentials and tries to connect to the specified access point.

The device advertises over Bluetooth LE.

The advertising data contains several fields to facilitate provisioning.
It contains the UUID of the Wi-Fi Provisioning Service and four bytes of service data, as described in the following table.

+----------+----------+----------+----------+
|  Byte 1  |  Byte 2  |  Byte 3  |  Byte 4  |
+==========+==========+==========+==========+
| Version  | Flag(LSB)| Flag(MSB)|   RSSI   |
+----------+----------+----------+----------+

* Version: 8-bit unsigned integer. The value is the version of the Wi-Fi Provisioning Service sample.

* Flag: 16-bit little endian field. Byte 2 (first byte on air) is LSB, and Byte 3 (second byte on air) is MSB.

  * Bit 0: Provisioning status. The value is ``1`` if the device is provisioned, otherwise it is ``0``.

  * Bit 1: Connection status. The value is ``1`` if Wi-Fi is connected, otherwise it is ``0``.

  * Bit 2 - 15: RFU

* RSSI: 8-bit signed integer. The value is the signal strength. It is valid only if the Wi-Fi is connected.

The advertising interval depends on the provisioning status.
If the device is not provisioned, the interval is 100 ms. If it is provisioned, the interval is one second.

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/provisioning`

.. include:: /includes/build_and_run.txt

The sample generates header and source files based on protocol buffer definitions in :file:`.proto` files during the build process.
You must install a protocol buffer compiler to generate the files.
See the :ref:`zephyr:nanopb_sample` in the Zephyr documentation for more information.

Testing
=======

|test_sample|

1. |connect_kit|
#. |connect_terminal|
#. Observe the following output:

   .. code-block:: console

      wifi_prov: BT Advertising successfully started

#. Open the nRF Wi-Fi Provisioner app.
#. Enable Bluetooth if it is disabled.
#. Search and connect to the device you want to provision.
   If the connection and pairing are established, observe the following output:

   .. code-block:: console

      wifi_prov: BT Connected: <configurator BT address>
      wifi_prov: BT pairing completed: <configurator BT address>, bonded: 0

#. Start an access point scan.
#. Enter the password, if required.
#. Establish a Wi-Fi connection.
#. Observe the following output, if the connection is successful:

   .. code-block:: console

      wpa_supp: wlan0: CTRL-EVENT-CONNECTED - Connection to <AP MAC Address> completed [id=0 id_str=]

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`wifi_prov_readme`
* :ref:`lib_wifi_credentials`

This sample also uses a module that can be found in the following location in the |NCS| folder structure:

* :file:`modules/lib/nanopb`

In addition, it uses the following Zephyr libraries:

* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/conn.h`
  * :file:`include/bluetooth/uuid.h`
  * :file:`include/bluetooth/gatt.h`
  * :file:`include/net/wifi.h`
  * :file:`include/net/wifi_mgmt.h`
