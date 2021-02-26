.. _chip_lock_sample:

Project Connected Home over IP: Door lock
#########################################

.. contents::
   :local:
   :depth: 2

This door lock sample demonstrates the usage of the `Project Connected Home over IP`_ application layer to build a door lock device with one basic bolt.
This device works as a Project CHIP accessory, meaning it can be paired and controlled remotely over a Project CHIP network built on top of a low-power, 802.15.4 Thread network.
You can use this sample as a reference for creating your own application.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf52840dk_nrf52840, nrf5340dk_nrf5340_cpuapp

For remote testing scenarios, you also need the following:

* If you want to commission the lock device and :ref:`control it remotely <chip_lock_sample_network_mode>` through a Thread network:

  * A smartphone compatible with Android for using the `Android CHIPTool`_ application as the Project CHIP controller.

.. note::
    |chip_gn_required_note|

Overview
********

The sample uses buttons for changing the lock and device states, and LEDs to show the state of these changes.
It can be tested in the following ways:

* Standalone, by using a single DK that runs the door lock application.
* Remotely over the Thread protocol, which requires more devices.

The remote control testing requires either commissioning by the Project CHIP controller device into a network or using the test mode.
Both methods can be enabled after :ref:`building and running the sample <chip_lock_sample_remote_control>`.

.. _chip_lock_sample_network_mode:

Remote testing in a network
===========================

.. chip_door_lock_sample_remote_testing_start

By default, the Project CHIP device has Thread disabled, and it must be paired with the Project CHIP controller over Bluetooth LE to get configuration from it if you want to use the device within a Thread network.
To do this, the device must be made discoverable manually (for security reasons) and the controller must get the commissioning information from the Project CHIP device and provision the device into the network.
For details, see the `Commissioning the device`_ section.

.. chip_door_lock_sample_remote_testing_end

.. _chip_lock_sample_test_mode:

Remote testing using test mode
==============================

.. chip_door_lock_sample_test_mode_start

Alternatively to the commissioning procedure, you can use the test mode, which allows to join a Thread network with default static parameters and static cryptographic keys.
|chip_sample_button3_note|

.. note::
    The test mode is not compliant with Project CHIP and it only works together with Project CHIP controller and other devices which use the same default configuration.

.. chip_door_lock_sample_test_mode_end

User interface
**************

.. chip_door_lock_sample_led1_start

LED 1:
    Shows the overall state of the device and its connectivity.
    The following states are possible:

    * Short Flash On (50 ms on/950 ms off) - The device is in the unprovisioned (unpaired) state and is waiting for a commissioning application to connect.
    * Rapid Even Flashing (100 ms on/100 ms off) - The device is in the unprovisioned state and a commissioning application is connected through Bluetooth LE.
    * Short Flash Off (950 ms on/50 ms off) - The device is fully provisioned, but does not yet have full Thread network or service connectivity.
    * Solid On - The device is fully provisioned and has full Thread network and service connectivity.

.. chip_door_lock_sample_led1_end

LED 2:
    Shows the state of the lock.
    The following states are possible:

    * Solid On - The bolt is extended and the door is locked.
    * Off - The bolt is retracted and the door is unlocked.
    * Rapid Even Flashing (50 ms on/50 ms off during 2 s) - The simulated bolt is in motion from one position to another.

Button 1:
    Initiates the factory reset of the device.

Button 2:
    Changes the lock state to the opposite one.

Button 3:
    Starts the Thread networking in the :ref:`test mode <chip_lock_sample_test_mode>` using the default configuration.

Button 4:
    Starts the the NFC tag emulation, enables Bluetooth LE advertising for the predefined period of time, and makes the device discoverable over Bluetooth LE.
    This button is used during the :ref:`commissioning procedure <chip_lock_sample_remote_control_commissioning>`.

SEGGER J-Link USB port:
    Used for getting logs from the device or for communicating with it through the command-line interface.

NFC port with antenna attached:
    Optionally used for obtaining the commissioning information from the Project CHIP device to start the :ref:`commissioning procedure <chip_lock_sample_remote_control>`.

Building and running
********************

.. |sample path| replace:: :file:`samples/connectedhomeip/lock`

.. include:: /includes/build_and_run.txt

Testing
=======

After building the sample and programming it to your development kit, test its basic features by performing the following steps:

#. |connect_kit|
#. |connect_terminal|
#. Observe that initially **LED 2** is lit, which means that the door lock is closed.
#. Press **Button 2** to unlock the door.
   **LED 2** is blinking while the lock is opening.
   After approximately 2 seconds, **LED 2** turns off permanently.
   The following messages appear on the console:

   .. code-block:: console

      I: Unlock Action has been initiated
      I: Unlock Action has been completed

#. Press **Button 2** one more time to lock the door again.
   **LED 2** starts blinking and then remains turned on.
   The following messages appear on the console:

   .. code-block:: console

      I: Lock Action has been initiated
      I: Lock Action has been completed

#. Press **Button 1** to initiate factory reset of the device.

The device is rebooted after all its settings are erased.

.. _chip_lock_sample_remote_control:

Enabling remote control
=======================

Remote control allows you to control the Project CHIP door lock device from a Thread network.

.. chip_door_lock_sample_remote_control_start

You can use one of the following options to enable this option:

* `Commissioning the device`_, which allows you to set up testing environment and remotely control the sample over a Project-CHIP-enabled Thread network.
* `Remote testing using test mode`_, which allows you to test the sample functionalities in a Thread network with default parameters, without commissioning.
  |chip_sample_button3_note|

.. chip_door_lock_sample_remote_control_end

.. _chip_lock_sample_remote_control_commissioning:

Commissioning the device
------------------------

.. chip_door_lock_sample_commissioning_start

To commission the device, go to the `Commissioning nRF Connect Accessory using Android CHIPTool`_ tutorial and complete the steps described there.
As part of this tutorial, you will build and program OpenThread RCP firmware, configure Thread Border Router, build and install `Android CHIPTool`_, commission the device, and send Project CHIP commands that cover scenarios described in the `Testing`_ section.

In Project CHIP, the commissioning procedure (called rendezvous) is done over Bluetooth LE between a Project CHIP device and the Project CHIP controller, where the controller has the commissioner role.
When the procedure is finished, the device should be equipped with all information needed to securely operate in the Project CHIP network.

During the last part of the commissioning procedure (the provisioning operation), Thread network credentials are sent from the Project CHIP controller to the Project CHIP device.
As a result, the device is able to join the Thread network and communicate with other Thread devices in the network.

.. note::
    Currently, Project CHIP samples do not support storing Project CHIP operational credentials exchanged during commissioning in the non-volatile memory.

To start the commissioning procedure, the controller must get the commissioning information from the Project CHIP device.
The data payload, which includes the device discriminator and setup PIN code, is encoded within a QR code, printed to the UART console, and can be shared using an NFC tag.

.. chip_door_lock_sample_commissioning_end

Dependencies
************

This sample uses Connected Home over IP library which includes the |NCS| platform integration layer:

* `Project Connected Home over IP`_

In addition, the sample uses the following |NCS| components:

* :ref:`dk_buttons_and_leds_readme`
* :ref:`nfc_uri`
* :ref:`lib_nfc_t2t`

The sample depends on the following Zephyr libraries:

* :ref:`zephyr:logging_api`
* :ref:`zephyr:kernel_api`
