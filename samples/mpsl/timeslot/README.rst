.. _timeslot_sample:

MPSL timeslot
#############

This sample demonstrates how to use :ref:`nrfxlib:mpsl_readme` and basic MPSL Timeslot functionality.

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

* One of the nRF52 development boards. The sample has been tested on:

  * nRF52 Development Kit (PCA10040)
  * nRF52840 Development Kit (PCA10056)

Building and Running
********************
.. |sample path| replace:: :file:`samples/mpsl/timeslot`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your board, test it by performing the following steps:

1. Connect to the board with a terminal emulator (for example, PuTTY).
   See :ref:`putty` for the required settings.
#. Follow the instructions in the terminal to open a session and start requesting timeslots.
   The terminal then prints the signal type for each timeslot callback:

   - If you press 'a', the timeslot callback requests a new timeslot.
     Observe that ``Timeslot start`` is printed until the session is closed.
   - If you press 'b', the timeslot callback ends the timeslot.
     Observe that only one ``Timeslot start`` is printed, followed by a ``Session idle``.

#. Press any key to close the session.
   Observe that ``Session closed`` is printed.

Dependencies
************

This sample uses the following `nrfxlib`_ libraries:

* :ref:`nrfxlib:mpsl_readme`

In addition, it uses the following Zephyr libraries:

* :file:`include/console.h`
* :ref:`zephyr:kernel`:

  * :file:`include/kernel.h`
  * :file:`include/irq.h`

* :file:`include/misc/printk.h`
* :file:`include/zephyr/types.h`
