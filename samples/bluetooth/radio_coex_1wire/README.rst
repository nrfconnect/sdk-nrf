.. _bluetooth_radio_coex_1wire_sample:

Bluetooth: External radio coexistence using 1-wire interface
############################################################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to use the 1-wire coexistence feature described in :ref:`Bluetooth® external radio coexistence <nrfxlib:bluetooth_coex>`.

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

The application generates a grant signal on the pin selected by ``coex-pta-grant-gpios``.
Connect this pin to the pin defined by the ``grant-gpios`` property in the DTS using a jumper cable.

On the ``nrf52840dk_nrf52840`` target, the default pins are **P0.26** and **P0.02**.

The board's :ref:`/zephyr,user <dt-zephyr-user>` node must have the ``coex-pta-grant-gpios`` property set in the devicetree.
You can use this sample's board overlay as an example.

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
