.. _matter_light_bulb_sample:
.. _chip_light_bulb_sample:

Matter: Light bulb
##################

.. contents::
   :local:
   :depth: 2

This light bulb sample demonstrates the usage of the `Matter`_ (formerly Project Connected Home over IP, Project CHIP) application layer to build a white dimmable light bulb device.
This device works as a Matter accessory, meaning it can be paired and controlled remotely over a Matter network built on top of a low-power, 802.15.4 Thread network.
You can use this sample as a reference for creating your own application.

.. note::
    This sample is self-contained and can be tested on its own.
    However, it is required when testing the :ref:`Matter light switch <matter_light_switch_sample>` sample.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf52840dk_nrf52840, nrf5340dk_nrf5340_cpuapp

For remote testing scenarios, you also need the following:

* If you want to commission the light bulb device and :ref:`control it remotely <matter_light_bulb_network_mode>` through a Thread network:

  * A smartphone compatible with Android for using the `Android CHIPTool`_ application as the Matter controller.

* If you want to use the :ref:`test mode <matter_light_bulb_sample_test_mode>` and control the light bulb using light switch:

  * :ref:`Matter light switch <matter_light_switch_sample>` programmed to another supported development kit.

.. note::
    |matter_gn_required_note|

Overview
********

The sample uses buttons to test changing the light bulb and device states, and LEDs to show the state of these changes.
It can be tested in the following ways:

* Standalone, by using a single DK that runs the light bulb application.
* Remotely over the Thread protocol, which requires more devices.

The remote control testing requires either commissioning by the Matter controller device into a network or using the test mode.
Both methods can be enabled after :ref:`building and running the sample <matter_light_bulb_sample_remote_control>`.

.. _matter_light_bulb_network_mode:

Remote testing in a network
===========================

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_remote_testing_start
    :end-before: matter_door_lock_sample_remote_testing_end

.. _matter_light_bulb_sample_test_mode:

Remote testing using test mode
==============================

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_test_mode_start
    :end-before: matter_door_lock_sample_test_mode_end

Using the test mode allows you to control the light bulb remotely without using a smartphone compatible with Android.
The light bulb device programmed with this sample can be used with the light switch device programmed with the :ref:`Matter light switch <matter_light_switch_sample>` sample to create a simplified Thread network.

Configuration
*************

|config|

User interface
**************

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_led1_start
    :end-before: matter_door_lock_sample_led1_end

LED 2:
    Shows the state of the light bulb.
    The following states are possible:

    * Solid On - The light bulb is on.
    * Off - The light bulb is off.

Button 1:
    Initiates the factory reset of the device.

Button 2:
    Changes the light bulb state to the opposite one.

Button 3:
    Starts the Thread networking in the :ref:`test mode <matter_light_bulb_sample_test_mode>` using the default configuration.

    When running application in light switch test mode it also starts publishing the light bulb service messages for a predefined period of time to advertise the light bulb device IP address to the light switch device (if used).
    If for some reason the light switch device is not able to receive messages during this predefined period of time, you can restart publishing service by pressing this button again.

Button 4:
    Starts the the NFC tag emulation, enables Bluetooth LE advertising for the predefined period of time, and makes the device discoverable over Bluetooth LE.
    This button is used during the :ref:`commissioning procedure <matter_light_bulb_sample_remote_control_commissioning>`.

SEGGER J-Link USB port:
    Used for getting logs from the device or communicating with it through the command-line interface.

NFC port with antenna attached:
    Optionally used for obtaining the commissioning information from the Matter device to start the :ref:`commissioning procedure <matter_light_bulb_sample_remote_control_commissioning>`.

Building and running
********************

.. |sample path| replace:: :file:`samples/matter/light_bulb`

.. include:: /includes/build_and_run.txt

Testing
=======

You can either test the sample's :ref:`basic features <matter_light_bulb_sample_basic_features_tests>` or use the light switch sample to test the light bulb's :ref:`communication with another device <matter_light_bulb_sample_light_switch_tests>`.

.. _matter_light_bulb_sample_basic_features_tests:

Testing basic features
----------------------

After building the sample and programming it to your development kit, test its basic features by completing the following steps:

#. |connect_kit|
#. |connect_terminal|
#. Observe that **LED 2** is off.
#. Press **Button 2** on the light bulb device.
   The **LED 2** turns on and the following messages appear on the console:

   .. code-block:: console

      I: Turn On Action has been initiated
      I: Turn On Action has been completed

#. Press **Button 2** again.
   The **LED 2** turns off and the following messages appear on the console:

   .. code-block:: console

      I: Turn Off Action has been initiated
      I: Turn Off Action has been completed

#. Press **Button 1** to initiate the factory reset of the device.

.. _matter_light_bulb_sample_light_switch_tests:

Testing communication with another device
-----------------------------------------

After building this sample and the :ref:`Matter light switch <matter_light_switch_sample>` sample and programming them to development kits, test communication between both devices by completing the following steps:

.. include:: ../light_switch/README.rst
    :start-after: matter_light_switch_sample_testing_start
    :end-before: matter_light_switch_sample_testing_end

.. _matter_light_bulb_sample_remote_control:

Enabling remote control
=======================

Remote control allows you to control the Matter light bulb device from a Thread network.

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_remote_control_start
    :end-before: matter_door_lock_sample_remote_control_end

.. _matter_light_bulb_sample_remote_control_commissioning:

Commissioning the device
------------------------

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_commissioning_start
    :end-before: matter_door_lock_sample_commissioning_end

Dependencies
************

This sample uses the Matter library, which includes the |NCS| platform integration layer:

* `Matter`_

In addition, the sample uses the following |NCS| components:

* :ref:`dk_buttons_and_leds_readme`
* :ref:`nfc_uri`
* :ref:`lib_nfc_t2t`

The sample depends on the following Zephyr libraries:

* :ref:`zephyr:logging_api`
* :ref:`zephyr:kernel_api`
