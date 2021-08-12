.. _ug_nrf_cloud:

Using nRF Cloud with the |NCS|
##############################

.. contents::
   :local:
   :depth: 2

Overview
********

`nRF Cloud`_ is `Nordic Semiconductor's IoT cloud platform`_ that offers services in the fields of connectivity, device management, and location services.
It allows you to remotely manage and update your IoT devices using Firmware Over The Air (FOTA).  It also helps your devices determine their location.  And it allows devices to report data to the cloud for collection and analysis later.

To read more about nRF Cloud, see the `nRF Cloud`_ website and the `nRF Cloud documentation`_.

The services nRF Cloud offers can be used multiple ways.

1. Via the `nRF Cloud`_ website over MQTT to the device
#. Over MQTT from the device to `nRF Cloud`_, with a customer-developed website or application which interacts with the `nRF Cloud REST API`_ to display device data and manage it in a customized way
#. Over REST from the device to the `nRF Cloud REST API`_
#. From a customer cloud service over REST to the `nRF Cloud REST API`_ in a proxy configuration, with connectivity from the device to the customer cloud service in whatever manner is suitable

NCS library support
*******************

The |NCS| provides nRF Cloud's :ref:`lib_nrf_cloud` library.
Enabling the library allows you to connect your devices to nRF Cloud and consume update, location, and connectivity services using MQTT or REST.

General documentation for the library and specific documentation for various services are located in the following places:

1. :ref:`lib_nrf_cloud` -- general library information as well as nRF Cloud FOTA
#. :ref:`lib_nrf_cloud_agps`
#. :ref:`lib_nrf_cloud_cell_pos`
#. :ref:`lib_nrf_cloud_pgps`

Applications and samples
************************

Applications that use the nRF Cloud library for services in |NCS| are:

* :ref:`asset_tracker`
* :ref:`asset_tracker_v2`

Samples which demonstrate specific nRF Cloud functionality are:

* :ref:`agps_sample`
* :ref:`cloud_client`
* :ref:`lte_sensor_gateway`
* :ref:`multicell_location`
