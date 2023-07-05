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

To connect to AWS IoT, complete the following steps:

1. :ref:`setup_aws_and_permissions`
#. :ref:`creating_a_thing_in_AWS_IoT`
#. :ref:`flash_certi_device`
#. :ref:`Configuring the application <configuring>`

See `AWS IoT Developer Guide`_ for general information about the Amazon Web Services IoT service.

.. _setup_aws_and_permissions:

Set up your AWS account and permissions
***************************************

To connect to AWS IoT, you need to set up an AWS account with the appropriate permissions.
Complete the steps documented in `Set up your AWS account`_.

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

   .. note::
      The policy example is only intended for development environments.
      All devices in your production fleet must have credentials with privileges that authorize only intended actions on specific resources.
      The specific permission policies can vary depending on the use case and should meet business and security requirements.
      For more information, refer to the example policies listed in `AWS IoT Core policy examples`_ and `Security best practices in AWS IoT Core`_.

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

For nRF70 Series devices, the TLS credentials need to be provisioned to the Mbed TLS stack at runtime before establishing a connection to AWS IoT.
This can be done by enabling the :kconfig:option:`CONFIG_AWS_IOT_PROVISION_CERTIFICATES` option and placing the credentials in the path specified by the :Kconfig:option:`CONFIG_AWS_IOT_CERTIFICATES_FILE` option.

.. _configuring:

Configuration
*************

Complete the following steps to set the required library options:

1. In the `AWS IoT console`_, navigate to :guilabel:`IoT core` > :guilabel:`Settings`.
#. Find the ``Device data endpoint`` address and set :kconfig:option:`CONFIG_AWS_IOT_BROKER_HOST_NAME` to this address string.
   The address can also be provided at runtime by setting the :kconfig:option:`CONFIG_AWS_IOT_BROKER_HOST_NAME_APP` option.
   See :ref:`lib_set_aws_hostname` for more details.
#. Set the option :kconfig:option:`CONFIG_AWS_IOT_CLIENT_ID_STATIC` to the name of the *Thing* created earlier in the process.
   This is not needed if the application sets the client ID at run time.
   If you still want to set a custom client ID, make sure that the :kconfig:option:`CONFIG_AWS_IOT_CLIENT_ID_APP` is disabled and set the :kconfig:option:`CONFIG_AWS_IOT_CLIENT_ID_STATIC` option to your desired client ID.
#. Set the security tag configuration :kconfig:option:`CONFIG_AWS_IOT_SEC_TAG` to the security tag, chosen while you :ref:`provisioned the certificates to the modem <flash_certi_device>`.

Optional library options
========================

To subscribe to the various `AWS IoT Device Shadow Topics`_ , set the following options:

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
* :kconfig:option:`CONFIG_AWS_IOT_BROKER_HOST_NAME_MAX_LEN`
* :kconfig:option:`CONFIG_AWS_IOT_BROKER_HOST_NAME_APP`


.. note::
   If you are using a longer device ID that is either set by the option :kconfig:option:`CONFIG_AWS_IOT_CLIENT_ID_STATIC` or passed in during initialization, it might be required to increase the value of the option :kconfig:option:`CONFIG_AWS_IOT_CLIENT_ID_MAX_LEN` for proper initialization of the library.

.. _aws_iot_usage:

Usage
*****

The :ref:`aws_iot` sample showcases the use of this library and can be used to verify a connection to AWS IoT.
To configure and run the sample, complete the steps described in :ref:`aws_iot_sample_server_setup` and :ref:`aws_iot_sample_building_and_running`.

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

The following code example shows how to subscribe to non-AWS specific topics:

.. code-block:: c

	#define CUSTOM_TOPIC_1	"my-custom-topic/example"
	#define CUSTOM_TOPIC_2	"my-custom-topic/example2"

	const struct aws_iot_topic_data topics_list[2] = {
		[0].str = CUSTOM_TOPIC_1,
		[0].len = strlen(CUSTOM_TOPIC_1),
		[1].str = CUSTOM_TOPIC_2,
		[1].len = strlen(CUSTOM_TOPIC_2)
	};

	err = aws_iot_subscription_topics_add(topics_list, ARRAY_SIZE(topics_list));
	if (err) {
		LOG_ERR("aws_iot_subscription_topics_add, error: %d", err);
		return err;
	}

	err = aws_iot_init(NULL, aws_iot_event_handler);
	if (err) {
		LOG_ERR("AWS IoT library could not be initialized, error: %d", err);
		return err;
	}

Publishing to non-AWS specific topics
=====================================

To publish to a non-AWS specific topic, complete the following steps:

* Populate a :c:struct:`aws_iot_topic_data` with the custom topics that you want to publish to.
  It is not necessary to set the topic type when populating the :c:struct:`aws_iot_topic_data` structure.
  This type is reserved for AWS IoT shadow topics.
* Pass in the entry that corresponds to the topic that the payload is to be published to in the message structure :c:struct:`aws_iot_data`.
  This structure is then passed into the :c:func:`aws_iot_send` function.

The following code example shows how to publish to non-AWS specific topics:

.. code-block:: c

	#define MY_CUSTOM_TOPIC_1 "my-custom-topic/example"
	#define MY_CUSTOM_TOPIC_1_IDX 0

	static struct aws_iot_topic_data pub_topics[1] = {
		[MY_CUSTOM_TOPIC_1_IDX].str = MY_CUSTOM_TOPIC_1,
		[MY_CUSTOM_TOPIC_1_IDX].len = strlen(MY_CUSTOM_TOPIC_1),
	};

	struct aws_iot_data msg = {
		/* Pointer to payload */
		.ptr = buf,

		/* Length of payload */
		.len = len,

		 /* Message ID , if not set it will be provided by the AWS IoT library */
		.message_id = id,

		/* Quality of Service level */
		.qos = MQTT_QOS_0_AT_MOST_ONCE,

		/* "my-custom-topic/example" */
		.topic = pub_topics[MY_CUSTOM_TOPIC_1_IDX]
	};

	err = aws_iot_send(&msg);
	if (err) {
		LOG_ERR("aws_iot_send, error: %d", err);
		return err;
	}

Setting client ID at run-time
=============================

The AWS IoT library also supports passing in the client ID at run time.
To enable this feature, set the ``client_id`` entry in the :c:struct:`aws_iot_config` structure that is passed in the :c:func:`aws_iot_init` function when initializing the library, and set the :kconfig:option:`CONFIG_AWS_IOT_CLIENT_ID_APP` Kconfig option.

.. _lib_set_aws_hostname:

Setting the AWS host name at runtime
====================================

The AWS IoT library also supports passing the endpoint address at runtime by setting the :kconfig:option:`CONFIG_AWS_IOT_BROKER_HOST_NAME_APP` option.
If this option is set, the ``host_name`` and ``host_name_len`` must be set in the :c:struct:`aws_iot_config` structure before it is passed into the :c:func:`aws_iot_init` function.
The length of your AWS host name (:kconfig:option:`CONFIG_AWS_IOT_BROKER_HOST_NAME`) must be shorter than the default value of :kconfig:option:`CONFIG_AWS_IOT_BROKER_HOST_NAME_MAX_LEN`, for proper initialization of the library.

.. _aws_iot_testing_and_debugging:

Testing and debugging
=====================

If you have issues with the library or sample, refer to :ref:`gs_testing`.

Topic monitoring
----------------

To observe incoming messages, navigate to the `AWS IoT console`_ and click :guilabel:`MQTT test client`.
Subscribe to the topic that you want to monitor, or use the wild card token **#** to monitor all topics.

.. _aws_iot_troubleshooting:

Troubleshooting
===============

For issues related to the library and |NCS| in general, refer to :ref:`known_issues`.

* If you are experiencing unexpected disconnects from AWS IoT, try decreasing the value of the :kconfig:option:`CONFIG_MQTT_KEEPALIVE` option or publishing data more frequently.
  AWS IoT specifies a maximum allowed keepalive of 1200 seconds (20 minutes), however in certain LTE networks, the Network Address Translation (NAT) timeout can be considerably lower.
  As a recommendation to prevent the likelihood of unexpected disconnects, set the option :kconfig:option:`CONFIG_MQTT_KEEPALIVE` to the highest value of the network NAT and maximum allowed MQTT keepalive.
* If publishing larger payloads fails, you might need to increase the value of the :kconfig:option:`CONFIG_AWS_IOT_MQTT_RX_TX_BUFFER_LEN` option.
* For nRF9160-based boards, the size of incoming messages cannot exceed approximately 2 kB.
  This is due to a limitation of the modem's internal TLS buffers.
  Messages that exceed this limitation will be dropped.

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
