.. _zigbee_network_coordinator_sample:

Zigbee: Network coordinator
###########################

This Zigbee network coordinator sample establishes the Zigbee network and commissions Zigbee devices that want to join the network.

You can use this sample together with the :ref:`Zigbee light bulb <zigbee_light_bulb_sample>` and the :ref:`Zigbee light switch <zigbee_light_switch_sample>` to set up a basic Zigbee network.

Overview
********

This Zigbee network coordinator sample demonstrates the Zigbee Coordinator role.
It is a minimal implementation that supports only the network steering commissioning mechanism.

Requirements
************

* One of the following development kits:

  * |nRF52840DK|
  * |nRF52833DK|

* One or both of the following samples:

  * The :ref:`Zigbee light bulb <zigbee_light_bulb_sample>` sample programmed on one or more separate devices.
  * The :ref:`Zigbee light switch <zigbee_light_switch_sample>` sample programmed on one or more separate devices.

You can mix different development kits.

User interface
**************

LED 3:
    Indicates whether the network is open or closed:

    * On - The network is open.
    * Off - The network is closed.

Button 1:
    Reopens the network for 180 seconds.

    .. note::
         The network is also opened after start-up.

Building and running
********************
.. |sample path| replace:: :file:`samples/zigbee/network_coordinator`

|enable_zigbee_before_testing|

.. include:: /includes/build_and_run.txt

.. _zigbee_network_coordinator_sample_testing:

Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. Turn on the development kit that runs the coordinator sample.
   When **LED 3** turns on, this development kit has become the Coordinator of the Zigbee network and the network is established.
#. Turn on the other development kits that you programmed.

   * When **LED 3** turns on the development kit that runs the light bulb sample, it has become a Router inside the network.
   * When **LED 3** turns on the development kit that runs the light switch sample, it has become an End Device, connected directly to the Coordinator.

   .. tip::
       If **LED 3** on the development kits does not turn on, press **Button 1** on the Coordinator to reopen the network.

#. Optionally, if you are testing with both the light bulb and the light switch samples, complete the following additional steps:

   #. Wait until **LED 4** on the development kit that runs the light switch sample turns on.
      This LED indicates that the switch found a light bulb to control.
   #. Use buttons on the development kit that runs the light switch sample to control the light bulb, as described in the light switch sample's user interface section.
      The result of using the buttons is reflected on the light bulb's **LED 4**.

You can now use buttons on the light switch to control the light bulb, as described in the :ref:`zigbee_light_switch_user_interface` section of the light switch sample page.

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
