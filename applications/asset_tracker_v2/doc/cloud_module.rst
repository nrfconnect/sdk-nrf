.. _asset_tracker_v2_cloud_module:

Cloud module
############

.. contents::
   :local:
   :depth: 2

The cloud module establishes and maintains the connection to a supported cloud service.
It uses the :ref:`Cloud wrapper API <api_cloud_wrapper>` to integrate and handle the client libraries present in the |NCS|.

Features
********

This section documents the various features implemented by the module.

Cloud services
==============

The cloud module supports communication with multiple cloud services, one service at the time.
The :ref:`Supported cloud services <supported_cloud_services>` table lists the cloud services supported by the module together with protocols and technologies used with them.
See :ref:`Cloud wrapper API <api_cloud_wrapper>` for more information on how each client library is integrated in the module.

.. _supported_cloud_services:

+------------------------------------------------------------------------------------+-------------------------------+
| Cloud service                                                                      | Protocols and technologies    |
+====================================================================================+===============================+
| `AWS IoT Core`_                                                                    |    `MQTT`_                    |
|                                                                                    +-------------------------------+
|                                                                                    |    `TLS`_                     |
|                                                                                    +-------------------------------+
|                                                                                    |    `TCP`_                     |
|                                                                                    +-------------------------------+
|                                                                                    |    :ref:`FOTA <nrf9160_fota>` |
|                                                                                    +-------------------------------+
|                                                                                    |    :ref:`lib_nrf_cloud_agps`  |
|                                                                                    +-------------------------------+
|                                                                                    |    :ref:`lib_nrf_cloud_pgps`  |
+------------------------------------------------------------------------------------+-------------------------------+
| `Azure IoT Hub`_                                                                   |    `MQTT`_                    |
|                                                                                    +-------------------------------+
|                                                                                    |    `TLS`_                     |
|                                                                                    +-------------------------------+
|                                                                                    |    `TCP`_                     |
|                                                                                    +-------------------------------+
|                                                                                    |    :ref:`FOTA <nrf9160_fota>` |
|                                                                                    +-------------------------------+
|                                                                                    |    :ref:`lib_nrf_cloud_agps`  |
|                                                                                    +-------------------------------+
|                                                                                    |    :ref:`lib_nrf_cloud_pgps`  |
+------------------------------------------------------------------------------------+-------------------------------+
| `nRF Cloud`_                                                                       |    `MQTT`_                    |
|                                                                                    +-------------------------------+
|                                                                                    |    `TLS`_                     |
|                                                                                    +-------------------------------+
|                                                                                    |    `TCP`_                     |
|                                                                                    +-------------------------------+
|                                                                                    |    :ref:`FOTA <nrf9160_fota>` |
|                                                                                    +-------------------------------+
|                                                                                    |    :ref:`lib_nrf_cloud_agps`  |
|                                                                                    +-------------------------------+
|                                                                                    |    :ref:`lib_nrf_cloud_pgps`  |
+------------------------------------------------------------------------------------+-------------------------------+
| `LwM2M`_ v1.1 compliant service (`Coiote Device Management`_, `Leshan homepage`_)  |    `LwM2M`_                   |
|                                                                                    +-------------------------------+
|                                                                                    |    `CoAP`_                    |
|                                                                                    +-------------------------------+
|                                                                                    |    `DTLS`_                    |
|                                                                                    +-------------------------------+
|                                                                                    |    `UDP protocol`_            |
|                                                                                    +-------------------------------+
|                                                                                    |    :ref:`FOTA <nrf9160_fota>` |
|                                                                                    +-------------------------------+
|                                                                                    |    :ref:`lib_nrf_cloud_agps`  |
+------------------------------------------------------------------------------------+-------------------------------+

.. _nrfcloud_agps_pgps:

nRF Cloud A-GPS and P-GPS
=========================

When the cloud module is configured to communicate with `AWS IoT Core`_, `Azure IoT Hub`_, or an `LwM2M`_ server, it supports processing of received A-GPS and P-GPS data using the :ref:`lib_nrf_cloud_agps` and :ref:`lib_nrf_cloud_pgps` libraries.
This enables the cloud service to fetch A-GPS and P-GPS data directly from `nRF Cloud`_ using REST calls and relay this data to the nRF9160 SiP using the pre-established cloud connection.
By reusing the pre-established connection, the application saves overhead related to maintaining multiple connections at the same time.
When configuring the application to communicate with nRF Cloud, A-GPS and P-GPS data are received directly from the service, and not by proxy.
For more information, see `nRF Cloud Location Services <nRF Cloud Location Services documentation_>`_.

FOTA
====

The client libraries supported by the cloud wrapper API all implement their own version of :ref:`FOTA <nrf9160_fota>`.
This enables the cloud to issue FOTA updates and update the application and modem firmware while the device is in field.
For additional documentation on the various FOTA implementations, refer to the respective client library documentation linked to in :ref:`Integration layers <integration_layers>`.

Full modem FOTA updates are only supported by nRF Cloud.
This application implements full modem FOTA only for the nRF9160 development kit version 0.14.0 and higher.
To enable full modem FOTA, add the ``-DEXTRA_CONF_FILE=overlay-full_modem_fota.conf`` parameter to your build command.

Also, specify your development kit version by appending it to the board name.
For example, if your development kit version is 1.0.1, use the board name ``nrf9160dk_nrf9160_ns@1_0_1`` in your build command.

Connection awareness
====================

The cloud module implements connection awareness by maintaining an internal state that is based on
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

   For setting a custom client ID, you need to set :ref:`CONFIG_CLOUD_CLIENT_ID_USE_CUSTOM <CONFIG_CLOUD_CLIENT_ID_USE_CUSTOM>` to ``y``.

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

* :kconfig:option:`CONFIG_AWS_IOT_BROKER_HOST_NAME`
* :kconfig:option:`CONFIG_AWS_IOT_SEC_TAG`

Configurations for Azure IoT Hub library
----------------------------------------

To enable communication with Azure IoT Hub, set the following options in the :file:`overlay-azure.conf` file:

* :kconfig:option:`CONFIG_AZURE_IOT_HUB_DPS_ID_SCOPE`
* :kconfig:option:`CONFIG_MQTT_HELPER_SEC_TAG`
* :kconfig:option:`CONFIG_AZURE_FOTA_SEC_TAG`

If not using the default DPS (Device Provisioning Service) host, ensure that the hostname option is correctly set using the following Kconfig option:

* :kconfig:option:`CONFIG_AZURE_IOT_HUB_DPS_HOSTNAME`

.. _assettracker_v2_cloudmodule_lwm2m:

Configurations for LwM2M integration layer
------------------------------------------

When building for LwM2M, the cloud module's default configuration is to communicate with AVSystem's `Coiote Device Management`_, with a runtime provisioned `Pre-shared key (PSK)`_ set by the :kconfig:option:`CONFIG_LWM2M_INTEGRATION_PSK` option.
This enables the device to work with `Coiote Device Management`_ without provisioning the PSK to the modem before running the application.
To allow the device to communicate with other LwM2M servers, modify the default configuration by changing the following Kconfig options:

* :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_SERVER`
* :kconfig:option:`CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP`
* :kconfig:option:`CONFIG_LWM2M_INTEGRATION_ENDPOINT_PREFIX`
* :kconfig:option:`CONFIG_LWM2M_INTEGRATION_PSK`
* :kconfig:option:`CONFIG_LWM2M_INTEGRATION_PROVISION_CREDENTIALS`

See :ref:`server setup <server_setup_lwm2m_client>` for information on how the `Coiote Device Management server`_ can be configured to communicate with the application.

.. important::
   In production, it is not recommended to use the default PSK that is automatically provisioned by the application.
   If possible, bootstrapping should be enabled to periodically change the PSK used in the connection.
   The PSK should also be provisioned to the modem before running the application.
   Disable the :kconfig:option:`CONFIG_LWM2M_INTEGRATION_PROVISION_CREDENTIALS` option and provision the PSK to a sec tag set by :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_SERVER_TLS_TAG` or :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_BOOTSTRAP_TLS_TAG`.


In addition to the steps documented in the aforementioned section, you must also enable manipulation of the application's real-time configurations through the `Coiote Device Management`_ console.
This is documented in :ref:`object_xml_config`.

Module hierarchy
****************

The following diagram illustrates the relationship between the cloud module, integration layers, and the client libraries.

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
