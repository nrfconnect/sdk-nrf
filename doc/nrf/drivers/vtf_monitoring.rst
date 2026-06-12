.. _vtf_monitoring:

VTF monitoring driver
#####################

.. contents::
   :local:
   :depth: 2

The VTF (Voltage-Temperature-Frequency) monitoring driver fetches and stores data that the nRF Wi-Fi driver uses to trigger re-calibrations or re-configurations of the Wi-Fi® subsystem.

The driver maintains a periodic snapshot of the following three channels:

* Battery voltage - Supply voltage in millivolts (mV).
* Die temperature - SoC die temperature in centi-degrees Celsius (centi-degC); for example, ``2500`` represents 25.00°C.
* XO frequency offset - Crystal oscillator frequency offset in parts per million (ppm).

Configuration
*************

Enable the monitoring system and select live or fixed channel data through Kconfig options.

Monitoring system
=================

To enable monitoring and storage of snapshots in ``vtf_snapshots``, use the :kconfig:option:`CONFIG_VTF_MONITORING` Kconfig option.
The rate for caching the snapshots is configured using the :kconfig:option:`CONFIG_VTF_SNAPSHOT_INTERVAL_MS` Kconfig option.
For optimal Wi-Fi performance, this should not be more than 10000 ms.
If no channels are configured for live updates, the defaults are stored in ``vtf_snapshots`` and no work queue items are created.

Live channel capture
====================

Each channel can capture new data asynchronously from the monitoring system's snapshot cache rate.
This allows other processes to use the captured data at higher rates, if required.
Currently, only die temperature supports live data reading.
Battery voltage and frequency offset live data capture will be added later.

To enable live data capture, valid ``init()`` and ``sample()`` functions must be provided to ``VTF_CHANNEL_DEFINE`` and enabled using, for example, the :kconfig:option:`CONFIG_VTF_DIE_TEMP_MONITOR` Kconfig option.

* ``init()``:

    * Called once during ``SYS_INIT`` at application priority.
    * Brings up any hardware or state needed for sampling.
    * Returns ``0`` on success, or a negative ``errno`` value on failure.
    * On failure, the channel falls back to its default value.

* ``sample(out)``:

    * Fills ``out`` with the latest reading for the channel.
    * Sets ``out->type`` to match the value field used.
    * Sets ``out->status`` to ``VTF_STATUS_OK`` when the reading is valid, or ``VTF_STATUS_ERROR``/ ``VTF_STATUS_UNINITIALISED`` otherwise.
    * Updates ``out->timestamp_ms`` with ``k_uptime_get()`` when appropriate.
    * Returns ``0`` on success, or a negative ``errno`` value on failure.

Die temperature
---------------

Use the following Kconfig options to enable and configure the die temperature monitoring:

* :kconfig:option:`CONFIG_VTF_DIE_TEMP_MONITOR`
* :kconfig:option:`CONFIG_VTF_DIE_TEMP_MONITOR_INTERVAL_MS`

.. note::
   Live data capture for battery voltage and frequency offset will be added at a later date.

Default values
==============

When a channel does not have live monitoring enabled, the snapshot holds a compile-time default value.
If no channels use live updates, no work queue items are created and the default values are held in ``vtf_snapshots``.
For the default values, see the following Kconfig options:

* :kconfig:option:`CONFIG_VTF_BATTERY_VOLTAGE_DEFAULT_VALUE`
* :kconfig:option:`CONFIG_VTF_DIE_TEMP_DEFAULT_VALUE`
* :kconfig:option:`CONFIG_VTF_FREQ_OFFSET_DEFAULT_VALUE`

Custom channel backends
=======================

Each channel has a reference implementation, for example, :file:`temperature_monitor.c`.
If the application needs to use one of these channels beyond the requirements for Wi-Fi, applications can replace any channel backend by supplying their own stronger ``init()`` and ``sample()`` functions.

.. code-block:: c

   #include "temperature_monitor.h"

   int die_temp_init(void)
   {
       /* Configure sensor and fetch data regularly */
       return 0;
   }

   int die_temp_sample(struct vtf_sample *out)
   {
       out->type = VTF_SAMPLE_TYPE_INT;
       out->value.i32 = /* centi-degC */;
       out->timestamp_ms = k_uptime_get();
       out->status = VTF_STATUS_OK;
       return 0;
   }

API documentation
*****************

| Header file: :file:`drivers/vtf_monitoring/vtf_monitoring.h`
| Source files: :file:`drivers/vtf_monitoring/vtf_capture.c`

.. doxygengroup:: vtf_monitoring
