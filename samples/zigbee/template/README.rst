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

.. table-from-sample-yaml::

You can use one or more of the development kits listed above and mix different development kits.

To test this sample, you also need to program the :ref:`Zigbee Network coordinator <zigbee_network_coordinator_sample>` sample on one separate device.

Overview
********

The Zigbee Template sample takes the Zigbee Router role and implements two clusters (Basic and Identify) that used to be required by the Zigbee Home Automation profile.
The Basic cluster provides attributes and commands for determining basic information about the node.
The Identify cluster allows to set the device into the identification mode, which provides a way to locate the device.

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
    Depending on how long the button is pressed:

    * If pressed for less than five seconds, it starts or cancels the Identify mode.
    * If pressed for five seconds, it initiates the `factory reset of the device <Resetting to factory defaults_>`_.
      The length of the button press can be edited using the :kconfig:option:`CONFIG_FACTORY_RESET_PRESS_TIME_SECONDS` Kconfig option from :ref:`lib_zigbee_application_utilities`.
      Releasing the button within this time does not trigger the factory reset procedure.

Building and running
********************
.. |sample path| replace:: :file:`samples/zigbee/template`

|enable_zigbee_before_testing|

.. include:: /includes/build_and_run.txt

.. _zigbee_application_template_testing:

Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

1. Turn on the development kit that runs the Network coordinator sample.

   When **LED 3** turns on, this development kit has become the Coordinator of the Zigbee network and the network is established.

#. Turn on the development kit that runs the Template sample.

   When **LED 3** turns on, the light bulb has become a Router inside the network.

   .. note::
        If **LED 3** does not turn on, press **Button 1** on the Coordinator to reopen the network.

The device running the Template sample is now part of the Zigbee network as a Router.
As a result, the network range is extended by the template application radio range.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_zigbee_error_handler`
* :ref:`lib_zigbee_application_utilities`
* Zigbee subsystem:

  * :file:`zb_nrf_platform.h`

* :ref:`dk_buttons_and_leds_readme`

It uses the following `sdk-nrfxlib`_ libraries:

* :ref:`nrfxlib:zboss` |zboss_version| (`API documentation`_)

In addition, it uses the following Zephyr libraries:

* :file:`include/zephyr.h`
* :file:`include/device.h`
* :ref:`zephyr:logging_api`
