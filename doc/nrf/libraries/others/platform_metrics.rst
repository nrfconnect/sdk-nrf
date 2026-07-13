.. _platform_metrics:

Platform metrics
################

.. contents::
   :local:
   :depth: 2

The platform metrics library retrieves and stores platform operating condition data.
Other subsystems can use this data to trigger recalibration, reconfiguration, or other adaptive behavior.

For example, a Wi-Fi® driver can use battery voltage and die temperature data to trigger recalibration of the Wi-Fi radio.

The library maintains a periodic snapshot of the following two channels:

* Battery voltage - Supply voltage in millivolts (mV).
* Die temperature - SoC die temperature in centi-degrees Celsius (centi-degC); for example, ``2500`` represents 25.00°C.

A board-level constant such as a crystal (XO) frequency offset is not modeled as a channel here, since it does not drift at runtime.
Consumers that need such a constant should read it directly from its own devicetree property instead.

Configuration
*************

Enable the monitoring system and use Kconfig options to select either live or fixed channel data.

Monitoring system
=================

To enable monitoring and storage of snapshots, use the :kconfig:option:`CONFIG_PLATFORM_METRICS` Kconfig option.
The rate for caching the snapshots is configured using the :kconfig:option:`CONFIG_PLATFORM_METRICS_SNAPSHOT_INTERVAL_MS` Kconfig option.
If no channels are configured for live updates, the defaults are stored in the snapshot cache and no work queue items are created.

Live channel capture
====================

Each channel can capture new data asynchronously from the monitoring system's snapshot cache rate.
This allows other processes to use the captured data at higher rates, if required.
Currently, only die temperature supports live data reading.
Live data capture for battery voltage is currently not supported.

To enable live data capture, valid ``init()`` and ``sample()`` functions must be provided to ``PLATFORM_METRICS_CHANNEL_DEFINE`` and enabled using, for example, the :kconfig:option:`CONFIG_PLATFORM_METRICS_DIE_TEMP_MONITOR` Kconfig option.

* ``init()``:

    * Called once during ``SYS_INIT`` at application priority.
    * Brings up any hardware or state needed for sampling.
    * Returns ``0`` on success, or a negative ``errno`` value on failure.
    * On failure, the channel falls back to its default value.

* ``sample(out)``:

    * Fills ``out`` with the latest reading for the channel.
    * Sets ``out->type`` to match the value field used.
    * Sets ``out->status`` to ``PLATFORM_METRICS_STATUS_OK`` when the reading is valid, or ``PLATFORM_METRICS_STATUS_ERROR``/ ``PLATFORM_METRICS_STATUS_UNINITIALISED`` otherwise.
    * Updates ``out->timestamp_ms`` with ``k_uptime_get()`` when appropriate.
    * Returns ``0`` on success, or a negative ``errno`` value on failure.

Die temperature
---------------

Use the following Kconfig options to enable and configure the die temperature monitoring:

* :kconfig:option:`CONFIG_PLATFORM_METRICS_DIE_TEMP_MONITOR`
* :kconfig:option:`CONFIG_PLATFORM_METRICS_DIE_TEMP_MONITOR_INTERVAL_MS`

.. note::
   Live data capture for battery voltage is currently not supported.

Default values
==============

When a channel does not have live monitoring enabled, the snapshot holds a compile-time default value.
If no channels use live updates, no work queue items are created and the default values are held in the snapshot cache.
For the default values, see the following Kconfig options:

* :kconfig:option:`CONFIG_PLATFORM_METRICS_BATTERY_VOLTAGE_DEFAULT_VALUE`
* :kconfig:option:`CONFIG_PLATFORM_METRICS_DIE_TEMP_DEFAULT_VALUE`

Providers
=========

A channel's live data comes from a provider consisting of a pair of ``init()`` and ``sample()`` functions registered with :c:macro:`PLATFORM_METRICS_CHANNEL_DEFINE`.
A board registers at most one provider per channel.
Since :c:macro:`PLATFORM_METRICS_CHANNEL_DEFINE` emits a unique symbol for each channel ID, registering two providers for the same channel causes the build to fail with a linker "multiple definition" error rather than silently racing at runtime.
There are two ways to provide a provider:

Devicetree-selected sensor provider
-----------------------------------

For components that already have a Zephyr sensor driver, no custom C code is required.
The die temperature channel's built-in provider (:file:`temperature_monitor.c`) reads data through the RTIO-based sensor API (``sensor_read()`` and :kconfig:option:`CONFIG_SENSOR_ASYNC_API`) from the device referenced by a devicetree chosen node. This also works with sensor drivers that only implement the RTIO ``submit()`` path and not the legacy ``sensor_sample_fetch()``/``sensor_channel_get()`` API, since Zephyr's sensor subsystem falls back to the legacy API internally for drivers that only implement it, and to the native RTIO path otherwise:

.. code-block:: devicetree

   chosen {
       nordic,platform-metrics-die-temp-sensor = &my_ambient_temp_sensor;
   };

If no ``nordic,platform-metrics-die-temp-sensor`` chosen node is present, the provider falls back to the SoC's internal die temperature sensor (nodelabel ``temp``).
As a result, it works out of the box on the DK without any devicetree changes.
Enable it with :kconfig:option:`CONFIG_PLATFORM_METRICS_DIE_TEMP_MONITOR`.

Custom registered provider
--------------------------

For anything that is not sensor-API-shaped, such as a proprietary fuel-gauge library, a raw register read, or a vendor SDK call, write a provider file and register it directly.
The provider's internals do not depend on the sensor subsystem, only the ``init()`` and ``sample()`` function signatures matter to this library:

.. code-block:: c

   #include <platform_metrics.h>

   static int my_provider_init(void)
   {
       /* Bring up whatever HW/library this needs. */
       return 0;
   }

   static int my_provider_sample(struct platform_metrics_sample *out)
   {
       out->type = PLATFORM_METRICS_SAMPLE_TYPE_INT;
       out->value.i32 = /* reading, in the channel's canonical unit */;
       out->timestamp_ms = k_uptime_get();
       out->status = PLATFORM_METRICS_STATUS_OK;
       return 0;
   }

   PLATFORM_METRICS_CHANNEL_DEFINE(my_channel_provider, PLATFORM_METRICS_CH_BATTERY_VOLTAGE,
                                    my_provider_sample, my_provider_init,
                                    PLATFORM_METRICS_SAMPLE_TYPE_INT, i32, 3300);

Gate the registration behind your own Kconfig option, and disable any built-in provider for the same channel (for example, :kconfig:option:`CONFIG_PLATFORM_METRICS_DIE_TEMP_MONITOR`).

API documentation
*****************

| Header file: :file:`include/platform_metrics.h`
| Source files: :file:`subsys/platform_metrics/platform_metrics_capture.c`

.. doxygengroup:: platform_metrics
