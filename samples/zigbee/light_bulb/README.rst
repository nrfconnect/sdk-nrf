.. _zigbee_light_bulb_sample:

Zigbee: Light bulb
##################

.. contents::
   :local:
   :depth: 2

This :ref:`Zigbee <ug_zigbee>` Light bulb sample demonstrates a simple light bulb whose brightness can be adjusted by another device.

You can use this sample with the :ref:`Zigbee Network coordinator <zigbee_network_coordinator_sample>` and the :ref:`Zigbee Light switch <zigbee_light_switch_sample>` to set up a basic Zigbee network.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

You can use one or more of the development kits listed above and mix different development kits.

To test this sample, you also need to program the following samples:

* The :ref:`Zigbee Network coordinator <zigbee_network_coordinator_sample>` sample on one separate device.
* The :ref:`Zigbee Light switch <zigbee_light_switch_sample>` sample on one or more separate devices.

Overview
********

The Zigbee Light bulb sample takes the Zigbee Router role and implements the Dimmable Light device specification, as defined in the Zigbee Home Automation public application profile.
This profile allows changing the brightness level of a LED of the light bulb.

Configuration
*************

|config|

FEM support
===========

.. include:: /includes/sample_fem_support.txt

User interface
**************

LED 1:
    Blinks to indicate that the main application thread is running.

LED 3:
    Turns on when the light bulb joins the network.

LED 4:
    Indicates the dimmable light option, that is changes to the light bulb brightness.
    It can be controlled by another Zigbee device in the network, for example a light switch.
    Blinks when the light bulb is in Identify mode.

Button 4:
    Depending on how long the button is pressed:

    * If pressed for less than five seconds, it starts or cancels the Identify mode.
    * If pressed for five seconds, it initiates the `factory reset of the device <Resetting to factory defaults_>`_.
      The length of the button press can be edited using the :kconfig:option:`CONFIG_FACTORY_RESET_PRESS_TIME_SECONDS` Kconfig option from :ref:`lib_zigbee_application_utilities`.
      Releasing the button within this time does not trigger the factory reset procedure.

Building and running
********************
.. |sample path| replace:: :file:`samples/zigbee/light_bulb`

|enable_zigbee_before_testing|

.. include:: /includes/build_and_run.txt

.. _zigbee_light_bulb_sample_testing:

Testing
=======

After programming the sample to your development kits, complete the following steps to test it:

1. Turn on the development kit that runs the Network coordinator sample.

   When **LED 3** turns on, this development kit has become the Coordinator of the Zigbee network and the network is established.

#. Turn on the development kit that runs the Light bulb sample.

   When **LED 3** turns on, the light bulb has become a Router inside the network.

   .. note::
      If **LED 3** does not turn on, press **Button 1** on the Coordinator to reopen the network.

#. Turn on the development kit that runs the Light switch sample.

   When **LED 3** turns on, the light switch has become an End Device, connected directly to the Coordinator.

#. Wait until **LED 4** on the development kit that runs the Light switch sample turns on.

   This LED indicates that the switch found a light bulb to control.

#. Use the buttons on the development kit that runs the :ref:`zigbee_light_switch_sample` sample to control the light bulb.

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

It uses the following `sdk-nrfxlib`_ libraries:

* :ref:`nrfxlib:zboss` |zboss_version| (`API documentation`_)

In addition, it uses the following Zephyr libraries:

* :file:`include/zephyr.h`
* :file:`include/device.h`
* :ref:`zephyr:logging_api`
* :ref:`zephyr:pwm_api`
