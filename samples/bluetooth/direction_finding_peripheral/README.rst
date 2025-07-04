.. _direction_finding_peripheral:

Bluetooth: Direction finding peripheral
#######################################

.. contents::
   :local:
   :depth: 2

The direction finding peripheral sample demonstrates Bluetooth® LE direction finding transmission as a response to a request received from a connected central device.

Requirements
************

.. include:: /samples/bluetooth/direction_finding_connectionless_tx/README.rst
   :start-after: bt_dir_finding_tx_req_start
   :end-before: bt_dir_finding_tx_req_end

Overview
********

The direction finding peripheral sample uses Constant Tone Extension (CTE), that is transmitted with periodic advertising PDUs.

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

.. include:: /samples/bluetooth/direction_finding_connectionless_tx/README.rst
   :start-after: bt_dir_finding_tx_aoa_mode_start
   :end-before: bt_dir_finding_tx_aoa_mode_end

.. include:: /samples/bluetooth/direction_finding_connectionless_tx/README.rst
   :start-after: bt_dir_finding_tx_aod_mode_start
   :end-before: bt_dir_finding_tx_aod_mode_end

.. include:: /samples/bluetooth/direction_finding_connectionless_tx/README.rst
   :start-after: bt_dir_finding_tx_ant_aod_start
   :end-before: bt_dir_finding_tx_ant_aod_end

Building and Running
********************
.. |sample path| replace:: :file:`samples/bluetooth/direction_finding_peripheral`

.. include:: /includes/build_and_run.txt

.. |sample_or_app| replace:: sample
.. |ipc_radio_dir| replace:: :file:`sysbuild/ipc_radio`

.. include:: /includes/ipc_radio_conf.txt

Testing
=======

|test_sample|

1. |connect_terminal_specific|
#. In the terminal window, check for information similar to the following::

      Bluetooth initialized
      <inf> bt_hci_core: HW Platform: Nordic Semiconductor (XxXXXX)
      <inf> bt_hci_core: HW Variant: nRF52x (XxXXXX)
      <inf> bt_hci_core: Firmware: Standard Bluetooth controller (XxXX) Version 3.0 Build X
      <inf> bt_hci_core: Identity: XX:XX:XX:XX:XX (random)
      <inf> bt_hci_core: HCI: version 5.3 (XxXX) revision XxXXXX, manufacturer XxXXXX
      <inf> bt_hci_core: LMP: version 5.3 (XxXX) subver XxXXXX

Dependencies
************

This sample uses the following Zephyr libraries:

* :file:`include/zephyr/types.h`
* :file:`lib/libc/minimal/include/errno.h`
* :file:`include/sys/printk.h`
* :file:`include/sys/byteorder.h`
* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/hci.h`
  * :file:`include/bluetooth/direction.h`
  * :file:`include/bluetooth/conn.h`
