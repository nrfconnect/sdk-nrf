.. _wifi_recal_monitor:

Wi-Fi recalibration monitor driver
###################################

.. contents::
   :local:
   :depth: 2

The Wi-Fi recalibration monitor driver fetches and stores operating-condition data that the nRF Wi-Fi driver uses to trigger re-calibrations or re-configurations of the Wi-Fi® subsystem.

The driver maintains a periodic snapshot of the following two channels:

* Battery voltage - Supply voltage in millivolts (mV).
* Die temperature - SoC die temperature in centi-degrees Celsius (centi-degC); for example, ``2500`` represents 25.00°C.

XO frequency offset is a per-board-design constant, not something that drifts at runtime, so it is not modeled as a channel here: the Wi-Fi driver reads it directly from a devicetree property on the Wi-Fi node instead.

Configuration
*************

Enable the monitoring system and select live or fixed channel data through Kconfig options.

Monitoring system
=================

To enable monitoring and storage of snapshots, use the :kconfig:option:`CONFIG_WIFI_RECAL_MONITOR` Kconfig option.
The rate for caching the snapshots is configured using the :kconfig:option:`CONFIG_WIFI_RECAL_SNAPSHOT_INTERVAL_MS` Kconfig option.
For optimal Wi-Fi performance, this should not be more than 10000 ms.
If no channels are configured for live updates, the defaults are stored in the snapshot cache and no work queue items are created.

Live channel capture
====================

Each channel can capture new data asynchronously from the monitoring system's snapshot cache rate.
This allows other processes to use the captured data at higher rates, if required.
Currently, only die temperature supports live data reading.
Battery voltage live data capture will be added later.

To enable live data capture, valid ``init()`` and ``sample()`` functions must be provided to ``WIFI_RECAL_CHANNEL_DEFINE`` and enabled using, for example, the :kconfig:option:`CONFIG_WIFI_RECAL_DIE_TEMP_MONITOR` Kconfig option.

* ``init()``:

    * Called once during ``SYS_INIT`` at application priority.
    * Brings up any hardware or state needed for sampling.
    * Returns ``0`` on success, or a negative ``errno`` value on failure.
    * On failure, the channel falls back to its default value.

* ``sample(out)``:

    * Fills ``out`` with the latest reading for the channel.
    * Sets ``out->type`` to match the value field used.
    * Sets ``out->status`` to ``WIFI_RECAL_STATUS_OK`` when the reading is valid, or ``WIFI_RECAL_STATUS_ERROR``/ ``WIFI_RECAL_STATUS_UNINITIALISED`` otherwise.
    * Updates ``out->timestamp_ms`` with ``k_uptime_get()`` when appropriate.
    * Returns ``0`` on success, or a negative ``errno`` value on failure.

Die temperature
---------------

Use the following Kconfig options to enable and configure the die temperature monitoring:

* :kconfig:option:`CONFIG_WIFI_RECAL_DIE_TEMP_MONITOR`
* :kconfig:option:`CONFIG_WIFI_RECAL_DIE_TEMP_MONITOR_INTERVAL_MS`

.. note::
   Live data capture for battery voltage will be added at a later date.

Default values
==============

When a channel does not have live monitoring enabled, the snapshot holds a compile-time default value.
If no channels use live updates, no work queue items are created and the default values are held in the snapshot cache.
For the default values, see the following Kconfig options:

* :kconfig:option:`CONFIG_WIFI_RECAL_BATTERY_VOLTAGE_DEFAULT_VALUE`
* :kconfig:option:`CONFIG_WIFI_RECAL_DIE_TEMP_DEFAULT_VALUE`

Providers
=========

A channel's live data comes from a *provider*: a pair of ``init()``/``sample()`` functions registered with :c:macro:`WIFI_RECAL_CHANNEL_DEFINE`.
A board registers at most one provider per channel; :c:macro:`WIFI_RECAL_CHANNEL_DEFINE` emits a unique symbol per channel ID, so registering two providers for the same channel fails the build with a linker "multiple definition" error rather than silently racing at runtime.
There are two ways to provide one:

Devicetree-selected sensor provider
------------------------------------

For components that already have a Zephyr sensor driver, no custom C is needed.
The die temperature channel's built-in provider (:file:`temperature_monitor.c`) reads via the standard sensor API (``sensor_sample_fetch()``/``sensor_channel_get()``) from whichever device a devicetree ``chosen`` node points to:

.. code-block:: devicetree

   chosen {
       nordic,wifi-recal-die-temp-sensor = &my_ambient_temp_sensor;
   };

If no ``nordic,wifi-recal-die-temp-sensor`` chosen node is present, the provider falls back to the SoC's own die temperature sensor (nodelabel ``temp``), which is why this works out of the box on the DK with no devicetree changes.
Enable it with :kconfig:option:`CONFIG_WIFI_RECAL_DIE_TEMP_MONITOR`.

Custom registered provider
---------------------------

For anything that is not sensor-API-shaped (a proprietary fuel-gauge library, a raw register read, a vendor SDK call, and so on), write a plain provider file and register it directly.
The provider's internals owe nothing to the sensor subsystem; only the ``init()``/``sample()`` signatures matter to this driver:

.. code-block:: c

   #include <drivers/wifi_recal_monitor/wifi_recal_monitor.h>

   static int my_provider_init(void)
   {
       /* Bring up whatever HW/library this needs. */
       return 0;
   }

   static int my_provider_sample(struct wifi_recal_sample *out)
   {
       out->type = WIFI_RECAL_SAMPLE_TYPE_INT;
       out->value.i32 = /* reading, in the channel's canonical unit */;
       out->timestamp_ms = k_uptime_get();
       out->status = WIFI_RECAL_STATUS_OK;
       return 0;
   }

   WIFI_RECAL_CHANNEL_DEFINE(my_channel_provider, WIFI_RECAL_CH_BATTERY_VOLTAGE, my_provider_sample,
                      my_provider_init, WIFI_RECAL_SAMPLE_TYPE_INT, i32, 3300);

Gate the registration behind your own Kconfig option, and make sure any built-in provider for the same channel (for example :kconfig:option:`CONFIG_WIFI_RECAL_DIE_TEMP_MONITOR`) stays disabled.

API documentation
*****************

| Header file: :file:`drivers/wifi_recal_monitor/wifi_recal_monitor.h`
| Source files: :file:`drivers/wifi_recal_monitor/wifi_recal_capture.c`

.. doxygengroup:: wifi_recal_monitor
