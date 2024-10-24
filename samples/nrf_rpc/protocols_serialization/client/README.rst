.. _nrf_rpc_protocols_serialization_client:

nRF RPC: Protocols serialization client
#######################################

.. contents::
   :local:
   :depth: 2

The Protocols serialization client sample demonstrates how to send remote procedure calls (RPC) to a protocols serialization server device, such as one running the :ref:`Protocols serialization server <nrf_rpc_protocols_serialization_server>` sample.
The RPCs are used to control Bluetooth® LE, :ref:`NFC <ug_nfc>` and :ref:`OpenThread <ug_thread_intro>` stacks running on the server device.
The client and server devices use the :ref:`nrfxlib:nrf_rpc` and the :ref:`nrf_rpc_uart` to communicate with each other.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

To test the sample, you also need another device running the :ref:`Protocols serialization server <nrf_rpc_protocols_serialization_server>` sample.

For testing the Bluetooth LE API serialization, you need to have the `nRF Connect for Mobile`_ app installed on your smartphone or tablet.

For testing the NFC API serialization, you also need a smartphone or tablet that can read NFC tags.

Overview
********

The Protocols serialization client sample is a thin client that does not include OpenThread, Bluetooth LE or NFC stacks.
Instead, it provides implementations of selected OpenThread, Bluetooth LE and NFC functions that forward the function calls over UART to the Protocols serialization server.
The client also includes a shell for testing the serialization.

Configuration
*************

|config|

Snippets
========

.. |snippet| replace:: :makevar:`client_SNIPPET`

The following snippets are available:

* ``ble`` - Enables the client part of the :ref:`Bluetooth LE RPC <ble_rpc>` and shell interface to serialized API.
  Also enables the :ref:`Bluetooth LE Nordic UART Service <nus_service_readme>`.
* ``coex`` - Enables shell commands for the :ref:`MPSL software coexistence <nrfxlib:mpsl_cx>` implementation on the server device.
* ``debug`` - Enables debugging the sample by enabling :c:func:`__ASSERT()` statements globally and verbose logging.
* ``log_rpc`` - Enables logging over RPC.
* ``openthread`` - Enables the client part of the OpenThread RPC.
* ``nfc`` - Enables the client part of the :ref:`NFC RPC <nfc_rpc>`.

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf_rpc/protocols_serialization/client`

.. include:: /includes/build_and_run.txt

You can modify the list of enabled features, which by default includes Bluetooth LE support and debug logs.

Testing
=======

To test the client sample, follow the instructions in the :ref:`protocols_serialization_server_sample_testing` section of the protocol serialization server sample test procedure.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`nfc_ndef_msg`

This sample uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_rpc`
