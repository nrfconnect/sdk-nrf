.. _asset_tracker_v2_location_module:

Location module
###############

.. contents::
   :local:
   :depth: 2

The location module controls the GNSS, cellular and Wi-Fi positioning functionality.
It can be used to retrieve the location of the device in the form of events containing a position, velocity and time (PVT) structure.

Features
********

This section documents the various features implemented by the module.

Location control
================

The module uses the :ref:`lib_location` library to communicate with the nRF9160 modem and
control its GNSS and LTE neighbor cell measurement functionalities as well as with the nRF7002 Wi-Fi positioning functionality.
A location request starts when the module receives an ``APP_EVT_DATA_GET`` event and
the ``APP_DATA_LOCATION`` type is listed in the event's ``data_list`` member containing the data types that shall be sampled.

The default configuration for the location request is set as follows:

* GNSS, Wi-Fi and cellular positioning are set in priority order of the location methods.
  Wi-Fi is omitted if it is not built into the application.
  If GNSS fails, both Wi-Fi and cellular positioning are tried.
  The priority order of the methods can be changed through :ref:`lib_location` library Kconfig options.
  Methods that should not be used can be configured by adding them into ``No Data List``.
  See :ref:`Real-time configurations <real_time_configs>` for more information.
* Timeout, which can be configured as part of :ref:`Real-time configurations <real_time_configs>` with ``Location timeout``.

The location module receives configuration updates as payload in the following two events:

* :c:enum:`DATA_EVT_CONFIG_INIT` - The event contains the value configured at build time or received from cloud and stored to flash.
  The event is distributed from the data module as part of the application initialization.
* :c:enum:`DATA_EVT_CONFIG_READY` - The event is received when the data module has processed an incoming message from cloud with a configuration update.

The module sends the following events based on the outcome of the location request:

* :c:enum:`LOCATION_MODULE_EVT_GNSS_DATA_READY`: GNSS position has been acquired
* :c:enum:`LOCATION_MODULE_EVT_CLOUD_LOCATION_DATA_READY`: Neighbor cell measurements or Wi-Fi access points (or both) have been obtained
* :c:enum:`LOCATION_MODULE_EVT_DATA_NOT_READY`: Location request failed
* :c:enum:`LOCATION_MODULE_EVT_TIMEOUT`: Timeout occurred
* :c:enum:`LOCATION_MODULE_EVT_AGPS_NEEDED`: A-GPS request should be sent to cloud
* :c:enum:`LOCATION_MODULE_EVT_PGPS_NEEDED`: P-GPS request should be sent to cloud

GNSS LNA configuration
======================

Different devices have different GNSS antenna and LNA setups depending on the antenna type (onboard or external).
The application uses the :ref:`lib_modem_antenna` library for configuring the LNA.
The library has Kconfig options to control the LNA using either the COEX0 or MAGPIO interface of the nRF9160.
See the library documentation for more details on how to configure the antenna and LNA.

GPS assistance data
===================

The location module receives requests for GPS assistance data from the :ref:`lib_location` library.
When the module receives an A-GPS request, it distributes it to the other modules as a :c:enum:`LOCATION_MODULE_EVT_AGPS_NEEDED` event that contains information about the type of assistance data needed.
Providing the requested A-GPS data typically reduces significantly the time it takes to acquire a GNSS fix.

Wi-Fi positioning
=================

Wi-Fi positioning is supported with an nRF7002 EK on the nRF9160 DK.
To enable Wi-Fi positioning and especially nRF7002 functionality, use a
special DTC overlay with the compiler option ``-DEXTRA_DTC_OVERLAY_FILE=nrf9160dk_with_nrf7002ek.overlay`` and a
configuration overlay ``-DEXTRA_CONF_FILE=overlay-nrf7002ek-wifi-scan-only.conf``.

To build for the nRF9160 DK with nRF7002 EK, use the ``nrf9160dk_nrf9160_ns`` build target with the ``SHIELD`` CMake option set to ``nrf7002ek`` and a scan-only overlay configuration.
The following is an example of the CLI command:

.. code-block:: console

   west build -p -b nrf9160dk_nrf9160_ns -- -DSHIELD=nrf7002ek -DEXTRA_DTC_OVERLAY_FILE=nrf9160dk_with_nrf7002ek.overlay -DEXTRA_CONF_FILE=overlay-nrf7002ek-wifi-scan-only.conf

Wi-Fi positioning has the following limitations:

* Only `nRF Cloud`_ is supported.
* The UI module must be disabled.
  Functionalities related to LEDs and buttons have conflicts with the nRF7002 EK.

.. note::

   The Wi-Fi configuration uses both flash and SRAM extensively.
   You can configure the number of scan results with the :kconfig:option:`CONFIG_LOCATION_METHOD_WIFI_SCANNING_RESULTS_MAX_CNT` Kconfig option to reduce SRAM consumption.
   You can also change the value of the :kconfig:option:`CONFIG_HEAP_MEM_POOL_SIZE` Kconfig option.

Module internals
================

The location module is a threadless module in terms of message processing.
In this sense, it differs from many other modules.

All incoming events from other modules are handled in the context of the Application Event Manager callback, because they all complete fast enough to not require a separate thread.

The :ref:`lib_location` library handles cellular and Wi-Fi positioning together when the location request method list has them next to each other.
This means that LTE neighbor cell measurements and Wi-Fi scanning results are combined into the same :c:enum:`LOCATION_EVT_CLOUD_LOCATION_EXT_REQUEST` event.
The location module responds to the :ref:`lib_location` library with unknown location resolution, because it does not request the location back from cloud service.

Configuration options
*********************

.. _CONFIG_LOCATION_MODULE:

CONFIG_LOCATION_MODULE
   Enables the location module.

Module states
*************

The location module has an internal state machine with the following states:

  * ``STATE_INIT`` - The initial state of the module in which it awaits the modem to be initialized and receive the location related configuration.
  * ``STATE_RUNNING`` - The module has performed all required initialization and can respond to requests to start a location request. The running state has two sub-states:

    * ``SUB_STATE_SEARCH`` - A location request is ongoing.
    * ``SUB_STATE_IDLE`` - The module is idling and can respond to a request to start a location request.
  * ``STATE_SHUTDOWN`` - The module has been shut down after receiving a request from the utility module.

State transitions take place based on input from other modules, such as the application module, data module, utility module and modem module.

Module events
*************

The :file:`asset_tracker_v2/src/events/location_module_event.h` header file contains a list of events sent by the location module with associated payload.

Dependencies
************

The module uses the following |NCS| libraries:

* :ref:`app_event_manager`
* :ref:`lib_location`

API documentation
*****************

| Header file: :file:`asset_tracker_v2/src/events/location_module_event.h`
| Source files: :file:`asset_tracker_v2/src/events/location_module_event.c`

.. doxygengroup:: location_module_event
   :project: nrf
   :members:
