.. _nrf_dm_usage:

Usage
#####

.. contents::
   :local:
   :depth: 2

You can use the Distance Measurement library as follows:

1. Call the :c:func:`nrf_dm_init` function.
   The library expects as arguments the PPI channel numbers, the antenna configuration, and a timer instance.
   The  :c:macro:`NRF_DM_DEFAULT_SINGLE_ANTENNA_CONFIG` macro provides a default antenna configuration.

   .. note::
      Only a single antenna is currently supported.

#. Configure the execution of the measurements using the :c:func:`nrf_dm_configure` function.
   Use the structure in :c:struct:`nrf_dm_config_t` to configure the library.

#. To perform a measurement, call the :c:func:`nrf_dm_proc_execute` function passing the corresponding timeout as an argument.
   This call should be executed at the same time on both the reflector and the initiator, but the reflector must start first.

   You can meet this synchronization requirement by starting the reflector earlier and increasing the timeout value, at the cost of higher power consumption.

#. Load the raw data using the :c:func:`nrf_dm_populate_report` function.

#. Call the :c:func:`nrf_dm_calc` function or the :c:func:`nrf_dm_high_precision_calc` function to perform the calculation.
   On the nRF53 Series, do this on the application core for a reduced execution time.
   On the network core of the nRF53 Series, the calculations are performed without an FPU which results in a much higher execution time.

nRF53 Series
************

The Distance Measurement library comes with two precompiled libraries for the nRF53 Series.

* The :file:`libnrf_dm.a` library implements the interface specified in :file:`nrf_dm.h`.
* The :file:`libnrf_dm_calc.a` library partially implements the interface specified in :file:`nrf_dm.h`.

  It only includes an implementation for the functions :c:func:`nrf_dm_calc` and :c:func:`nrf_dm_high_precision_calc`.

The :file:`libnrf_dm_calc.a` library allows computing of the distance estimates on the application core.
On the network core, the calculation is done without an FPU, which results in a much higher execution time.

The libraries can be used as follows:

1. Perform the measurement on the network core as explained above.

#. On the network core, load the raw data using the :c:func:`nrf_dm_populate_report` function.

#. Transfer the populated report from the network core to the application core.

#. On the application core, call the :c:func:`nrf_dm_calc` function or the :c:func:`nrf_dm_high_precision_calc` function to perform the calculation.
