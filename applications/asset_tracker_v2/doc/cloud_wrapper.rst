.. _api_cloud_wrapper:

Cloud wrapper API
#################

.. contents::
   :local:
   :depth: 2

The cloud wrapper API is a generic API used for controlling the connection to a supported |NCS| client library through :ref:`Integration layers <integration_layers>`.
It exposes generic functions such as ``send``, ``connect``, and ``disconnect``, hiding the functionality that is specific to a single client library implementation.

Integration layers
******************

The :ref:`Integration layers <integration_layers>` table lists the |NCS| client libraries that are supported by the cloud wrapper API and the associated integration layers.
Each integration layer interacts with a specific client library and contains the code required to properly set up and maintain a connection to the designated cloud service.
Based on the Kconfig options listed in the table, the corresponding combination of integration layer and the client library is linked in.
For example, if :kconfig:option:`CONFIG_AWS_IOT` is enabled, CMake links in the :file:`asset_tracker_v2/src/cloud/aws_iot_integration.c` file that integrates the :ref:`lib_aws_iot` library.
This takes place in the :file:`asset_tracker_v2/src/cloud/CMakeLists.txt` file.

.. note::
   The various integration layers all share a common header file :file:`asset_tracker_v2/src/cloud/cloud_wrapper.h` that exposes generic functions to send and receive data from the integration layer.

.. _integration_layers:

+-----------------------------+------------------------------------------------------------------+--------------------------------------------+
| Client library              | Integration layer                                                | Kconfig option                             |
+=============================+==================================================================+============================================+
| :ref:`lib_aws_iot`          |   :file:`asset_tracker_v2/src/cloud/aws_iot_integration.c`       | :kconfig:option:`CONFIG_AWS_IOT`           |
+-----------------------------+------------------------------------------------------------------+--------------------------------------------+
| :ref:`lib_azure_iot_hub`    |   :file:`asset_tracker_v2/src/cloud/azure_iot_hub_integration.c` | :kconfig:option:`CONFIG_AZURE_IOT_HUB`     |
+-----------------------------+------------------------------------------------------------------+--------------------------------------------+
| :ref:`lib_nrf_cloud`        |   :file:`asset_tracker_v2/src/cloud/nrf_cloud_integration.c`     | :kconfig:option:`CONFIG_NRF_CLOUD_MQTT`    |
+-----------------------------+------------------------------------------------------------------+--------------------------------------------+
| :ref:`lwm2m_interface`      |   :file:`asset_tracker_v2/src/cloud/lwm2m_integration.c`         | :kconfig:option:`CONFIG_LWM2M_INTEGRATION` |
+-----------------------------+------------------------------------------------------------------+--------------------------------------------+

.. _lwm2m_integration_details:

LwM2M
=====

The application integrates LwM2M through the following APIs:

* :ref:`LwM2M API <lwm2m_interface>` from Zephyr
* :ref:`LwM2M client utils API <lib_lwm2m_client_utils>`, and :ref:`LwM2M location assistance API <lib_lwm2m_location_assistance>` from |NCS|

.. note::

   The LwM2M integration does not have support for sending of batched data.

Bootstrapping and credential handling
-------------------------------------

When the option :kconfig:option:`CONFIG_LWM2M_INTEGRATION_PSK` is enabled, the modem is provisioned at run time after boot with a `pre-shared key (PSK) <Pre-Shared Key (PSK)_>`_ set by :kconfig:option:`CONFIG_LWM2M_INTEGRATION_PSK`.

If :kconfig:option:`CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP` is enabled, the PSK is provisioned to a security tag dedicated to the bootstrap server connection.
During bootstrapping, the application receives a separate newly generated key from the bootstrap server that is provisioned to a security tag dedicated to the management server connection.
This PSK is used in the management server that the application connects to, after bootstrapping is completed.

If :kconfig:option:`CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP` is disabled, the PSK that is provisioned after boot is provisioned to the security tag dedicated to the management server and the application connects to the management server directly.

In a production scenario, it is recommended to generate and preprovision the bootstrap server PSK prior to running the application.
You can do this by disabling the :kconfig:option:`CONFIG_LWM2M_INTEGRATION_PROVISION_CREDENTIALS` option and following the steps described in :ref:`Preparing for production <lwm2m_client_provisioning>`.

The security tags that are used for the management and bootstrap server connections are set by the :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_BOOTSTRAP_TLS_TAG` and :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_SERVER_TLS_TAG` options, respectively.
When preprovisioning credentials, make sure the correct TLS security tag is used.

.. note::
   Bootstrapping enables the application to rotate security credentials after deployment, which is recommended from a security standpoint.

Queue mode
----------

Due to short NAT timeouts (approximately 60 seconds) and firewalls in UDP delivery networks, the application enables LwM2M Queue mode.
In LwM2M Queue mode, the Zephyr LwM2M engine closes and opens a new socket for every transmission to cloud.
This means that a new DLTS handshake is performed for every update to cloud.
The overhead associated with DTLS handshakes is mitigated by enabling TLS session resumption.
This enables the modem to restore the previously negotiated TLS session with the server and it does not require a full TLS handshake.

The time that the LwM2M engine polls for data after the last correspondence with cloud is set by the :kconfig:option:`CONFIG_LWM2M_QUEUE_MODE_UPTIME` Kconfig option.
Increasing this value beyond 60 seconds does not cause any change due to the NAT and firewall issue mentioned previously.
But it can be increased if the LTE network allows it.
If increasing the :kconfig:option:`CONFIG_LWM2M_QUEUE_MODE_UPTIME` option, make sure that the LTE PSM active timeout set by :kconfig:option:`CONFIG_LTE_PSM_REQ_RAT` is also increased to a value greater than the Queue mode uptime.
This ensures that the modem goes into LTE PSM only after the LwM2M engine has finished polling for incoming data.

.. note::

   The :kconfig:option:`CONFIG_LTE_PSM_REQ_RAT` option only increases the value that is requested by the modem from the network, not what is actually given by the network.

Send operation
--------------

The application exclusively uses the `LwM2M v1.1`_ send operation to send data to the server.
This operation enables the application to explicitly send data to the server, whenever it wants.
The user can set up the server to enable periodic notifications for objects and resources, but there is no guarantee that the resources will change value between notifications using that approach.
The application adheres to its :ref:`Real-time configurations <real_time_configs>` when it samples and sends data to cloud, which is the recommended approach to reconfiguring the application.
The :ref:`Real-time configurations <real_time_configs>` can be manipulated using an application specific ``configuration object`` accessible from cloud.
In order to use this object, some additional steps are required that are documented in :ref:`object_xml_config`.

Data addressing
***************

Each integration layer routes data to specific endpoints based on the content of the data and the cloud wrapper API call.
The :ref:`Data routing tables <data_filtering>` list the endpoints that are used in each cloud service implementation.

.. _data_filtering:

AWS IoT topics
==============

The following tables list the various topics used in the AWS IoT implementation.

Device-to-cloud (D2C)
---------------------

+------------------------------+---------------------------------------------------------+
|              Data            |            Topic                                        |
+==============================+=========================================================+
| A-GNSS requests              | ``<imei>/agnss/get``                                    |
+------------------------------+---------------------------------------------------------+
| P-GPS requests               | ``<imei>/pgps/get``                                     |
+------------------------------+---------------------------------------------------------+
| Neighbor cell measurements   | ``<imei>/ncellmeas``                                    |
+------------------------------+---------------------------------------------------------+
| Button presses               | ``<imei>/messages``                                     |
+------------------------------+---------------------------------------------------------+
| Sensor/device data           | ``$aws/things/<imei>/shadow/update``                    |
+------------------------------+---------------------------------------------------------+
| Device configuration         | ``$aws/things/<imei>/shadow/update``                    |
+------------------------------+---------------------------------------------------------+
| Buffered sensor/device data  | ``<imei>/batch``                                        |
+------------------------------+---------------------------------------------------------+

Cloud-to-device (C2D)
---------------------

+------------------------------+---------------------------------------------------------+
|              Data            |            Topic                                        |
+==============================+=========================================================+
| A-GNSS response              | ``<imei>/agnss``                                        |
+------------------------------+---------------------------------------------------------+
| P-GPS response               | ``<imei>/pgps``                                         |
+------------------------------+---------------------------------------------------------+
| Device configuration updates | ``$aws/things/<imei>/shadow/delta``                     |
|                              +---------------------------------------------------------+
|                              | ``$aws/things/<imei>/shadow/get/accepted``              |
|                              +---------------------------------------------------------+
|                              | ``$aws/things/<imei>/shadow/get/accepted/desired/cfg``  |
+------------------------------+---------------------------------------------------------+

Azure IoT Hub topics
====================

For simplicity, the following table omits certain meta values present in topics and property bags used in Azure IoT Hub.
For more information on MQTT topics and property bags in Azure IoT Hub, refer to the `Azure IoT Hub MQTT protocol support`_ documentation.

Device-to-cloud (D2C)
---------------------

+------------------------------+---------------------------------------------+---------------+
|               Data           |             Topic                           | Property bag  |
+==============================+=============================================+===============+
| A-GNSS requests              | ``devices/<imei>/messages/events/``         | ``agnss=get`` |
+------------------------------+---------------------------------------------+---------------+
| P-GPS requests               | ``devices/<imei>/messages/events/``         | ``pgps=get``  |
+------------------------------+---------------------------------------------+---------------+
| Neighbor cell measurements   | ``devices/<imei>/messages/events/``         | ``ncellmeas`` |
+------------------------------+---------------------------------------------+---------------+
| Button presses               | ``devices/<imei>/messages/events/``         |     NA        |
+------------------------------+---------------------------------------------+---------------+
| Sensor/device data           | ``$iothub/twin/PATCH/properties/reported/`` |     NA        |
+------------------------------+---------------------------------------------+---------------+
| Device configuration         | ``$iothub/twin/PATCH/properties/reported/`` |     NA        |
+------------------------------+---------------------------------------------+---------------+
| Buffered sensor/device data  | ``devices/<imei>/messages/events/``         | ``batch``     |
+------------------------------+---------------------------------------------+---------------+

Cloud-to-device (C2D)
---------------------

+------------------------------+------------------------------------------+-----------------+
|               Data           |             Topic                        | Property bag    |
+==============================+==========================================+=================+
| A-GNSS response              | ``devices/<imei>/messages/devicebound/`` | ``agnss=result``|
+------------------------------+------------------------------------------+-----------------+
| P-GPS response               | ``devices/<imei>/messages/devicebound/`` | ``pgps=result`` |
+------------------------------+------------------------------------------+-----------------+
| Device configuration updates | ``$iothub/twin/res/<code>/``             |      NA         |
+------------------------------+------------------------------------------+-----------------+

nRF Cloud topics
================

For more information on topics used in the nRF Cloud connection, refer to the `nRF Cloud MQTT API`_ documentation.

Device-to-cloud (D2C)
---------------------

+------------------------------+----------------------------------------------------+
|              Data            |            AWS IoT topic                           |
+==============================+====================================================+
| A-GNSS requests              | ``<topic_prefix>/<imei>/d2c``                      |
+------------------------------+----------------------------------------------------+
| P-GPS requests               | ``<topic_prefix>/<imei>/d2c``                      |
+------------------------------+----------------------------------------------------+
| Neighbor cell measurements   | ``<topic_prefix>/<imei>/d2c``                      |
+------------------------------+----------------------------------------------------+
| Wi-Fi access points          | ``<topic_prefix>/<imei>/d2c``                      |
+------------------------------+----------------------------------------------------+
| Button presses               | ``<topic_prefix>/<imei>/d2c``                      |
+------------------------------+----------------------------------------------------+
| Sensor/device data           | ``<topic_prefix>/<imei>/d2c``                      |
+------------------------------+----------------------------------------------------+
| Device configuration         | ``$aws/things/<imei>/shadow/update``               |
+------------------------------+----------------------------------------------------+
| Buffered sensor/device data  | ``<topic_prefix>/<imei>/d2c/batch``                |
+------------------------------+----------------------------------------------------+

Cloud-to-device (C2D)
---------------------

+------------------------------+----------------------------------------------------+
|              Data            |            AWS IoT topic                           |
+==============================+====================================================+
| A-GNSS response              | ``<topic_prefix>/<imei>/c2d``                      |
+------------------------------+----------------------------------------------------+
| P-GPS response               | ``<topic_prefix>/<imei>/c2d``                      |
+------------------------------+----------------------------------------------------+
| Device configuration updates | ``$aws/things/<imei>/shadow/delta``                |
|                              +----------------------------------------------------+
|                              | ``$aws/things/<imei>/shadow/get/accepted``         |
|                              +----------------------------------------------------+
|                              | ``$<imei>/shadow/get/accepted/desired/cfg``        |
+------------------------------+----------------------------------------------------+

LwM2M objects
=============

For more information on objects used in LwM2M, refer to the `OMA LwM2M Object and Resource Registry`_.

+------------------------------------------------------------------+----------------------+
|              Objects                                             |            Object ID |
+==================================================================+======================+
| LwM2M Server                                                     | 1                    |
+------------------------------------------------------------------+----------------------+
| Device                                                           | 3                    |
+------------------------------------------------------------------+----------------------+
| Connectivity Monitoring                                          | 4                    |
+------------------------------------------------------------------+----------------------+
| Firmware Update                                                  | 5                    |
+------------------------------------------------------------------+----------------------+
| Location                                                         | 6                    |
+------------------------------------------------------------------+----------------------+
| Temperature                                                      | 3303                 |
+------------------------------------------------------------------+----------------------+
| Humidity                                                         | 3304                 |
+------------------------------------------------------------------+----------------------+
| Pressure                                                         | 3323                 |
+------------------------------------------------------------------+----------------------+
| Push Button                                                      | 3347                 |
+------------------------------------------------------------------+----------------------+
| ECID-Signal Measurement Information (Neighbor cell measurements) | 10256                |
+------------------------------------------------------------------+----------------------+
| Ground Fix (Cellular and Wi-Fi location)                         | 33626                |
+------------------------------------------------------------------+----------------------+
| GNSS Assistance (A-GNSS / P-GPS)                                 | 33625                |
+------------------------------------------------------------------+----------------------+
| Configuration (see :ref:`object_xml_config`)                     | 50009                |
+------------------------------------------------------------------+----------------------+

.. _object_xml_config:

Uploading XML definition for configuration object
-------------------------------------------------

The application defines a proprietary ``Configuration object`` that the LwM2M Server needs to be made aware of to enable the manipulation of its resources using the web console.
If you are using `Coiote Device Management`_, complete the following steps to add the LwM2M object definition:

1. Open `Coiote Device Management server`_.
#. Click the device inventory icon (second icon from the top) in the left pane in the UI.

   .. figure:: /images/coiote_device_mgmt_server_ui.png
      :alt: Coiote Device Management Server UI

      Coiote Device Management Server UI

#. Locate your Device ID and click on :guilabel:`Management`.
#. Click :guilabel:`Objects` in the left vertical tabs section.
#. Click :guilabel:`Add new LwM2M object definition`.
#. Upload the file :file:`nrf/applications/asset_tracker_v2/src/cloud/lwm2m_integration/config_object_descript.xml` or copy and paste the contents of the file to the textbox.
#. Click :guilabel:`Import`, :guilabel:`Refresh data model` and :guilabel:`Yes, execute task now`.

After completing the previous steps, the configuration object is detected in the console and you can set the different resources in the object.
These resources configure the real-time behavior of the application and maps directly to the configurations listed in :ref:`Real-time configurations <real_time_configs>`.

Dependencies
************

This module uses the following |NCS| libraries and drivers:

* :ref:`lib_nrf_cloud`
* :ref:`lib_aws_iot`
* :ref:`lib_azure_iot_hub`
* :ref:`lib_lwm2m_client_utils`
* :ref:`lib_lwm2m_location_assistance`
* :ref:`lwm2m_interface`

API documentation
*****************

| Header file: :file:`asset_tracker_v2/src/cloud/cloud_wrapper.h`
| Source files: :file:`asset_tracker_v2/src/cloud/nrf_cloud_integration.c`
                :file:`asset_tracker_v2/src/cloud/aws_iot_integration.c`
                :file:`asset_tracker_v2/src/cloud/azure_iot_hub_integration.c`
                :file:`asset_tracker_v2/src/cloud/lwm2m_integration.c`

.. doxygengroup:: cloud_wrapper
   :project: nrf
   :members:
