.. _ug_nrf_cloud:

Using nRF Cloud with the |NCS|
##############################

.. contents::
   :local:
   :depth: 2

`nRF Cloud`_ is `Nordic Semiconductor's IoT cloud platform`_ that offers services in the fields of connectivity, device management, and location services.

Overview
********

nRF Cloud allows you to remotely manage and update your IoT devices using :term:`Firmware Over-the-Air (FOTA) <Firmware Over-the-Air (FOTA) update>`.
It also helps your devices determine their location.
Additionally, it allows devices to report data to the cloud for collection and analysis later.
To read more about nRF Cloud, see the `nRF Cloud`_ website and the `nRF Cloud documentation`_.

You can use the services offered by nRF Cloud in the following scenarios:

1. Device connected to nRF Cloud over MQTT. The services can be used from nRF Cloud.
#. Device connected to nRF Cloud over MQTT, with a customer-developed website or application that interacts with the `nRF Cloud REST API`_ to display device data and manage it in a customized way.
#. Device connected to nRF Cloud over REST, interacting using the `nRF Cloud REST API`_.
#. Device connected to a customer cloud service in a suitable manner. The services can be used from the customer cloud service that communicates over REST to the nRF Cloud REST API in a proxy configuration.

Choosing a protocol: MQTT or REST
*********************************

When choosing a protocol, consider the following:
* How often does the device transmit data?
* Which cloud APIs does the device need to access?
* What are the power consumption requirements for the device?
* What are the network data usage requirements for the device?
* What are the carrier's network settings (NAT timeout, eDRX/PSM) and how will the settings affect device behavior?

MQTT has a higher (data/power) cost to set up a connection.  However, the data size of an MQTT publish event is smaller than a comparable REST transaction.
MQTT may be preferred if a device is able to maintain a connection to the broker and sends/receives data frequently.
REST may be preferred if a device sends data infrequently or does not need to receive unsolicited data from the cloud.

REST overview
=============

* The device initiates a TLS connection to nRF Cloud.
* nRF Cloud supports a connection keep-alive/idle time of 60 seconds for REST API sockets.
* For authentication, the device must send a JSON Web Token (JWT) with each REST transaction.
  The JWT is approximately 450 bytes, but can be larger depending on the claims.
* Each REST transaction contains HTTP headers, including the JWT, and any API specific payload.

MQTT overview
=============

* The device initiates a mutual-TLS (mTLS) connection to the nRF Cloud MQTT broker.
* The MQTT keep-alive time can be set by the device and can be longer than 60s.
* Device authentication through mTLS lasts throughout the MQTT connection.
* Once connected, the device subscribes to the desired MQTT topics.
* Each MQTT publish event contains the MQTT topic and the payload.

|NCS| library support
*********************

The |NCS| provides the :ref:`lib_nrf_cloud` library, which if enabled, allows you to connect your devices to nRF Cloud and use the update, location, and connectivity services using MQTT or REST.

For more information on the various services, see the following documentation:

1. :ref:`lib_nrf_cloud_agps`
#. :ref:`lib_nrf_cloud_cell_pos`
#. :ref:`lib_nrf_cloud_fota`
#. :ref:`lib_nrf_cloud_pgps`

Applications and samples
************************

The following application uses the :ref:`lib_nrf_cloud` for services in |NCS|:

* :ref:`asset_tracker_v2`

The following samples demonstrate specific nRF Cloud functionality:

* :ref:`cloud_client`
* :ref:`gnss_sample`
* :ref:`lte_sensor_gateway`
* :ref:`multicell_location`
* :ref:`nrf_cloud_mqtt_multi_service`
* :ref:`nrf_cloud_rest_fota`
* :ref:`nrf_cloud_rest_device_message`
* :ref:`nrf_cloud_rest_cell_pos_sample`
