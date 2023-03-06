.. _asset_tracker_v2_modem_module:

Modem module
############

.. contents::
   :local:
   :depth: 2

The modem module communicates with the modem to control the LTE connection and retrieve information from the modem about the device and LTE network.

Features
********

This section documents the features implemented by the module.

LTE connection handling
=======================

The module uses the :ref:`lte_lc_readme` library to control and monitor the LTE connection.
The module registers an event handler with :ref:`lte_lc_readme` to receive updates from the modem about the state of the LTE connection and network information.
If you use the :ref:`liblwm2m_carrier_readme` library, refer to :ref:`modem_module_carrier_lib` for additional information on LTE connection handling.

In its default configuration, the modem module makes an attempt to establish an LTE connection immediately when it is initialized.
There is no timeout on the connection attempt, so the modem searches for a suitable LTE network until it finds one.

When the device is registered to an LTE network, the LTE connection is considered to be established, and a :c:enum:`MODEM_EVT_LTE_CONNECTED` event is sent.
If the network registration is lost at some point, a :c:enum:`MODEM_EVT_LTE_DISCONNECTED` event is sent.

If the device connects to a new LTE cell, an :c:enum:`LTE_LC_EVT_CELL_UPDATE` is sent.
The event contains cell ID and tracking area code as payload, which can be used to determine the device's coarse position.

If the modem module receives an ``APP_EVT_LTE_DISCONNECT`` event, it instructs the modem to detach from the network, and a subsequent :c:enum:`MODEM_EVT_LTE_DISCONNECTED` event can be expected after the disconnect.

To configure the LTE connection parameters, refer to the :ref:`lte_lc_readme` Kconfig options.
These options can be used to select the access technology to use (LTE-M or NB-IoT), default values for :term:`Power Saving Mode (PSM)` timers and more.

Note that some network parameters, such as PSM timer values, are requests to the network and might not be granted as requested, and the network might grant a different value than the requested values or deny the request altogether.
The actual values that are received from the network are distributed in :c:enum:`MODEM_EVT_LTE_PSM_UPDATE` events.

.. _modem_module_carrier_lib:

Carrier library support
=======================

The modem module has support for the :ref:`liblwm2m_carrier_readme` library.
When the library is enabled, the modem module leaves the LTE connection handling to the library in order to adhere to the specific carrier's requirements.

If the :ref:`liblwm2m_carrier_readme` library starts a modem firmware update, the modem module sends a :c:enum:`MODEM_EVT_CARRIER_FOTA_PENDING` event.
This event signals that a modem firmware FOTA download is about to start, and all the other TLS connections in the system must be closed, including any cloud connection.
After a successful FOTA, :c:enum:`MODEM_EVT_CARRIER_REBOOT_REQUEST` is sent, indicating that the application must perform a graceful shutdown and reboot the device to apply the new modem firmware.
If a FOTA download fails, a :c:enum:`MODEM_EVT_CARRIER_FOTA_STOPPED` event is sent, and the application might again establish TLS connections and continue normal operation.

For more details and configuration options, you can refer to :ref:`liblwm2m_carrier_readme`.

Modem information
=================

When the application module sends out an ``APP_EVT_DATA_GET`` event, the modem module checks the requested data list for relevant requests:

* ``APP_DATA_MODEM_STATIC`` - Static modem data, such as configured system mode (any combination of LTE-M, NB-IoT and GNSS), ICCID, modem firmware version, application version and board version.
* ``APP_DATA_MODEM_DYNAMIC`` - Dynamic modem data, such as Cell ID, tracking area code, RSRP, IP address and PLMN (MCCMNC).
* ``APP_DATA_BATTERY`` - Voltage of the modem's power supply.

The module uses :ref:`modem_info_readme` to acquire information about the modem, LTE network and the modem's power supply.
The response for the three different data types is sent as separate events:

* Static data as :c:enum:`MODEM_EVT_MODEM_STATIC_DATA_READY`
* Dynamic data as :c:enum:`MODEM_EVT_MODEM_DYNAMIC_DATA_READY`

If the sampling of data fails, a corresponding error message is sent through one of the following events:

* :c:enum:`MODEM_EVT_MODEM_STATIC_DATA_NOT_READY`
* :c:enum:`MODEM_EVT_MODEM_DYNAMIC_DATA_NOT_READY`
* :c:enum:`MODEM_EVT_BATTERY_DATA_NOT_READY`

Module internals
****************

The modem module has an internal thread with a message queue for processing.
When an event is received in the :ref:`app_event_manager` handler, the event is converted to a message and put into the module's queue for processing in thread context.
This gives the module the flexibility to call functions that might take some time to complete.

Module states
=============

The modem module has an internal state machine with the following states:

  * ``STATE_DISCONNECTED`` - The module has performed all required initializations and is ready to establish an LTE connection.
  * ``STATE_CONNECTING`` - The modem is currently searching for a suitable LTE network and attempting to establish a connection.
  * ``STATE_CONNECTED`` - The device is connected to an LTE network.
  * ``STATE_SHUTDOWN`` - The module has been shut down after receiving a request from the utility module.

State transitions take place based on input from other modules through the Application Event Manager handler and the LTE link controller handler.

Configuration options
*********************

You can set the following options to configure the modem module:

.. _CONFIG_MODEM_MODULE:

CONFIG_MODEM_MODULE - Configuration for modem module
   This option enables the modem module.

.. _CONFIG_MODEM_THREAD_STACK_SIZE:

CONFIG_MODEM_THREAD_STACK_SIZE -  Configuration for thread stack size
   This option configures the modem module thread stack size.

.. _CONFIG_MODEM_SEND_ALL_SAMPLED_DATA:

CONFIG_MODEM_SEND_ALL_SAMPLED_DATA - Configuration for sending all sampled data
   By default, the modem module sends only events with sampled data that has changed since the last sampling.
   To send unchanged data also, enable this option.

.. _CONFIG_MODEM_AUTO_REQUEST_POWER_SAVING_FEATURES:

CONFIG_MODEM_AUTO_REQUEST_POWER_SAVING_FEATURES - Configuration for automatic requests of PSM
   The module automatically requests PSM from the LTE network.
   If PSM is granted by the network, it results in reduction of the modem's power consumption.
   Note that the device is not reachable from the cloud when it is in PSM.
   The device exits PSM whenever the application sends data, or the configured PSM TAU (Tracking Area Update) interval has passed.
   To not request PSM from the network, disable this option.

For more information on LTE configuration options, see :ref:`lte_lc_readme`.

Module events
*************

The :file:`asset_tracker_v2/src/events/modem_module_event.h` header file contains a list of the events sent by the modem module.

Dependencies
************

The module uses the following |NCS| libraries:

* :ref:`app_event_manager`
* :ref:`lte_lc_readme`
* :ref:`modem_info_readme`

API documentation
*****************

| Header file: :file:`asset_tracker_v2/src/events/modem_module_event.h`
| Source files: :file:`asset_tracker_v2/src/events/modem_module_event.c`, :file:`asset_tracker_v2/src/modules/modem_module.c`

.. doxygengroup:: modem_module_event
   :project: nrf
   :members:
