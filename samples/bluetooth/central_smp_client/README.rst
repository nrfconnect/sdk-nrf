.. _bluetooth_central_dfu_smp:

Bluetooth: Central SMP Client
#############################

.. contents::
   :local:
   :depth: 2

The Central Simple Management Protocol (SMP) Client sample demonstrates how to use the :ref:`dfu_smp_readme` to connect to an SMP Server and send a simple echo command.
The response, which is received as CBOR-encoded data, is decoded and printed.

.. note::
   This sample does not provide the means to program a device using DFU.
   It demonstrates the communication between SMP Client and SMP Server.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample also requires a device running `mcumgr`_ with transport protocol over Bluetooth® Low Energy, for example, another development kit running the :zephyr:code-sample:`smp-svr`.

.. note::
   This sample does not program the device using DFU.

Overview
********

After connecting, the sample starts MTU size negotiation, discovers the GATT database of the server, and configures the DFU SMP Client.
When the configuration is complete, the sample is ready to send SMP commands.

To send an echo command, press **Button 1** on the development kit.
The sent string contains a number that is automatically incremented.
This way, you can verify if the correct response is received.
The response is decoded using the `zcbor`_ library and displayed after that.

User interface
**************

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      Button 1:
         Send an echo command.

   .. group-tab:: nRF54 DKs

      Button 0:
         Send an echo command.


Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/central_smp_client`

.. include:: /includes/build_and_run_ns.txt

.. _bluetooth_central_dfu_smp_testing:

Testing
=======

|test_sample|

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      1. |connect_kit|
      #. |connect_terminal|
      #. Reset the kit.
      #. Observe that the text "Starting Bluetooth Central SMP Client sample" is printed on the COM listener running on the computer and the device starts scanning for Peripherals with SMP.
      #. Program the :zephyr:code-sample:`smp-svr` to another development kit.
         See the documentation for that sample only in the section "Building the sample application".
         When you have built the sample, run the following command to program it to the development kit::

            west flash

      #. Observe that the kits connect.
         When service discovery is completed, the event logs are printed on the Central's terminal.
         If you connect to the Server with a terminal emulator, you can observe that it prints "connected".
      #. Press **Button 1** on the Client.
         Observe messages similar to the following::

            Echo test: 1
            Echo response part received, size: 28.
            Total response received - decoding
            {_"r": "Echo message: 1"}

      #. Press the Reset button on the Central to disconnect the devices.
         Observe that the kits automatically reconnect and that it is again possible to send data between the two kits.

   .. group-tab:: nRF54 DKs

      1. |connect_kit|
      #. |connect_terminal|
      #. Reset the kit.
      #. Observe that the text "Starting Bluetooth Central SMP Client sample" is printed on the COM listener running on the computer and the device starts scanning for Peripherals with SMP.
      #. Program the :zephyr:code-sample:`smp-svr` to another development kit.
         See the documentation for that sample only in the section "Building the sample application".
         When you have built the sample, run the following command to program it to the development kit::

            west flash

      #. Observe that the kits connect.
         When service discovery is completed, the event logs are printed on the Central's terminal.
         If you connect to the Server with a terminal emulator, you can observe that it prints "connected".
      #. Press **Button 0** on the Client.
         Observe messages similar to the following::

            Echo test: 1
            Echo response part received, size: 28.
            Total response received - decoding
            {_"r": "Echo message: 1"}

      #. Press the Reset button on the Central to disconnect the devices.
         Observe that the kits automatically reconnect and that it is again possible to send data between the two kits.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`dfu_smp_readme`
* :ref:`gatt_dm_readme`
* :ref:`nrf_bt_scan_readme`

It uses the following Zephyr libraries:

* :file:`include/zephyr/types.h`
* :file:`boards/arm/nrf*/board.h`
* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``

* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/gatt.h`
  * :file:`include/bluetooth/hci.h`
  * :file:`include/bluetooth/uuid.h`

In addition, it uses the following external library that is distributed with Zephyr:

* `zcbor`_

The sample also uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
