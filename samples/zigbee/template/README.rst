.. _zigbee_template_sample:

Zigbee: Template
################

.. contents::
   :local:
   :depth: 2

This :ref:`Zigbee <ug_zigbee>` sample is a minimal implementation of the Zigbee Router role.

You can use this sample as the starting point for developing your own Zigbee device.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf52840dk_nrf52840, nrf52833dk_nrf52833, nrf5340dk_nrf5340_cpuapp, nrf21540dk_nrf52840

You can use one or more of the development kits listed above and mix different development kits.

For this sample to work, the following samples also need to be programmed:

* The :ref:`Zigbee network coordinator <zigbee_network_coordinator_sample>` sample on one separate device.

Overview
********

The Zigbee template sample takes the Zigbee Router role and implements two clusters (Basic and Identify) that used to be required by the Zigbee Home Automation profile.
The Basic cluster provides attributes and commands for determining basic information about the node.
The Identify cluster allows to put the device into the identification mode, which provides a way to locate the device.

.. _zigbee_template_configuration:

Configuration
*************

|config|

FEM support
===========

.. include:: /includes/sample_fem_support.txt

User interface
**************

LED 3:
    Turns on when the device joins the network.

LED 4:
    Blinks to indicate that the identification mode is on.

Button 4:
    Toggles the identification mode on the device.

Building and running
********************
.. |sample path| replace:: :file:`samples/zigbee/template`

|enable_zigbee_before_testing|

.. include:: /includes/build_and_run.txt

.. _zigbee_application_template_testing:

Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. Turn on the development kit that runs the network coordinator sample.
   When **LED 3** turns on, this development kit has become the Coordinator of the Zigbee network and the network is established.
#. Turn on the development kit that runs the template sample.
   When **LED 3** turns on, the light bulb has become a Router inside the network.

   .. tip::
        If **LED 3** does not turn on, press **Button 1** on the Coordinator to reopen the network.

The device running the template sample is now part of the Zigbee network as a Router.
As a result, the network range is extended by the template application radio range.

Dependencies
************

This sample uses the following |NCS| libraries:

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
