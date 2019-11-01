.. _lib_aws_iot:

AWS IoT
#######

The Amazon Web Services Internet-of-Things (AWS IoT) library enables applications to connect to, and exchange messages with the AWS IoT message broker.
The library supports the following technologies:

* TLS secured MQTT transmission protocol
* Firmware-Over-The-Air (FOTA)

.. note::
   For more information regarding AWS FOTA, see the documentation on :ref:`lib_aws_fota` library and :ref:`aws_fota_sample` sample.
   This library and the :ref:`lib_aws_fota` library share common steps related to `AWS IoT console`_ management.

Setting up a connection to AWS IoT
**********************************

Setting up a secure MQTTS connection to the AWS IoT message broker consists of the following steps:

1. Creating a Thing
#. Generating a CA certificate.
#. Flashing the certificates to the on-board modem of the nRF9160 based board.
#. Configuring the library options.

For more information on creating a thing in AWS IoT and generating the certificate, see :ref:`creating_a_thing_in_AWS_IoT`.

.. note::
   The default name used for the *thing* by the AWS IoT library  is ``my-thing``.

Flashing the certificates to the on-board modem of the nRF9160 based board
**************************************************************************
1. `Download nRF Connect for Desktop`_
#. Update the modem firmware on the on-board modem of the nRF9160 based board to the latest version by following the steps in `Updating the nRF9160 DK cellular modem`_.
#. Build and program the  :ref:`at_client_sample` sample to the nRF9160 based board as explained in :ref:`gs_programming`.
#. Launch the `LTE Link Monitor`_ application, which is implemented as part of `nRF Connect for Desktop`_.
#. Click **Certificate manager** located at the top right corner.
#. Copy and paste the certificates, downloaded earlier from AWS IoT, into the respective entries (``CA certificate``, ``Client certificate``, ``Private key``).
#. Select a desired security tag and click **Update certificates**.

.. note::
   The default security tag *16842753* is reserved for the :ref:`lib_nrf_cloud`.

Configuring library options
***************************
To configure the library options, complete the following steps:

1. In the `AWS IoT console`_, navigate to **IoT core** -> **Manage** -> **things** and click on the entry for the *thing*, created during the steps of :ref:`creating_a_thing_in_AWS_IoT`.
#. Navigate to **interact**, find the **Rest API Endpoint** and set the configurable option :option:`CONFIG_AWS_IOT_BROKER_HOST_NAME` to this address.
#. Set the option :option:`CONFIG_AWS_IOT_CLIENT_ID_STATIC` to the name of the *thing* created during the aforementioned steps.
#. Set the security tag configuration :option:`CONFIG_AWS_IOT_SEC_TAG` to the security tag, chosen while `Flashing the certificates to the on-board modem of the nRF9160 based board`_.

Initializing
************

The module is initialized by calling the  :cpp:func:`aws_iot_init` function.
If this API fails, the application must not use any APIs of the module.

Connecting
**********

.. note::
   The API requires that a configuration structure :c:type:`aws_iot_config` is declared in the application and passed into the :cpp:func:`aws_iot_init` and :cpp:func:`aws_iot_connect` functions.
   This exposes the application to the MQTT socket used for the connection, which is polled on, in the application.
   It also enables the application to pass in a client id (*thingname*) at runtime.

After initialization, the :cpp:func:`aws_iot_connect` function must be called, to connect to the AWS IoT broker.
If the API fails, the application must retry the connection.
During an attempt to connect to the AWS Iot broker, the library tries to establish a connection using a TLS handshake.
This can take some time, usually in the span of seconds.
For the duration of the TLS handshake, the API blocks.

After a successful connection, the API subscribes to AWS IoT Shadow topics and application specific topics, depending on the configuration of the library.

Polling on MQTT socket
**********************

After a successful return of :cpp:func:`aws_iot_connect` function, the MQTT socket must be polled on, in addition to the periodic calls to :cpp:func:`aws_iot_ping` (to keep the connection to the AWS IoT broker alive) and :cpp:func:`aws_iot_input` (to get the data from the AWS IoT broker).

The code section below demonstrates how socket polling can be done in the main application after the :cpp:func:`aws_iot_init` function has been called.

   .. code-block:: c

      connect:
         err = aws_iot_connect(&config);
         if (err) {
            printk("aws_iot_connect failed: %d\n", err);
         }

         struct pollfd fds[] = {
            {
               .fd = config.socket,
               .events = POLLIN
            }
         };

         while (true) {
            err = poll(fds, ARRAY_SIZE(fds),
               K_SECONDS(CONFIG_MQTT_KEEPALIVE / 3));

            if (err < 0) {
               printk("poll() returned an error: %d\n", err);
               continue;
            }

            if (err == 0) {
               aws_iot_ping();
               continue;
            }

            if ((fds[0].revents & POLLIN) == POLLIN) {
               aws_iot_input();
            }

            if ((fds[0].revents & POLLNVAL) == POLLNVAL) {
               printk("Socket error: POLLNVAL\n");
               printk("The AWS IoT socket was unexpectedly closed.\n");
               return;
            }

            if ((fds[0].revents & POLLHUP) == POLLHUP) {
               printk("Socket error: POLLHUP\n");
               printk("Connection was closed by the AWS IoT broker.\n");
               return;
            }

            if ((fds[0].revents & POLLERR) == POLLERR) {
               printk("Socket error: POLLERR\n");
               printk("AWS IoT broker connection was unexpectedly closed.\n");
               return;
            }
      }

Configuration
*************

To subscribe to *AWS shadow topics*, set the following options:

- :option:`CONFIG_AWS_IOT_TOPIC_GET_ACCEPTED_SUBSCRIBE`
- :option:`CONFIG_AWS_IOT_TOPIC_GET_REJECTED_SUBSCRIBE`
- :option:`CONFIG_AWS_IOT_TOPIC_UPDATE_ACCEPTED_SUBSCRIBE`
- :option:`CONFIG_AWS_IOT_TOPIC_UPDATE_REJECTED_SUBSCRIBE`
- :option:`CONFIG_AWS_IOT_TOPIC_UPDATE_DELTA_SUBSCRIBE`
- :option:`CONFIG_AWS_IOT_TOPIC_DELETE_ACCEPTED_SUBSCRIBE`
- :option:`CONFIG_AWS_IOT_TOPIC_DELETE_REJECTED_SUBSCRIBE`

To subscribe to non AWS specific topics, specify the number of additional topics that needs to be subscribed to, by setting the following option:

- :option:`CONFIG_AWS_IOT_APP_SUBSCRIPTION_LIST_COUNT`

.. note::
   The :cpp:func:`aws_iot_subscription_topics_add` function must be called with a list containing application topics, after calling :cpp:func:`aws_iot_init` and before calling :cpp:func:`aws_iot_connect` .

To connect to the AWS IoT broker, set the following mandatory options (specified in the `Configuring library options`_ section):

- :option:`CONFIG_AWS_IOT_SEC_TAG`
- :option:`CONFIG_AWS_IOT_BROKER_HOST_NAME`
- :option:`CONFIG_AWS_IOT_CLIENT_ID_STATIC`

To enable the application to optionally pass a client id at runtime, set the ``client_id`` entry in the :c:type:`aws_iot_config` structure passed in the :cpp:func:`aws_iot_init` function and set the following option:

- :option:`CONFIG_AWS_IOT_CLIENT_ID_APP`

.. note::
   By default, the library uses the static configurable option :option:`CONFIG_AWS_IOT_CLIENT_ID_STATIC` for the client id.

.. note::
   The AWS IoT library is compatible with the generic *cloud_api* library, a generic API that supports interchangeable cloud backends, statically and at runtime.

API documentation
*****************

| Header file: :file:`include/net/aws_iot.h`
| Source files: :file:`subsys/net/lib/aws_iot/src/`

.. doxygengroup:: aws_iot
   :project: nrf
   :members:
