.. _lwm2m_client_sample_desc:

Sample description
##################

.. contents::
   :local:
   :depth: 2

The LwM2M Client sample demonstrates the usage of the :term:`Lightweight Machine to Machine (LwM2M)` protocol to connect a Thingy:91 or an nRF91 Series DK to an LwM2M Server through LTE.
To achieve this, the sample uses the Zephyr's :ref:`lwm2m_interface` client and |NCS| :ref:`lib_lwm2m_client_utils` library.
The former provides a device vendor agnostic client implementation, whereas the latter includes all the Nordic specific bits and pieces.

The sample also supports a proprietary mechanism to fetch location assistance data from `nRF Cloud`_ by proxying it through the LwM2M Server.
For this, the sample makes use of the :ref:`lib_lwm2m_location_assistance` library.

Requirements
************

.. _supported boards:

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Additionally, the sample requires an activated SIM card, and an LwM2M Server such as `Leshan Demo Server`_ or AVSystem's `Coiote Device Management`_ server.
To know more about the AVSystem integration with |NCS|, see :ref:`ug_avsystem`.

Overview
********

LwM2M is an application layer protocol based on CoAP over UDP.
It is designed to expose various resources for reading, writing, and executing through an LwM2M Server in a very lightweight environment.
The client sends data such as button and switch states, accelerometer data, temperature, and GNSS position to the LwM2M Server.
It can also receive activation commands such as buzzer activation and light control.

.. note::
   The GNSS module interferes with the LTE connection.
   If GNSS is not needed or if the device is in an area with poor connection, disable the GNSS module.
   You can disable the GNSS module by setting the configuration option :ref:`CONFIG_APP_GNSS <CONFIG_APP_GNSS>` to ``n``.


The sample implements the following LwM2M objects:

.. list-table:: LwM2M objects
   :header-rows: 1

   *  - LwM2M objects
      - Object ID
      - Thingy:91
      - nRF91 Series DK
   *  - LwM2M Server
      - 1
      - Yes
      - Yes
   *  - Device
      - 3
      - Yes
      - Yes
   *  - Connectivity Monitoring
      - 4
      - Yes
      - Yes
   *  - FOTA
      - 5
      - Yes
      - Yes
   *  - Location
      - 6
      - Yes
      - Yes
   *  - Accelerometer
      - 3313
      - Yes
      - Simulated
   *  - Color
      - 3335
      - Yes
      - Simulated
   *  - Temperature
      - 3303
      - Yes
      - Simulated
   *  - Pressure
      - 3323
      - Yes
      - Simulated
   *  - Humidity
      - 3304
      - Yes
      - Simulated
   *  - Generic Sensor
      - 3300
      - Yes
      - Simulated
   *  - Light Control
      - 3311
      - Yes
      - Yes
   *  - Push Button
      - 3347
      - Yes
      - Yes
   *  - Buzzer
      - 3338
      - Yes
      - No
   *  - On/Off Switch
      - 3342
      - No
      - Yes
   *  - GNSS Assistance
      - 33625
      - Yes
      - Yes
   *  - Ground Fix
      - 33626
      - Yes
      - Yes
   *  - Visible Wi-Fi Access Point
      - 33627
      - Yes
      - Yes
   *  - Advanced Firmware Update
      - 33629
      - Yes
      - Yes

User interface
**************

Button 1:
   Triggers GNSS-based location service

Button 2:
   Triggers cell-based location service


.. _state_diagram:

State diagram
=============

The following diagram shows states and transitions for the LwM2M Client:

.. figure:: /images/lwm2m_client_state_diagram.svg
   :alt: LwM2M Client state diagram

When the device boots up, the sample first connects to the LTE network and initiates the LwM2M connection.
If there are errors, in most error cases, the sample tries to reconnect the LwM2M Client.
In the case of network errors, it tries to reconnect the LTE network.
When the number of retries to restore the network connection exceeds three times, the sample falls back to the bootstrap.
This enables the recovery in the cases where the LwM2M Client credentials are outdated or removed from the server.

.. _dtls_support:

DTLS Support
============

The sample has DTLS security enabled by default.
You need to provide the following information to the LwM2M Server before you can make a successful connection:

* Client endpoint
* Identity
* `Pre-shared key (PSK) <Pre-Shared Key (PSK)_>`_

See :ref:`server setup <server_setup_lwm2m_client>` for instructions on providing the information to the server.

Optional configuration:

* DTLS Connection Identifier, requires modem version 1.3.5 or newer.

.. _notifications_lwm2m:

Notifications
=============

LwM2M specifies the Notify operation, which can be used to notify the server of changes to the value of a resource field, for example, the measured value of a temperature sensor.
This allows active monitoring while using minimal bandwidth, as notifications can be sent with the updated values without the need for the server querying the client regularly.

To enable notifications, the server must initiate an observation request on one or more resources of interest.
For more information, see :ref:`notifications_setup_lwm2m`.

.. _sensor_module_lwm2m:

Sensor Module
=============

The sample has a sensor module which, if enabled, reads the selected sensors and updates the client's LwM2M resource values.
Each sensor can be enabled separately.

The sensor module is intended to be used together with notifications.
If notifications are enabled for a Sensor Value resource and the corresponding sensor is enabled in the sensor module, a notification will be sent when the value changes.
The frequency of notification packets is configured by LwM2M attributes set by the server.

See :ref:`sensor_module_options` for information on enabling and configuring the sensor module.

Sensor simulation
=================

If a sensor simulator is defined in devicetree with the ``sensor_sim`` node label, it will be used over real devices.
This is useful, for example, on an nRF91 Series DK, where only simulated sensor data is available, as it does not have any of the external sensors needed for actual measurements.

Configuration
*************

|config|
You can configure the sample either by editing the :file:`prj.conf` file and the relevant overlay files, or through menuconfig or guiconfig.

Setup
=====

Before building and running the sample, complete the following steps:

1. Select the device you plan to test.
#. Select the LwM2M Server for testing.
#. Setup the LwM2M Server by completing the steps listed in :ref:`server_setup_lwm2m_client`.
   This step retrieves the server address and the security tag that will be needed during the next steps.
#. :ref:`server_addr_PSK`.

.. |dtls_support| replace:: The same credentials must be provided in the :guilabel:`Leshan Demo Server Security configuration` page (see :ref:`dtls_support` for instructions).

.. _server_setup_lwm2m_client:

.. include:: /includes/lwm2m_common_server_setup.txt

.. _server_addr_PSK:

Set the server address and PSK
------------------------------

1. Open :file:`src/prj.conf`.
#. Set :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_SERVER` to the correct server URL:

   * For `Leshan Demo Server`_ - ``coaps://leshan.eclipseprojects.io:5684`` (`public Leshan Demo Server`_).
   * For `Coiote Device Management`_ - ``coaps://eu.iot.avsystem.cloud:5684`` (`Coiote Device Management server`_).
   * For `Leshan Bootstrap Demo Server <public Leshan Bootstrap Server Demo_>`_ - ``coaps://leshan.eclipseprojects.io:5784``
   * For Coiote bootstrap server - ``coaps://eu.iot.avsystem.cloud:5694``
#. Set :kconfig:option:`CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP` if bootstrap is used.
#. Set :ref:`CONFIG_APP_LWM2M_PSK <CONFIG_APP_LWM2M_PSK>` to the hexadecimal representation of the PSK used when registering the device with the server.

See the :ref:`lwm2m_configuration_files` section for overlays that set the mentioned options.

.. _notifications_setup_lwm2m:

Enabling notifications
----------------------

Enabling notifications for a resource varies slightly from server to server.
The client must be connected to the server to enable notifications.
Following are the instructions for enabling notifications in the Leshan Demo server and Coiote Device Management server:

.. tabs::

   .. tab:: Leshan Demo Server

      1. Open the `Leshan Demo Server web UI`_.
      #. Identify your device in the :guilabel:`Clients` tab and select it.
      #. Select the desired object in the menu on the left in the UI.
      #. Identify one or more resources you want to track and click :guilabel:`OBS` next to it.

         * You can track either a single resource or all the resources of an object. It is recommended to track only the resources that are expected to change.
         * If you want to use the :ref:`sensor_module_lwm2m`, at least the Sensor Value resource must be tracked for all sensors enabled in the Sensor Module.

   .. tab:: Coiote Device Management server

      1. Open `Coiote Device Management server`_.
      #. Click :guilabel:`Device inventory` tab in the top.
      #. Identify your device in the list and click on the anchor text corresponding to the device ID in the **Identity** column.
      #. Click the :guilabel:`Objects` tab in the new menu to the left, just below :guilabel:`Dashboard`.
      #. Identify one or more objects that you want to receive notifications from, and expand it by clicking on them.
      #. Identify one or more resources of the object that you want to track:

         * You can track either a single resource or all the resources of an object. It is recommended to track only the resources that are expected to change.
         * If you want to use the :ref:`sensor_module_lwm2m`, at least the Sensor Value resource must be tracked for all sensors enabled in the Sensor Module.

      #. Click the :guilabel:`Value Tracking` button of the selected resource.
      #. Select :guilabel:`Observe` or :guilabel:`Monitoring` from the dropdown menu.

         * Selecting :guilabel:`Observe` will only update the Value field of the resource when it receives a notification.
         * Selecting :guilabel:`Monitoring` will additionally create a graph of the logged datapoints.

      #. Click :guilabel:`Limit data usage` to configure how often notifications are sent.

Avoiding re-writing credentials to modem
----------------------------------------

Every time the sample starts, it provisions the keys to the modem and this is only needed once.
To speed up the start up, you can prevent the provisioning by completing the following steps using |VSC|:

1. In |nRFVSC|, `build the sample <How to build an application_>`_.
#. Under **Actions**, click :guilabel:`Kconfig`.
#. Click :guilabel:`Application sample`.
#. Under **LwM2M objects**, remove the key value next to :guilabel:`LwM2M pre-shared key for communication`.
#. Save and close the configuration.

The provisioning can also be prevented by setting the :kconfig:option:`CONFIG_APP_LWM2M_PSK` Kconfig option to an empty string in the :file:`prj.conf` file.
You can also edit this configuration using menuconfig.
|config|

For the changes to be added, rebuild the sample.

Configuration options
=====================

Check and configure the following configuration options for the sample:

Server options
--------------

.. _CONFIG_APP_LWM2M_PSK:

CONFIG_APP_LWM2M_PSK - Configuration for the PSK
   The sample configuration sets the hexadecimal representation of the PSK used when registering the device with the server.
   To prevent provisioning of the key to the modem, set this option to an empty string.

.. _CONFIG_APP_ENDPOINT_PREFIX:

CONFIG_APP_ENDPOINT_PREFIX - Configuration for setting prefix for endpoint name
   This configuration option changes the prefix of the endpoint name.

LwM2M objects options
---------------------

.. _CONFIG_APP_TEMP_SENSOR:

CONFIG_APP_TEMP_SENSOR - Configuration for enabling an LwM2M Temperature sensor object
   The sample configuration enables an LwM2M Temperature sensor object.
   All compatible objects are enabled by default.
   Disabled objects will not be visible in the server.
   You can use this configuration option for other LwM2M objects also by modifying the option accordingly.

.. _CONFIG_APP_GNSS:

CONFIG_APP_GNSS - Configuration for enabling GNSS functionality
   This configuration might interfere with LTE if the GNSS conditions are not optimal.
   Disable this option if GNSS is not needed.

.. _CONFIG_GNSS_PRIORITY_ON_FIRST_FIX:

CONFIG_GNSS_PRIORITY_ON_FIRST_FIX - Configuration for prioritizing GNSS over LTE during the search for first fix
   Enabling this option makes it significantly easier for the GNSS module to find a position but will also affect performance for the rest of the application during the search for first fix.

.. _CONFIG_LWM2M_IPSO_APP_COLOUR_SENSOR_VERSION_1_0:

CONFIG_LWM2M_IPSO_APP_COLOUR_SENSOR_VERSION_1_0 - Configuration for selecting the IPSO Color sensor object version
   The configuration option sets the version of the OMA IPSO object specification to 1.0.
   The user-defined Color sensor IPSO object uses this version.

.. _CONFIG_LWM2M_IPSO_APP_COLOUR_SENSOR_VERSION_1_1:

CONFIG_LWM2M_IPSO_APP_COLOUR_SENSOR_VERSION_1_1 - Configuration for selecting the IPSO Color sensor object version
   The configuration option sets the version of the OMA IPSO object specification to 1.1.
   The user-defined Color sensor IPSO object uses this version.

.. _CONFIG_APP_CUSTOM_VERSION:

CONFIG_APP_CUSTOM_VERSION - Configuration to set custom application version reported in the Device object
   The configuration option allows to specify custom application version to be reported to the LwM2M Server.
   The option has the current |NCS| version as the default value.

.. _sensor_module_options:

Sensor module options
---------------------

.. _CONFIG_SENSOR_MODULE:

CONFIG_SENSOR_MODULE - Configuration for periodic sensor reading
   This configuration option enables periodic reading of sensors and updating the resource values when the change is sufficiently large.
   If you enable the option, the sample notifies the server if a change in one or more resources is observed.

.. _CONFIG_SENSOR_MODULE_TEMP:

CONFIG_SENSOR_MODULE_TEMP - Configuration to enable Temperature sensor
   This configuration option enables the Temperature sensor in the Sensor Module.

.. _CONFIG_SENSOR_MODULE_ACCEL:

CONFIG_SENSOR_MODULE_ACCEL - Configuration to enable accelerometer
   This configuration option enables the accelerometer.

.. _CONFIG_SENSOR_MODULE_PRESS:

CONFIG_SENSOR_MODULE_PRESS - Configuration for pressure reading
   This configuration option enables the reading of pressure values.

.. _CONFIG_SENSOR_MODULE_HUMID:

CONFIG_SENSOR_MODULE_HUMID - Configuration for humidity reading
   This configuration option enables the reading of humidity values.

.. _CONFIG_SENSOR_MODULE_GAS_RES:

CONFIG_SENSOR_MODULE_GAS_RES - Configuration for gas resistance reading
   This configuration option enables the reading of gas resistance values.

.. _CONFIG_SENSOR_MODULE_LIGHT:

CONFIG_SENSOR_MODULE_LIGHT - Configuration for light reading
   This configuration option enables the reading of light values.

.. _CONFIG_SENSOR_MODULE_COLOR:

CONFIG_SENSOR_MODULE_COLOR - Configuration for color
   This configuration option enables the reading of color values.

Additional configuration
========================

Check also the default configurations and any additional configurations of the sample listed in this section.

Default configuration
---------------------

The sample use the following default configurations.

LwM2M configuration:

* Protocol version: 1.0
* Binding mode: Queue
* Device Management server: Leshan Demo server
* Security: Enabled with PSK, DTLS Connection Identifier, and DTLS session caching
* Registration life time: 12 hours
* Coap ACK initial timeout: 4 seconds
* Enable LwM2M tickless mode power optimization
* LwM2M shell

Modem configurations:

* Network Mode: LTE-M with GNSS.
* PSM: Enabled TAU 12 hours, RAT 30 seconds
* Paging window: LTE 1.28 seconds and NB-IoT 2.56 seconds
* eDRX: Enabled, with request of 5.12 seconds on LTE and 20.48 seconds on NB-IoT.
* TAU pre-warning enabled, notification triggers registration update and TAU will be sent with the update which decreases power consumption.

Modem proprietary PSM
---------------------

Add the :file:`overlay-aggressive-psm.conf` overlay file to enable optimized PSM setup and proprietary PSM mode.
This configuration disables eDRX, because it will request 10 seconds RAT and 12 hours TAU period.
Proprietary PSM enables power saving when network does not allow PSM.
The modem enters the PSM state after the configured RAT period when the connection is released.

.. note::

   Proprietary PSM is only supported with modem firmware v2.x.
   Enabling this file for an older modem version generates the following error message:

   .. code-block:: console

      <err> lte_lc: Failed to configure proprietary PSM, err -14


Configuration for external FOTA
-------------------------------

The sample supports UART2 connection on the nRF9160 SiP to onboard an nRF52840 SiP with or without MCUboot recovery mode.
The nRF9160 SiP needs to enable UART2 on the devicetree using the following configuration files and recovery mode overlay files:

* :file:`overlay-mcumgr_client.conf` - Defines the configuration for external FOTA client.
  This requires an additional devicetree overlay file :file:`nrf9160dk_mcumgr_client_uart2.overlay`.
* :file:`overlay-mcumgr_reset.conf` - Enables MCUboot recovery mode.
  This requires an additional devicetree overlay file :file:`nrf9160dk_recovery.overlay`.

.. _overlay_advanced_fw_object:

To enable the experimental Advanced Firmware Update object for the external FOTA, use the following overlay configuration files:

* :file:`overlay-adv-firmware.conf` - Enables the experimental Advanced Firmware Update object.
* :file:`overlay-lwm2m-1.1.conf` - Enables the LwM2M version 1.1.

You also need one of the following `Coiote Device Management`_ server configurations:

* :file:`overlay-avsystem.conf` - For the `Coiote Device Management`_ server.
* :file:`overlay-avsystem-bootstrap.conf` - For Coiote in bootstrap mode.

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Various library options
-----------------------

Check and configure the following LwM2M options that are used by the sample:

* :kconfig:option:`CONFIG_LWM2M_PEER_PORT` - LwM2M Server port.
* :kconfig:option:`CONFIG_LWM2M_ENGINE_MAX_OBSERVER` - Maximum number of resources that can be tracked.
  You must increase this value if you want to observe more than 10 resources.
* :kconfig:option:`CONFIG_LWM2M_ENGINE_MAX_MESSAGES` - Maximum number of LwM2M message objects.
  You must increase this value if many notifications will be sent at once.
* :kconfig:option:`CONFIG_LWM2M_ENGINE_MAX_PENDING` - Maximum number of pending LwM2M message objects.
  You must increase this value if you want to observe more than 10 resources.
* :kconfig:option:`CONFIG_LWM2M_ENGINE_MAX_REPLIES` - Maximum number of LwM2M reply objects.
  You must increase this value if many notifications will be sent at once.
* :kconfig:option:`CONFIG_LWM2M_COAP_BLOCK_SIZE` - Increase if you need to add several new LwM2M objects to the sample, as the registration procedure contains information about all the LwM2M objects in one block.
* :kconfig:option:`CONFIG_LWM2M_ENGINE_DEFAULT_LIFETIME` - Configure default LwM2M registration lifetime.
* :kconfig:option:`CONFIG_LWM2M_UPDATE_PERIOD` - Set this option to configure how often the client sends ``I'm alive`` messages to the server.
* :kconfig:option:`CONFIG_LWM2M_IPSO_TEMP_SENSOR_VERSION_1_0` - Sets the IPSO Temperature sensor object version to 1.0.
  You can use this configuration option for other IPSO objects also by modifying the option accordingly.
  See the `LwM2M Object and Resource Registry`_ for a list of objects and their available versions.
* :kconfig:option:`CONFIG_LWM2M_IPSO_TEMP_SENSOR_VERSION_1_1` - Sets the IPSO Temperature sensor object version to 1.1.
  You can use this configuration option for other IPSO objects also by modifying the option accordingly. See the `LwM2M Object and Resource Registry`_ for a list of objects and their available versions.
* :kconfig:option:`CONFIG_LWM2M_SHELL` - Enables the Zephyr shell and all LwM2M specific commands.
* :kconfig:option:`CONFIG_LWM2M_TLS_SESSION_CACHING` - Enables TLS session caching to prevent a full TLS handshake for every send.
* :kconfig:option:`CONFIG_LWM2M_RD_CLIENT_SUSPEND_SOCKET_AT_IDLE` - Enables the skipping of socket close at RX off idle state, which optimizes power consumption.
* :kconfig:option:`CONFIG_LWM2M_QUEUE_MODE_UPTIME` - Specifies time (in seconds) the device must stay online after sending a message to the server.
* :kconfig:option:`CONFIG_LWM2M_SECONDS_TO_UPDATE_EARLY` - Specifies time in seconds before the registration timeout, when the LWM2M registration update is sent by the engine.
* :kconfig:option:`CONFIG_LTE_PSM_REQ_RPTAU` - Sets the :term:`Power saving mode (PSM)` for requested periodic TAU. Data format is described in `3GPP TS 24.008 Ch. 10.5.7.4a`_.
* :kconfig:option:`CONFIG_LTE_PSM_REQ_RAT` - Sets the :term:`Power saving mode (PSM)` for requested active time. Data format is described in `3GPP TS 24.008 Ch. 10.5.7.3`_.
* :kconfig:option:`CONFIG_LTE_EDRX_REQ` - Enables request for use of Extended DRX (eDRX).
* :kconfig:option:`CONFIG_LTE_EDRX_REQ_VALUE_LTE_M` - Sets the eDRX value to request when LTE-M is used. The format is half a byte in a four-bit format. The eDRX value refers to bit 4 to 1 of octet 3 of the Extended DRX parameters information element. See `3GPP TS 24.008, subclause 10.5.5.32`_.
* :kconfig:option:`CONFIG_LTE_EDRX_REQ_VALUE_NBIOT` - Sets the eDRX value to request when NB-IoT is used. The format is half a byte in a four-bit format. The eDRX value refers to bit 4 to 1 of octet 3 of the Extended DRX parameters information element. See `3GPP TS 24.008, subclause 10.5.5.32`_.
* :kconfig:option:`CONFIG_LTE_PTW_VALUE_LTE_M` - Sets the Paging Time Window value to be requested when enabling eDRX. The value will apply to LTE-M. The format is a string with half a byte in 4-bit format, corresponding to bits 8 to 5 in octet 3 of eDRX information element according to `3GPP TS 24.008, subclause 10.5.5.32`_.
* :kconfig:option:`CONFIG_LTE_PTW_VALUE_NBIOT` - Sets the Paging Time Window value to be requested when enabling eDRX. The value applies to NB-IoT. The format is a string with half a byte in 4-bit format, corresponding to bits 8 to 5 in octet 3 of eDRX information element according to `3GPP TS 24.008, subclause 10.5.5.32`_.
* :kconfig:option:`CONFIG_LTE_LC_TAU_PRE_WARNING_NOTIFICATIONS` - Enables notifications before Tracking Area Update (TAU). Notification triggers LWM2M registration update and TAU will be sent together with the user data. This decreases power consumption.

.. note::
   The nRF91 Series modem negotiates PSM and eDRX modes with the network it is trying to connect.
   The network can either accept the values, assign different values or reject them.

For Thingy:91, configure the ADXL362 accelerometer sensor range by choosing one of the following options (default value is |plusminus| 2 g):

* :kconfig:option:`CONFIG_ADXL362_ACCEL_RANGE_2G` - Sensor range of |plusminus| 2 g.
* :kconfig:option:`CONFIG_ADXL362_ACCEL_RANGE_4G` - Sensor range of |plusminus| 4 g.
* :kconfig:option:`CONFIG_ADXL362_ACCEL_RANGE_8G` - Sensor range of |plusminus| 8 g.

Resolution depends on range: |plusminus| 2 g has higher resolution than |plusminus| 4 g, which again has higher resolution than |plusminus| 8 g.

If you use an external GNSS antenna, add the following configuration:

* :kconfig:option:`CONFIG_MODEM_ANTENNA_GNSS_EXTERNAL` - Selects an external GNSS antenna.

Location assistance options
---------------------------

Check and configure the following library options that are used by the sample:

* :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_GROUND_FIX_OBJ_SUPPORT` - Uses Ground Fix Location object (ID 33626).
  Used with nRF Cloud to estimate the location of the device based on the cell neighborhood and Wi-Fi AP neighborhood.
* :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_GNSS_ASSIST_OBJ_SUPPORT` - Uses GNSS Assistance object (ID 33625).
  Used with nRF Cloud to request assistance data for the GNSS module.
* :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGNSS` - nRF Cloud provides A-GNSS assistance data and the GNSS-module in the device uses the data for obtaining a GNSS fix, which is reported back to the LwM2M Server.
* :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS` - nRF Cloud provides P-GPS predictions and the GNSS-module in the device uses the data for obtaining a GNSS fix, which is reported back to the LwM2M Server.
* :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_NEIGHBOUR_CELL_LISTENER` - Disable this option if you provide your own method of populating the LwM2M objects (ID 10256) containing the cell neighborhood information.

.. _lwm2m_configuration_files:

Configuration files
===================

The sample provides predefined configuration files for typical use cases.

* :file:`prj.conf` - Default configuration file.

LwM2M Device management server:

* :file:`overlay-leshan-bootstrap.conf` - Enables LwM2M bootstrap support with Leshan demo server.
* :file:`overlay-avsystem.conf` - Uses `Coiote Device Management`_ server.
* :file:`overlay-avsystem-bootstrap.conf` - Uses Coiote in bootstrap mode.

NB-IoT:

* :file:`overlay-nbiot.conf` - Enables the use of NB-IoT.

LwM2M v1.1:

* :file:`overlay-lwm2m-1.1.conf` - Enables LwM2M v1.1 protocol version.

Firmware update:

* :file:`overlay-adv-firmware.conf` - Enables experimental Advanced Firmware Update object.
* :file:`overlay-fota_helper.conf` - Enables faster response for evaluating FOTA.

Location assistance:

* :file:`overlay-assist-agnss.conf` - Enables A-GNSS assistance.
* :file:`overlay-assist-cell.conf` - Enables cell-based location assistance.
* :file:`overlay-assist-pgps.conf` - Enables P-GPS assistance in the sample.

Location service requires `Coiote Device Management`_ server and LwM2M v1.1.

Power savings:

* :file:`overlay-aggressive-psm.conf` - Enables optimized PSM setup and proprietary PSM mode.
* :file:`overlay-lowpower.conf` - Disables serial console to bring the power consumption down.

LwM2M v1.1 conformance testing:

* :file:`overlay-lwm2m-1.1-core-interop.conf` - Allows running of Core Specific Objects Test cases.
* :file:`overlay-lwm2m-1.1-object-interop.conf` - Allows running of Additional Objects Test cases.

.. _build_lwm2m:

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/lwm2m_client`

.. include:: /includes/build_and_run_ns.txt

After building and running the sample, you can locate your device in the server:

* Leshan - Devices are listed under **Clients**.
* Coiote - Devices are listed under **Device inventory**.

You can also optionally enable notifications for the resources so that the server actively monitors the resources.

Queue Mode
==========

The sample uses LwM2M queue mode by default.
In this mode, the device need not actively listen for incoming packets and the client can reduce power consumption by being in the sleep state for longer duration.

Queue mode is suitable for LTE connection where devices are not continuously reachable from the network.
Both network and the modem try to release any resources as soon as possible.
In LTE, the Radio Resource Control (RRC) protocol sometimes causes the close of idle connections as early as 10 seconds of idle time.
This depends on the network operator.
Another network dependent feature is the NAT timeout.
Some networks drop unused UDP mappings after 30 seconds even if the RFC recommendation is 2 minutes.
Therefore, after a short sleeping period, the device would not be addressable from the network as the mapping would not exist.
Hence, by default, after contacting the LwM2M Server, the device is configured to listen for 10 seconds after receiving the last packet.
After that idle period, the device enables eDRX and PSM power saving modes if those are supported by the network.
The device wakes up from sleep mode when it needs to send data.

Bootstrap support
=================

To successfully run the bootstrap procedure, you must first register the device in the LwM2M Bootstrap Server.
See :ref:`registering your device to an LwM2M Bootstrap Server <bootstrap_server_reg>` for instructions.

To build the LwM2M Client with LwM2M bootstrap support, use the :file:`overlay-avsystem-bootstrap.conf` or :file:`overlay-leshan-bootstrap.conf` configuration overlay.
For example:

.. parsed-literal::
   :class: highlight

   west build -b *board_target* -- -DEXTRA_CONF_FILE=overlay-leshan-bootstrap.conf

|board_target|

In bootstrap mode, application does not overwrite the PSK key from the modem so :ref:`CONFIG_APP_LWM2M_PSK <CONFIG_APP_LWM2M_PSK>` is not used.
Please refer to :ref:`lwm2m_client_provisioning` for instructions how to provision bootstrap keys.


MCUmgr client for external FOTA
===============================

Use one of the following build commands to evaluate external FOTA:

   .. tabs::

      .. group-tab:: MCUboot recovery mode with bootstrap

         To build for MCUboot recovery mode with bootstrap, use the following command:

         .. code-block:: console

            west build  --pristine -b nrf9160dk/nrf9160/ns --  -DEXTRA_CONF_FILE="overlay-adv-firmware.conf;overlay-fota_helper.conf;overlay-avsystem-bootstrap.conf;overlay-lwm2m-1.1.conf;overlay-mcumgr_client.conf; overlay-mcumgr_reset.conf" -DEXTRA_DTC_OVERLAY_FILE="nrf9160dk_mcumgr_client_uart2.overlay;nrf9160dk_recovery.overlay"

      .. group-tab:: MCUboot recovery mode without bootstrap

         To build for MCUboot recovery mode without bootstrap, use the following command:

         .. code-block:: console

            west build  --pristine -b nrf9160dk/nrf9160/ns --  -DEXTRA_CONF_FILE="overlay-adv-firmware.conf;overlay-fota_helper.conf;overlay-avsystem.conf;overlay-lwm2m-1.1.conf;overlay-mcumgr_client.conf; overlay-mcumgr_reset.conf" -DEXTRA_DTC_OVERLAY_FILE="nrf9160dk_mcumgr_client_uart2.overlay;nrf9160dk_recovery.overlay"

      .. group-tab:: MCUmgr client with bootstrap

         To build for MCUmgr client with bootstrap, use the following command:

         .. code-block:: console

            west build  --pristine -b nrf9160dk/nrf9160/ns --  -DEXTRA_CONF_FILE="overlay-adv-firmware.conf;overlay-fota_helper.conf;overlay-avsystem-bootstrap.conf;overlay-lwm2m-1.1.conf;overlay-mcumgr_client.conf" -DEXTRA_DTC_OVERLAY_FILE="nrf9160dk_mcumgr_client_uart2.overlay"

      .. group-tab:: MCUmgr client without bootstrap

         To build for MCUmgr client without bootstrap, use the following command:

         .. code-block:: console

            west build  --pristine -b nrf9160dk/nrf9160/ns --  -DEXTRA_CONF_FILE="overlay-adv-firmware.conf;overlay-fota_helper.conf;overlay-avsystem.conf;overlay-lwm2m-1.1.conf;overlay-mcumgr_client.conf" -DEXTRA_DTC_OVERLAY_FILE="nrf9160dk_mcumgr_client_uart2.overlay"


See :ref:`lwm2m_client_fota_external_mcu` for details.

Testing
=======

|test_sample|

   #. |connect_kit|
   #. |connect_terminal|
   #. Observe that the sample starts in the terminal window.
   #. Check that the device is connected to the chosen LwM2M Server.
   #. Press **Button 1** on nRF91 Series DK or **SW3** on Thingy:91 and confirm that the button event appears in the terminal.
   #. Check that the button press event has been registered on the LwM2M Server by confirming that the press count has been updated.
   #. Retrieve sensor data from various sensors and check if values are reasonable.
   #. Test GNSS module:

      a. Ensure that :ref:`CONFIG_GNSS_PRIORITY_ON_FIRST_FIX <CONFIG_GNSS_PRIORITY_ON_FIRST_FIX>` is enabled.
      #. Ensure that you are in a location with good GNSS signal, preferably outside.
      #. Wait for the GNSS to receive a fix, which will be displayed in the terminal.
         It might take several minutes for the first fix.

   #. Try to enable or disable some sensors in menuconfig and check if the sensors
      appear or disappear correspondingly in the LwM2M Server.

.. _lwmwm_client_testing_shell:

Testing with the LwM2M shell
----------------------------

To test the sample using :ref:`lwm2m_shell`, complete the following steps:

.. note::

   To enable any LwM2M v1.1 specific commands (for example, ``LwM2M SEND``), choose any of the v1.1 overlay files.

1. Open a terminal emulator and observe that the development kit produces an output similar to the above section.
#. Verify that the shell inputs are working correctly by running few commands.
   You can, for example, run commands to trigger a registration update, pause and start the client.

   a. Registration update:

      .. code-block:: console

         uart:~$ lwm2m update

   #. Pause the client:

      .. code-block:: console

         uart:~$ lwm2m pause

   #. Resume the client:

      .. code-block:: console

         uart:~$ lwm2m resume

Firmware Over-the-Air (FOTA)
============================

You can update the firmware of the device if you are using Coiote Device Management server or Leshan server.
Application firmware updates and modem firmware (both full and delta) updates are supported.

The client supports Push and Pull modes for image delivery.
Recommend transport types are CoAP or HTTP for Pull mode.
Coiote Device Management server also supports Multi component FOTA object, which allows updating multiple instances at the same time.

Use :file:`overlay-adv-firmware.conf` overlay file to enable the experimental Advanced Firmware Update object.
Advanced firmware requires `Coiote Device Management`_ server that supports it.
Refer to :ref:`lwm2m_client_fota` for more details.

.. note::

   You can use the :file:`overlay-fota_helper.conf` configuration file to enable faster responses when using queue mode binding.
   This configuration uses an update period of 60 seconds.

To update the firmware, complete the following steps:

   .. tabs::

      .. group-tab:: Coiote Basic Firmware update

         1. Identify the firmware image file to be uploaded to the device.
            See :ref:`lte_modem` and :ref:`nrf91_fota` for more information.
         #. Open `Coiote Device Management server`_ and click :guilabel:`Firmware update`.
         #. Click :guilabel:`Update Firmware`.
         #. Click :guilabel:`Basic Firmware update`.
         #. Click :guilabel:`Upload Firmware`.
         #. Select or upload the binary under **Upload New** or **from Resources** and click :guilabel:`Save`.
         #. Click :guilabel:`Next`.
         #. Set the image delivery mode, transport type, and timeout values.
            Coiote Device Management server recommends to use the **Pull** delivery mode because it is fail safe operation.
            Select the transport type (CoAP or HTTP) for the pull operation.
         #. Click :guilabel:`Next` to continue.
         #. Click :guilabel:`Schedule Update` after checking that the update process summary is correct.
         #. The firmware update starts at next registration update.

      .. group-tab:: Coiote Multi-component Firmware update

         Use the :file:`overlay-adv-firmware.conf` overlay file for multi component FOTA.

         1. Identify the firmware image file to be uploaded to the device.
            See :ref:`lte_modem` and :ref:`nrf91_fota` for more information.
         #. Open `Coiote Device Management server`_ and click :guilabel:`Firmware update`.
         #. Click :guilabel:`Update Firmware`.
         #. Click :guilabel:`Multi-component Firmware update`.
         #. Edit firmware update name if necessary and click :guilabel:`Next`.
         #. Select :guilabel:`application` component from tab and click :guilabel:`Upload Firmware`.
            Select or upload the binary under **Upload New** or **from Resources** and click :guilabel:`Save`.
            Click :guilabel:`Add new component`.
            Select :guilabel:`modem:xxx` component from tab and click :guilabel:`Upload Firmware`.
            Select or upload the binary under **Upload New** or **from Resources** and click :guilabel:`Save`.
         #. Click :guilabel:`Next`.
         #. Set the image delivery mode, transport type, and timeout values.
            Coiote Device Management server recommends to use the **Pull** delivery mode because it is fail safe operation.
            Select the transport type (CoAP or HTTP) for the pull operation.
         #. Click :guilabel:`Next` to continue.
         #. Click :guilabel:`Schedule Update` after checking that the update process summary is correct.
         #. The firmware update starts at next registration update.

      .. group-tab:: Leshan Firmware update

         1. Identify the firmware image file to be uploaded to the device. See :ref:`lte_modem` and :ref:`nrf91_fota` for more information.
         #. Open `Leshan Demo Server`_ and select :guilabel:`Firmware Update` from the left.
         #. Click :guilabel:`OBS` to observe the firmware update object.
         #. Upload the firmware image onto the :guilabel:`Package 5/0/0` resource or write the URL to the :guilabel:`Package URI 5/0/1` resource.
         #. Observe in the terminal window that the image file is being downloaded.
            The download will take some time.
         #. If the observation was successful, you should see that the :guilabel:`State 5/0/3` resource changed to `1`, which means the download is in progress.
         #. When the download has completed, you should see that the :guilabel:`State 5/0/3` resource changed to `2`, which means the download is complete.
         #. Click the :guilabel:`Update 5/0/2` resource to start the update process.
         #. The firmware of the device now updates and the device reconnects or reboots when the update is complete.


Dependencies
************

This sample application uses the following |NCS| libraries and drivers:

* :ref:`lib_lwm2m_client_utils`
* :ref:`lib_lwm2m_location_assistance`
* :ref:`modem_info_readme`
* :ref:`at_parser_readme`
* :ref:`dk_buttons_and_leds_readme`
* :ref:`lte_lc_readme`
* :ref:`lib_date_time`
* :ref:`lib_dfu_target`
* :ref:`lib_fmfu_fdev`
* :ref:`lib_fota_download`
* :ref:`lib_download_client`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

It uses the following Zephyr libraries:

* :ref:`gpio_api`
* :ref:`lwm2m_interface`
* :ref:`pwm_api`
* :ref:`zephyr:sensor`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
