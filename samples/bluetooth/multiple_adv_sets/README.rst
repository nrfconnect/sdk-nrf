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

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

For testing this sample, you need to have the `nRF Connect for Mobile`_ app installed on your smartphone or tablet.

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
When connected, the non-connectable advertiser is still shown on your scanner device.
After disconnection, the connectable advertising starts again.

User interface
**************

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      LED 1:
         Blinks with a period of two seconds with the duty cycle set to 50% when the main loop is running and the device is advertising.

      LED 2:
         Lit when the development kit is connected.

   .. group-tab:: nRF54 DKs

      LED 0:
         Blinks with a period of two seconds with the duty cycle set to 50% when the main loop is running and the device is advertising.

      LED 1:
         Lit when the development kit is connected.

Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/multiple_adv_sets`

.. include:: /includes/build_and_run_ns.txt

.. |sample_or_app| replace:: sample
.. |ipc_radio_dir| replace:: :file:`sysbuild/ipc_radio`

.. include:: /includes/ipc_radio_conf.txt

.. _multiple_adv_sets_testing:

Testing
=======

After programming the sample to your dongle or development kit, test it by performing the following steps:

1. |connect_terminal_specific|
#. Reset the kit.
#. Start the `nRF Connect for Mobile`_ application on your smartphone or tablet.

   The device is advertising as ``Nordic multi adv sets`` and ``Nordic Beacon``.

#. Select the connectable variant and connect to the device from the application.
#. In the **SCANNER** tab check if ``Nordic Beacon`` is still advertising.

   You can use it to open the Nordic website.

#. Disconnect from the device.
#. In the **SCANNER** tab, check again if there is an advertiser with the name ``Nordic multi adv sets``.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* :file:`include/kernel.h`

* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/conn.h`
  * :file:`include/bluetooth/uuid.h`
  * :file:`include/bluetooth/services/dis.h`

The sample also uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
