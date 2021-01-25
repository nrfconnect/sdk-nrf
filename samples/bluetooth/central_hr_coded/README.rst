.. _bluetooth_central_hr_coded:

Bluetooth: Central Heart Rate Monitor with Coded PHY
####################################################

.. contents::
   :local:
   :depth: 2

The Central Heart Rate Monitor with Coded PHY offers similar functionality to the :ref:`zephyr:bluetooth_central_hr` sample from Zephyr.
However, this sample specifically looks for heart rate monitors using LE Coded PHY.

Overview
********

The sample demonstrates a Bluetooth LE Central role functionality by scanning for other Bluetooth LE devices that run a Heart Rate Server with LE Coded PHY support.
It then establishes a connection to the first Peripheral device in range.
It can be used together with the :ref:`peripheral_hr_coded` sample.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp_and_cpuappns, nrf52840dongle_nrf52840, nrf52840dk_nrf52840

.. include:: /includes/hci_rpmsg_overlay.txt

The sample also requires a device running a Heart Rate Server with LE Coded PHY support to connect to.
For example, another development kit running the :ref:`peripheral_hr_coded` sample.

Building and Running
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
      [NOTIFICATION] Heart Rate 113 bpm
      [NOTIFICATION] Heart Rate 114 bpm
      [NOTIFICATION] Heart Rate 115 bpm

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`gatt_dm_readme`
* :ref:`nrf_bt_scan_readme`

In addition, it uses the following Zephyr libraries:

* ``include/zephyr/types.h``
* ``include/errno.h``
* ``include/zephyr.h``
* ``include/sys/printk.h``
* ``include/sys/byteorder.h``
* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``

* :ref:`zephyr:bluetooth_api`:

* ``include/bluetooth/bluetooth.h``
* ``include/bluetooth/conn.h``
* ``include/bluetooth/gatt.h``
* ``include/bluetooth/uuid.h``
