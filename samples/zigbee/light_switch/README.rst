.. _zigbee_light_switch_sample:

Zigbee: Light switch
####################

The Zigbee light switch sample can be used to change the state of light sources on other devices within the same Zigbee network.

You can use this sample together with the :ref:`Zigbee network coordinator <zigbee_network_coordinator_sample>` and the :ref:`Zigbee light bulb <zigbee_light_bulb_sample>` to set up a basic Zigbee network.

Overview
********

The light switch sample demonstrates the Zigbee End Device role and implements the Dimmer Light Switch profile.

Once the light switch is successfully commissioned, it sends a broadcast message to find devices with the implemented Level Control and On/Off clusters.
The light switch remembers the device network address from the first response, at which point it can be controlled with the development kit buttons.

Sleepy End Device behavior
===================================

The light switch supports the :ref:`zigbee_ug_sed` that enables the sleepy behavior for the end device, for a significant conservation of energy.

The sleepy behavior can be enabled by pressing **Button 3** while the light switch sample is booting.

Requirements
************

* One or more of the following development kits:

  * |nRF52840DK|
  * |nRF52833DK|

* The :ref:`zigbee_network_coordinator_sample` sample programmed on one separate device.
* The :ref:`zigbee_light_bulb_sample` sample programmed on one or more separate devices.

You can mix different development kits.

.. _zigbee_light_switch_user_interface:

User interface
**************

LED 3:
    Indicates whether the device is connected to a Zigbee network.
    When connected, the LED is on and solid.

LED 4:
    Indicates that the light switch has found a light bulb to control.
    When the light bulb is found, the LED is on and solid.

Button 1:
    When the light bulb has been turned off, pressing this button once turns it back on.

    Pressing this button for a longer period of time increases the brightness of the light bulb.

Button 2:
    After the successful commissioning (light switch's **LED 3** turned on), pressing this button once turns off the light bulb connected to the network (light bulb's **LED 4**).

    Pressing this button for a longer period of time decreases the brightness of the **LED 4** of the connected light bulb.

.. note::
    If the brightness level is decreased to the minimum, the effect of turning on the light bulb might not be noticeable.

Button 3:
    When pressed while resetting the board, enables the :ref:`zigbee_ug_sed`.

Building and running
********************

.. |sample path| replace:: :file:`samples/zigbee/light_switch`

|enable_zigbee_before_testing|

.. include:: /includes/build_and_run.txt

.. _zigbee_light_switch_testing:

Testing
=======

After programming the sample to your development kits, test it by performing the following steps:

1. Turn on the development kit that runs the network coordinator sample.
   When **LED 3** turns on, this development kit has become the Coordinator of the Zigbee network.
#. Turn on the development kit that runs the light bulb sample.
   When **LED 3** turns on, the light bulb has become a Router inside the network.

   .. tip::
        If **LED 3** does not turn on, press **Button 1** on the Coordinator to reopen the network.

#. Turn on the development kit that runs the light switch sample.
   When **LED 3** turns on, the light switch has become an End Device, connected directly to the Coordinator.
#. Wait until **LED 4** on the light switch node turns on.
   This LED indicates that the light switch found a light bulb to control.

You can now use buttons on the development kit to control the light bulb, as described in :ref:`zigbee_light_switch_user_interface`.

Sample output
-------------

The sample logging output can be observed through a serial port.
For more details, see :ref:`putty`.

Dependencies
************

This sample uses the following |NCS| libraries:

* Zigbee subsystem:

  * :file:`zb_nrf_platform.h`
  * :file:`zigbee_helpers.h`
  * :file:`zb_error_handler.h`

* :ref:`dk_buttons_and_leds_readme`

This sample uses the following `nrfxlib`_ libraries:

* :ref:`nrfxlib:zboss`

In addition, it uses the following Zephyr libraries:

* :file:`include/zephyr.h`
* :file:`include/device.h`
* :ref:`zephyr:logging_api`
* :ref:`zephyr:pwm_api`
