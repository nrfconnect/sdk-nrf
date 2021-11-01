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

|NCS| library support
*********************

The |NCS| provides the :ref:`lib_nrf_cloud` library, which if enabled, allows you to connect your devices to nRF Cloud and use the update, location, and connectivity services using MQTT or REST.

For more information on the various services, see the following documentation:

1. :ref:`lib_nrf_cloud_agps`
#. :ref:`lib_nrf_cloud_cell_pos`
#. :ref:`lib_nrf_cloud_pgps`

Applications and samples
************************

The following applications use the :ref:`lib_nrf_cloud` for services in |NCS|:

* :ref:`asset_tracker`
* :ref:`asset_tracker_v2`

The following samples demonstrate specific nRF Cloud functionality:

* :ref:`agps_sample`
* :ref:`cloud_client`
* :ref:`gnss_sample`
* :ref:`lte_sensor_gateway`
* :ref:`multicell_location`
