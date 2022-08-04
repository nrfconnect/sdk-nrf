.. _nrf_cloud_rest_device_message_replicate:

nRF9160: nRF Cloud REST Device Message
######################################

.. contents::
   :local:
   :depth: 2

The REST Device Message sample demonstrates how to use the `nRF Cloud REST API`_ to send `Device Messages <nRF Cloud Device Messages_>`_ using the ``SendDeviceMessage`` REST endpoint.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample requires an `nRF Cloud`_ account.

Your device must be provisioned with `nRF Cloud`_. If it is not, follow the instructions in `Provisioning <nrf_cloud_rest_device_message_replicate_provisioning_>`_.

.. note::
   This sample requires modem firmware v1.3.x or later.

Limitations
***********

The `nRF Cloud REST API`_ requires all requests to be authenticated with a JSON Web Token (JWT).
See  `nRF Cloud Security`_ for more details.
Generating valid JWTs requires the network carrier to provide date and time to the modem, so the sample must first connect to an LTE carrier and determine the current date and time before REST requests can be sent.

Note also that the `nRF Cloud REST API`_ is stateless.
This differs from the `nRF Cloud MQTT API`_, which requires you to establish and maintain an `MQTT`_ connection while sending `Device Messages <nRF Cloud Device Messages_>`.

User interface
**************

Once the device is provisioned and connected, it will periodically send messages to the cloud in 60 seconds.

.. _nrf_cloud_rest_device_message_replicate_provisioning:

Provisioning
************

Your device must be provisioned using your NRF CLoud account for this sample to function.

You only need to do this once for each device.

Configuration
*************
|config|

Configuration options
=====================

This is a simple sample, so you only need to provision your device to your NRF Cloud account.

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/nrf_cloud_rest_device_message_tol2`

.. include:: /includes/build_and_run_ns.txt

The configuration file for this sample is located in :file:`samples/nrf9160/nrf_cloud_rest_device_message_tol2`.
See :ref:`configure_application` for information on how to configure the parameters.

Querying Device Messages over REST API
**************************************

To query the Device Messages received by the `nRF Cloud`_ backend, send a GET request to the `ListMessages <nRF Cloud REST API ListMessages_>`_ endpoint.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_nrf_cloud`
* :ref:`lib_nrf_cloud_rest`
* :ref:`lib_modem_jwt`
* :ref:`lte_lc_readme`
* :ref:`dk_buttons_and_leds_readme`
* :ref:`modem_info_readme`
* :ref:`lib_at_host`
