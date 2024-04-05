.. _nrf_rpc_remote:

nRF RPC
###########

.. contents::
   :local:
   :depth: 2

The Protocols serialization server application demonstrates how to receive remote procedure calls (RPC) from a Protocols serialization client device.
The RPCs are used to control Bluetooth LE and OpenThread stacks running on the server device.
The client and server devices use nRF RPC library and the UART interface to communicate with each other.

Requirements
************

The sample supports the following development kits for testing the network status:

.. table-from-sample-yaml::
