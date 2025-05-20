.. _bluetooth_radio_coex_1wire_sample:

Bluetooth: External radio coexistence using 1-wire interface
############################################################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to use the 1-wire coexistence feature described in :ref:`ug_radio_mpsl_cx_generic_1wire`.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

The application sets up an advertiser and configures the 1-wire coexistence feature.
Enter ``g`` to grant or ``d`` to deny Bluetooth radio activity.

* When Bluetooth radio activity is granted, packets are transmitted.
* When Bluetooth radio activity is denied, no packets are transmitted.
* The number of packets transmitted is tracked by counting the RADIO->READY event.
* The total number of transmitted packets is printed every second.

Wiring
******

The application generates a grant signal on the pin selected by ``coex-pta-grant-gpios``.
Connect this pin to the pin defined by the ``grant-gpios`` property in the DTS using a jumper cable.

The following table shows the default pins used for this sample for each supported target:

.. table::
   :align: center

   +----------------------------+----------------------+-------------+
   | Target                     | coex-pta-grant-gpios | grant-gpios |
   +============================+======================+=============+
   | nrf52840dk/nrf52840        | P0.26                | P0.02       |
   +----------------------------+----------------------+-------------+
   | nrf54l15dk/nrf54l15/cpuapp | P0.00                | P0.02       |
   +----------------------------+----------------------+-------------+
   | nrf54h20dk/nrf54h20/cpurad | P0.00                | P0.02       |
   +----------------------------+----------------------+-------------+


The board's :ref:`/zephyr,user <dt-zephyr-user>` node must have the ``coex-pta-grant-gpios`` property set in the devicetree.
You can use this sample's board overlay as an example.

.. note::
   For the ``nrf54h20dk/nrf54h20/cpurad`` target in the board's devicetree file, you must define a node with the compatible property set to ``coex-pta-grant-gpios``.
   Then set the ``coex-pta-grant-gpios`` property of that node to the desired pin.
   This is needed to properly configure the UICR register and get a pin access from the core the application is running on.
   See the :file:`boards/nrf54h20dk_nrf54h20_cpurad.overlay` file for an example.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/radio_coex_1wire`

.. include:: /includes/build_and_run.txt

Testing
=======

|test_sample|

1. Build and program the development kit.
#. |connect_terminal|
#. Observe the number of packets transmitted.
#. Enter ``d`` to deny packet transmission.
#. Observe that packet transmissions have stopped.
#. Enter ``g`` to grant packet transmission.
#. Observe that packet transmissions have restarted.

Dependencies
************

This sample uses the following `sdk-nrfxlib`_ libraries:

* :ref:`nrfxlib:mpsl`
* :ref:`nrfxlib:softdevice_controller`

It also uses drivers from the `nrfx`_ libraries.

It uses the following Zephyr libraries:

* :file:`include/console/console.h`
* :ref:`zephyr:bluetooth_api`
