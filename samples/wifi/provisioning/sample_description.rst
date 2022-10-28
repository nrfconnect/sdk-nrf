.. _wifi_provisioning_sample_desc:

Sample description
##################

.. contents::
   :local:
   :depth: 2

The Wi-Fi Provisioning Service sample demonstrates how to provision a device with Nordic Semiconductor's Wi-Fi chipsets over Bluetooth® Low Energy.

.. _wifi_provisioning_app:

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

The sample requires a smartphone (configurator) with Nordic Semiconductor's nRF Wi-Fi Provisioner app installed in one of the following versions:

* `nRF Wi-Fi Provisioner mobile app for Android`_
* `nRF Wi-Fi Provisioner mobile app for iOS`_

Overview
********

With this sample, you can provision a Wi-Fi device that lacks input or output capability.
The sample is divided into three parts:

* Task and event handling component: Handles provisioning-related tasks and events.
* Transport layer: Based on Bluetooth Low Energy and exchanges information between the device and the nRF Wi-Fi Provisioner app.
* Configuration management component: Manages the provisioning data (in RAM and flash) accessed by multiple threads.

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/provisioning`

.. include:: /includes/build_and_run.txt

The sample generates header and source files based on protocol buffer definitions in :file:`.proto` files during the build process.
You must install a protocol buffer compiler to generate the files.
See the :ref:`zephyr:nanopb_sample` in the zephyr documentation for more information.

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

This sample uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_security`

This sample also uses a module that can be found in the following location in the |NCS| folder structure:

* :file:`modules/lib/nanopb`

In addition, it uses the following Zephyr libraries:

* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/conn.h`
  * :file:`include/bluetooth/uuid.h`
  * :file:`include/bluetooth/gatt.h`

* :ref:`zephyr:settings_api`:

  * :file:`include/settings/settings.h`
