.. _latency_readme:

GATT Latency Service
####################

The GATT Latency Service is a custom service containing a single writable characteristic.
This characteristic can be used to calculate the round-trip time of a GATT Write operation.

Service UUID
************

The 128-bit service UUID is 67136E01-58DB-F39B-3446-FDDE58C0813A.

Characteristics
***************

This service has one characteristic.

Latency Characteristic (67136E02-58DB-F39B-3446-FDDE58C0813A)
=============================================================

Write
   The server is waiting for a write request from the client, then it will automatically reply a write response back.


API documentation
*****************

| Header file: :file:`include/bluetooth/services/latency.h`
| Source file: :file:`subsys/bluetooth/services/latency.c`

.. doxygengroup::  bt_gatt_latency
   :project: nrf
   :members:
