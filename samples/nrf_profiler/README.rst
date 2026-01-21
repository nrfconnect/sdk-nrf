.. _nrf_profiler_sample:

nRF Profiler
############

.. contents::
   :local:
   :depth: 2

The nRF Profiler sample demonstrates the functionality of the :ref:`nrf_profiler` library.
It shows how to use the nRF Profiler to log and visualize data about custom events that are not part of the :ref:`app_event_manager`.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample initializes the :ref:`nrf_profiler` and registers and periodically profiles the occurrences of the following event types:

* Event without data (``no data event``) - This event is used to signal the occurrence of an event only.
  It does not contain additional data.
* Event with data (``data event``) - There are several numerical values associated with this event.
  The values are updated periodically.

Configuration
*************

|config|

Building and running
********************
.. |sample path| replace:: :file:`samples/nrf_profiler`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

After programming the sample to your development kit, you can test it by performing the following steps:

1. Go to the :file:`scripts/nrf_profiler` folder.
#. Run the script :file:`real_time_plot.py`, with the name that should be used to store the data as the argument.
   For example:

   .. code-block:: console

      python3 real_time_plot.py *test_name*

   This command generates a :file:`test_name.csv` file and a :file:`test_name.json` file.
   The script opens a GUI window that displays events as dots on timelines, similar to the following plot.

   .. figure:: ../../doc/nrf/images/app_event_manager_profiling_sample.png
      :scale: 50 %
      :alt: Example of nRF Profiler host tools GUI window

      Example of nRF Profiler host tools GUI window

   See the :ref:`nrf_profiler_script_visualization_GUI` section in the nRF Profiler host tools documentation for more information about the GUI.

#. |connect_terminal|
   After you connect, the sample will display messages in the terminal.
#. Calculate the nRF Profiler event propagation statistics (statistics for time intervals between nRF Profiler events) from the previously collected dataset using the following command:

   .. code-block:: console

      python3 calc_stats.py *test_name* stats_nordic_presets/nrf_profiler.json

   The :file:`stats_nordic_presets/nrf_profiler.json` file specifies the nRF Profiler events used for the calculations.
   The file refers to the events used by the sample.
   See the :ref:`nrf_profiler_script_calculating_statistics` section in the nRF Profiler host tools documentation for more information.

Dependencies
************

This sample uses the following |NCS| subsystems:

* :ref:`nrf_profiler`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
