.. _aws_iot:

nRF9160: AWS IoT
################

.. contents::
   :local:
   :depth: 2

The AWS IoT sample demonstrates how an nRF9160-based device communicates with the AWS IoT message broker over MQTT.
This sample showcases the use of the :ref:`lib_aws_iot` library.
The sample implements :ref:`lib_aws_fota` through :ref:`lib_aws_iot`.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample publishes two different types of messages in `JSON`_ object string format to the AWS IoT message broker.
These messages are sent to the AWS IoT shadow topic ``$aws/things/<thingname>/shadow/update``.

Below are the two types of messages that are published:

* Type 1: The message comprises of a battery voltage value sampled from the nRF9160 SiP and a corresponding timestamp in milliseconds (UNIX time), that is retrieved from the :ref:`lib_date_time` library.

* Type 2: The message adds a configurable firmware version number to type 1 messages.
  This firmware version number is used in correlation with FOTA DFU, which is supported by the sample and the :ref:`lib_aws_iot` library.
  See the :ref:`lib_aws_fota` library for information on how to create FOTA jobs.

A type 2 message is only published upon an initial connection to the sample, while a type 1 message is published sequentially with a configurable time in between each publishing of the data.
In addition to publishing data, the sample also subscribes to the AWS IoT shadow delta topic, and two customizable application-specific topics.
The customizable topics are not part of the AWS IoT shadow service and must therefore be passed to the :ref:`lib_aws_iot` library using the :c:func:`aws_iot_subscription_topics_add` function.

Configuration
*************

|config|

.. _setup_awsiot:

Setup
=====

To run this sample and connect to AWS IoT, complete the steps described in the :ref:`lib_aws_iot` documentation.
This documentation retrieves the AWS IoT broker *hostname*, *security tag*, and *client ID*.
The corresponding options that must be set for each of the aforementioned values are:

* :kconfig:option:`CONFIG_AWS_IOT_BROKER_HOST_NAME`
* :kconfig:option:`CONFIG_AWS_IOT_SEC_TAG`
* :kconfig:option:`CONFIG_AWS_IOT_CLIENT_ID_STATIC`

Set these options in the project configuration file located at :file:`samples/nrf9160/aws_iot/prj.conf`.
For documentation related to FOTA DFU, see :ref:`lib_aws_fota`.

.. _configure_options:

Configuration options
=====================

The application-specific configurations used in the sample are listed below.
They are located in :file:`samples/nrf9160/aws_iot/Kconfig`.

.. _CONFIG_AWS_IOT_SAMPLE_APP_VERSION:

CONFIG_AWS_IOT_SAMPLE_APP_VERSION
   Publishes the application version number to the AWS IoT message broker.

.. _CONFIG_AWS_IOT_SAMPLE_PUBLICATION_INTERVAL_SECONDS:

CONFIG_AWS_IOT_SAMPLE_PUBLICATION_INTERVAL_SECONDS
   Configures the time interval between each publishing of the message.

.. _CONFIG_AWS_IOT_SAMPLE_CONNECTION_RETRY_TIMEOUT_SECONDS:

CONFIG_AWS_IOT_SAMPLE_CONNECTION_RETRY_TIMEOUT_SECONDS
   Configures the number of seconds between each AWS IoT connection retry.

.. _CONFIG_AWS_IOT_SAMPLE_DEVICE_ID_USE_HW_ID:

CONFIG_AWS_IOT_SAMPLE_DEVICE_ID_USE_HW_ID
   Configures the sample to use HWID as Device ID.

.. note::

   The sample sets the option :kconfig:option:`CONFIG_MQTT_KEEPALIVE` to the maximum allowed value, 1200 seconds (20 minutes) as specified by AWS IoT Core.
   This is to limit the IP traffic between the device and the AWS IoT message broker for supporting a low power sample.
   In certain LTE networks, the NAT timeout can be considerably lower than 1200 seconds.
   As a recommendation, and to prevent the likelihood of getting disconnected unexpectedly, set the option :kconfig:option:`CONFIG_MQTT_KEEPALIVE` to the lowest timeout limit (Maximum allowed MQTT keepalive and NAT timeout).

.. _aws_iot_sample_building_and_running:

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/aws_iot`

.. include:: /includes/build_and_run_ns.txt

.. note::

   The sample might require increasing the values of the :kconfig:option:`CONFIG_AWS_IOT_MQTT_RX_TX_BUFFER_LEN` and :kconfig:option:`CONFIG_AWS_IOT_MQTT_PAYLOAD_BUFFER_LEN` options.

After building the sample, program it to your development kit.

Testing
=======

|test_sample|

1. |connect_kit|
#. |connect_terminal|
#. Power on or reset the kit.
#. Observe the output in the terminal over UART.

Sample output
=============

When the sample runs, the device boots, and the sample displays the following output in the terminal over UART:

.. code-block:: console

        *** Booting Zephyr OS build v2.3.0-rc1-ncs1-snapshot1-6-gad0444b058ef  ***
        <inf> aws_iot_sample: AWS IoT sample started, version: v1.0.0
        <inf> aws_iot_sample: LTE cell changed: Cell ID: 33703711, Tracking area: 2305
        <inf> aws_iot_sample: PSM parameter update: TAU: -1, Active time: -1
        <inf> aws_iot_sample: RRC mode: Connected
        <inf> aws_iot_sample: Network registration status: Connected - roaming
        <inf> aws_iot_sample: PSM parameter update: TAU: 3600, Active time: 60
        <inf> aws_iot_sample: RRC mode: Idle
        <inf> aws_iot_sample: AWS_IOT_EVT_CONNECTING
        <inf> aws_iot_sample: RRC mode: Connected
        <inf> aws_iot_sample: RRC mode: Idle
        <inf> aws_iot_sample: RRC mode: Connected
        <inf> aws_iot_sample: AWS_IOT_EVT_CONNECTED
        <inf> aws_iot_sample: Publishing: {
        "state":    {
                "reported":    {
                "app_version":    "v1.0.0",
                "batv":    4304,
                "ts":    1592305354579
                }
        }
        } to AWS IoT broker
        <inf> aws_iot_sample: AWS_IOT_EVT_READY
        <inf> aws_iot_sample: RRC mode: Idle
        <inf> aws_iot_sample: LTE cell changed: Cell ID: 34237195, Tracking area: 2305
        <inf> aws_iot_sample: Publishing: {
        "state":    {
                "reported":    {
                "batv":    4308,
                "ts":    1592305414579
                }
        }
        } to AWS IoT broker
        <inf> aws_iot_sample: Next data publication in 60 seconds
        <inf> aws_iot_sample: RRC mode: Connected
        <inf> aws_iot_sample: RRC mode: Idle
        <inf> aws_iot_sample: LTE cell changed: Cell ID: 33703711, Tracking area: 2305
        <inf> aws_iot_sample: RRC mode: Connected
        <inf> aws_iot_sample: Publishing: {
        "state":    {
                "reported":    {
                "batv":    4308,
                "ts":    1592305474596
                }
        }
        } to AWS IoT broker
        <inf> aws_iot_sample: Next data publication in 60 seconds

To observe incoming messages in the AWS IoT console, navigate to the AWS IoT Core service and
click :guilabel:`MQTT test client`.
To observe messages that are sent from the device, subscribe to the **$aws/things/<client_id>/shadow/update** shadow topic or the wild card token **#**.

.. figure:: /images/aws_mqtt_test_client.PNG
   :alt: AWS IoT Core MQTT test client

   AWS IoT Core MQTT test client

Testing and debugging
=====================

If you have issues with the sample, refer to :ref:`gs_testing`.

Dependencies
************

This sample uses the following |NCS| libraries and drivers:

* :ref:`lib_aws_iot`
* :ref:`lib_date_time`
* :ref:`lte_lc_readme`
* :ref:`modem_info_readme`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
