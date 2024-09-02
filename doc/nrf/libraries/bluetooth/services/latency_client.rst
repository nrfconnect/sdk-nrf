.. _latency_client_readme:

GATT Latency Client
###################

.. contents::
   :local:
   :depth: 2

You can use the Latency Client to interact with a connected peer that is running the Latency service with the :ref:`latency_readme`.
It writes data to the Latency characteristic and waits for a response to count the time spent of a GATT Write operation.

Latency Characteristic
**********************

To request latency data from the Latency Characteristic, use the :c:func:`bt_latency_request` function.
The request sending procedure is asynchronous, so the request data to be sent must remain valid until a dedicated callback notifies you that the write request has been completed.

API documentation
*****************

| Header file: :file:`include/bluetooth/services/latency_client.h`
| Source file: :file:`subsys/bluetooth/services/latency_client.c`

.. doxygengroup::  bt_latency_c
   :project: nrf
   :members:
