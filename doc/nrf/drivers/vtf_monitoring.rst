.. _vtf_monitoring:

VTF monitoring driver
#####################

.. contents::
   :local:
   :depth: 2

The VTF (Voltage / Temperature / Frequency) monitoring driver fetches and stores data that the nRF Wi-Fi driver uses to trigger re-calibrations / re-configurations of the Wi-Fi subsystem.


The driver maintains a periodic snapshot of three channels:

* **Battery voltage** — supply voltage in millivolts (mV).
* **Die temperature** — SoC die temperature in centi-degrees Celsius (centi-degC); for example, ``2500`` represents 25.00 °C.
* **XO frequency offset** — crystal oscillator frequency offset in parts per million (ppm).


Configuration
*************

Enable the monitoring system and select live or fixed channel data through Kconfig options.


Monitoring System
================

To enable monitoring and storage of snapshots in `vtf_snapshots` use :kconfig:option:`CONFIG_VTF_MONITORING`. 
The rate at which snapshots are cached is configured with :kconfig:option:`CONFIG_VTF_SNAPSHOT_INTERVAL_MS`. For optimal Wi-Fi performance this should not be more than 10000ms.
If no channels are configured for live updates the defaults are stored in `vtf_snapshots` and no work queue items are created.

Live Channel Capture
===================

Each channel can capture new data asynchronously to the monitoring systems snapshot cache rate. This allows other processes to make use of the captured data at faster rates if required.
Currently, only die temperature supports live data reading. Battery voltage and frequency offset live data capture will be added at a later date.


To enable live data capture, valid `init()` and `sample()` functions must be provided to VTF_CHANNEL_DEFINE and enabled via Kconfig e.g. :kconfig:option:`CONFIG_DIE_TEMP_MONITOR`.

``init()``
  Called once during ``SYS_INIT`` at application priority.
  Bring up any hardware or state needed for sampling.
  Return ``0`` on success, or a negative ``errno`` value on failure.
  A failed init causes the channel to fall back to its default value.

``sample(out)``
  Fill *out* with the latest reading for the channel.
  Set ``out->type`` to match the value field used.
  Set ``out->status`` to ``VTF_STATUS_OK`` when the reading is valid, or ``VTF_STATUS_ERROR`` / ``VTF_STATUS_UNINITIALISED`` otherwise.
  Update ``out->timestamp_ms`` with ``k_uptime_get()`` when appropriate.
  Return ``0`` on success, or a negative ``errno`` value on failure.


Die Temperature
--------------

:kconfig:option:`CONFIG_CONFIG_DIE_TEMP_MONITOR`
  Enable reference implementation live die-temperature monitoring using the on-chip temperature sensor.
  Selects :kconfig:option:`CONFIG_VTF_MONITORING`, :kconfig:option:`CONFIG_SENSOR`, and :kconfig:option:`CONFIG_TEMP_NRF5`.

:kconfig:option:`CONFIG_CONFIG_DIE_TEMP_MONITOR_INTERVAL_MS`
  Interval between die-temperature sensor reads, in milliseconds.
  Default: ``10000``.
  Available when :kconfig:option:`CONFIG_DIE_TEMP_MONITOR` is set.

Battery Voltage
--------------

Live data capture will be added at a later date.

Frequency Offset
---------------

Live data capture will be added at a later date.

Default values
==============

When a channel does not have live monitoring enabled, the snapshot holds a compile-time default.
If no channels use live updates, no work queue items are created and the defaults are held in `vtf_snapshots`.

:kconfig:option:`CONFIG_VTF_BATTERY_VOLTAGE_DEFAULT_VALUE`
  Default battery voltage in mV.
  Default: ``3300`` (3.3 V).

:kconfig:option:`CONFIG_VTF_DIE_TEMP_DEFAULT_VALUE`
  Default die temperature in centi-degC.
  Default: ``2500`` (25.00 °C).

:kconfig:option:`CONFIG_VTF_FREQ_OFFSET_DEFAULT_VALUE`
  Default XO frequency offset in ppm.
  Default: ``100``.


Custom channel backends
=======================

Whilst each channel has a reference implementation e.g. :file:`temperature_monitor.c`.
In the event the application needs to make use of one of these channels beyond the requirements for Wi-Fi, applications can replace any channel backend by supplying their own stronger ``init()`` and ``sample()`` functions.

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
| Die-temperature header: :file:`drivers/vtf_monitoring/temperature_monitor.h`
| Source files: :file:`drivers/vtf_monitoring/vtf_capture.c`, :file:`drivers/vtf_monitoring/temperature_monitor.c`
