.. _timeslot_sample:

MPSL timeslot
#############

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to use :ref:`nrfxlib:mpsl` and basic MPSL Timeslot functionality.

Overview
********

The sample opens a timeslot session and starts requesting timeslots when a key is pressed in the terminal.

* If 'a' is pressed, the callback for the first timeslot requests a new timeslot.
* If 'b' is pressed, the callback for the first timeslot ends the timeslot.

The first timeslot is always of type 'earliest'.
Any following timeslots are of type 'normal'.
In each timeslot callback, the signal type of the callback is posted to a message queue.
A separate thread reads the message queue and prints the timeslot signal type.
The timeslot session is closed when any key is pressed in the terminal.

Requirements
************

The sample supports any one of the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpunet, nrf52840dk_nrf52840, nrf52dk_nrf52832

.. note::
   For nRF5340 DK, this sample is only supported on the network core (nrf5340dk_nrf5340_cpunet), and the :ref:`nrf5340_empty_app_core` sample must be programmed to the application core.

Building and Running
********************

.. |sample path| replace:: :file:`samples/mpsl/timeslot`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. |connect_terminal|
#. Follow the instructions in the terminal to open a session and start requesting timeslots.
   The terminal then prints the signal type for each timeslot callback:

   * If you press 'a', the timeslot callback requests a new timeslot.
     Observe that ``Timeslot start`` is printed until the session is closed.
   * If you press 'b', the timeslot callback ends the timeslot.
     Observe that only one ``Timeslot start`` is printed, followed by a ``Session idle``.

#. Press any key to close the session.
   Observe that ``Session closed`` is printed.

Dependencies
************

This sample uses the following `sdk-nrfxlib`_ libraries:

* :ref:`nrfxlib:mpsl`

In addition, it uses the following Zephyr libraries:

* :file:`include/console.h`
* :ref:`zephyr:kernel_api`:

  * :file:`include/kernel.h`
  * :file:`include/irq.h`

* :file:`include/sys/printk.h`
* :file:`include/zephyr/types.h`
