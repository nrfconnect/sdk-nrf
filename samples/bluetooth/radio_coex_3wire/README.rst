.. _bluetooth_radio_coex_3wire_sample:

Bluetooth: External radio coexistence using 3-wire interface
############################################################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to use :ref:`Bluetooth® external radio coexistence <nrfxlib:bluetooth_coex>`.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

The application sets up an advertiser and implements a rudimentary PTA to test the coexistence.
Enter ``g`` to grant or ``d`` to deny requests from Bluetooth.
Since this is a basic implementation, the decision to grant or deny does not consider priority or direction of the requests.

* When requests from Bluetooth are granted, packets are transmitted.
* When requests from Bluetooth are denied, no packets are transmitted.
* Number of packets transmitted is tracked by counting the RADIO->READY event.
* The total number of transmitted packets is printed every second.

The application generates a grant signal on the pin selected by ``coex-pta-grant-gpios``.
Connect this pin to the pin defined by the ``grant-gpios`` property in the DTS using a jumper.

* On ``nrf52840dk_nrf52840``, the default pins are **P0.26** and **P0.02**.

The application senses request signal on the pin selected by ``coex-pta-req-gpios``.
Connect this pin to the pin defined by the ``req-gpios`` property in the DTS using a jumper.

* On ``nrf52840dk_nrf52840``, the default pins are **P0.28** and **P0.03**.

These two properties are defined in the DTS:

The board's :ref:`/zephyr,user <dt-zephyr-user>` node must have ``coex-pta-grant-gpios`` and ``coex-pta-req-gpios`` properties set.
You can use this sample's board overlay as an example.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/radio_coex_3wire`

.. include:: /includes/build_and_run.txt

Testing
=======

To test this sample, complete the following steps:

1. Connect pin ``coex-pta-grant-gpios`` to pin ``grant-gpios`` on your development kit.
#. Connect pin ``coex-pta-req-gpios`` to pin ``req-gpios`` on your development kit.
#. Build and program the development kit.
#. |connect_terminal|
#. Observe the number of packets transmitted.
#. Enter ``d`` to deny packet transmission.
#. Enter ``g`` to grant packet transmission.

Dependencies
************

This sample uses the following `sdk-nrfxlib`_ libraries:

* :ref:`nrfxlib:mpsl`
* :ref:`nrfxlib:softdevice_controller`

It also uses drivers from the `nrfx`_ libraries.

It uses the following Zephyr libraries:

* ``include/console/console.h``
* :ref:`zephyr:bluetooth_api`
