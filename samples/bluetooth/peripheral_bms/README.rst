.. _peripheral_bms:

Bluetooth: Peripheral Bond Management Service (BMS)
###################################################

.. contents::
   :local:
   :depth: 2

The peripheral BMS sample demonstrates how to use the :ref:`bms_readme`.

Overview
********

When connected, the sample waits for Client's requests to perform any bond deleting operation.

It supports up to two simultaneous Client connections.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp_and_cpuappns, nrf52840dk_nrf52840, nrf52dk_nrf52832, nrf52dk_nrf52810

The sample also requires a Bluetooth Low Energy dongle and nRF Connect for Desktop.

User interface
**************

LED 1:
   Blinks with a period of 2 seconds with the duty cycle set to 50% when the main loop is running and the device is advertising.

LED 2:
   On when connected.

Building and Running
********************
.. |sample path| replace:: :file:`samples/bluetooth/peripheral_bms`

.. include:: /includes/build_and_run.txt

.. _peripheral_bms_testing:

Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. |connect_terminal_specific|
#. Reset the kit.
#. Start `nRF Connect for Desktop`_ and select the connected device that is used for communication.
#. Connect to the device from nRF Connect.
   The device is advertising as "Nordic_BMS".
#. Bond with the device:

   a. Click the :guilabel:`Settings` button for the device in nRF Connect.
   b. Select :guilabel:`Pair`.
   c. Check :guilabel:`Perform Bonding`.
   d. Click :guilabel:`Pair`.

#. Check the logs to verify that the connection security is updated.
#. Disconnect the device in nRF Connect.
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
   ``06`` is the command to delete all bonds on the Server, followed by the authorization code "ABCD".
#. Disconnect the device to trigger the bond deletion procedures.
#. Reconnect the devices again and verify that the connection security is not updated.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`bms_readme`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* ``include/zephyr/types.h``
* ``lib/libc/minimal/include/errno.h``
* ``include/sys/printk.h``
* :ref:`GPIO Interface <zephyr:api_peripherals>`
* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/conn.h``
  * ``include/bluetooth/uuid.h``
  * ``include/bluetooth/gatt.h``
