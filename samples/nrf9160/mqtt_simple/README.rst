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

  * nRF9160 DK board (PCA10090)

* :ref:`secure_partition_manager` must be programmed on the board.

  The sample is configured to compile and run as a non-secure application on nRF91's Cortex-M33.
  Therefore, it requires the :ref:`secure_partition_manager` that prepares the required peripherals to be available for the application.

Building and running
********************

This sample can be found under :file:`samples/nrf9160/mqtt_simple` in the |NCS| folder structure.

The sample is built as a non-secure firmware image for the nrf9160_pca10090ns board.
It can be programmed independently from the Secure Partition Manager firmware.

See :ref:`gs_programming` for information about how to build and program the application.
Note that you must program two different applications as described in the following section.

Programming the sample
======================

When you connect the nRF9160 DK board to your computer, you must first make sure that the :ref:`secure_partition_manager` sample is programmed:

1. Put the **SW5** switch (marked debug/prog) in the **NRF91** position to program the main controller.
#. Build the :ref:`secure_partition_manager` sample and program it.
#. Build the Simple MQTT sample (this sample) and program it.
#. Verify that the sample was programmed successfully by connecting to the serial port with a terminal emulator (for example, PuTTY) and checking the output.
   See :ref:`putty` for the required settings.

Testing
=======

After programming the sample and all prerequisites to the board, test it by performing the following steps:

1. Connect the USB cable and power on your nRF9160 DK.
#. Open a teminal emulator and observe that the kit prints the following information::

       The MQTT simple sample started
#. Observe that the kit connects to the configured MQTT broker (``CONFIG_MQTT_BROKER_HOSTNAME``) after it gets LTE connection.
   Now the kit is ready to echo whatever data is sent to it on the configured subscribe topic (``CONFIG_MQTT_SUB_TOPIC``).
#. Use an MQTT client like mosquitto to subscribe to and publish data to the broker.
   Observe that the kit publishes all data that you publish to ``CONFIG_MQTT_SUB_TOPIC`` on ``CONFIG_MQTT_PUB_TOPIC``.


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
