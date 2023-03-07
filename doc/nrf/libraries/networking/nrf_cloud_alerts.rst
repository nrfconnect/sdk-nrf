.. _lib_nrf_cloud_alerts:

nRF Cloud Alerts
################

.. contents::
   :local:
   :depth: 2

This library is an enhancement to the :ref:`lib_nrf_cloud` library.
It enables applications to generate and transmit messages that comply with the Alerts feature of nRF Cloud.
Alerts messages are compact JSON documents that indicate a critical event, such as low battery or unexpected reset.

Overview
********

This library provides an API for either REST-based or MQTT-based applications to send alerts to nRF Cloud.
For MQTT-based applications, you can enable or disable alerts remotely using the nRF Cloud portal or REST API.

Each alert contains the following elements, as defined in the :c:struct:`nrf_cloud_alert_info` structure :

* :c:member:`type` - A value of the :c:enum:`nrf_cloud_alert_type` enum.
* :c:member:`value` - Use ``0`` if not otherwise needed.
* :c:member:`description` - Use NULL to suppress transmission of this field.
* :c:member:`sequence` - Sequence number used when no timestamp is available.
* :c:member:`ts_ms` - Timestamp if accurate time is known.

The range of the alert type enumeration is split into two parts:

* Values less than :c:enumerator:`ALERT_TYPE_CUSTOM` are predefined.
* Values greater or equal to that are for application-specific use.

Use the alert value to indicate more information about an alert.
For example, for :c:enumerator:`ALERT_TYPE_TEMPERATURE`, the alert value could be the temperature in degrees Celsius.

Use the description string to give more detail about a specific alert, if required.

The library sets the sequence number and timestamp automatically.
The nRF Cloud Alert service uses these values to present the alerts in the actual sequence they were generated on the device.

Supported features
==================

If the :ref:`lib_date_time` library is enabled, and the current date and time are known, the timestamp field of the alert will be automatically set accordingly.
Otherwise, the sequence number will be set to a monotonically-increasing value.

Requirements
************

The device must be connected to nRF Cloud before calling the :c:func:`nrf_cloud_alert_send` function.
The :c:func:`nrf_cloud_rest_alert_send` function initiates the connection as needed.

Configuration
*************

Configure the following options to enable or disable the library and to select the data transport method:

* :kconfig:option:`CONFIG_NRF_CLOUD_ALERTS`
* :kconfig:option:`CONFIG_NRF_CLOUD_MQTT` or :kconfig:option:`CONFIG_NRF_CLOUD_REST`

See :ref:`configure_application` for information on how to change configuration options.

Usage
*****

To use this library, complete the following steps:

1. Include the :file:`nrf_cloud_alerts.h` file.
#. Define a local variable of type :c:struct:`nrf_cloud_alert_info`.
#. Initialize the structure members:

  * :c:member:`type` - A value of the :c:enum:`nrf_cloud_alert_type` enum.
  * :c:member:`value` - Use 0 if not otherwise needed.
  * :c:member:`description` -Use NULL to suppress transmission of this field.

#. Call either :c:func:`nrf_cloud_alert_send` when connected to nRF Cloud using MQTT or :c:func:`nrf_cloud_rest_alert_send` when using REST.

Samples using the library
*************************

The following |NCS| samples use this library:

* :ref:`nrf_cloud_mqtt_multi_service`
* :ref:`nrf_cloud_rest_device_message`

Limitations
***********

For REST-based applications, you can enable or disable alerts only at compile time.

Dependencies
************

This library uses the following |NCS| libraries:

* :ref:`lib_nrf_cloud`
* :ref:`lib_nrf_cloud_rest`
* :ref:`lib_date_time`

API documentation
*****************

| Header file: :file:`include/net/nrf_cloud_alerts.h`
| Source files: :file:`subsys/net/lib/nrf_cloud/src/nrf_cloud_alerts.c`

.. doxygengroup:: nrf_cloud_alerts
   :project: nrf
   :members:
