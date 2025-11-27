.. _nrf_rpc_protocols_serialization_client:

nRF RPC: Protocols serialization client
#######################################

.. contents::
   :local:
   :depth: 2

The Protocols serialization client sample demonstrates how to make remote procedure calls (RPC) to a server device running the :ref:`Protocols serialization server <nrf_rpc_protocols_serialization_server>` sample.
The RPCs are used to control :ref:`OpenThread <ug_thread_intro>`, Bluetooth® LE, and :ref:`NFC <ug_nfc>` stacks running on the server device.
The client and server devices use the :ref:`nrfxlib:nrf_rpc` and the :ref:`nrf_rpc_uart` to communicate with each other.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

To test the sample, you also need another device running the :ref:`Protocols serialization server <nrf_rpc_protocols_serialization_server>` sample.

For testing the Bluetooth LE API serialization, you need the `nRF Connect for Mobile`_ app installed on your smartphone or tablet.

For testing the NFC API serialization, you need a smartphone or tablet that can read NFC tags.

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

* ``ble`` - Enables the client part of the :ref:`Bluetooth LE RPC <ble_rpc>` and ``bt`` shell commands.
  It also enables the :ref:`nus_service_readme` and :ref:`throughput_readme`.
* ``coex`` - Enables ``coex`` shell commands for controlling the :ref:`MPSL software coexistence <nrfxlib:mpsl_cx>` implementation on the server device.
* ``debug`` - Enables debugging features, such as :c:func:`__ASSERT()` statements and verbose logging.
* ``log_rpc`` - Enables the log forwarder part of the :ref:`Logging RPC <log_rpc>` and ``log_rpc`` shell commands.
* ``openthread`` - Enables the client part of the :ref:`OpenThread RPC <ot_rpc>` and ``ot`` shell commands.
* ``nfc`` - Enables the client part of the :ref:`NFC RPC <nfc_rpc>` and ``nfc`` shell commands.

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf_rpc/protocols_serialization/client`

.. include:: /includes/build_and_run.txt

Testing
=======

To test the client sample, follow the instructions in the :ref:`protocols_serialization_server_sample_testing` section of the protocol serialization server sample test procedure.

.. note::
   When using the nRF54L15 DK or nRF54LM20 DK, do not press **Button 1** or **Button 2**.
   The GPIO pins connected to these buttons are used by the UART peripheral for communication with the server device.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`nfc_ndef_msg`

This sample uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_rpc`
