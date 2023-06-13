.. _lib_nrf_cloud_pgps:

nRF Cloud P-GPS
###############

.. contents::
   :local:
   :depth: 2

The nRF Cloud P-GPS library enables applications to request and process :term:`Predicted GPS (P-GPS)` data from `nRF Cloud`_ to be used with the nRF9160 SiP.
This library is an enhancement to the :ref:`lib_nrf_cloud` library.
It can be used with or without :term:`Assisted GPS (A-GPS)` data from nRF Cloud.

Overview
********

To get a position fix, a :term:`Global Navigation Satellite System (GNSS)` module needs information such as the satellite orbital data broadcasted by the satellites.

Predicted GPS (P-GPS) is a form of assistance that reduces the :term:`Time to First Fix (TTFF)`, the time needed by a GNSS module to estimate its position.
It is provided through :term:`nRF Cloud` services.
In P-GPS, nRF Cloud provides data containing information about the estimated orbits (`Ephemerides <Ephemeris_>`_) of the 32 GPS satellites for up to two weeks.
Each set of ephemerides predictions is valid for a specific four-hour period within the set of all provided predictions.
A device using P-GPS downloads the ephemeris predictions from the cloud, stores them in its flash memory, and later injects them into the GNSS module when needed.

P-GPS is designed for devices that are frequently disconnected from the cloud but need periodic GNSS fixes as quickly as possible to save power.
This is possible because a device does not need to download ephemerides from the satellite broadcast.
However, P-GPS should not be used for general use cases that already work with :term:`Assisted GPS (A-GPS)` only.

.. note::
   When using two-week ephemeris prediction sets, the TTFF towards the end of the second week increases due to the accumulated errors in the predictions and the decreases in the number of satellite ephemerides in the later prediction periods.

P-GPS requires a cloud connection approximately once a week to download new predictions, depending on the configuration settings.
With A-GPS, new ephemerides are needed on average every two hours, or if the fix interval is longer, whenever GNSS is started.

.. note::
   To use the nRF Cloud P-GPS service, you need an nRF Cloud account, and the device needs to be associated with the account.

Configuration
*************

Configure the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS` Kconfig option to enable or disable the use of this library.

Configure one of the following options to control the network transport for P-GPS requests and responses:

* :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_TRANSPORT_MQTT`
* :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_TRANSPORT_NONE`

Configure one of the following options to control the network transport for downloading P-GPS predictions:

* :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_DOWNLOAD_TRANSPORT_HTTP`
* :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_DOWNLOAD_TRANSPORT_CUSTOM`

Configure these additional options to refine the behavior of P-GPS:

* :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_PREDICTION_PERIOD`
* :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_NUM_PREDICTIONS`
* :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_REPLACEMENT_THRESHOLD`
* :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_DOWNLOAD_FRAGMENT_SIZE`
* :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_REQUEST_UPON_INIT`

Configure the :kconfig:option:`CONFIG_NRF_CLOUD_AGPS` option if you need your application to also use A-GPS, for time and coarse position data and to get the fastest TTFF.
Using A-GPS also improves the accuracy because of ionospheric corrections.

.. note::
   Disable this option if you do not want to use A-GPS (due to data costs or expected frequent loss of cloud connectivity).

You must also configure the following options for storing settings, for having accurate clock time, and for having a location to store predictions:

* :kconfig:option:`CONFIG_FLASH`
* :kconfig:option:`CONFIG_FCB`
* :kconfig:option:`CONFIG_SETTINGS_FCB`
* :kconfig:option:`CONFIG_DATE_TIME`

The P-GPS library requires a storage location in the flash memory for storing the P-GPS prediction data.
There are three ways to define this storage location:

* To use a dedicated partition, enable the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_STORAGE_PARTITION` option.

  By default, this partition is stored in the main SoC flash.
  This partition can optionally be located in external flash for the nRF9160 development kit version 0.14.0 and later.
  This conserves space in the main flash for storing code or other data.

  To use an external flash partition, enable the following options in your project's configuration file or place them in a configuration overlay file (for example :file:`samples/cellular/nrf_cloud_mqtt_multi_service/overlay_pgps_ext_flash.conf`):

  * :kconfig:option:`CONFIG_SPI`
  * :kconfig:option:`CONFIG_SPI_NOR`
  * :kconfig:option:`CONFIG_PM_OVERRIDE_EXTERNAL_DRIVER_CHECK`
  * :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_STORAGE_PARTITION`
  * :kconfig:option:`CONFIG_PM_PARTITION_REGION_PGPS_EXTERNAL`
  * :kconfig:option:`CONFIG_SPI_NOR_FLASH_LAYOUT_PAGE_SIZE` set to 4096

  If you are using the P-GPS external flash partition and full modem FOTA, ensure the FMFU partition is also enabled:

  * :kconfig:option:`CONFIG_DFU_TARGET_FULL_MODEM_USE_EXT_PARTITION`

  Also, specify your development kit version by appending it to the board name.
  For example, if your development kit version is 1.0.1, use the board name ``nrf9160dk_nrf9160_ns@1_0_1`` in your build command.

  .. note::
     If you fail to specify the version, the build system selects a baseline board version which does not include external flash.
     This results in a build failure:

     .. parsed-literal::
        :class: highlight

        devicetree error: /chosen: undefined node label 'mx25r64'

  Finally, add the following to a device tree overlay for your board.

  .. code-block:: console

     / {
       chosen {
         nordic,pm-ext-flash = &mx25r64;
       };
     };

     /* Enable high drive mode for the SPI3 pins to get a square signal at 8 MHz */
     &spi3_default {
       group1 {
         nordic,drive-mode = <NRF_DRIVE_H0H1>;
       };
     };

  This is typically placed in a file within your application's source folder in a :file:`boards` subfolder.
  See an example provided in the file :file:`samples/cellular/nrf_cloud_mqtt_multi_service/boards/nrf9160dk_nrf9160_ns_0_14_0.overlay`.

* To use the MCUboot secondary partition as storage, enable the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_STORAGE_MCUBOOT_SECONDARY` option.

  Use this option if the flash memory for your application is too full to use a dedicated partition, and the application uses MCUboot for FOTA updates but not for MCUboot itself.

  Do not use this option if you are using MCUboot as a second-stage upgradable bootloader and also have FOTA updates enabled for MCUboot itself, not just the application (using :kconfig:option:`CONFIG_SECURE_BOOT` and :kconfig:option:`CONFIG_BUILD_S1_VARIANT`).
  Otherwise, the P-GPS library prevents the full completion of MCUboot update, and the first-stage immutable bootloader reverts MCUboot to its previous image.

* To use an application-specific storage, enable the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_STORAGE_CUSTOM` option.

  You must also pass the address and the size of your custom location in the flash memory to the :c:func:`nrf_cloud_pgps_init` function.

  .. note::
     The address must be aligned to a flash page boundary, and the size must be equal to or greater than 2048 bytes times the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_NUM_PREDICTIONS` option.

  Use this option if you do not use MCUboot and you want complete control over the storing location of P-GPS data in the flash memory.

See :ref:`configure_application` for information on how to change configuration options.

Usage
*****

A device can use P-GPS together with A-GPS.
This provides the following advantages:

* It shortens TTFF compared to using only P-GPS.
* A-GPS provides ionospheric correction data, which improves accuracy.

In particular, A-GPS is beneficial when the device is powered on.
Getting GPS system time and coarse location using A-GPS can significantly shorten the time needed for the first fix.
As long as the modem remains powered on, GNSS "knows" the previous location and the current time with sufficient accuracy.

To improve accuracy, ionospheric correction data can also be downloaded periodically using A-GPS.
If used, the ionospheric correction data should be updated at least every 24 hours.

There should be no need to download almanacs with A-GPS, because it is assumed that the device has valid ephemerides (predictions) for all satellites available at all times.
Because ephemerides and almanacs do not need to be downloaded with A-GPS, the amount of data is very small, less than 200 bytes.

Initialization
==============

Ideally, once the device has connected to the cloud, the application must call the P-GPS initialization function.
If a connection is not available, initialization must still be called.
If the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_REQUEST_UPON_INIT` option is disabled, the initialization function does not automatically download missing P-GPS data.
In these cases, predictions might be unavailable until a connection is established to the cloud.

.. note::
   Each prediction requires 2 kB of flash.
   For prediction period of 240 minutes (four hours), and with 42 predictions in a week, the flash requirement adds up to 84 kB.

The P-GPS subsystem's :c:func:`nrf_cloud_pgps_init` function takes a pointer to a :c:struct:`nrf_cloud_pgps_init_param` structure.
If the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_STORAGE_CUSTOM` option is enabled, the structure must specify the storage base address and the storage size in the flash memory where the P-GPS subsystem stores predictions.
It can optionally pass a pointer to a :c:func:`pgps_event_handler_t` callback function.

.. note::
   The storage base address must be aligned to the flash memory page boundary.

Time
====

The proper operation of the P-GPS subsystem depends on an accurate sense of time.
For use cases where a cloud connection can be established easily, use the :ref:`lib_date_time` library with NTP enabled.
Otherwise, use a battery-backed real-time clock calendar chip so that accurate time is available regardless of cloud availability after reset.

Transport mechanisms
====================

Complete these three steps to request and store P-GPS data in the device:

1. Send the request to `nRF Cloud`_.
#. Receive a URL from nRF Cloud pointing to the prediction set.
#. Download the predictions from the URL.

The first two steps use the network transport defined by the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_TRANSPORT` option.

The default configuration selects the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_TRANSPORT_MQTT` option if the :kconfig:option:`CONFIG_NRF_CLOUD_MQTT` option is active.
MQTT use is built into the P-GPS library.

The library uses REST or other transports by means of the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_TRANSPORT_NONE` option.
REST support is not built into the P-GPS library and must be provided by the application.
The :ref:`gnss_sample` sample is one example of using REST.

The third step uses the network transport defined by the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_DOWNLOAD_TRANSPORT` option.

The P-GPS library uses the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_DOWNLOAD_TRANSPORT_HTTP` option when using MQTT for the main transport.
Applications that use REST for the main transport usually use the HTTP option for the download transport.
This download transport is built into the P-GPS library.

Alternatively, use the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_DOWNLOAD_TRANSPORT_CUSTOM` with :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_TRANSPORT_NONE` to manage the full flow of data outside of the P-GPS library.
Call the following functions when using this configuration:

1. :c:func:`nrf_cloud_pgps_begin_update`
#. :c:func:`nrf_cloud_pgps_process_update`
#. :c:func:`nrf_cloud_pgps_finish_update`

Requesting and processing P-GPS data
====================================

The library offers two different ways to control the timing of P-GPS cloud requests:

* Direct

  * If :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_TRANSPORT_MQTT` is enabled:

    * Call the function :c:func:`nrf_cloud_pgps_request_all` to request a full set of predictions.
    * Alternatively, pass a properly initialized :c:struct:`gps_pgps_request` structure to the :c:func:`nrf_cloud_pgps_request` function.

  * If :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_TRANSPORT_NONE` and :kconfig:option:`CONFIG_NRF_CLOUD_REST` are enabled:

    * Pass a properly initialized :c:struct:`nrf_cloud_rest_pgps_request` structure to the :c:func:`nrf_cloud_rest_pgps_data_get` function.
    * Pass the response to the :c:func:`nrf_cloud_pgps_process` function.
    * If either call fails, call the :c:func:`nrf_cloud_pgps_request_reset` function.

* Indirect

   * Call :c:func:`nrf_cloud_pgps_init`.
   * Call :c:func:`nrf_cloud_pgps_preemptive_updates`.
   * Call :c:func:`nrf_cloud_pgps_notify_prediction`.

The indirect methods are used in the :ref:`asset_tracker_v2` application.
They are simpler to use than the direct methods.
The direct method is used in the :ref:`gnss_sample` sample.

When nRF Cloud responds with the requested P-GPS data, the library sends the :c:enum:`CLOUD_EVT_DATA_RECEIVED` event.
The application's :c:func:`cloud_evt_handler_t` function receives this event.
The handler calls the :c:func:`nrf_cloud_pgps_process` function that parses the data and stores it.

Finding a prediction and injecting to modem
===========================================

A P-GPS prediction for the current date and time can be retrieved using one of the following methods:

* Directly, by calling the function :c:func:`nrf_cloud_pgps_find_prediction`
* Indirectly, by calling the function :c:func:`nrf_cloud_pgps_notify_prediction`

The indirect method is used in the :ref:`gnss_sample` sample and in the :ref:`asset_tracker_v2` application.

The application can inject the data contained in the prediction to the GNSS module in the modem by calling the :c:func:`nrf_cloud_pgps_inject` function.
This must be done when event :c:enumerator:`NRF_MODEM_GNSS_EVT_AGPS_REQ` is received from the GNSS interface.
After injecting the prediction, call the :c:func:`nrf_cloud_pgps_preemptive_updates` function to update the prediction set as needed.

A prediction is also automatically injected to the modem every four hours whenever the current prediction expires and the next one begins (if the next one is available in flash).

Interaction with the GNSS interface
===================================

The P-GPS subsystem, like several other nRF Cloud subsystems, is event driven.

Following are the two GNSS events relating to P-GPS that an application receives through the GNSS interface:

* :c:enumerator:`NRF_MODEM_GNSS_EVT_AGPS_REQ` - Occurs when the GNSS module requires assistance data.
* :c:enumerator:`NRF_MODEM_GNSS_EVT_FIX` - Occurs once a fix is attained.

When the application receives the :c:enumerator:`NRF_MODEM_GNSS_EVT_AGPS_REQ` event, it must call the :c:func:`nrf_cloud_pgps_notify_prediction` function.
This event results in the callback of the application's :c:func:`pgps_event_handler_t` function when a valid P-GPS prediction set is available.
It passes the :c:enum:`PGPS_EVT_AVAILABLE` event and a pointer to the :c:struct:`nrf_cloud_pgps_prediction` structure to the handler.

The application must pass this prediction to the :c:func:`nrf_cloud_pgps_inject` function, along with either the :c:struct:`nrf_modem_gnss_agps_data_frame` structure read from the GNSS interface after the :c:enumerator:`NRF_MODEM_GNSS_EVT_AGPS_REQ` event or NULL.

If the device does not move distances longer than a few dozen kilometers before it gets a new GNSS fix, it can pass the latitude and longitude read after the :c:enumerator:`NRF_MODEM_GNSS_EVT_FIX` event to the :c:func:`nrf_cloud_pgps_set_location` function.
The P-GPS subsystem uses this stored location for the next GNSS request for position assistance when A-GPS assistance is not enabled or is unavailable.
If the device moves further between fix attempts, such a mechanism can be detrimental to short TTFF, as the saved position might be too inaccurate to be useful.

The application can also call the :c:func:`nrf_cloud_pgps_preemptive_updates` function to discard expired predictions and replace them with newer ones, before the expiration of the entire set of predictions.
This can be useful in use cases where cloud connections are available infrequently.
The :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_REPLACEMENT_THRESHOLD` option sets the minimum number of valid predictions remaining before such an update occurs.

For best performance, applications can call the P-GPS functions mentioned in this section from workqueue handlers rather than directly from various callback functions.

The P-GPS subsystem itself generates events that can be passed to a registered callback function.
See :c:enum:`nrf_cloud_pgps_event_type`.

Dependencies
************

This library uses the following |NCS| libraries:

* :ref:`lib_date_time`
* :ref:`lib_download_client`
* :ref:`modem_info_readme`
* :ref:`lib_nrf_cloud`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:gnss_interface`

It uses the following Zephyr subsystem:

* :ref:`zephyr:settings_api`

API documentation
*****************

| Header file: :file:`include/net/nrf_cloud_pgps.h`
| Source files: :file:`subsys/net/lib/nrf_cloud/src/`

.. doxygengroup:: nrf_cloud_pgps
   :project: nrf
   :members:
