.. _lib_aws_iot:

AWS IoT
#######

.. contents::
   :local:
   :depth: 2

The Amazon Web Services Internet-of-Things (AWS IoT) library enables applications to connect to, and exchange messages with the `AWS IoT Core`_ service.
The library supports the following technologies:

* TLS secured MQTT transmission protocol
* Firmware-Over-The-Air (FOTA)

To connect to AWS IoT, the following steps must be completed:

1. :ref:`creating_a_thing_in_AWS_IoT`
#. :ref:`flash_certi_device`
#. :ref:`Configuring the application <configuring>`

See `AWS IoT Developer Guide`_ for general information about the Amazon Web Services IoT service.

.. _creating_a_thing_in_AWS_IoT:

Create a Thing in AWS IoT
*************************

Before you can use this library, you must create a *Thing* for your client in AWS IoT.
The *Thing* must be connected to a security policy.
For testing, you can use a permissive policy, but make sure to update the policy to be more restrictive before you go into production.
See `AWS IoT Developer Guide: Basic Policy Variables`_ and `AWS IoT Developer Guide: Security Best Practices`_ for more information about policies.

To create a Thing for your device:

1. Log in to the `AWS IoT console`_.
#. Go to :guilabel:`Security` > :guilabel:`Policies` and select :guilabel:`Create policy`.
#. Enter a name and define your policy.
   For testing purposes, you can use the following policy (click on :guilabel:`JSON`, copy, and paste it):

   .. code-block:: javascript

      {
         "Version": "2012-10-17",
         "Statement": [
             {
               "Effect": "Allow",
               "Action": "iot:*",
               "Resource": "*"
             }
          ]
       }

#. Click :guilabel:`Create`.
#. Go to :guilabel:`Manage` > :guilabel:`All devices`> :guilabel:`Things` and select :guilabel:`Create things`.
#. Click :guilabel:`Create Thing`.
#. Enter a name.
   The default name used by the library is ``my-thing``.

   .. note::

      If you are working with an application, that automatically sets the client ID to the IMEI of the device at run-time, you must locate the IMEI of the device and use that as the *Thing name*.
      If you choose a different name, make sure to :ref:`configure a custom client ID <configuring>` before building.
#. Accept the defaults and continue to the next step.
#. Select :guilabel:`Auto-generate a new certificate` to generate new certificates.
#. Select the policy that you created in a previous step.
#. Click on :guilabel:`Create Thing`.
#. Download the following certificates and keys for later use:

   * Thing certificate (:file:`*-certificate.pem.crt`)
   * The private key (:file:`*.private.pem.key`)
   * The root CA (choose the Amazon Root CA 1, :file:`AmazonRootCA1.pem`)
#. Click :guilabel:`Done`.

The certificates that you created or added for your Thing in AWS IoT must be stored on your device so that it can successfully connect to AWS IoT.

.. _flash_certi_device:

Provisioning of the certificates
********************************

.. include:: /includes/cert-flashing.txt

.. _configuring:

Configuration
*************

Complete the following steps to set the required library options:

1. In the `AWS IoT console`_, navigate to :guilabel:`IoT core` > :guilabel:`Settings`.
#. Find the ``Device data endpoint`` address and set :kconfig:option:`CONFIG_AWS_IOT_BROKER_HOST_NAME` to this address string.
#. Set the option :kconfig:option:`CONFIG_AWS_IOT_CLIENT_ID_STATIC` to the name of the *Thing* created earlier in the process.
   This is not needed if the application sets the client ID at run time.
   If you still want to set a custom client ID, make sure that the :kconfig:option:`CONFIG_AWS_IOT_CLIENT_ID_APP` is disabled and set the :kconfig:option:`CONFIG_AWS_IOT_CLIENT_ID_STATIC` option to your desired client ID.
#. Set the security tag configuration :kconfig:option:`CONFIG_AWS_IOT_SEC_TAG` to the security tag, chosen while you :ref:`provisioned the certificates to the modem <flash_certi_device>`.

Optional library options
========================

To subscribe to the various `AWS IoT Device Shadow Topics`_, set the following options:

* :kconfig:option:`CONFIG_AWS_IOT_TOPIC_GET_ACCEPTED_SUBSCRIBE`
* :kconfig:option:`CONFIG_AWS_IOT_TOPIC_GET_REJECTED_SUBSCRIBE`
* :kconfig:option:`CONFIG_AWS_IOT_TOPIC_UPDATE_ACCEPTED_SUBSCRIBE`
* :kconfig:option:`CONFIG_AWS_IOT_TOPIC_UPDATE_REJECTED_SUBSCRIBE`
* :kconfig:option:`CONFIG_AWS_IOT_TOPIC_UPDATE_DELTA_SUBSCRIBE`
* :kconfig:option:`CONFIG_AWS_IOT_TOPIC_DELETE_ACCEPTED_SUBSCRIBE`
* :kconfig:option:`CONFIG_AWS_IOT_TOPIC_DELETE_REJECTED_SUBSCRIBE`

Other options:

* :kconfig:option:`CONFIG_AWS_IOT_LAST_WILL`
* :kconfig:option:`CONFIG_AWS_IOT_LAST_WILL_TOPIC`
* :kconfig:option:`CONFIG_AWS_IOT_LAST_WILL_MESSAGE`
* :kconfig:option:`CONFIG_AWS_IOT_MQTT_RX_TX_BUFFER_LEN`
* :kconfig:option:`CONFIG_AWS_IOT_MQTT_PAYLOAD_BUFFER_LEN`
* :kconfig:option:`CONFIG_AWS_IOT_CLIENT_ID_APP`
* :kconfig:option:`CONFIG_AWS_IOT_CLIENT_ID_STATIC`
* :kconfig:option:`CONFIG_AWS_IOT_CLIENT_ID_MAX_LEN`


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
   The connection attempt can fail due to several reasons related to the network.
   Due to this its recommended to implement a routine that tries to reconnect the device upon a disconnect.

During an attempt to connect to the AWS IoT broker, the library tries to establish a connection using a TLS handshake, which usually spans a few seconds.
When the library has established a connection and subscribed to all the configured and passed-in topics, it will propagate the :c:enumerator:`AWS_IOT_EVT_READY` event to signify that the library is ready to be used.

Subscribing to non-AWS specific topics
======================================

To subscribe to non-AWS specific topics, complete the following steps:

* Specify the number of additional topics that needs to be subscribed to, by setting the :kconfig:option:`CONFIG_AWS_IOT_APP_SUBSCRIPTION_LIST_COUNT` option.
* Pass a list containing application specific topics in the :c:func:`aws_iot_subscription_topics_add` function, after the :c:func:`aws_iot_init` function call and before the :c:func:`aws_iot_connect` function call.

Setting client ID at run-time
=============================

The AWS IoT library also supports passing in the client ID at run time.
To enable this feature, set the ``client_id`` entry in the :c:struct:`aws_iot_config` structure that is passed in the :c:func:`aws_iot_init` function when initializing the library, and set the :kconfig:option:`CONFIG_AWS_IOT_CLIENT_ID_APP` Kconfig option.

AWS FOTA
========

The library automatically includes and enables support for FOTA using the :ref:`lib_aws_fota` library.
To create a FOTA job, refer to the :ref:`lib_aws_fota` documentation.

API documentation
*****************

| Header file: :file:`include/net/aws_iot.h`
| Source files: :file:`subsys/net/lib/aws_iot/src/`

.. doxygengroup:: aws_iot
   :project: nrf
   :members:
