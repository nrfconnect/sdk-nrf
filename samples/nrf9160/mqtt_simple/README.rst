.. _mqtt_simple_sample:

nRF9160: Simple MQTT
####################

The Simple MQTT sample demonstrates how to easily connect an nRF9160 SiP to an MQTT broker and send and receive data.

Overview
*********

The sample connects to an MQTT broker and publishes whatever data it receives on the configured subscribe topic to the configured publish topic.

Requirements
************

* The following development board:

  * |nRF9160DK|

* .. include:: /includes/spm.txt

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/mqtt_simple`

.. include:: /includes/build_and_run_nrf9160.txt

Testing
=======

After programming the sample and all prerequisites to the board, test it by performing the following steps:

1. Connect the USB cable and power on or reset your nRF9160 DK.
#. Open a terminal emulator and observe that the kit prints the following information::

       The MQTT simple sample started
#. Observe that the kit connects to the configured MQTT broker (``MQTT_BROKER_HOSTNAME``) after it gets LTE connection.
   Now the kit is ready to echo whatever data is sent to it on the configured subscribe topic (``MQTT_SUB_TOPIC``).
#. Use an MQTT client like mosquitto to subscribe to and publish data to the broker.
   Observe that the kit publishes all data that you publish to ``MQTT_SUB_TOPIC`` on ``MQTT_PUB_TOPIC``.

Troubleshooting
===============

Public MQTT brokers might be unstable.
If you experience problems connecting to the MQTT broker, try switching to another broker by changing the value of the ``MQTT_BROKER_HOSTNAME`` configuration option.


Dependencies
************

This sample uses the following libraries:

From |NCS|
  * ``drivers/lte_link_control``

From nrfxlib
  * :ref:`nrfxlib:bsdlib`

From Zephyr
  * :ref:`MQTT <zephyr:networking_reference>`

In addition, it uses the following samples:

From |NCS|
  * :ref:`secure_partition_manager`
