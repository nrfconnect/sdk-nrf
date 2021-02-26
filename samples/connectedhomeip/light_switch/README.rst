.. _chip_light_switch_sample:

Project Connected Home over IP: Light switch
############################################

.. contents::
   :local:
   :depth: 2

This light switch sample demonstrates the usage of the `Project Connected Home over IP`_ application layer to build a basic controller device for the :ref:`CHIP light bulb <chip_light_bulb_sample>` sample.
This device works as a Project CHIP controller, meaning that it can control a light bulb remotely over a Project CHIP network built on top of a low-power, 802.15.4 Thread network.
You can use this sample as a reference for creating your own application.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf52840dk_nrf52840, nrf5340dk_nrf5340_cpuapp

For this sample to work, you also need the :ref:`Project CHIP light bulb <chip_light_bulb_sample>` sample programmed to another supported development kit.

.. note::
    |chip_gn_required_note|

Overview
********

The light switch must be put into the test mode and paired with a light bulb device to be able to send Project CHIP light bulb control commands such as toggling the light and setting its brightness level.

.. _chip_light_switch_sample_test_mode:

Test mode
=========

Unlike other samples, such as :ref:`Project CHIP door lock <chip_lock_sample>`, the light switch sample does not support Project CHIP commissioning over Bluetooth LE.
To make the light switch and the light bulb devices able to communicate with each other, they must be initialized with the same static Thread network parameters and static Project CHIP cryptographic keys.

Pressing **Button 3** activates the test mode before enabling the pairing phase on the device.

.. _chip_light_switch_sample_pairing:

Pairing
=======

After the test mode is activated and both devices are initialized with the same network parameters, the pairing phase starts.
During this phase, the light bulb device periodically sends multicast messages with static content.
Once such a message is intercepted by the light switch device, the light switch is made aware of the IP address of the nearby light bulb device and the pairing is done.

User interface
**************

LED 1:
    Shows the overall state of the device and its connectivity.
    The following states are possible:

    * Solid Off - The device is not paired with any light bulb device.
    * Short Flash On (100 ms on/900 ms off) - The device is in the :ref:`pairing <chip_light_switch_sample_pairing>` phase.
    * Solid On - The device is paired with a light bulb device.

Button 1:
    Initiates the factory reset of the device.

Button 2:
    Toggles the light on the paired light bulb device.

Button 3:
    Enables :ref:`pairing <chip_light_switch_sample_pairing>` for 10 seconds.
    It also starts the Thread networking in the :ref:`test mode <chip_light_switch_sample_test_mode>` if the Thread network is not yet initialized.

Button 4:
    Brightens the light on the paired light bulb device.
    The brightness level is increased gradually while the button is being pressed.
    When the button is pressed for too long, the brightness level wraps around and becomes the lowest possible.

SEGGER J-Link USB port:
    Used for getting logs from the device or for communicating with it through the command-line interface.

Building and running
********************

.. |sample path| replace:: :file:`samples/connectedhomeip/light_switch`

.. include:: /includes/build_and_run.txt

Testing
=======

After building this and :ref:`CHIP light bulb <chip_light_bulb_sample>` samples, and programming them to your development kits, test communication between both devices by performing the following steps:

.. chip_light_switch_sample_testing_start

#. Complete the following actions for both devices:

   #. |connect_kit|
   #. |connect_terminal|

#. On both devices, press **Button 1** to reset them to factory settings.
#. Pair both devices by completing the following steps:

   a. On the light switch device, press **Button 3** to enable :ref:`pairing <chip_light_switch_sample_pairing>` on this device.
      The light switch becomes the Thread network Leader.
      The following messages appear in the console for the light switch device:

      .. code-block:: console

         I: Device is not commissioned to a Thread network. Starting with the default configuration
         I: Starting light bulb discovery

   b. On the light bulb device, press **Button 3** to enable pairing on this device.
      The following messages appear on the console for the light bulb device:

      .. code-block:: console

         I: Device is not commissioned to a Thread network. Starting with the default configuration
         I: Started Publish service

      At this point, the light bulb is discovered by the light switch device and the following messages appear on the console for the light switch device:

      .. code-block:: console

         I: Stopping light bulb discovery
         I: Pairing with light bulb fdde:ad00:beef:0:7b0:750e:6d96:49e9

#. On the light switch device, press **Button 2** to turn off the light on the light bulb device.
   The following message appears on the console for the light switch device:

   .. code-block:: console

      I: Toggling the light

   **LED 2** on the light bulb device turns off.
#. On the light switch device, press **Button 2** to turn on the light again.
   **LED 2** on the light bulb device turns back on.
#. On the light switch device, press **Button 4** and hold it for a few seconds.
   Messages similar to the following one appear in the console:

   .. code-block:: console

    I: Setting brightness level to 7

   The brightness of **LED 2** on the light bulb device changes while the button is pressed.

.. chip_light_switch_sample_testing_end

Dependencies
************

This sample uses Connected Home over IP library which includes the |NCS| platform integration layer:

* `Project Connected Home over IP`_

In addition, the sample uses the following |NCS| components:

* :ref:`dk_buttons_and_leds_readme`

The sample depends on the following Zephyr libraries:

* :ref:`zephyr:logging_api`
* :ref:`zephyr:kernel_api`
