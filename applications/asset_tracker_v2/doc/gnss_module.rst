.. _asset_tracker_v2_gnss_module:

GNSS module
###########

.. contents::
   :local:
   :depth: 2

The GNSS module controls the GNSS functionality of the nRF9160.
It can be used to acquire the location of the device, either in the form of events containing a position, velocity and time (PVT) structure or NMEA sentences.


Features
********

This section documents the various features implemented by the module.

GNSS control
============

The module uses the :ref:`nrfxlib:gnss_interface` to communicate with the nRF9160 modem and control its GNSS functionality.
A GNSS search is started when the module receives an ``APP_EVT_DATA_GET`` event and the ``APP_DATA_GNSS`` type is listed in the event's ``data_list`` member containing the data types that shall be sampled.

The default GNSS configuration is set as follows:

* Single-fix mode, where the GNSS searches for a defined interval until it acquires a position fix or times out.
* Timeout set to either the value configured at build time or to the dynamically updated value set by the cloud side and stored to flash.
  Values stored in flash take precedence.

The GNSS module receives configuration updates as payload in the following two events:

* ``DATA_EVT_CONFIG_INIT`` - The event contains the value configured at build time or received from cloud and stored to flash.
  The event is distributed from the data module as part of the application initialization.
* ``DATA_EVT_CONFIG_READY`` - The event is received when the data module has processed an incoming message from cloud with a configuration update.

When a position fix is acquired, the ``GNSS_EVT_DATA_READY`` event is sent from the module.
If a timeout occurred, ``GNSS_EVT_TIMEOUT`` is sent.

LNA configuration
=================

Different devices have different antenna and LNA setups depending on the antenna type (onboard or external).
The module has Kconfig options to control the LNA using either the COEX0 or MAGPIO interface of the nRF9160.
See :ref:`configuration_options` for more details on how to configure the antenna and LNA.

GPS assistance data
===================

The GNSS module receives requests for GPS assistance data from the nRF9160 modem.
When an A-GPS request is received, the module distributes it to the other modules as a ``GNSS_EVT_AGPS_NEEDED`` event that contains information about the type of assistance data needed.
Providing the requested A-GPS data may reduce the time it takes to acquire a position fix.

The module can also store the last known location if :ref:`CONFIG_GNSS_MODULE_PGPS_STORE_LOCATION <CONFIG_GNSS_MODULE_PGPS_STORE_LOCATION>` is enabled.
This can improve the time to fix when predicted GPS assistance data is used.

Module internals
================

The GNSS module is a threadless module in terms of message processing.
In this sense, it differs from the other modules.
It has a thread for processing GNSS events from the modem library, whereas other modules use a thread for processing messages.
The thread is used to offload data processing from the GNSS callbacks coming from the modem library, as these are called from interrupt context and no time consuming tasks should be performed there.

All incoming events from other modules are handled in the context of the event manager callback, as they all complete fast enough to not require a separate thread.

.. _configuration_options:

Configuration options
*********************

.. _CONFIG_GNSS_MODULE:

CONFIG_GNSS_MODULE
   Enables the GNSS module. The module can report data either as a structure of position, velocity, and time (PVT) information, or as an NMEA string of GGA type.

.. _CONFIG_GNSS_MODULE_PVT:

CONFIG_GNSS_MODULE_PVT
   Selects the PVT format. Includes position, velocity, and time (PVT) information in the ``GNSS_EVT_DATA_READY`` events that are sent when the GNSS acquires a position fix.

.. _CONFIG_GNSS_MODULE_NMEA:

CONFIG_GNSS_MODULE_NMEA
   Selects the NMEA format. Includes an NMEA string of the GGA type in the ``GNSS_EVT_DATA_READY`` events that are sent when the GNSS acquires a position fix.

.. _CONFIG_GNSS_MODULE_ANTENNA_ONBOARD:

CONFIG_GNSS_MODULE_ANTENNA_ONBOARD
   This configures the target to use an onboard GNSS antenna. Sends antenna configurations to the modem when you compile for Thingy:91 or nRF9160 DK.

.. _CONFIG_GNSS_MODULE_ANTENNA_EXTERNAL:

CONFIG_GNSS_MODULE_ANTENNA_EXTERNAL
   This configures the target to use an external GNSS antenna.

If P-GPS is used, the last known position from a GNSS fix can be stored and injected to the modem together with the relevant ephemeris.
This may, depending on the use case and the device's movements, reduce the time to fix.
To enable this feature, use the following option:

.. _CONFIG_GNSS_MODULE_PGPS_STORE_LOCATION:

CONFIG_GNSS_MODULE_PGPS_STORE_LOCATION
   This stores and injects the last known position from a GNSS fix to the modem with the relevant ephemeris.

For more information on P-GPS, see :ref:`lib_nrf_cloud_pgps`.

Module states
*************

The GNSS module has an internal state machine with the following states:

  * ``STATE_INIT`` - The initial state of the module in which it awaits the modem to be initialized and receive the GNSS configuration.
  * ``STATE_RUNNING`` - The module has performed all required initialization and can respond to requests to start GNSS searches. The running state has two sub-states:

    * ``SUB_STATE_SEARCH`` - A GNSS search is ongoing.
    * ``SUB_STATE_IDLE`` - The module is idling and can respond to a request to start a GNSS search.
  * ``STATE_SHUTDOWN`` - The module has been shut down after receiving a request from the util module.

State transitions take place based on input from other modules, such as the app module, data module, util module and modem module.

Module events
*************

The :file:`asset_tracker_v2/src/events/gnss_module_event.h` header file contains a list of the events sent by the GNSS module with the associated payload.

Dependencies
************

The module uses the following |NCS| libraries:

* :ref:`event_manager`
* :ref:`nrfxlib:gnss_interface`

API documentation
*****************

| Header file: :file:`asset_tracker_v2/src/events/gnss_module_event.h`
| Source files: :file:`asset_tracker_v2/src/events/gnss_module_event.c`

.. doxygengroup:: gnss_module_event
   :project: nrf
   :members:
