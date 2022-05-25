.. _zigbee_network_coordinator_sample:

Zigbee: Network coordinator
###########################

.. contents::
   :local:
   :depth: 2

This :ref:`Zigbee <ug_zigbee>` Network coordinator sample establishes the Zigbee network and commissions Zigbee devices that want to join the network.

You can use this sample together with the :ref:`Zigbee Light bulb <zigbee_light_bulb_sample>` and the :ref:`Zigbee Light switch <zigbee_light_switch_sample>` to set up a basic Zigbee network.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

You can use one of the development kits listed above.

Optionally, you can use this sample with one or both of the following samples:

* The :ref:`Zigbee Light bulb <zigbee_light_bulb_sample>` sample programmed on one or more separate devices.
* The :ref:`Zigbee Light switch <zigbee_light_switch_sample>` sample programmed on one or more separate devices.

You can mix different development kits.

Overview
********

This Zigbee Network coordinator sample demonstrates the Zigbee Coordinator role and implements two clusters (Basic and Identify) that used to be required by the Zigbee Home Automation profile.
It is a minimal implementation that supports only the network steering commissioning mechanism.
The Basic cluster provides attributes and commands for determining basic information about the node.
The Identify cluster allows to set the device into the identification mode, which provides a way to locate the device.

Configuration
*************

|config|

FEM support
===========

.. include:: /includes/sample_fem_support.txt

.. _zigbee_network_coordinator_user_interface:

User interface
**************

LED 1:
    Blinks to indicate that the main application thread is running.

LED 3:
    Indicates whether the network is open or closed:

    * On - The network is open.
    * Off - The network is closed.

LED 4:
    Blinks when the Network coordinator is in Identify mode.

Button 1:
    Reopens the network for 180 seconds.

    .. note::
         The network is also opened after startup.

Button 4:
    Depending on how long the button is pressed:

    * If pressed for less than five seconds, it starts or cancels the Identify mode.
    * If pressed for five seconds, it initiates the `factory reset of the device <Resetting to factory defaults_>`_.
      The length of the button press can be edited using the :kconfig:option:`CONFIG_FACTORY_RESET_PRESS_TIME_SECONDS` Kconfig option from :ref:`lib_zigbee_application_utilities`.
      Releasing the button within this time does not trigger the factory reset procedure.

Building and running
********************
.. |sample path| replace:: :file:`samples/zigbee/network_coordinator`

|enable_zigbee_before_testing|

.. include:: /includes/build_and_run.txt

.. _zigbee_network_coordinator_sample_testing:

Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

1. Turn on the development kit that runs the coordinator sample.

   * When **LED 1** starts blinking, the main application thread has started.
   * When **LED 3** turns on, this development kit has become the Coordinator of the Zigbee network and the network is established.

#. Turn on the other development kits that you programmed.

   * When **LED 3** turns on the development kit that runs the Light bulb sample, it has become a Router inside the network.
   * When **LED 3** turns on the development kit that runs the Light switch sample, it has become an End Device, connected directly to the Coordinator.

   .. note::
       If **LED 3** on the development kits does not turn on, press **Button 1** on the Coordinator to reopen the network.

#. Optionally, if you are testing with both the Light bulb and the Light switch samples, complete the following additional steps:

   a. Wait until **LED 4** on the development kit that runs the Light switch sample turns on.

      This LED indicates that the switch found a light bulb to control.

   #. Use buttons on the development kit that runs the Light switch sample to control the light bulb, as described in the Light switch sample's user interface section.

      The result of using the buttons is reflected on the light bulb's **LED 4**.

You can now use buttons on the light switch to control the light bulb, as described in the :ref:`zigbee_light_switch_user_interface` section of the Light switch sample page.

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
