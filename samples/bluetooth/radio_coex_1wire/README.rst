.. _bluetooth_radio_coex_1wire_sample:

Bluetooth: External radio coexistence using one-wire interface
################################################################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to use the 1-wire coexistence feature described in :ref:`Bluetooth® external radio coexistence <nrfxlib:bluetooth_coex>`.

Overview
********

The application sets up an advertiser and configures the 1-wire coexistence feature.
The user can enter ``g`` to grant or ``d`` to deny Bluetooth radio activity.

* When Bluetooth radio activity is granted, packets will be transmitted.
* When Bluetooth radio activity is denied, no packets will be transmitted.
* The number of packets transmitted is tracked by counting RADIO->READY event.
* The total number of transmitted packets will be printed every second.

The application generates a grant signal on the pin selected by ``coex-pta-grant-gpios``.
This pin has to be connected to the pin defined by the ``grant-gpios`` property in the DTS using a jumper cable.

* On ``nrf52840dk_nrf52840``, the default pins are **P0.26** and **P0.02**.

The board's :ref:`/zephyr,user <dt-zephyr-user>` node must have the ``coex-pta-grant-gpios`` property set in the device tree.
This sample's board overlay can be used as an example.

Requirements
************

The sample supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf52840dk_nrf52840

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/radio_coex_1wire`

.. include:: /includes/build_and_run.txt

Testing
=======

1. Build and program the development kit.
#. |connect_terminal|
#. Watch the number of packets transmitted.
#. Enter ``d`` to deny packet transmission.
#. Observe that packet transmissions have stopped.
#. Enter ``g`` to grant packet transmission.
#. Observe that packet packet transmissions have re-started.

Dependencies
************

This sample uses the following `sdk-nrfxlib`_ libraries:

* :ref:`nrfxlib:mpsl`
* :ref:`nrfxlib:softdevice_controller`

The sample also uses drivers from `nrfx`_ libraries.

The sample uses the following Zephyr libraries:

* ``include/console/console.h``
* :ref:`zephyr:bluetooth_api`
