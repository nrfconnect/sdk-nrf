.. _zigbee_shell_sample:

Zigbee: Shell
#############

.. contents::
   :local:
   :depth: 2

This :ref:`Zigbee <ug_zigbee>` shell sample demonstrates a Zigbee router (with the possibility of being a coordinator) that uses the :ref:`lib_zigbee_shell` library for interaction.

The sample can be used for several purposes, including:

* Initial configuration of the network - forming a network as coordinator, adding devices to the network with the install codes, setting the extended PAN ID.
* Benchmarking - measuring time needed for a message to travel from one node to another.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf52840dk_nrf52840, nrf52833dk_nrf52833, nrf5340dk_nrf5340_cpuapp

You can use one or more of the development kits listed above and mix different development kits.

For this sample to work, the following samples also need to be programmed:

* The :ref:`Zigbee network coordinator <zigbee_network_coordinator_sample>` sample on one separate device or using a second Zigbee shell sample.

Additionally, you can use this sample with any other :ref:`Zigbee <ug_zigbee>` sample application for testing the communication or other sample-specific functionalities.

Overview
********

The Zigbee shell sample takes the Zigbee Router role and implements two clusters: Basic and Identify.
The Basic cluster provides attributes and commands for determining basic information about the node.
The Identify cluster allows to put the device into the identification mode, which provides a way to locate the device.
The device also includes all Zigbee shell commands that allow for discovering, controlling and testing other Zigbee devices.

Sample logging
==============

The Zigbee shell sample does not use the default logging settings for the |NCS| samples.
Unlike the default sample logging in the |NCS|, the Zigbee shell sample's logger is configured to also log module name and timestamps for every message.
The sample has log shell commands enabled for configuring logging, for example the logging level for each log module.

The :ref:`lib_zigbee_shell` library also enables the :ref:`lib_zigbee_logger_endpoint` library by default.
This library will log information about every ZCL packet received, which can be useful for debugging.
You can enable or disable logging from the endpoint logger module by using the commands described in the `Enabling and disabling endpoint logging`_.

Available shell interfaces
==========================

You can run the Zigbee shell commands after connecting and configuring any of the :ref:`supported backends <lib_zigbee_shell>` for testing.
These interfaces are completely independent one from another and can be used simultaneously or exclusively.
For information about setup, see :ref:`gs_testing`.

User interface
**************

LED 3:
    Turns on when the device joins the network.

LED 4:
    Blinks to indicate that the identification mode is on.

Button 4:
    Toggles the identification mode on the device.

All other interactions with the application can be handled using **serial** communication.
See :ref:`zigbee_cli_reference` for available **serial** commands.

Configuration
*************

|config|

Enabling and disabling endpoint logging
=======================================

Zigbee shell sample has :ref:`Zigbee endpoint logger library <lib_zigbee_logger_endpoint>` enabled by default and will log every ZCL packet received.

You can enable and disable logs from endpoint logger using the ``log enable`` and ``log disable`` shell commands with the appropriate log module instance name, respectively:

* To disable the logs from Zigbee endpoint logger, use the following command:

  .. code-block::

     log disable zigbee.eprxzcl

* To enable logs from Zigbee endpoint logger and set its logging to the info level (``inf``), use the following command:

  .. code-block::

     log enable inf zigbee.eprxzcl

  You can also use the following command to see other available logging levels:

  .. code-block::

     log enable --help

Building and running
********************
.. |sample path| replace:: :file:`samples/zigbee/shell`

|enable_zigbee_before_testing|

.. include:: /includes/build_and_run.txt

.. _zigbee_shell_sample_testing:

Testing
=======

In this testing procedure, both of the development kits are programmed with the Zigbee shell sample.
One of these samples acts as Zigbee Coordinator, the second one as Zigbee Router.

After building the sample and programming it to your development kits, test it by performing the following steps:

#. Turn on the development kits.
#. Set up the **serial** connection with the development kits using one of the `Available shell interfaces`_.
#. Set one shell device to work as coordinator by running the following shell command:

   .. code-block::

      bdb role zc

   This shell device is now the shell coordinator node.
#. Issue the following command on the shell coordinator node to start a new Zigbee network:

   .. code-block::

      bdb start

#. Run the following command on the second board programmed with the shell sample:

   .. code-block::

      bdb start

   The shell device joins the network.
#. Check that the shell device has commissioned by running the following command:

   .. code-block::

      zdo short

   The command returns the acquired short address of the shell device.
#. Check the communication between the nodes by issuing a ping request with the acquired short address value and the payload size:

   .. parsed-literal::
      :class: highlight

      zcl ping *zdo_short_address* *payload_size*

   For example:

   .. code-block::

      zcl ping 0x2485 10

   The ping time response is returned when the ping is successful, followed by the additional information from the endpoint logger.
   For example:

   .. code-block::

      Ping time: 20 ms
      Done
      [00:00:21.261,810] <inf> zigbee.eprxzcl: Received ZCL command (0): src_addr=0x2485(short) src_ep=64 dst_ep=64 cluster_id=0xbeef
      profile_id=0x0104 cmd_dir=0 common_cmd=0 cmd_id=0x01 cmd_seq=0 disable_def_resp=1 manuf_code=void payload=[cdcdcdcdcdcdcdcdcdcd] (0)

#. Disable the endpoint logger:

   .. code-block::

      log disable zigbee.eprxzcl

#. Issuing another ping request:

   .. code-block::

      zcl ping 0x2485 10

   The result does not include the endpoint logger information anymore.
   For example:

   .. code-block::

      Ping time: 20 ms
      Done

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_zigbee_shell`
* :ref:`lib_zigbee_error_handler`
* :ref:`lib_zigbee_application_utilities`
* Zigbee subsystem:

  * :file:`zb_nrf_platform.h`

* :ref:`dk_buttons_and_leds_readme`

This sample uses the following `sdk-nrfxlib`_ libraries:

* :ref:`nrfxlib:zboss` |zboss_version| (`API documentation`_)

In addition, it uses the following Zephyr libraries:

* :file:`include/zephyr.h`
* :file:`include/device.h`
* :ref:`zephyr:logging_api`
