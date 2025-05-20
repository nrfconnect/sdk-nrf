.. _peripheral_bms:

Bluetooth: Peripheral Bond Management Service (BMS)
###################################################

.. contents::
   :local:
   :depth: 2

The peripheral BMS sample demonstrates how to use the :ref:`bms_readme`.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

The sample also requires a Bluetooth® Low Energy dongle and the `Bluetooth Low Energy app`_.

Overview
********

When connected, the sample waits for the client's requests to perform any bond-deleting operation.

It supports up to two simultaneous client connections.

User interface
**************

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      LED 1:
         Blinks, toggling on/off every second, when the main loop is running and the device is advertising.

      LED 2:
         Lit when connected.

   .. group-tab:: nRF54 DKs

      LED 0:
         Blinks, toggling on/off every second, when the main loop is running and the device is advertising.

      LED 1:
         Lit when connected.

Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/peripheral_bms`

.. include:: /includes/build_and_run.txt

.. _peripheral_bms_testing:

Testing
=======

|test_sample|

1. |connect_terminal_specific|
#. Reset the kit.
#. Start `nRF Connect for Desktop`_.
#. Open the `Bluetooth Low Energy app`_ and select the connected device that is used for communication.
#. Connect to the device from the app.
   The device is advertising as ``Nordic_BMS``.
#. Bind with the device:

   a. Click the :guilabel:`Settings` button for the device in the app.
   #. Select :guilabel:`Pair`.
   #. Select :guilabel:`Keyboard and display` in the IO capabilities setting.
   #. Select :guilabel:`Perform Bonding`.
   #. Click :guilabel:`Pair`.

#. Check the logs to verify that the connection security is updated.
#. Disconnect the device in the app.
#. Reconnect again and verify that the connection security is updated automatically.
#. Verify that the Feature Characteristic of the Bond Management Service displays ``10 08 02``.
   This means that the following features are supported:

   * Deletion of the bonds for the current connection of the requesting device.
   * Deletion of all bonds on the Server with the Authorization Code.
   * Deletion of all bonds on the Server except the ones of the requesting device with the Authorization Code.

#. Write ``03`` to the Bond Management Service Control Point Characteristic.
   ``03`` is the command to delete the current bond.
#. Disconnect the device to trigger the bond deletion procedures.
#. Reconnect the devices and verify that the connection security is not updated.
#. Bond both devices again.
#. Write ``06 41 42 43 44`` to the Bond Management Service Control Point Characteristic.
   ``06`` is the command to delete all bonds on the server, followed by the authorization code ``ABCD``.
#. Disconnect the device to trigger the bond deletion procedures.
#. Reconnect the devices again and verify that the connection security is not updated.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`bms_readme`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* :file:`include/zephyr/types.h`
* :file:`lib/libc/minimal/include/errno.h`
* :file:`include/sys/printk.h`
* :ref:`GPIO Interface <zephyr:api_peripherals>`
* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/conn.h`
  * :file:`include/bluetooth/uuid.h`
  * :file:`include/bluetooth/gatt.h`
