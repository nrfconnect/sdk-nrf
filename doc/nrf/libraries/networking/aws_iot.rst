.. _lib_aws_iot:

AWS IoT
#######

.. contents::
   :local:
   :depth: 2

The Amazon Web Services Internet-of-Things (AWS IoT) library enables applications to connect to, and exchange messages with the AWS IoT message broker.
The library supports the following technologies:

* TLS secured MQTT transmission protocol
* Firmware-Over-The-Air (FOTA)

For more information regarding AWS FOTA, see the documentation for the :ref:`lib_aws_fota` library and the :ref:`aws_fota_sample` sample.

.. note::
   The AWS IoT library and the :ref:`lib_aws_fota` library share common steps related to `AWS IoT console`_ management.

Requirements
************

The :ref:`aws_iot` sample available for the nRF9160 DK showcases the use of this library.
As part of building the sample, you :ref:`create a thing in AWS IoT <creating_a_thing_in_AWS_IoT>` and generate certificates for the TLS connection.

.. _set_up_conn_to_iot:

Configuration
*************

After meeting the `Requirements`_, complete the following configuration procedures to set up a secure MQTT connection to the AWS IoT message broker:

* Program the certificates to the on-board modem of the nRF9160-based kit
* Configure the required library options
* Configure the optional library options

See the following sections for detailed information.

.. note::
   The default *thing* name used in the AWS IoT library is ``my-thing``.

.. _flash_certi_device:

Program the certificates to the on-board modem of the nRF9160-based kit
=======================================================================

To program the certificates, complete the following steps:

1. `Download nRF Connect for Desktop`_.
#. Update the modem firmware on the on-board modem of the nRF9160-based kit to the latest version by following the steps in :ref:`nrf9160_gs_updating_fw_modem`.
#. Build and program the :ref:`at_client_sample` sample to the nRF9160-based kit as explained in :ref:`gs_programming`.
#. Launch the `LTE Link Monitor`_ application, which is implemented as part of `nRF Connect for Desktop`_.
#. Click :guilabel:`Certificate manager` located at the top right corner.
#. Copy and paste the certificates downloaded earlier from the AWS IoT console into the respective entries (``CA certificate``, ``Client certificate``, ``Private key``).
#. Select a desired security tag and click on :guilabel:`Update certificates`.

.. note::
   The default security tag set by the :guilabel:`Certificate manager` *16842753* is reserved for communications with :ref:`lib_nrf_cloud`.

Configure the required library options
======================================

Complete the following steps:

1. Set the following options to establish a connection to the AWS IoT message broker:

   * :kconfig:option:`CONFIG_AWS_IOT_SEC_TAG`
   * :kconfig:option:`CONFIG_AWS_IOT_BROKER_HOST_NAME`
   * :kconfig:option:`CONFIG_AWS_IOT_CLIENT_ID_STATIC`

#. Complete the following steps to set the required library options:

   a. In the `AWS IoT console`_, navigate to :guilabel:`IoT core` > :guilabel:`Settings`.
   #. Find the ``Endpoint`` address and set :kconfig:option:`CONFIG_AWS_IOT_BROKER_HOST_NAME` to this address string.
   #. Set the option :kconfig:option:`CONFIG_AWS_IOT_CLIENT_ID_STATIC` to the name of the *thing* created earlier in the process.
   #. Set the security tag configuration :kconfig:option:`CONFIG_AWS_IOT_SEC_TAG` to the security tag, chosen while you `Program the certificates to the on-board modem of the nRF9160-based kit`_.

Configure the optional library options
======================================

To subscribe to the various `AWS IoT Device Shadow Topics`_, set the following options:

* :kconfig:option:`CONFIG_AWS_IOT_TOPIC_GET_ACCEPTED_SUBSCRIBE`
* :kconfig:option:`CONFIG_AWS_IOT_TOPIC_GET_REJECTED_SUBSCRIBE`
* :kconfig:option:`CONFIG_AWS_IOT_TOPIC_UPDATE_ACCEPTED_SUBSCRIBE`
* :kconfig:option:`CONFIG_AWS_IOT_TOPIC_UPDATE_REJECTED_SUBSCRIBE`
* :kconfig:option:`CONFIG_AWS_IOT_TOPIC_UPDATE_DELTA_SUBSCRIBE`
* :kconfig:option:`CONFIG_AWS_IOT_TOPIC_DELETE_ACCEPTED_SUBSCRIBE`
* :kconfig:option:`CONFIG_AWS_IOT_TOPIC_DELETE_REJECTED_SUBSCRIBE`

To subscribe to non-AWS specific topics, complete the following steps:

* Specify the number of additional topics that needs to be subscribed to, by setting the :kconfig:option:`CONFIG_AWS_IOT_APP_SUBSCRIPTION_LIST_COUNT` option
* Pass a list containing application specific topics in the :c:func:`aws_iot_subscription_topics_add` function, after the :c:func:`aws_iot_init` function call and before the :c:func:`aws_iot_connect` function call

The AWS IoT library also supports passing in the client ID at run time.
To enable this feature, set the ``client_id`` entry in the :c:struct:`aws_iot_config` structure that is passed in the :c:func:`aws_iot_init` function when initializing the library, and set the :kconfig:option:`CONFIG_AWS_IOT_CLIENT_ID_APP` Kconfig option.

.. note::
   If you are using a longer device ID that is either set by the option :kconfig:option:`CONFIG_AWS_IOT_CLIENT_ID_STATIC` or passed in during initialization, it might be required to increase the value of the option :kconfig:option:`CONFIG_AWS_IOT_CLIENT_ID_MAX_LEN` for proper initialization of the library.

Initializing the library
========================

The library is initialized by calling the :c:func:`aws_iot_init` function.
If this API call fails, the application must not make any other API calls to the library.

Connecting to the AWS IoT message broker
========================================

After the initialization, the :c:func:`aws_iot_connect` function must be called to connect to the AWS IoT broker.
If this API call fails, the application must retry the connection by calling :c:func:`aws_iot_connect` again.

.. note::
   The connection attempt can fail due to a number of reasons related to external network.
   Implement a reconnection routine that tries to reconnect the device upon a disconnect.

During an attempt to connect to the AWS IoT broker, the library tries to establish a connection using a TLS handshake, which usually spans a few seconds.
When the library has established a connection and subscribed to all the configured and passed-in topics, it will propagate the :c:enumerator:`AWS_IOT_EVT_READY` event to signify that the library is ready to be used.

API documentation
*****************

| Header file: :file:`include/net/aws_iot.h`
| Source files: :file:`subsys/net/lib/aws_iot/src/`

.. doxygengroup:: aws_iot
   :project: nrf
   :members:
