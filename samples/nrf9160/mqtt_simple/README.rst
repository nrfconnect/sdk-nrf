.. _mqtt_simple_sample:

nRF9160: Simple MQTT
####################

.. contents::
   :local:
   :depth: 2

The Simple MQTT sample demonstrates how to easily connect an nRF9160 SiP to an MQTT broker and send and receive data.

Requirements
************

The sample supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9160dk_nrf9160ns

Additionally, the sample supports :ref:`qemu_x86`.

.. include:: /includes/spm.txt

Overview
*********

The sample connects to an MQTT broker and publishes the data it receives on the configured subscribe topic to the configured publish topic.
On a button press event, the sample publishes the configured message to the configured publish topic.
By default, the sample can establish a secure (TLS) connection or a non-secure connection to the configured MQTT broker.
The sample disables power saving modes (PSM and eDRX) so that network events are processed as soon as possible.

Configuration
*************

|config|

Configuration options
=====================

Check and configure the following configuration options for the sample:

.. option:: CONFIG_MQTT_BROKER_HOSTNAME - MQTT Broker host name

This configuration option defines the MQTT Broker host name.

.. option:: CONFIG_MQTT_CLIENT_ID - MQTT Client ID

This configuration option specifies the MQTT Client ID.

.. option:: CONFIG_MQTT_SUB_TOPIC - MQTT Subscribe topic

This configuration option sets the MQTT Subscribe topic.

.. option:: CONFIG_MQTT_PUB_TOPIC - MQTT Publish topic

This configuration option sets the MQTT Publish topic.

.. option:: CONFIG_MQTT_BROKER_PORT - MQTT Broker Port

This configuration option specifies the port number associated with the MQTT broker.

.. option:: CONFIG_BUTTON_EVENT_PUBLISH_MSG - Button event publish message

This configuration option specifies the message text which is published on a button press.

.. option:: CONFIG_BUTTON_EVENT_BTN_NUM - Button number for publish

This configuration option specifies the button number which, when pressed, will publish an MQTT message.

.. option:: CONFIG_MQTT_RECONNECT_DELAY_S - MQTT broker reconnect delay

This configuration option specifies the delay (in seconds) before attempting to reconnect to the broker.

.. option:: CONFIG_LTE_CONNECT_RETRY_DELAY_S - LTE connection retry delay

This configuration option specifies delay (in seconds) before attempting to retry LTE connection.

Configuration files
=====================

The sample provides the following predefined configuration files for the following development kits:

* ``prj.conf`` - For nRF9160 DK
* ``prj_qemu_x86.conf`` - For x86 Emulation (QEMU)

In addition, the sample provides overlay configuration files, which are used to enable additional features in the sample:

* ``overlay-tls.conf`` - TLS overlay configuration file for nRF9160 DK
* ``overlay-qemu-x86-tls.conf`` - TLS overlay configuration file for x86 Emulation (QEMU)
* ``overlay-carrier.conf`` - LWM2M carrier support for nRF9160 DK

They are located in ``samples/nrf9160/mqtt_simple`` folder.


To add a specific overlay configuration file to the build, add the ``-- -DOVERLAY_CONFIG=<overlay_config_file>`` parameter to the ``west build`` command.
The following command builds the sample with the TLS configuration for nRF9160 DK:

  .. code-block:: console

     west build -b nrf9160dk_nrf9160ns -- -DOVERLAY_CONFIG=overlay-tls.conf

.. note::
   The CA certificate for the default MQTT broker is included in the project and automatically provisioned after boot if the sample is built with the TLS configuration.




Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/mqtt_simple`

.. include:: /includes/build_and_run_nrf9160.txt

Testing
=======

|test_sample|

1. |connect_kit|
#. |connect_terminal|
#. Reset your nRF9160 DK.
#. Observe that the sample displays the following information in the terminal::

       The MQTT simple sample started
#. Observe that the development kit connects to the configured MQTT broker (:option:`CONFIG_MQTT_BROKER_HOSTNAME`) after it gets the LTE connection.
   At this stage, the development kit is ready to echo the data sent to it on the configured subscribe topic (:option:`CONFIG_MQTT_SUB_TOPIC`).
#. Use an MQTT client like `Mosquitto`_ to subscribe to and publish data to the broker.
   Observe that the development kit publishes all the data that you publish to :option:`CONFIG_MQTT_SUB_TOPIC` on :option:`CONFIG_MQTT_PUB_TOPIC`.

Sample output
=============

The following serial UART output is displayed in the terminal emulator:

.. code-block:: console

      *** Booting Zephyr OS build v2.4.0-ncs1-rc1-6-g45f2d5cf8ea4  ***
      <inf> mqtt_simple: The MQTT simple sample started
      <inf> mqtt_simple: LTE Link Connecting...
      <inf> mqtt_simple: LTE Link Connected!
      <inf> mqtt_simple: Disabling PSM and eDRX
      <inf> mqtt_simple: IPv4 Address found 137.135.83.217
      <inf> mqtt_simple: MQTT client connected
      <inf> mqtt_simple: Subscribing to: my/subscribe/topic len 18
      <inf> mqtt_simple: SUBACK packet id: 1234
      <inf> mqtt_simple: Publishing: Hello from nRF91 MQTT Simple Sample
      <inf> mqtt_simple: to topic: my/publish/topic len: 16
      <inf> mqtt_simple: PUBACK packet id: 51700



Troubleshooting
===============

Public MQTT brokers might be unstable.
If you experience problems connecting to the MQTT broker, try switching to another MQTT broker by changing the value of the :option:`CONFIG_MQTT_BROKER_HOSTNAME` configuration option.

.. note::
   If the :option:`CONFIG_MQTT_BROKER_HOSTNAME` configuration option is changed and the overlay TLS configuration is used, the included CA certificate must be updated with the CA certificate for
   the newly configurated MQTT broker.


Dependencies
************

This sample uses the following |NCS| library:

* :ref:`lte_lc_readme`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

It uses the following Zephyr library:

* :ref:`MQTT <zephyr:networking_api>`

In addition, it uses the following |NCS| sample:

* :ref:`secure_partition_manager`
