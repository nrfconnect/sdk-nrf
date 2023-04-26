.. _bluetooth_central_hr_coded:

Bluetooth: Central Heart Rate Monitor with Coded PHY
####################################################

.. contents::
   :local:
   :depth: 2

The Central Heart Rate Monitor with Coded PHY offers similar functionality to the :ref:`zephyr:bluetooth_central_hr` sample from Zephyr.
However, this sample specifically looks for heart rate monitors using LE Coded PHY.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/hci_rpmsg_overlay.txt

The sample also requires a device running a Heart Rate Server with LE Coded PHY support to connect to.
For example, another development kit running the :ref:`peripheral_hr_coded` sample.

Overview
********

The sample demonstrates a Bluetooth® LE Central role functionality by scanning for other Bluetooth LE devices that run a Heart Rate Server with LE Coded PHY support, which is not available in Zephyr Bluetooth LE Controller.
See :ref:`ug_ble_controller` for more information.
It then establishes a connection to the first Peripheral device in range.
You can use it together with the :ref:`peripheral_hr_coded` sample.

Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/central_hr_coded`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, you can test it by connecting to another development kit that runs the :ref:`peripheral_hr_coded`.

1. |connect_terminal_specific|
#. Reset the kit.
#. Program the other kit with the :ref:`peripheral_hr_coded` sample.
#. Wait until the Coded advertiser is detected by the Central.
   In the terminal window, check for information similar to the following::

      Filters matched. Address: xx.xx.xx.xx.xx.xx (random) connectable: yes
      Connection pending
      Connected: xx.xx.xx.xx.xx.xx (random), tx_phy 4, rx_phy 4
      The discovery procedure succeeded

#. Observe that the received notifications are output in the terminal window::

      [SUBSCRIBED]
      Heart Rate Measurement notification received:

        Heart Rate Measurement Value Format: 8 - bit
        Sensor Contact detected: 1
        Sensor Contact supported: 1
        Energy Expended present: 0
        RR-Intervals present: 0

        Heart Rate Measurement Value: 113 bpm

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`gatt_dm_readme`
* :ref:`nrf_bt_scan_readme`
* :ref:`lib_hrs_client_readme`

In addition, it uses the following Zephyr libraries:

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
* :file:`include/bluetooth/gatt.h`
* :file:`include/bluetooth/uuid.h`
