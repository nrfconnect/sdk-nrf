.. _throughput_readme:

GATT Throughput Service
#######################

.. contents::
   :local:
   :depth: 2

The GATT Throughput Service is a custom service that receives and writes data and returns metrics about these operations.

To test GATT throughput, the client (central) writes without response to the characteristic on the server (peripheral).
The client can then read the characteristic to retrieve the metrics.

The service calls the :c:func:`bt_gatt_write_without_response` function to send data to the characteristic.
This wraps the payload into an ATT_WRITE_CMD PDU in a single L2CAP B-frame.
Each payload has an overhead of 7 bytes: 3 bytes for the ATT header and 4 bytes for the L2CAP header.

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
   * Four bytes unsigned: Total bytes of ATT payload received
   * Four bytes unsigned: Throughput in bits per second


API documentation
*****************

| Header file: :file:`include/bluetooth/services/throughput.h`
| Source file: :file:`subsys/bluetooth/services/throughput.c`

.. doxygengroup::  bt_throughput

References
***********

For more information about the protocol layers and ATT operations involved in this sample, see the following chapters in the `Bluetooth Core Specification`_:

   * Volume 3, Part A, Section 3.1: Connection-oriented channels in Basic L2CAP mode
   * Volume 3, Part F, Section 3.4.5.3: ATT_WRITE_CMD
   * Volume 3, Part G, Section 4.9.1: Write Without Response
