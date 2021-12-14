.. _multiple_adv_sets:

Bluetooth: Multiple advertising sets
####################################

.. contents::
   :local:
   :depth: 2

The Multiple advertising sets sample demonstrates how to use the Bluetooth® advertising sets.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp_and_cpuapp_ns, nrf52840dk_nrf52840, nrf52dk_nrf52832, nrf52833dk_nrf52833, nrf52833dk_nrf52820, nrf21540dk_nrf52840

Overview
********

The sample implements two advertising sets:

  * Non-connectable advertising with the URI to the https://www.nordicsemi.com website in the advertising data.
    It shows the Bluetooth Broadcaster role (*Beacon*) functionality.
  * Connectable advertising with the 16-bit UUID of the Device Information GATT Service in the advertising data.
    It shows the Bluetooth Peripheral role functionality.

When you start the sample, it creates two advertising sets.
Use your scanner device to observe the two advertisers:

* ``Nordic Beacon`` that you can use to open the nRF Beacon website.
* ``Nordic multi adv sets`` that you can use to establish a connection with the device.

Use your central device to establish the connection with this sample.
When connected, the non-connectable advertiser is still presented on your scanner device.
After disconnection, the connectable advertising starts again.

User interface
**************

LED 1:
   Blinks when the main loop is running (that is, the device is advertising) with a period of two seconds, duty cycle 50%.

LED 2:
   On when the development kit is connected.

Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/multiple_adv_sets`

.. include:: /includes/build_and_run.txt

.. _multiple_adv_sets_testing:

Testing
=======

After programming the sample to your dongle or development kit, test it by performing the following steps:

1. |connect_terminal_specific|
#. Reset the kit.
#. Start the `nRF Connect for Mobile`_ application on your smartphone or tablet.

   The device is advertising as ``Nordic multi adv sets`` and ``Nordic Beacon``.

#. Select the connectable variant and connect to the device from the application.
#. On the :guilabel:`scanner` tab check if ``Nordic Beacon`` still advertises.

   You can use it to open the Nordic website.

#. Disconnect from the device.
#. In the :guilabel:`scanner` tab, check again if there is an advertiser with the name ``Nordic multi adv sets``.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* ``include/kernel.h``

* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/conn.h``
  * ``include/bluetooth/uuid.h``
  * ``include/bluetooth/services/dis.h``
