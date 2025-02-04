.. _direction_finding_connectionless_rx:

Bluetooth: Direction finding connectionless locator
###################################################

.. contents::
   :local:
   :depth: 2

The direction finding connectionless locator sample application demonstrates Bluetooth® LE direction finding reception.

Requirements
************

.. include:: /samples/bluetooth/direction_finding_central/README.rst
   :start-after: bt_dir_finding_central_req_start
   :end-before: bt_dir_finding_central_req_end

Overview
********

The direction finding connectionless locator sample application uses Constant Tone Extension (CTE), that is received and sampled with periodic advertising PDUs.

.. include:: /samples/bluetooth/direction_finding_central/README.rst
   :start-after: bt_dir_finding_central_ov_start
   :end-before: bt_dir_finding_central_ov_end

Configuration
*************

.. include:: /samples/bluetooth/direction_finding_central/README.rst
   :start-after: bt_dir_finding_central_conf_start
   :end-before: bt_dir_finding_central_conf_end

.. include:: /samples/bluetooth/direction_finding_central/README.rst
   :start-after: bt_dir_finding_central_5340_conf_start
   :end-before: bt_dir_finding_central_5340_conf_end

.. include:: /samples/bluetooth/direction_finding_central/README.rst
   :start-after: bt_dir_finding_central_aod_start
   :end-before: bt_dir_finding_central_aod_end

.. include:: /samples/bluetooth/direction_finding_central/README.rst
   :start-after: bt_dir_finding_central_ant_aoa_start
   :end-before: bt_dir_finding_central_ant_aoa_end

To successfully use the direction finding locator when the AoA mode is enabled, provide the following data related to antenna matrix design:

.. include:: /samples/bluetooth/direction_finding_central/README.rst
   :start-after: bt_dir_finding_central_conf_list_start
   :end-before: bt_dir_finding_central_conf_list_end

.. include:: /samples/bluetooth/direction_finding_central/README.rst
   :start-after: bt_dir_finding_central_ant_pat_start
   :end-before: bt_dir_finding_central_ant_pat_end

.. include:: /samples/bluetooth/direction_finding_central/README.rst
   :start-after: bt_dir_finding_central_cte_start
   :end-before: bt_dir_finding_central_cte_end

Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/direction_finding_connectionless_rx`

.. include:: /includes/build_and_run.txt

.. |sample_or_app| replace:: sample
.. |ipc_radio_dir| replace:: :file:`sysbuild/ipc_radio`

.. include:: /includes/ipc_radio_conf.txt

Testing
=======

|test_sample|

1. |connect_terminal_specific|
#. In the terminal window, check for information similar to the following::


      Starting Connectionless Locator Demo
      Bluetooth initialization...success
      Scan callbacks register...success.
      Periodic Advertising callbacks register...success.
      Start scanning...success
      Waiting for periodic advertising...
      [DEVICE]: XX:XX:XX:XX:XX:XX, AD evt type X, Tx Pwr: XXX, RSSI XXX C:X S:X D:X SR:X E:1 Prim: XXX, Secn: XXX, Interval: XXX (XXX ms), SID: X
      success. Found periodic advertising.
      Creating Periodic Advertising Sync...success.
      Waiting for periodic sync...
      PER_ADV_SYNC[0]: [DEVICE]: XX:XX:XX:XX:XX:XX synced, Interval XXX (XXX ms), PHY XXX
      success. Periodic sync established.
      Enable receiving of CTE...
      success. CTE receive enabled.
      Scan disable...Success.
      Waiting for periodic sync lost...
      PER_ADV_SYNC[X]: [DEVICE]: XX:XX:XX:XX:XX:XX, tx_power XXX, RSSI XX, CTE XXX, data length X, data: XXX
      CTE[X]: samples count XX, cte type XXX, slot durations: X [us], packet status XXX, RSSI XXX

Dependencies
************

This sample uses the following Zephyr libraries:

* :file:`include/zephyr/types.h`
* :file:`lib/libc/minimal/include/errno.h`
* :file:`include/sys/printk.h`
* :file:`include/sys/byteorder.h`
* :file:`include/sys/util.h`
* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/direction.h`
