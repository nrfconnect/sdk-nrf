.. _latency_client_readme:

GATT Latency Client
###################

.. contents::
   :local:
   :depth: 2

The Latency Client can be used to interact with a connected peer that is running the Latency service with the :ref:`latency_readme`.
It writes data to the Latency characteristic and wait for a response to count the time spend of a GATT Write operation.

Latency Characteristic
**********************

To send data to the Latency Characteristic, use the send API of this module.
The sending procedure is asynchronous, so the data to be sent must remain valid until a dedicated callback notifies you that the Write Request has been completed.

API documentation
*****************

| Header file: :file:`include/bluetooth/services/latency_client.h`
| Source file: :file:`subsys/bluetooth/services/latency_client.c`

.. doxygengroup::  bt_latency_c
   :project: nrf
   :members:
