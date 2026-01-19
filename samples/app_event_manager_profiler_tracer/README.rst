.. _app_event_manager_profiling_tracer_sample:

Application Event Manager profiling tracer
##########################################

.. contents::
   :local:
   :depth: 2

The Application Event Manager profiling tracer sample demonstrates the functionality of profiling :ref:`app_event_manager` events using the :ref:`nrf_profiler` and the :ref:`app_event_manager_profiler_tracer` modules.

Just testing.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample application consists of the following modules that communicate using events:

* Module A (:file:`module_a.c`)
* Module B (:file:`module_b.c`)

Module A waits for a configuration event sent by :file:`main.c`.
After receiving this event, Module A starts sending periodic events every second to Module B.
Module A also has a counter that increases before a :c:struct:`one_sec_event` is sent.
The counter value is transmitted during the event.

Module B waits for the :c:struct:`one_sec_event` events from Module A.
Every time it receives a :c:struct:`one_sec_event`, the module checks if the value it transmits is equal to ``5``.
If it is equal to ``5``, Module B sends a :c:struct:`five_sec_event` to Module A.

When Module A receives a :c:struct:`five_sec_event` from Module B, it zeros the counter, sends a series of 50 :c:struct:`burst_event`, and simulates work for 100 ms by busy waiting.

.. msc::
   hscale = "1.3";
   Main [label="main.c"],ModA [label="Module A"],ModB [label="Module B"];
   Main>>ModA [label="Configuration event"];
   ModA rbox ModA [label="Increases the counter"];
   ModA>>ModB [label="One-second event, counter value 1"];
   ModB rbox ModB [label="Checks the counter"];
   ModA rbox ModA [label="Increases the counter"];
   ModA>>ModB [label="One-second event, counter value 2"];
   ModB rbox ModB [label="Checks the counter"];
   |||;
   ...;
   ...;
   |||;
   ModA rbox ModA [label="Increases the counter"];
   ModA>>ModB [label="One-second event, counter value 5"];
   ModB rbox ModB [label="Checks the counter\nCounter value matches 5"];
   ModA<<ModB [label="Five-second event"];
   ModA rbox ModA [label="Zeroes the counter\nSends 50 burst events\nSimulates work for 100 ms"];

Configuration
*************

|config|

Building and running
********************
.. |sample path| replace:: :file:`samples/app_event_manager_profiler_tracer`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. Run the script :file:`real_time_plot.py` located in the :file:`scripts/nrf_profiler` folder, with the name that is to be used to store the data as argument.
   For example, run the following command to generate a :file:`test_name.csv` and a :file:`test_name.json` file using the *test_name* argument:

   .. code-block:: console

      python3 real_time_plot.py *test_name*

   The script opens a GUI window that displays events as points on timelines, similar to the following:

   .. figure:: ../../doc/nrf/images/app_event_manager_profiling_sample.png
      :scale: 50 %
      :alt: nRF Profiler host tools GUI window

      nRF Profiler host tools GUI window

#. Use the scroll wheel to zoom into interesting parts on a plot.
   See the :ref:`nrf_profiler_script_visualization_GUI` section in the nRF Profiler host tools documentation for more information about how to navigate in the GUI window.
#. Click the middle mouse button to highlight an event submission or processing for tracking, and to display the event data as on a figure:

   .. figure:: ../../doc/nrf/images/app_event_manager_profiling_sample_zoom.png
      :scale: 50 %
      :alt: nRF Profiler host tools GUI window zoomed in

      nRF Profiler host tools GUI window zoomed in

   On this image, the zoom focuses on actions triggered by the fifth one-second event.
   The five-second event triggers 50 burst events.
   One event submission and corresponding processing time is highlighted in green.
#. Check the results for the generated :file:`test_name.csv` and :file:`test_name.json` files.
   See how events are logged with data transmitted by the event.
#. Calculate the nRF Profiler event propagation statistics (statistics for time intervals between nRF Profiler events) from the previously collected dataset using the following command:

   .. code-block:: console

      python3 calc_stats.py *test_name* stats_nordic_presets/app_event_manager_profiler_tracer.json

   The :file:`stats_nordic_presets/app_event_manager_profiler_tracer.json` file specifies the nRF Profiler events used for the calculations.
   The file refers to the events used by the sample.
   See the :ref:`nrf_profiler_script_calculating_statistics` section in the nRF Profiler host tools documentation for more information.

Dependencies
************

This sample uses the following |NCS| subsystems:

* :ref:`app_event_manager`
* :ref:`app_event_manager_profiler_tracer`

In addition, it uses the following Zephyr subsystems:

* :ref:`zephyr:logging_api`

The sample also uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
