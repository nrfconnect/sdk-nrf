.. _throughput_readme:

GATT Throughput Service
#######################

.. contents::
   :local:
   :depth: 2

The GATT Throughput Service is a custom service that receives and writes data and returns metrics about these operations.

To test GATT throughput, the client (central) writes without response to the characteristic on the server (peripheral).
The client can then read the characteristic to retrieve the metrics.

The GATT Throughput Service is used in the :ref:`ble_throughput` sample.

Service UUID
************

The 128-bit service UUID is ``0483DADD-6C9D-6CA9-5D41-03AD4FFF4ABB``.

Characteristics
***************

This service has one characteristic.

Throughput (``0x1524``)
=======================

Write Without Response
   * Write any data to the characteristic to measure throughput.
   * Write one byte to the characteristic to reset the metrics.

Read
   The read operation returns 3*4 bytes (12 bytes) that contain the metrics:

   * Four bytes unsigned: Number of GATT writes received
   * Four bytes unsigned: Total bytes received
   * Four bytes unsigned: Throughput in bits per second


API documentation
*****************

| Header file: :file:`include/throughput.h`
| Source file: :file:`subsys/bluetooth/services/throughput.c`

.. doxygengroup::  bt_throughput
   :project: nrf
   :members:
