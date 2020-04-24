.. _zigbee_light_bulb_sample:

Zigbee: Light bulb
##################

This Zigbee light bulb sample demonstrates a simple light bulb whose brightness can be regulated by a different device.

You can use this sample together with the :ref:`Zigbee network coordinator <zigbee_network_coordinator_sample>` and Zigbee light switch to set up a basic Zigbee network.

Overview
********

The Zigbee light bulb sample assumes the Zigbee Router role and implements the Dimmable Light profile.
This profile allows to change the brightness level of a LED on the development kit that runs the light bulb sample.

In the default sample configuration, the changes to the light bulb brightness are reflected on LED 4.

Requirements
************

* One or more of the following development kits:

  * |nRF52840DK|
  * |nRF52833DK|

* The :ref:`Zigbee network coordinator <zigbee_network_coordinator_sample>` sample programmed on one separate device.
* The Zigbee light switch sample programmed on one or more separate devices.

You can mix different development kits.

User interface
**************

LED 3:
    Turns on when the light bulb joins the network.

LED 4:
    Indicates the dimmable light option.
    It can be controlled by another Zigbee device in the network, for example a light switch.

Button 4:
    Puts the light bulb in Identify mode.

Building and running
********************
.. |sample path| replace:: :file:`samples/zigbee/light_bulb`

|enable_zigbee_before_testing|

.. include:: /includes/build_and_run.txt

.. _zigbee_light_bulb_sample_testing:

Testing
=======

After programming the sample to your development kits, test it by performing the following steps:

1. Turn on the development kit that runs the network coordinator sample.
   When LED 3 turns on, this development kit has become the coordinator of the Zigbee network and the network is established.
#. Turn on the development kit that runs the light bulb sample.
   When LED 3 turns on, the light bulb has become a Router inside the network.

   .. tip::
        If LED 3 does not turn on, press Button 1 on the coordinator to reopen the network.

#. Turn on the development kit that runs the light switch sample.
   When LED 3 turns on, the light switch has become an End Device, connected directly to the Coordinator.
#. Wait until LED 4 on the development kit that runs the light switch sample turns on.
   This LED indicates that the switch found a light bulb to control.
#. Use buttons on the development kit that runs the light switch sample to control the light bulb, as described in the light switch sample's user interface section.
   The result of using the buttons is reflected on the light bulb's LED 4.


Dependencies
************

This sample uses the following |NCS| libraries:

* Zigbee subsystem:

  * ``zb_nrf_platform.h``
  * ``zigbee_helpers.h``
  * ``zb_error_handler.h``

* :ref:`dk_buttons_and_leds_readme`

This sample uses the following `nrfxlib`_ libraries:

* ZBOSS Zigbee Stack

In addition, it uses the following Zephyr libraries:

* ``include/zephyr.h``
* ``include/device.h``
* :ref:`zephyr:logging_api`
* :ref:`zephyr:pwm_api`
