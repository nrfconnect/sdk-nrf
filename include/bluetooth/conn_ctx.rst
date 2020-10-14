.. _bt_conn_ctx_readme:

Bluetooth connection context
############################

.. contents::
   :local:
   :depth: 2

Data related to a Bluetooth connection can be stored in a Bluetooth connection context.
The Bluetooth connection context library can be used with Bluetooth LE services that require the connection context to support multilink functionality for the GATT server role.

Each instance of the library can store the contexts for a configurable number of Bluetooth connections (see :ref:`zephyr:bluetooth_connection_mgmt` in the Zephyr documentation).

The following Bluetooth LE service shows how to use this library: :ref:`hids_readme`


API documentation
*****************

| Header file: :file:`include/bluetooth/conn_ctx.h`
| Source file: :file:`subsys/bluetooth/conn_ctx.c`

.. doxygengroup:: bt_conn_ctx
   :project: nrf
   :members:
