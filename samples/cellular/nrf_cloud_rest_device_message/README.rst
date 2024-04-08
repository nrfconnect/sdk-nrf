.. _nrf_cloud_rest_device_message:

Cellular: nRF Cloud REST Device Message
#######################################

.. contents::
   :local:
   :depth: 2

The REST Device Message sample demonstrates how to use the `nRF Cloud REST API`_ to send `Device Messages <nRF Cloud Device Messages_>`_ using the ``SendDeviceMessage`` REST endpoint.
Every button press sends a message to nRF Cloud.

It also demonstrates use of the :ref:`lib_nrf_cloud_alert` and the :ref:`lib_nrf_cloud_log` libraries.
The sample sends an alert when the device first comes online.
It also sends a log message indicating the sample version, as well as when the button is pressed.

You can also configure the sample to use the `nRF Cloud Provisioning Service`_ with the :ref:`lib_nrf_provisioning` library.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample requires an `nRF Cloud`_ account.

Your device must be provisioned with `nRF Cloud`_. If it is not, follow the instructions in `Provisioning <nrf_cloud_rest_device_message_provisioning_>`_.

.. note::
   This sample requires modem firmware v1.3.x or later.

Limitations
***********

The `nRF Cloud REST API`_ requires all requests to be authenticated with a JSON Web Token (JWT).
See `nRF Cloud Security`_ for more details.
Generating valid JWTs requires the network carrier to provide date and time to the modem, so the sample must first connect to an LTE carrier and determine the current date and time before REST requests can be sent.

Note also that the `nRF Cloud REST API`_ is stateless.
This differs from the `nRF Cloud MQTT API`_, which requires you to establish and maintain an `MQTT`_ connection while sending `Device Messages <nRF Cloud Device Messages_>`_.

User interface
**************

Once the device is provisioned and connected, each press of **Button 1** generates a device-to-cloud button press `Device Message <nRF Cloud Device Messages_>`_ over REST.
These messages are sent to the non-bulk D2C (device-to-cloud) topic, detailed in `topics used by devices running the nRF Cloud library <nRF Cloud MQTT Topics_>`_.

The configured LTE LED (**LED 1** by default) (:ref:`CONFIG_REST_DEVICE_MESSAGE_LTE_LED_NUM <CONFIG_REST_DEVICE_MESSAGE_LTE_LED_NUM>`) is lit once an LTE connection is established and JWT tokens are ready to be generated.

The configured Send LED (**LED 2** by default) (:ref:`CONFIG_REST_DEVICE_MESSAGE_SEND_LED_NUM <CONFIG_REST_DEVICE_MESSAGE_SEND_LED_NUM>`) is lit when a REST request is being sent.

.. _nrf_cloud_rest_device_message_provisioning:

Provisioning and onboarding your device to nRF Cloud
****************************************************

You must onboard your device to nRF Cloud for this sample to function.
You only need to do this once for each device.

This sample supports two methods to onboard your device:

* Using `nRF Cloud Provisioning Service`_ and the auto-onboarding option.
   This is the preferred option for nRF91x1-based devices.
   The nRF Cloud Provisioning Service auto-onboarding is currently compatible with REST and CoAP but not MQTT connectivity to nRF Cloud; for that, use the next method.
* Using Just-in-time provisioning.
   This is the legacy option.
   Use this method for nRF9160-based devices.

.. _remote_prov_auto_onboard:

Remote provisioning and auto-onboarding with nRF Cloud
======================================================

Build and run the sample with the remote provisioning overlay as explained below: :ref:`nrf_cloud_rest_device_message_remote_provisioning_overlay`.
Follow the steps outlined in `device claiming <nRF Cloud device claiming_>`_.
See `nRF Cloud Auto-onboarding`_ for more information.
The device ID is in the UUID format, not the legacy 'nrf-\ *IMEI*\ ' format.

Just-in-time provisioning (JITP) with nRF Cloud
===============================================

Complete the following steps to onboard your device:

1. Enable the :ref:`CONFIG_REST_DEVICE_MESSAGE_DO_JITP <CONFIG_REST_DEVICE_MESSAGE_DO_JITP>` option.
#. Press button 1 when prompted at startup.
#. Follow the instructions for just-in-time provisioning (JITP) printed to UART.

See `nRF Cloud Just-In-Time-Provisioning`_ for more information.
The device ID is in the legacy 'nrf-\ *IMEI*\ ' format.

Configuration
*************
|config|

Configuration options
=====================

Set the following configuration options for the sample:

.. _CONFIG_REST_DEVICE_MESSAGE_LTE_LED_NUM:

CONFIG_REST_DEVICE_MESSAGE_LTE_LED_NUM - LTE LED number
   This configuration option defines which LED is used to indicate LTE connection success.

.. _CONFIG_REST_DEVICE_MESSAGE_SEND_LED_NUM:

CONFIG_REST_DEVICE_MESSAGE_SEND_LED_NUM - Send LED number
   This configuration option defines which LED is used to indicate a REST request is being sent.

.. _CONFIG_REST_DEVICE_MESSAGE_DO_JITP:

CONFIG_REST_DEVICE_MESSAGE_DO_JITP - Enable prompt to perform JITP over REST
   This configuration option defines whether the application prompts the user for just-in-time provisioning on startup.

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/nrf_cloud_rest_device_message`

.. include:: /includes/build_and_run_ns.txt

The configuration file for this sample is located in :file:`samples/cellular/nrf_cloud_rest_device_message`.
See :ref:`configure_application` for information on how to configure the parameters.

nRF Cloud logging overlay
=========================

To enable `Zephyr Logging`_ to nRF Cloud using the :ref:`lib_nrf_cloud_log` library, add the following parameter to your build command:

``-DEXTRA_CONF_FILE=overlay_nrfcloud_logging.conf``

This overlay allows the sample and various subsystems that have logging enabled to send their logs to nRF Cloud.
Set the :kconfig:option:`CONFIG_NRF_CLOUD_LOG_OUTPUT_LEVEL` option to the log level of messages to send to nRF Cloud, such as ``4`` for debug log messages.
Set the :kconfig:option:`CONFIG_NRF_CLOUD_REST_DEVICE_MESSAGE_SAMPLE_LOG_LEVEL_DBG` option so that log messages are generated on each button press.

.. _nrf_cloud_rest_device_message_remote_provisioning_overlay:

Remote provisioning overlay
===========================

This overlay is for use with nRF91x1-based devices only.
To enable remote provisioning with the `nRF Cloud Provisioning Service`_ add the following parameter to your build command:

``-DEXTRA_CONF_FILE=overlay-nrf_provisioning.conf``

This overlay enables the :ref:`lib_nrf_provisioning` library and its provisioning shell.
It configures the device ID to use the UUID format, not the legacy 'nrf-\ *IMEI*\ ' format.
The sample will periodically check for provisioning commands.
Press **Button 2** to manually initiate a check for provisioning commands.

Press **Button 1** to have a message sent to nRF Cloud:

   .. parsed-literal::
      :class: highlight

      {
        "appId": "BUTTON",
        "messageType": "DATA",
        "data": "1",
        "timestamp": "4/19/2023 12:34:21 PM"
      }

Querying Device Messages over REST API
**************************************

To query the Device Messages received by the `nRF Cloud`_ backend, send a GET request to the `ListMessages <nRF Cloud REST API ListMessages_>`_ endpoint.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_nrf_cloud`
* :ref:`lib_nrf_cloud_rest`
* :ref:`lib_nrf_cloud_alert`
* :ref:`lib_nrf_cloud_log`
* :ref:`lib_modem_jwt`
* :ref:`lte_lc_readme`
* :ref:`dk_buttons_and_leds_readme`
* :ref:`modem_info_readme`
* :ref:`lib_at_host`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
