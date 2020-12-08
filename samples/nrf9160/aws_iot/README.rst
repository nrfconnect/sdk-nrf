.. _aws_iot:

nRF9160: AWS IoT
################

.. contents::
   :local:
   :depth: 2

The AWS IoT sample shows the communication of an nRF9160-based device with the AWS IoT message broker over MQTT.
This sample uses the :ref:`lib_aws_iot` library.

Overview
********

The sample publishes two different types of messages in `JSON`_ object string format to the AWS IoT message broker.
These messages are sent to the AWS IoT shadow topic ``$aws/things/<thingname>/shadow/update``.

Below are the two types of messages that are published:

* Type 1: The message comprises of a battery voltage value sampled from the nRF9160 SiP and a corresponding timestamp in milliseconds (UNIX time), that is retrieved from the :ref:`lib_date_time` library.

* Type 2: The message adds a configurable firmware version number to type 1 messages. This firmware version number is used in correlation with FOTA DFU, which is supported by the sample and the :ref:`lib_aws_iot` library.

A type 2 message is only published upon an initial connection to the sample, while a type 1 message is published sequentially with a configurable time in between each publishing of the data.
In addition to publishing data, the sample also subscribes to the AWS IoT shadow delta topic, and two customizable application specific topics.
The customizable topics are not part of the AWS IoT shadow and must therefore be passed to the :ref:`lib_aws_iot` library using the :c:func:`aws_iot_subscription_topics_add` function.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: thingy91_nrf9160ns, nrf9160dk_nrf9160ns

.. include:: /includes/spm.txt

.. _setup_awsiot:

Setup
*****

Before starting this sample, you should complete the following steps that are described in the :ref:`lib_aws_iot` documentation:

1. `Setting up an AWS account`_
#. :ref:`set_up_conn_to_iot`
#. :ref:`Programming device certificates <flash_certi_device>`
#. :ref:`Configuring the sample options <configure_options>`

For FOTA DFU related documentation, see :ref:`aws_fota_sample`.

.. _configure_options:

Configuration
*************

The configurations used in the sample are listed below. They are located in ``aws_iot/src/prj.conf``.

.. option:: CONFIG_APP_VERSION

Publishes the application version number to the AWS IoT message broker.

.. option:: CONFIG_PUBLICATION_INTERVAL_SECONDS

Configures the time interval between each publishing of the message.

.. note::

   The sample sets the option :option:`CONFIG_MQTT_KEEPALIVE` to the maximum allowed value, 1200 seconds (20 minutes) as specified by AWS IoT Core.
   This is to limit the IP traffic between the device and the AWS IoT message broker for supporting a low power sample.
   However, note that in certain LTE networks, the NAT timeout can be considerably lower than 1200 seconds.
   So as a recommendation, and to prevent the likelihood of getting disconnected unexpectedly, the option :option:`CONFIG_MQTT_KEEPALIVE` must be set to the lowest of the aforementioned timeout limits (Maximum allowed MQTT keepalive and NAT timeout).

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/aws_iot`
.. include:: /includes/build_and_run.txt
.. include:: /includes/spm.txt

Testing
=======

1. Make sure that you have completed the steps in :ref:`setup_awsiot`.
   This retrieves the AWS IoT broker hostname, security tag and client-id.

#. Set the :option:`CONFIG_AWS_IOT_BROKER_HOST_NAME`, :option:`CONFIG_AWS_IOT_SEC_TAG`, and :option:`CONFIG_AWS_IOT_CLIENT_ID_STATIC` options to reflect the values retrieved during step 1.
#. Program the sample to hardware.

.. note::

   The sample might require increasing the values of :option:`CONFIG_AWS_IOT_MQTT_RX_TX_BUFFER_LEN` and :option:`CONFIG_AWS_IOT_MQTT_PAYLOAD_BUFFER_LEN` options.

Sample Output
=============

When the sample runs, the device boots, and the sample displays the following output in the terminal over UART:

.. code-block:: console

        *** Booting Zephyr OS build v2.3.0-rc1-ncs1-snapshot1-6-gad0444b058ef  ***
        AWS IoT sample started, version: v1.0.0
        LTE cell changed: Cell ID: 33703711, Tracking area: 2305
        PSM parameter update: TAU: -1, Active time: -1
        RRC mode: Connected
        Network registration status: Connected - roaming
        PSM parameter update: TAU: 3600, Active time: 60
        RRC mode: Idle
        AWS_IOT_EVT_CONNECTING
        RRC mode: Connected
        RRC mode: Idle
        RRC mode: Connected
        AWS_IOT_EVT_CONNECTED
        Publishing: {
        "state":    {
                "reported":    {
                "app_version":    "v1.0.0",
                "batv":    4304,
                "ts":    1592305354579
                }
        }
        } to AWS IoT broker
        AWS_IOT_EVT_READY
        RRC mode: Idle
        LTE cell changed: Cell ID: 34237195, Tracking area: 2305
        Publishing: {
        "state":    {
                "reported":    {
                "batv":    4308,
                "ts":    1592305414579
                }
        }
        } to AWS IoT broker
        Next data publication in 60 seconds
        RRC mode: Connected
        RRC mode: Idle
        LTE cell changed: Cell ID: 33703711, Tracking area: 2305
        RRC mode: Connected
        Publishing: {
        "state":    {
                "reported":    {
                "batv":    4308,
                "ts":    1592305474596
                }
        }
        } to AWS IoT broker
        Next data publication in 60 seconds

Dependencies
************

This sample uses the following |NCS| libraries and drivers:

* :ref:`lib_aws_iot`
* :ref:`lib_date_time`
* :ref:`lte_lc_readme`
* :ref:`modem_info_readme`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following sample:

* :ref:`secure_partition_manager`
