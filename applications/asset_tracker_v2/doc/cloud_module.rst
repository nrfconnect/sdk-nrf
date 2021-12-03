.. _asset_tracker_v2_cloud_module:

Cloud module
############

.. contents::
   :local:
   :depth: 2

The cloud module establishes and maintains the connection to a supported cloud service.
Use the :ref:`Cloud wrapper API <api_cloud_wrapper>` to integrate and handle the client libraries present in the |NCS|.

Features
********

This section documents the various features implemented by the module.

Cloud services
==============

The cloud module supports communication with multiple cloud services, one service at the time.
The :ref:`Supported cloud services <supported_cloud_services>` table lists the cloud services supported by the module together with protocols and technologies used with them.
See :ref:`Cloud wrapper API <api_cloud_wrapper>` for more information on how each client library is integrated in the module.

.. _supported_cloud_services:

+------------------+-------------------------------+
| Cloud service    | Protocols and technologies    |
+==================+===============================+
| `AWS IoT Core`_  |    `MQTT`_                    |
|                  +-------------------------------+
|                  |    `TCP`_                     |
|                  +-------------------------------+
|                  |    `TLS`_                     |
|                  +-------------------------------+
|                  |    :ref:`FOTA <nrf9160_fota>` |
|                  +-------------------------------+
|                  |    :ref:`lib_nrf_cloud_agps`  |
|                  +-------------------------------+
|                  |    :ref:`lib_nrf_cloud_pgps`  |
+------------------+-------------------------------+
| `Azure IoT Hub`_ |    `MQTT`_                    |
|                  +-------------------------------+
|                  |    `TCP`_                     |
|                  +-------------------------------+
|                  |    `TLS`_                     |
|                  +-------------------------------+
|                  |    :ref:`FOTA <nrf9160_fota>` |
|                  +-------------------------------+
|                  |    :ref:`lib_nrf_cloud_agps`  |
|                  +-------------------------------+
|                  |    :ref:`lib_nrf_cloud_pgps`  |
+------------------+-------------------------------+
| `nRF Cloud`_     |    `MQTT`_                    |
|                  +-------------------------------+
|                  |    `TCP`_                     |
|                  +-------------------------------+
|                  |    `TLS`_                     |
|                  +-------------------------------+
|                  |    :ref:`FOTA <nrf9160_fota>` |
|                  +-------------------------------+
|                  |    :ref:`lib_nrf_cloud_agps`  |
|                  +-------------------------------+
|                  |    :ref:`lib_nrf_cloud_pgps`  |
+------------------+-------------------------------+

.. _nrfcloud_agps_pgps:

nRF Cloud A-GPS and P-GPS
=========================

When the cloud module is configured to communicate with `AWS IoT Core`_ or `Azure IoT Hub`_, it supports processing of received A-GPS and P-GPS data using the :ref:`lib_nrf_cloud_agps` and :ref:`lib_nrf_cloud_pgps` libraries.
This enables the cloud to fetch A-GPS and P-GPS data from `nRF Cloud`_ using REST calls and relay the data to the nRF9160 SiP using the pre-established connection to `AWS IoT Core`_ or `Azure IoT Hub`_.
Maintaining multiple cloud connections at the same time is not feasible because of high data traffic and energy consumption.
Establishing a secure connection typically consists of multiple kB of exchanged data with the cloud service.
When configuring the application to communicate with nRF Cloud, A-GPS and P-GPS data are received directly from the service, in contrast to the AWS IoT and Azure IoT Hub implementations.

FOTA
====

The client libraries supported by the cloud wrapper API all implement their own version of :ref:`FOTA <nrf9160_fota>`.
This enables the cloud to issue FOTA updates and update the application and modem firmware while the device is in field.
For additional documentation on the various FOTA implementations, refer to the respective client library documentation linked to in :ref:`Integration layers <integration_layers>`.

Connection awareness
====================

The cloud module implements connection awareness by maintaing an internal state that is based on
events from the modem module and callbacks from the :ref:`Cloud wrapper API <api_cloud_wrapper>`.

If the module is disconnected, it will try to reconnect while the LTE connection is still valid.
To adjust the number of reconnection attempts, set the :ref:`CONFIG_CLOUD_CONNECT_RETRIES <CONFIG_CLOUD_CONNECT_RETRIES>` option.
Reconnection is implemented with a binary backoff based on the following lookup table:

.. code-block:: c

   static struct cloud_backoff_delay_lookup backoff_delay[] = {
      { 32 }, { 64 }, { 128 }, { 256 }, { 512 },
      { 2048 }, { 4096 }, { 8192 }, { 16384 }, { 32768 },
      { 65536 }, { 131072 }, { 262144 }, { 524288 }, { 1048576 }
   };

If the module reaches the maximum number of reconnection attempts, the application receives an error event notification of type :c:enum:`CLOUD_EVT_ERROR`, causing the application to perform a reboot.

Configuration options
*********************

.. _CONFIG_CLOUD_THREAD_STACK_SIZE:

CONFIG_CLOUD_THREAD_STACK_SIZE - Cloud module thread stack size
   This option increases the cloud module's internal thread stack size.

.. _CONFIG_CLOUD_CLIENT_ID_USE_CUSTOM:

CONFIG_CLOUD_CLIENT_ID_USE_CUSTOM - Configuration for enabling the use of a custom cloud client ID
   This option is used to enable the use of a custom client ID for connection to the respective cloud service.
   By default, the cloud module uses the IMEI of the nRF9160-based device as the client ID.

.. _CONFIG_CLOUD_CLIENT_ID:

CONFIG_CLOUD_CLIENT_ID - Configuration for providing a custom cloud client ID
   This option sets the custom client ID for the respective cloud service.
   For setting a custom client ID, you need to set :kconfig:`CONFIG_CLOUD_CLIENT_ID_USE_CUSTOM` to ``y``.

.. _CONFIG_CLOUD_CONNECT_RETRIES:

CONFIG_CLOUD_CONNECT_RETRIES - Configuration that sets the number of cloud reconnection attempts
   This option sets the number of times that a connection will be re-attempted upon a disconnect from the cloud service.

.. _mandatory_config:

Mandatory configurations
========================

To be able to use a supported cloud client library, you need to set a few mandatory Kconfig options.
These typically include the cloud service hostname and the security tag associated with the certificates used to establish a connection.
Before running the application, you need to provision the certificates to the modem using the same security tag.
For more information on how to set up a connection and provision certificates to the modem, see the documentation for the respective client library in :ref:`Integration layers <integration_layers>`.

.. note::
   There are no mandatory configuration settings for the :ref:`lib_nrf_cloud` library.
   The nRF9160 DK and Thingy91 come preprovisioned with certificates required to establish a connection to nRF Cloud.
   The default configuration of the :ref:`lib_nrf_cloud` library uses the security tag that the nRF Cloud certificates are stored to.

Configurations for AWS IoT library
----------------------------------

To enable communication with AWS IoT, set the following options in the :file:`overlay-aws.conf` file:

* :kconfig:`CONFIG_AWS_IOT_BROKER_HOST_NAME`
* :kconfig:`CONFIG_AWS_IOT_SEC_TAG`

Configurations for Azure IoT Hub library
----------------------------------------

To enable communication with Azure IoT Hub, set the following options in the :file:`overlay-azure.conf` file:

* :kconfig:`CONFIG_AZURE_IOT_HUB_DPS_HOSTNAME`
* :kconfig:`CONFIG_AZURE_IOT_HUB_DPS_ID_SCOPE`
* :kconfig:`CONFIG_AZURE_IOT_HUB_SEC_TAG`
* :kconfig:`CONFIG_AZURE_FOTA_SEC_TAG`

Module hierarchy
****************

The following diagram illustrates the relationship between the cloud module, integration layers, and client libraries.

.. figure:: /images/asset_tracker_v2_cloud_module_hierarchy.svg
    :alt: Cloud module hierarchy

    Cloud module hierarchy

Module states
*************

The cloud module has an internal state machine with the following states:

* ``STATE_LTE_INIT`` - The initial state of the module in which it awaits the modem to be initialized.
* ``STATE_LTE_DISCONNECTED`` - The module has performed all required initialization and waits for the modem to connect to LTE.
* ``STATE_LTE_CONNECTED`` - The modem is connected to LTE and the internal cloud connection routine starts. This state has two sub-states:

   * ``SUB_STATE_CLOUD_DISCONNECTED`` - The cloud service is disconnected.
   * ``SUB_STATE_CLOUD_CONNECTED`` - The cloud service is connected, data can now be sent.

* ``STATE_SHUTDOWN`` - The module has been shut down after receiving a request to do so from the util module.

State transitions take place based on events from other modules, such as the app module, data module, and util module.

Module events
*************

The :file:`asset_tracker_v2/src/events/cloud_module_event.h` header file contains a list of various events sent by the module.

Dependencies
************

This module uses the following |NCS| libraries and drivers:

* :ref:`api_cloud_wrapper`
* :ref:`lib_nrf_cloud_agps`
* :ref:`lib_nrf_cloud_pgps`

API documentation
*****************

| Header file: :file:`asset_tracker_v2/src/events/cloud_module_event.h`
| Source files: :file:`asset_tracker_v2/src/events/cloud_module_event.c`
                :file:`asset_tracker_v2/src/modules/cloud_module.c`

.. doxygengroup:: cloud_module_event
   :project: nrf
   :members:
