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
Based on the Kconfig options listed in the table, the corresponding integration layer + client library combination is linked in.
For example, if :kconfig:`CONFIG_AWS_IOT` is enabled, CMake links in the :file:`asset_tracker_v2/src/cloud/aws_iot_integration.c` file that integrates the :ref:`lib_aws_iot` library.
This takes place in the :file:`asset_tracker_v2/src/cloud/CMakeLists.txt` file.

.. note::
   The various integration layers all share a common header file :file:`asset_tracker_v2/src/cloud/cloud_wrapper.h` that exposes generic functions to send and receive data from the integration layer.

.. _integration_layers:

+---------------------------+------------------------------------------------------------------+----------------------------------+
| Client library            | Integration layer                                                | Kconfig option                   |
+===========================+==================================================================+==================================+
| :ref:`lib_aws_iot`        |   :file:`asset_tracker_v2/src/cloud/aws_iot_integration.c`       | :kconfig:`CONFIG_AWS_IOT`        |
+---------------------------+------------------------------------------------------------------+----------------------------------+
| :ref:`lib_azure_iot_hub`  |   :file:`asset_tracker_v2/src/cloud/azure_iot_hub_integration.c` | :kconfig:`CONFIG_AZURE_IOT_HUB`  |
+---------------------------+------------------------------------------------------------------+----------------------------------+
| :ref:`lib_nrf_cloud`      |   :file:`asset_tracker_v2/src/cloud/nrf_cloud_integration.c`     | :kconfig:`CONFIG_NRF_CLOUD_MQTT` |
+---------------------------+------------------------------------------------------------------+----------------------------------+

Data addressing
***************

Each integration layer routes data to specific endpoints based on the content of the data and the cloud wapper API call.
The :ref:`Data routing tables <data_filtering>` list the endpoints that are used in each cloud service implementation.

.. _data_filtering:

AWS IoT topics
==============

Device-to-Cloud
---------------

+------------------------------+--------------------------------------------------------+
|              Data            |            Topic                                       |
+==============================+========================================================+
| A-GPS requests               | ``<imei>/agps/get``                                    |
+------------------------------+--------------------------------------------------------+
| P-GPS requests               | ``<imei>/pgps/get``                                    |
+------------------------------+--------------------------------------------------------+
| Neighbor cell measurements   | ``<imei>/ncellmeas``                                   |
+------------------------------+--------------------------------------------------------+
| Button presses               | ``<imei>/messages``                                    |
+------------------------------+--------------------------------------------------------+
| Sensor/device data           | ``$aws/things/<imei>/shadow/update``                   |
+------------------------------+--------------------------------------------------------+
| Device configuration         | ``$aws/things/<imei>/shadow/update``                   |
+------------------------------+--------------------------------------------------------+
| Buffered sensor/device data  | ``<imei>/batch``                                       |
+------------------------------+--------------------------------------------------------+

Cloud-to-Device
---------------

+------------------------------+--------------------------------------------------------+
|              Data            |            Topic                                       |
+==============================+========================================================+
| A-GPS response               | ``<imei>/agps``                                        |
+------------------------------+--------------------------------------------------------+
| P-GPS response               | ``<imei>/pgps``                                        |
+------------------------------+--------------------------------------------------------+
| Device configuration updates | ``$aws/things/<imei>/shadow/delta``                    |
|                              +--------------------------------------------------------+
|                              | ``$aws/things/<imei>/shadow/get/accepted``             |
|                              +--------------------------------------------------------+
|                              | ``$aws/things/<imei>/shadow/get/accepted/desired/cfg`` |
+------------------------------+--------------------------------------------------------+

Azure IoT Hub topics
====================

For simplicity, the following table omits certain metavalues present in topics and property bags used in Azure IoT Hub.
For more information on MQTT topics and property bags in Azure IoT Hub, refer to the `Azure IoT Hub MQTT protocol support`_ documentation.

Device-to-Cloud
---------------

+------------------------------+---------------------------------------------+--------------+
|               Data           |             Topic                           | Property bag |
+==============================+=============================================+==============+
| A-GPS requests               | ``devices/<imei>/messages/events/``         | ``agps=get`` |
+------------------------------+---------------------------------------------+--------------+
| P-GPS requests               | ``devices/<imei>/messages/events/``         | ``pgps=get`` |
+------------------------------+---------------------------------------------+--------------+
| Neighbor cell measurements   | ``devices/<imei>/messages/events/``         | ``ncellmeas``|
+------------------------------+---------------------------------------------+--------------+
| Button presses               | ``devices/<imei>/messages/events/``         | NA           |
+------------------------------+---------------------------------------------+--------------+
| Sensor/device data           | ``$iothub/twin/PATCH/properties/reported/`` | NA           |
+------------------------------+---------------------------------------------+--------------+
| Device configuration         | ``$iothub/twin/PATCH/properties/reported/`` | NA           |
+------------------------------+---------------------------------------------+--------------+
| Buffered sensor/device data  | ``devices/<imei>/messages/events/``         | ``batch``    |
+------------------------------+---------------------------------------------+--------------+

Cloud-to-Device
---------------

+------------------------------+------------------------------------------+----------------+
|               Data           |             Topic                        | Property bag   |
+==============================+==========================================+================+
| A-GPS response               | ``devices/<imei>/messages/devicebound/`` | ``agps=result``|
+------------------------------+------------------------------------------+----------------+
| P-GPS response               | ``devices/<imei>/messages/devicebound/`` | ``pgps=result``|
+------------------------------+------------------------------------------+----------------+
| Device configuration updates | ``$iothub/twin/res/<code>/``             | NA             |
+------------------------------+------------------------------------------+----------------+

nRF Cloud topics
================

For more information on topics used in the nRF Cloud connection, refer to the `nRF Cloud MQTT API`_ documentation.

Device-to-Cloud
---------------

+------------------------------+----------------------------------------------------+
|              Data            |            AWS IoT topic                           |
+==============================+====================================================+
| A-GPS requests               | ``<topic_prefix>/<imei>/d2c``                      |
+------------------------------+----------------------------------------------------+
| P-GPS requests               | ``<topic_prefix>/<imei>/d2c``                      |
+------------------------------+----------------------------------------------------+
| Neighbor cell measurements   | ``<topic_prefix>/<imei>/d2c``                      |
+------------------------------+----------------------------------------------------+
| Button presses               | ``<topic_prefix>/<imei>/d2c``                      |
+------------------------------+----------------------------------------------------+
| Sensor/device data           | ``<topic_prefix>/<imei>/d2c``                      |
+------------------------------+----------------------------------------------------+
| Device configuration         | ``$aws/things/<imei>/shadow/update``               |
+------------------------------+----------------------------------------------------+
| Buffered sensor/device data  | ``<topic_prefix>/<imei>/d2c/batch``                |
+------------------------------+----------------------------------------------------+

Cloud-to-Device
---------------

+------------------------------+----------------------------------------------------+
|              Data            |            AWS IoT topic                           |
+==============================+====================================================+
| A-GPS response               | ``<topic_prefix>/<imei>/c2d``                      |
+------------------------------+----------------------------------------------------+
| P-GPS response               | ``<topic_prefix>/<imei>/c2d``                      |
+------------------------------+----------------------------------------------------+
| Device configuration updates | ``$aws/things/<imei>/shadow/delta``                |
|                              +----------------------------------------------------+
|                              | ``$aws/things/<imei>/shadow/get/accepted``         |
|                              +----------------------------------------------------+
|                              | ``$<imei>/shadow/get/accepted/desired/cfg``        |
+------------------------------+----------------------------------------------------+

Dependencies
************

This module uses the following |NCS| libraries and drivers:

* :ref:`lib_nrf_cloud`
* :ref:`lib_aws_iot`
* :ref:`lib_azure_iot_hub`

API documentation
*****************

| Header file: :file:`asset_tracker_v2/src/cloud/cloud_wrapper.h`
| Source files: :file:`asset_tracker_v2/src/cloud/nrf_cloud_integration.c`
                :file:`asset_tracker_v2/src/cloud/aws_iot_integration.c`
                :file:`asset_tracker_v2/src/cloud/azure_iot_hub_integration.c`

.. doxygengroup:: cloud_wrapper
   :project: nrf
   :members:
