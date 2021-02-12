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

.. _set_up_conn_to_iot:

Setting up a connection to AWS IoT
**********************************

Setting up a secure MQTT connection to the AWS IoT message broker consists of the following steps:

1. :ref:`creating_a_thing_in_AWS_IoT` and generating certificates for the TLS connection.
#. Programming the certificates to the on-board modem of the nRF9160-based kit.
#. Configuring the required library options.
#. Configuring the optional library options.

.. note::
   The default *thing* name used in the AWS IoT library is ``my-thing``.

.. _flash_certi_device:

Programming the certificates to the on-board modem of the nRF9160-based kit
===========================================================================

To program the certificates, complete the following steps:

1. `Download nRF Connect for Desktop`_.
#. Update the modem firmware on the on-board modem of the nRF9160-based kit to the latest version by following the steps in `Updating the nRF9160 DK cellular modem`_.
#. Build and program the :ref:`at_client_sample` sample to the nRF9160-based kit as explained in :ref:`gs_programming`.
#. Launch the `LTE Link Monitor`_ application, which is implemented as part of `nRF Connect for Desktop`_.
#. Click :guilabel:`Certificate manager` located at the top right corner.
#. Copy and paste the certificates downloaded earlier from the AWS IoT console into the respective entries (``CA certificate``, ``Client certificate``, ``Private key``).
#. Select a desired security tag and click on :guilabel:`Update certificates`.

.. note::
   The default security tag set by the :guilabel:`Certificate manager` *16842753* is reserved for communications with :ref:`lib_nrf_cloud`.

Configuring the required library options
========================================

To establish a connection to the AWS IoT message broker, set the following options:

* :option:`CONFIG_AWS_IOT_SEC_TAG`
* :option:`CONFIG_AWS_IOT_BROKER_HOST_NAME`
* :option:`CONFIG_AWS_IOT_CLIENT_ID_STATIC`

To configure the required library options, complete the following steps:

1. In the `AWS IoT console`_, navigate to :guilabel:`IoT core` -> :guilabel:`Manage` -> :guilabel:`things` and click on the entry for the *thing* created during the steps of :ref:`creating_a_thing_in_AWS_IoT`.
#. Navigate to :guilabel:`interact`, find the ``Rest API Endpoint`` and set the configurable option :option:`CONFIG_AWS_IOT_BROKER_HOST_NAME` to this address string.
#. Set the option :option:`CONFIG_AWS_IOT_CLIENT_ID_STATIC` to the name of the *thing* created during the aforementioned steps.
#. Set the security tag configuration :option:`CONFIG_AWS_IOT_SEC_TAG` to the security tag, chosen while `Programming the certificates to the on-board modem of the nRF9160-based kit`_.

Configuring the optional library options
========================================

To subscribe to the various `AWS IoT Device Shadow Topics`_, set the following options:

* :option:`CONFIG_AWS_IOT_TOPIC_GET_ACCEPTED_SUBSCRIBE`
* :option:`CONFIG_AWS_IOT_TOPIC_GET_REJECTED_SUBSCRIBE`
* :option:`CONFIG_AWS_IOT_TOPIC_UPDATE_ACCEPTED_SUBSCRIBE`
* :option:`CONFIG_AWS_IOT_TOPIC_UPDATE_REJECTED_SUBSCRIBE`
* :option:`CONFIG_AWS_IOT_TOPIC_UPDATE_DELTA_SUBSCRIBE`
* :option:`CONFIG_AWS_IOT_TOPIC_DELETE_ACCEPTED_SUBSCRIBE`
* :option:`CONFIG_AWS_IOT_TOPIC_DELETE_REJECTED_SUBSCRIBE`

To subscribe to non-AWS specific topics, complete the following steps:

* Specify the number of additional topics that needs to be subscribed to, by setting the :option:`CONFIG_AWS_IOT_APP_SUBSCRIPTION_LIST_COUNT` option
* Pass a list containing application specific topics in the :c:func:`aws_iot_subscription_topics_add` function, after the :c:func:`aws_iot_init` function call and before the :c:func:`aws_iot_connect` function call

The AWS IoT library also supports passing in the client ID at run time.
To enable this feature, set the ``client_id`` entry in the :c:struct:`aws_iot_config` structure that is passed in the :c:func:`aws_iot_init` function when initializing the library, and set the following option:

* :option:`CONFIG_AWS_IOT_CLIENT_ID_APP`

.. note::
   If you are using a longer device ID that is either set by the option :option:`CONFIG_AWS_IOT_CLIENT_ID_STATIC` or passed in during initialization, it might be required to increase the value of the option :option:`CONFIG_AWS_IOT_CLIENT_ID_MAX_LEN` for proper initialization of the library.

.. note::
   The AWS IoT library is compatible with the :ref:`cloud_api_readme`, a generic API that enables multiple cloud backends to be interchanged, statically and at run time.
   To enable the use of the cloud, API set the configurable option :option:`CONFIG_CLOUD_API`, in addition to the other selected library options.

Initializing the library
************************

The library is initialized by calling the :c:func:`aws_iot_init` function.
If this API call fails, the application must not make any other API calls to the library.

Connecting to the AWS IoT message broker
****************************************

After the initialization, the :c:func:`aws_iot_connect` function must be called to connect to the AWS IoT broker.
If this API call fails, the application must retry the connection by calling :c:func:`aws_iot_connect` again.
Note that the connection attempt can fail due to any number of external network related reasons.
So, it is recommended to implement a reconnection routine that tries to reconnect the device upon a disconnect.
During an attempt to connect to the AWS IoT broker, the library tries to establish a connection using a TLS handshake, which usually spans a few seconds.
When the library has established a connection and subscribed to all the configured and passed-in topics, it will propagate the :c:enumerator:`AWS_IOT_EVT_READY` event to signify that the library is ready to be used.

API documentation
*****************

| Header file: :file:`include/net/aws_iot.h`
| Source files: :file:`subsys/net/lib/aws_iot/src/`

.. doxygengroup:: aws_iot
   :project: nrf
   :members:
