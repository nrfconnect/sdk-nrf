.. _peripheral_hr_coded:

Bluetooth: Peripheral Heart Rate Monitor with Coded PHY
#######################################################

.. contents::
   :local:
   :depth: 2

The Peripheral Heart Rate Monitor with Coded PHY offers similar functionality to the :zephyr:code-sample:`ble_peripheral_hr` sample from Zephyr.
However, this sample supports LE Coded PHY.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/hci_ipc_overlay.txt

The sample also requires a device running a Heart Rate Server with LE Coded PHY support to connect to.
For example, another development kit running the :ref:`bluetooth_central_hr_coded` sample.

Overview
********

The sample demonstrates a basic Bluetooth® Low Energy Peripheral role functionality that exposes the Heart Rate GATT Service with LE Coded PHY support, which is not available in Zephyr Bluetooth LE Controller (See :ref:`ug_ble_controller` for more information).
Once it connects to a Central device, it generates dummy heart rate values.
You can use it together with the :ref:`bluetooth_central_hr_coded` sample.

User interface
**************

The user interface of the sample depends on the hardware platform you are using.

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      LED 1:
         Blinks, toggling on/off every second, when the main loop is running and the device is advertising.

      LED 2:
         Lit when the development kit is connected.

   .. group-tab:: nRF54 DKs

      LED 0:
         Blinks, toggling on/off every second, when the main loop is running and the device is advertising.

      LED 1:
         Lit when the development kit is connected.

Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/peripheral_hr_coded`

.. include:: /includes/build_and_run.txt


Testing
=======

After programming the sample to your development kit, you can test it by connecting to another development kit that runs the :ref:`bluetooth_central_hr_coded`.

1. |connect_terminal_specific|
#. Reset the kit.
#. Program the other kit with the :ref:`bluetooth_central_hr_coded` sample.
#. Wait until the Coded advertiser is detected by the Central.
   In the terminal window, check for information similar to the following::

      Connected: xx.xx.xx.xx.xx.xx (random), tx_phy 4, rx_phy 4

#. In the terminal window, observe that notifications are enabled::

      <inf> hrs: HRS notifications enabled

Dependencies
************

This sample uses the following |NCS| library:

* :ref:`dk_buttons_and_leds_readme`

This sample uses the following Zephyr libraries:

* :file:`include/zephyr/types.h`
* :file:`include/errno.h`
* :file:`include/zephyr.h`
* :file:`include/sys/printk.h`
* :file:`include/sys/byteorder.h`
* :ref:`zephyr:kernel_api`:

  * :file:`include/kernel.h`

* :ref:`zephyr:bluetooth_api`:

* :file:`include/bluetooth/bluetooth.h`
* :file:`include/bluetooth/conn.h`
* :file:`include/bluetooth/uuid.h`
* :file:`include/bluetooth/gatt.h`
* :file:`include/bluetooth/services/bas.h`
* :file:`include/bluetooth/services/hrs.h`
