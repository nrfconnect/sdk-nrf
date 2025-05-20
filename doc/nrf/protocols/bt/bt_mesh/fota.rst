.. _ug_bt_mesh_fota:

Performing Device Firmware Updates (DFU) in Bluetooth Mesh
##########################################################

The |NCS| supports two DFU methods when updating your firmware over-the-air (FOTA) for your Bluetooth® Mesh devices and applications.

* DFU over Bluetooth Mesh using the Zephyr Bluetooth Mesh DFU subsystem that implements the Mesh Device Firmware Update Model specification version 1.0 and Mesh Binary Large Object Transfer Model specification version 1.0.
* Point-to-point DFU over Bluetooth Low Energy using the MCUmgr subsystem and the Simple Management Protocol (SMP).

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   dfu_over_bt_mesh
   dfu_over_ble
