.. _profiler_sample:

Profiler
########

The Profiler sample demonstrates the functionality of the Profiler subsystem.
It shows how to use the Profiler to log and visualize data about custom events that are not part of the :ref:`event_manager`.

Overview
********

The sample initializes the Profiler, registers two event types, and profiles their occurrences periodically.

Event without data (``no data event``):
  This event is used to signal the occurrence of an event only.
  It does not contain additional data.

Event with data (``data event``):
  There are several numerical values associated with this event.
  The values are updated periodically.


Requirements
************

* One of the following development boards:

  * nRF9160 DK board (PCA10090)
  * nRF52840 Development Kit board (PCA10056)
  * nRF52 Development Kit board (PCA10040)
  * nRF51 Development Kit board (PCA10028)


Building and running
********************

This sample can be found under :file:`samples/profiler` in the |NCS| folder structure.

See :ref:`gs_programming` for information about how to build and program the application.


Testing
=======

After programming the sample to your board, you can test it by running the script ``real_time_plot.py`` (located under :file:`scripts/profiler`).
As arguments, pass the csv and json file names that should be used to store the data (for example, run ``real_time_plot.py data.csv data.json``).

The script opens a GUI window that displays events as points on timelines.
See the Profiler documentation for more information.

Connect to the board with a terminal emulator (for example, PuTTY) to see messages displayed by the sample.
See :ref:`putty` for the required settings.


Dependencies
************

This sample uses the following |NCS| subsystems:

* profiler
