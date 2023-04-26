.. _ble_rpc_host:

Bluetooth: Host for nRF RPC Bluetooth Low Energy
################################################

.. contents::
   :local:
   :depth: 2

The nRF RPC Host sample demonstrates the Bluetooth® Low Energy (LE) stack with the :ref:`nrfxlib:nrf_rpc` library that exposes the stack's interface to another device or CPU using `Remote Procedure Calls (RPC)`_.
On an nRF53 Series device, this sample is supposed to run on the network core and it provides the Bluetooth LE functionality for the application core.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

.. note::
   Currently, only a subset of Zephyr's Bluetooth APIs is available over Bluetooth nRF RPC.
   For more details about the limitations, see the :ref:`ble_rpc_api` of the Bluetooth nRF RPC library.

The host (network core) is running the full Bluetooth LE stack.
It receives serialized function calls that it decodes and executes, then sends response data to the client (application core).

When the sample starts, it displays the welcome prompt "Starting nRF RPC bluetooth host".

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/rpc_host`

You must program this sample to the nRF5340 network core.

.. include:: /includes/build_and_run.txt

.. _rpc_host_debug:

Debug build
===========

To build the sample with a debugging configuration, use the ``-DOVERLAY_CONFIG=overlay-debugging.conf'`` flag in your build.

See :ref:`cmake_options` for instructions on how to add this option to your build.
For example, when building on the command line, enter the following command:

.. code-block:: console

   west build samples/bluetooth/rpc_host -- -DOVERLAY_CONFIG=overlay-debugging.conf

.. _rpc_host_testing:

Example build
=============

The recommended way of building the nRF RPC Host sample is to use the multi-image feature of the build system, building the sample with the same Bluetooth configuration as the application core sample.
In this way, the sample is built automatically as a child image when the :kconfig:option:`CONFIG_BT_RPC_STACK` option is enabled.

See :ref:`configure_application` for information about how to configure a sample.

1. Build the sample with the same Bluetooth configuration as the application core sample.
   For more details, see: :ref:`ble_rpc`.

#. Build the :ref:`peripheral_uart` on the application core.
   This sample works out of the box and does not require configuration changes.

#. In the Peripheral UART sample directory, run the following command:

   .. code-block:: console

      west build -b nrf5340dk_nrf5340_cpuapp -- -DCONFIG_BT_RPC_STACK=y

You can also build the :ref:`peripheral_hids_mouse` sample using the above command.
This sample requires some additional configuration in the :file:`samples/bluetooth/peripheral_hids_mouse/child_image/rpc_host.conf` file.
You can take it as an example on how to create configuration for your own application.

Testing
=======
After programming the example build to your development kit, complete the following steps to test it:

1. Connect the dual core development kit to the computer using a USB cable.
   The development kit is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. |connect_terminal|
#. Reset the development kit.
#. Observe that the terminal connected to the network core displays "Starting nRF RPC Bluetooth host".
#. On the terminal connected to the application core, you can observe your Bluetooth application is running.

Dependencies
************

This sample uses the following |NCS| library:

* :ref:`ble_rpc`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_rpc`
