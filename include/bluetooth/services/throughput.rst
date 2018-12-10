.. _throughput_readme:

GATT Throughput Service
#######################

The GATT Throughput Service is a custom service that receives writes and returns metrics about these writes.
To test GATT throughput, the client (central) writes without response to the characteristic on the server (peripheral).
The client can then read the characteristic to retrieve the metrics.

The GATT Throughput Service is used in the :ref:`ble_throughput` sample.

Service UUID
************

The 128-bit service UUID is 0xBB, 0x4A, 0xFF, 0x4F, 0xAD, 0x03, 0x41, 0x5D, 0xA9, 0x6C, 0x9D, 0x6C, 0xDD, 0xDA, 0x83, 0x04.

Characteristics
***************

This service has one characteristic.

Throughput (0x1524)
===================

Write Without Response
   * Write any data to the characteristic to measure throughput.
   * Write 0 bytes to the characteristic to reset the metrics.

Read
   The read operation returns 3*4 bytes (12 bytes) that contain the metrics:

   * 4 bytes unsigned: Number of GATT writes received
   * 4 bytes unsigned: Total bytes received
   * 4 bytes unsigned: Throughput in bits per second


API documentation
*****************

.. doxygengroup::  nrf_bt_throughput
   :project: nrf
   :members:
