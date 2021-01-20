.. _profiler_sample:

Profiler
########

.. contents::
   :local:
   :depth: 2

The Profiler sample demonstrates the functionality of the :ref:`profiler` subsystem.
It shows how to use the Profiler to log and visualize data about custom events that are not part of the :ref:`event_manager`.

Overview
********

The sample initializes the :ref:`profiler`, registers two event types, and profiles their occurrences periodically.

Event without data (``no data event``):
  This event is used to signal the occurrence of an event only.
  It does not contain additional data.

Event with data (``data event``):
  There are several numerical values associated with this event.
  The values are updated periodically.


Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9160dk_nrf9160ns, nrf52840dk_nrf52840, nrf52dk_nrf52832

Building and running
********************
.. |sample path| replace:: :file:`samples/profiler`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, you can test it by running the script ``real_time_plot.py`` (located under :file:`scripts/profiler`).
As an argument, pass the name that should be used to store the data.
For example, run ``real_time_plot.py test_name`` to generate a :file:`test_name.csv` and a :file:`test_name.json` file.

The script opens a GUI window that displays events as points on timelines.
See the Profiler documentation for more information.

Connect to the development kit with a terminal emulator (for example, PuTTY) to see messages displayed by the sample.
See :ref:`putty` for the required settings.

.. tip::
   If you use SEGGER Embedded Studio, make sure to stop debugging before you run Python scripts.
   Otherwise, you may observe problems with accessing RTT data by the profiler scripts.

Dependencies
************

This sample uses the following |NCS| subsystems:

* :ref:`profiler`
