.. _lib_nrf_cloud_pgps:

nRF Cloud P-GPS
###############

.. contents::
   :local:
   :depth: 2

The nRF Cloud P-GPS library enables applications to request and process predicted GPS data from `nRF Cloud`_ to be used with the nRF9160 SiP.
This library is an enhancement to the :ref:`lib_nrf_cloud` library.
It can be used with or without Assisted GPS (`A-GPS`_) data from nRF Cloud.

P-GPS is intended for specific use cases.
It is not targeted for general use cases that already work with A-GPS.
P-GPS is designed for devices that are frequently disconnected from the cloud but need periodic GPS fixes as quickly as possible to save power.

To get a position fix, a GPS receiver needs information such as the satellite orbital data, exact date and time of the day, and accurate hardware clock frequency data.
GPS satellites broadcast this information in a pattern, which repeats every 12.5 minutes.
Predicted GPS data contains information about the estimated orbits (`Ephemerides <Ephemeris_>`_) of the 32 GPS satellites for up to a two-week period, with each set of ephemerides predictions being valid for a specific four-hour period within the set of all provided predictions.
These ephemeris predictions are downloaded from the cloud, stored by the device in flash memory, and later injected into the GPS unit when needed.

.. note::

   If two-week prediction sets are used, TTFF towards the end of the second week will increase due to accumulated errors in the predictions and decrease in number of satellite ephemerides in the later prediction periods.

If nRF Cloud services such as A-GPS or P-GPS are used either individually or in combination, the broadcasted information and predictions of satellite data can be downloaded at a faster rate from nRF Cloud than from the GPS satellites.

The use of P-GPS reduces Time to First Fix (TTFF) (time for a GPS device to estimate its position) when compared to using no assistance at all.
Further, it only requires a cloud connection approximately once a week, depending on configuration.
On the other hand, when only A-GPS is used, a cloud connection is needed each time.
If you use P-GPS along with A-GPS, TTFF is faster compared to an implementation with P-GPS only.
The amount of cloud data needed for each fix is smaller during each fix compared to an implementation with A-GPS only.
With proper configuration, A-GPS can be used with P-GPS, when a cloud connection is available, and acquire fast fixes even without a cloud connection.
This is possible as long as the stored P-GPS data is still valid, and current date and time (accurate to a few seconds) and the most recent location (accurate to a few dozen kilometers) are known.

.. note::
   To use the nRF Cloud P-GPS service, an nRF Cloud account is needed, and the device needs to be associated with a user's account.

Configuration
*************

Configure the following options to enable or disable the use of this library:

* :kconfig:`CONFIG_NRF_CLOUD`
* :kconfig:`CONFIG_NRF_CLOUD_PGPS`
* :kconfig:`CONFIG_NRF_CLOUD_MQTT` or :kconfig:`CONFIG_NRF_CLOUD_REST`

Configure these additional options to refine the behavior of P-GPS:

* :kconfig:`CONFIG_NRF_CLOUD_PGPS_PREDICTION_PERIOD`
* :kconfig:`CONFIG_NRF_CLOUD_PGPS_NUM_PREDICTIONS`
* :kconfig:`CONFIG_NRF_CLOUD_PGPS_REPLACEMENT_THRESHOLD`
* :kconfig:`CONFIG_NRF_CLOUD_PGPS_DOWNLOAD_FRAGMENT_SIZE`

Configure both of the following options if you need your application to use A-GPS as well, for coarse time and position data and to get the fastest TTFF:

* :kconfig:`CONFIG_NRF_CLOUD_AGPS`
* :kconfig:`CONFIG_AGPS`

If A-GPS is not desired (due to data costs, low power requirements, or expected frequent loss of cloud connectivity), both options listed above must be disabled.

For an application that uses P-GPS, the following options must be configured for storing settings, for having accurate clock time, and for having a location to store predictions:

* :kconfig:`CONFIG_FLASH`
* :kconfig:`CONFIG_FCB`
* :kconfig:`CONFIG_SETTINGS_FCB`
* :kconfig:`CONFIG_DATE_TIME`
* :kconfig:`CONFIG_BOOTLOADER_MCUBOOT`
* :kconfig:`CONFIG_IMG_MANAGER`
* :kconfig:`CONFIG_MCUBOOT_IMG_MANAGER`

See :ref:`configure_application` for information on how to change configuration options.

Initialization
**************

Ideally, once the device has connected to the cloud, the application must call the P-GPS initialization function.
If a connection is not available, initialization must still be called.
In this case, predictions will be unavailable if all valid predictions have expired, until a connection is established to the cloud in the future.

.. note::
   Each prediction requires 2 KB of flash. For prediction periods of 240 minutes (four hours), and with 42 predictions per week, the flash requirement adds up to 84 KB.

The P-GPS subsystem's :c:func:`nrf_cloud_pgps_init` function takes a pointer to a :c:struct:`nrf_cloud_pgps_init_param` structure.
The structure at a minimum must specify the storage base address and the storage size in flash, where P-GPS subsystem stores predictions.
It can optionally pass a pointer to a :c:func:`pgps_event_handler_t` callback function.

As an example, the :ref:`gnss_sample` sample shows how to pass the address of the :ref:`secondary MCUboot partition <mcuboot_ncs>`.
The address is defined by the ``PM_MCUBOOT_SECONDARY_ADDRESS`` macro and the ``PM_MCUBOOT_SECONDARY_SIZE`` macro.
These are automatically defined by the build system in the file :file:`pm_config.h`.
This partition is safe to store data until a FOTA job is received.
To avoid loss during FOTA, application developers can opt to store predictions in another location.

Time
****

The proper operation of the P-GPS subsystem depends on an accurate sense of time.
For use cases where a cloud connection can be established easily, use the :ref:`lib_date_time` library with NTP enabled.
Otherwise, a battery-backed real-time clock calendar chip must be used so that accurate time is available regardless of cloud availability after reset.

Requesting and processing P-GPS data
************************************

P-GPS data can be requested from the cloud using one of the following methods:

* Directly:

  * If :kconfig:`CONFIG_NRF_CLOUD_MQTT` is enabled:

   * Call the function :c:func:`nrf_cloud_pgps_request_all` to request a full set of predictions.
   * Pass a properly initialized :c:struct:`gps_pgps_request` structure to the :c:func:`nrf_cloud_pgps_request` function.

  * If :kconfig:`CONFIG_NRF_CLOUD_REST` is enabled:

   * Pass a properly initialized :c:struct:`nrf_cloud_rest_pgps_request` structure to the :c:func:`nrf_cloud_rest_pgps_data_get` function.

* Indirectly:

  * If :kconfig:`CONFIG_NRF_CLOUD_MQTT` is enabled:

   * Call :c:func:`nrf_cloud_pgps_init`, with no valid predictions present in flash, or with some or all of the predictions expired.
   * Call :c:func:`nrf_cloud_pgps_preemptive_updates`.
   * Call :c:func:`nrf_cloud_pgps_notify_prediction`.

  * If :kconfig:`CONFIG_NRF_CLOUD_REST` is enabled:

   * N/A

The indirect methods are used in the :ref:`asset_tracker_v2` application.
They are simpler to use than the direct methods.
The direct method is used in the :ref:`gnss_sample` sample.

When nRF Cloud responds with the requested P-GPS data, the application's :c:func:`cloud_evt_handler_t` function must call the :c:func:`nrf_cloud_pgps_process` function when it receives the :c:enum:`CLOUD_EVT_DATA_RECEIVED` event.
The function parses the data and stores it.

Finding a prediction and injecting to modem
*******************************************

A P-GPS prediction for the current date and time can be retrieved using one of the following methods:

* Directly, by calling the function :c:func:`nrf_cloud_pgps_find_prediction`
* Indirectly, by calling the function :c:func:`nrf_cloud_pgps_notify_prediction`

The indirect method is used in the :ref:`gnss_sample` sample and in the :ref:`asset_tracker_v2` application.

The application can inject the data contained in the prediction to the GPS unit in the modem by calling the :c:func:`nrf_cloud_pgps_inject` function.
This must be done when the GPS driver callback indicates that assistance is needed.

A prediction is also automatically injected to the modem every four hours whenever the current prediction expires and the next one begins (if the next one is available in flash).

Interaction with the GPS driver
*******************************

The P-GPS subsystem, like several other nRF Cloud subsystems, is event driven.

Following are the two GPS events relating to P-GPS that an application receives through the GPS driver callback:

* :c:enumerator:`GPS_EVT_AGPS_DATA_NEEDED` - Occurs when the GPS module requires assistance data.
* :c:enumerator:`GPS_EVT_PVT_FIX` - Occurs once a fix is attained.

When the application receives the :c:enumerator:`GPS_EVT_AGPS_DATA_NEEDED` event, it must call :c:func:`nrf_cloud_pgps_notify_prediction`.
This event results in the call back of the application's :c:func:`pgps_event_handler_t` function when a valid P-GPS prediction set is available.
It will pass the :c:enum:`PGPS_EVT_AVAILABLE` event and a pointer to :c:struct:`nrf_cloud_pgps_prediction` to the handler.

The application must pass this prediction to :c:func:`nrf_cloud_pgps_inject`, along with either the :c:struct:`gps_agps_request` passed to the GPS driver callback earlier with the :c:enumerator:`GPS_EVT_AGPS_DATA_NEEDED` event or NULL.

If the use case for the application is such that the device will not move distances greater than a few dozen kilometers before it gets a new GPS fix, it can pass the latitude and longitude provided in :c:enumerator:`GPS_EVT_PVT_FIX` to :c:func:`nrf_cloud_pgps_set_location`.
The P-GPS subsystem will use this stored location for the next GPS request for position assistance when A-GPS assistance is not enabled or is unavailable.
If the use case involves possible long-distance travel between fix attempts, such a mechanism can be detrimental to short TTFF, as the saved position might be too inaccurate to be a benefit.

The application can also call :c:func:`nrf_cloud_pgps_preemptive_updates` to discard expired predictions and replace them with newer ones, prior to the expiration of the entire set of predictions.
This can be useful for customer use cases where cloud connections are available infrequently.
The :kconfig:`CONFIG_NRF_CLOUD_PGPS_REPLACEMENT_THRESHOLD` sets the minimum number of valid predictions remaining before such an update occurs.

For best performance, applications can call the P-GPS functions mentioned in this section from workqueue handlers rather than directly from various callback functions.

The P-GPS subsystem itself generates events that can be passed to a registered callback function.
See :c:enum:`nrf_cloud_pgps_event_type`.

API documentation
*****************

| Header file: :file:`include/net/nrf_cloud_pgps.h`
| Source files: :file:`subsys/net/lib/nrf_cloud/src/`

.. doxygengroup:: nrf_cloud_pgps
   :project: nrf
   :members:
