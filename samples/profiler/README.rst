.. _profiler_sample:

Profiler
########

.. contents::
   :local:
   :depth: 2

The Profiler sample demonstrates the functionality of the :ref:`profiler` subsystem.
It shows how to use the Profiler to log and visualize data about custom events that are not part of the :ref:`event_manager`.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9160dk_nrf9160_ns, nrf52840dk_nrf52840, nrf52dk_nrf52832, nrf21540dk_nrf52840

Overview
********

The sample initializes the :ref:`profiler` and registers and periodically profiles the occurrences of the following event types:

* Event without data (``no data event``) - This event is used to signal the occurrence of an event only.
  It does not contain additional data.
* Event with data (``data event``) - There are several numerical values associated with this event.
  The values are updated periodically.

Configuration
*************

|config|

Building and running
********************
.. |sample path| replace:: :file:`samples/profiler`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, you can test it by performing the following steps:

1. Go to the :file:`scripts/profiler` folder.
#. Run the script :file:`real_time_plot.py`, with the name that should be used to store the data as the argument.
   For example:

   .. parsed-literal::
      :class: highlight

      real_time_plot.py *test_name*

   This command generates a :file:`test_name.csv` file and a :file:`test_name.json` file.
   The script opens a GUI window that displays events as dots on timelines, similar to the following diagram.

   .. include:: ../../doc/nrf/libraries/others/profiler.rst
      :start-after: profiler_GUI_start
      :end-before: profiler_GUI_end

   See the :ref:`profiler_backends_custom_visualization` section in the Profiler documentation for more information about the GUI.

#. |connect_terminal|

After you connect, the sample will display messages in the terminal.

.. note::
   If you use SEGGER Embedded Studio, make sure to stop debugging before you run Python scripts.
   Otherwise, you may observe problems with accessing RTT data by the profiler scripts.

Dependencies
************

This sample uses the following |NCS| subsystems:

* :ref:`profiler`
