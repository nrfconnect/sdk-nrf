.. _matter_template_sample:

Matter: Template
################

.. contents::
   :local:
   :depth: 2

This sample demonstrates a minimal implementation of the :ref:`Matter <ug_matter>` (formerly Project Connected Home over IP, Project CHIP) application layer.
This basic implementation enables the commissioning on the device, which allows it to join a Matter network.

Use this sample as a reference for developing your own application.
See the :ref:`ug_matter_creating_accessory` page for an overview of steps that you need to complete.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf52840dk_nrf52840, nrf5340dk_nrf5340_cpuapp, nrf21540dk_nrf52840

For testing purposes, that is to commission the device and :ref:`control it remotely <matter_template_network_mode>` through a Thread network, you also need a Matter controller device :ref:`configured on PC or smartphone <ug_matter_configuring>` (which requires additional hardware depending on which setup you choose).

.. note::
    |matter_gn_required_note|

Overview
********

The sample starts the Bluetooth® LE advertising automatically and prepares the Matter device for commissioning into a Matter-enabled Thread network.
The sample uses an LED to show the state of the connection.
You can press a button to start the factory reset when needed.

.. _matter_template_network_mode:

Remote testing in a network
===========================

Testing in a Matter-enabled Thread network requires a Matter controller that you can configure either on PC or mobile.
By default, the Matter accessory device has Thread disabled.
You must pair it with the Matter controller over Bluetooth LE to get configuration from the controller if you want to use the device within a Thread network.
You can enable the controller after :ref:`building and running the sample <matter_template_network_testing>`.

To pair the device, the controller must get the commissioning information from the Matter accessory device and provision the device into the network.

Commissioning in Matter
-----------------------

In Matter, the commissioning procedure takes place over Bluetooth LE between a Matter accessory device and the Matter controller, where the controller has the commissioner role.
When the procedure has completed, the device is equipped with all information needed to securely operate in the Matter network.

During the last part of the commissioning procedure (the provisioning operation), the Matter controller sends the Thread network credentials to the Matter accessory device.
As a result, the device can join the Thread network and communicate with other Thread devices in the network.

To start the commissioning procedure, the controller must get the commissioning information from the Matter accessory device.
The data payload, which includes the device discriminator and setup PIN code, is encoded within a QR code, printed to the UART console.

Configuration
*************

|config|

FEM support
===========

.. include:: /includes/sample_fem_support.txt

User interface
**************

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_led1_start
    :end-before: matter_door_lock_sample_led1_end

Button 1:
     If pressed for 6 seconds, it initiates the factory reset of the device.
     Releasing the button within the 6-second window cancels the factory reset procedure.

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_jlink_start
    :end-before: matter_door_lock_sample_jlink_end

Building and running
********************

.. |sample path| replace:: :file:`samples/matter/template`

.. include:: /includes/build_and_run.txt

Testing
=======

When you have built the sample and programmed it to your development kit, it automatically starts the Bluetooth LE advertising and the **LED1** starts flashing (Short Flash On).
At this point, you can press **Button 1** for 6 seconds to initiate the factory reset of the device.

.. note::
    If you are new to Matter, commission the Matter device using the Android Mobile Controller when :ref:`setting up Matter development environment <ug_matter_configuring_mobile>`.

.. _matter_template_network_testing:

Testing in a network
--------------------

To test the sample in a Matter-enabled Thread network, complete the following steps:

.. matter_template_sample_testing_start

1. |connect_kit|
#. |connect_terminal|
#. Commission the device into a Thread network by following the guides linked on the :ref:`ug_matter_configuring` page for the Matter controller you want to use.
   As part of this procedure, you will complete the following steps:

   * Configure Thread Border Router.
   * Build and install the Matter controller.
   * Commission the device.
   * Send Matter commands.

   At the end of this procedure, **LED 1** of the Matter device programmed with the sample starts flashing in the Short Flash Off state.
   This indicates that the device is fully provisioned, but does not yet have full Thread network connectivity.
#. Press **Button 1** for 6 seconds to initiate the factory reset of the device.

The device reboots after all its settings are erased.

Dependencies
************

This sample uses the Matter library, which includes the |NCS| platform integration layer:

* `Matter`_

In addition, the sample uses the following |NCS| components:

* :ref:`dk_buttons_and_leds_readme`

The sample depends on the following Zephyr libraries:

* :ref:`zephyr:logging_api`
* :ref:`zephyr:kernel_api`
