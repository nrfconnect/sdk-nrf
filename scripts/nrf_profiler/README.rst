.. _nrf_profiler_script:

nRF Profiler host tools
#######################

The nRF Profiler host tools is a set of Python scripts that you can use to visualize and analyze the nRF Profiler events received over RTT from an embedded device running the :ref:`nrf_profiler` library.
You can use the scripts for both :ref:`app_event_manager` events and custom events.

Overview
********

Some of the scripts are used for connecting with an embedded device running the :ref:`nrf_profiler` and collecting the nRF Profiler events over RTT.
The collected profiling data is stored as local files and can be processed later.
Other scripts are used for visualizing, processing, and analyzing the data or calculating statistics.
These scripts rely on locally stored files created earlier.

Requirements
************

To use the nRF Profiler host tools, install the required packages and libraries specified by the :file:`requirements.txt` file using the following command:

.. code-block:: console

   pip3 install --user -r requirements.txt

Apart from this, make sure to enable and configure the :ref:`nrf_profiler` library on a connected embedded device.
The library introduces an RTT-based interface for providing profiling data.
See the library documentation for details.

.. tip::
   The :ref:`nrf_profiler_sample` sample and the :ref:`app_event_manager_profiling_tracer_sample` sample provide an out-of-the-box nRF Profiler integration.

Using the scripts
*****************

The scripts collecting profiling data from an embedded device use CSV files for storing event occurrences and JSON files for storing event descriptions.
A pair of related files shares a common dataset name.
You can use the dataset name to identify the files for data visualization or calculating statistics.

See the following subsections for information about available scripts and example usages.
Use the ``--help`` argument while executing a given script to display help and learn more about available options.

Data collection and visualization
=================================

You can use the following scripts to collect the profiling data and visualize the data:

* :file:`data_collector.py` - The script connects to the device using RTT, receives profiling data, and saves it to files.
  When running the script from the command line, provide the time for collecting data and the dataset name.
  For example:

  .. code-block:: console

     python3 data_collector.py 5 test1

  In this command, ``5`` is the time value (in seconds) for collecting data and ``test1`` is the dataset name.
* :file:`plot_from_files.py` - The script plots events from the dataset that is provided as the command-line argument.
  For example:

  .. code-block:: console

     python3 plot_from_files.py test1

  In this command, ``test1`` is the dataset name.
* :file:`real_time_plot.py` - The script connects to the device using RTT, plots data in real-time, and saves the data.
  When running the script from the command line, provide the dataset name.
  For example:

  .. code-block:: console

     python3 real_time_plot.py test1

  The script terminates when the window displaying the real-time plot is closed.

.. _nrf_profiler_script_visualization_GUI:

Data visualization GUI
----------------------

When you plot the profiling data, the nRF Profiler events are visualized in a GUI window.

The visual output can look as follows:

.. figure:: ../../doc/nrf/images/app_event_manager_profiling_sample.png
   :scale: 50 %
   :alt: Example of nRF Profiler host tools visualization

   Example of nRF Profiler host tools visualization

Profiled events visualization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The nRF Profiler events are visualized in the following way in the plot:

* ``Red dot`` indicates submission of a custom nRF Profiler event or an Application Event Manager event.
* ``Blue vertical rectangle`` indicates processing of the Application Event Manager event (not used for custom nRF Profiler events).

You can use the middle mouse button to mark an event submission or processing for tracking, and to display the event data.

Using GUI while plotting
~~~~~~~~~~~~~~~~~~~~~~~~

The GUI also supports the following actions:

* The :guilabel:`Start/Stop` button can be used to pause or resume the real-time plot translation while plotting the data in real time (:file:`real_time_plot.py`).
* Scrolling on the plot using the mouse wheel zooms it in or out.

  * While plot translation is paused, scrolling zooms to the cursor location.
  * While plot translation is active, scrolling zooms to the right edge of the plot.

Additionally, during a pause or when plotting data from files:

* Click and drag with the left mouse button to pan the plot.
* Click the left or right mouse button to place a vertical line at the cursor location.
  When two lines are present, the application measures the time between them and displays it.

Merging profiling data from multiple devices
============================================

The scripts allow you to analyze data propagation among multiple embedded devices.
The :file:`merge_data.py` script combines data from ``test_p`` and ``test_c`` datasets into one dataset ``test_merged``.
It also provides clock drift compensation based on the synchronization events ``sync_event_p`` and ``sync_event_c``.
The clock drift compensation is necessary to properly measure times between events coming from different devices as every device uses its own clock source.
As command-line arguments, provide the names of events used for synchronization for a Peripheral (``sync_event_p``) and a Central (``sync_event_c``), as well as names of datasets for the Peripheral (``test_p``), the Central (``test_c``), and the merge result (``test_merged``).
For example:

.. code-block:: console

   python3 merge_data.py test_p sync_event_p test_c sync_event_c test_merged

The newly created dataset (``test_merged``) contains the nRF Profiler events registered by both devices with compensated clock drift.
You can use it  for visualization or calculating statistics.

.. note::
   The synchronization profiler events need to be periodically generated exactly at the same time on both devices.
   The :ref:`nrf_profiler` library does not implement the generation of the synchronization events.
   An application must provide its own implementation.
   See the :ref:`nrf_desktop_nrf_profiler_sync` for an example implementation used by the :ref:`nrf_desktop` application.

Dependencies
************

The scripts depend on Python packages and libraries specified by the :file:`requirements.txt` file.
Apart from them, the scripts depend on the :ref:`nrf_profiler` library running on an embedded device to provide nRF Profiler events.
