.. _zigbee_light_bulb_sample:

Zigbee: Light bulb
##################

.. contents::
   :local:
   :depth: 2

This :ref:`Zigbee <ug_zigbee>` light bulb sample demonstrates a simple light bulb whose brightness can be regulated by another device.

You can use this sample with the :ref:`Zigbee network coordinator <zigbee_network_coordinator_sample>` and the :ref:`Zigbee light switch <zigbee_light_switch_sample>` to set up a basic Zigbee network.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf52840dk_nrf52840, nrf52833dk_nrf52833, nrf5340dk_nrf5340_cpuapp, nrf21540dk_nrf52840

You can use one or more of the development kits listed above and mix different development kits.

For this sample to work, the following samples also need to be programmed:

* The :ref:`Zigbee network coordinator <zigbee_network_coordinator_sample>` sample on one separate device.
* The :ref:`Zigbee light switch <zigbee_light_switch_sample>` sample on one or more separate devices.

Overview
********

The Zigbee light bulb sample takes the Zigbee Router role and implements the Dimmable Light profile.
This profile allows changing the brightness level of a LED of the light bulb.

Configuration
*************

|config|

FEM support
===========

.. |fem_file_path| replace:: :file:`samples/zigbee/common`

.. include:: /includes/sample_fem_support.txt

User interface
**************

LED 3:
    Turns on when the light bulb joins the network.

LED 4:
    Indicates the dimmable light option, that is changes to the light bulb brightness.
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
   When **LED 3** turns on, this development kit has become the Coordinator of the Zigbee network and the network is established.
#. Turn on the development kit that runs the light bulb sample.
   When **LED 3** turns on, the light bulb has become a Router inside the network.

   .. tip::
        If **LED 3** does not turn on, press **Button 1** on the Coordinator to reopen the network.

#. Turn on the development kit that runs the light switch sample.
   When **LED 3** turns on, the light switch has become an End Device, connected directly to the Coordinator.
#. Wait until **LED 4** on the development kit that runs the light switch sample turns on.
   This LED indicates that the switch found a light bulb to control.
#. Use buttons on the development kit that runs the light switch sample to control the light bulb, as described in the light switch sample's user interface section.
   The result of using the buttons is reflected on the light bulb's **LED 4**.

You can now use buttons on the light switch to control the light bulb, as described in the :ref:`zigbee_light_switch_user_interface` section of the light switch sample page.

Dependencies
************

This sample uses the following |NCS| libraries:

* :file:`include/zigbee/zigbee_error_handler.h`
* :ref:`lib_zigbee_application_utilities`
* Zigbee subsystem:

  * :file:`zb_nrf_platform.h`

* :ref:`dk_buttons_and_leds_readme`

This sample uses the following `sdk-nrfxlib`_ libraries:

* :ref:`nrfxlib:zboss`

In addition, it uses the following Zephyr libraries:

* :file:`include/zephyr.h`
* :file:`include/device.h`
* :ref:`zephyr:logging_api`
* :ref:`zephyr:pwm_api`
