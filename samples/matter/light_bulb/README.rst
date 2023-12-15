.. _matter_light_bulb_sample:
.. _chip_light_bulb_sample:

Matter: Light bulb
##################

.. contents::
   :local:
   :depth: 2

This light bulb sample demonstrates the usage of the :ref:`Matter <ug_matter>` application layer to build a white dimmable light bulb device.
This device works as a Matter accessory device, meaning it can be paired and controlled remotely over a Matter network built on top of a low-power, 802.15.4 Thread network or on top of a Wi-Fi network.
Support for both Thread and Wi-Fi is mutually exclusive and depends on the hardware platform, so only one protocol can be supported for a specific light bulb device.
You can use this sample as a reference for creating your own application.

.. note::
    This sample is self-contained and can be tested on its own.
    However, it is required when testing the :ref:`Matter light switch <matter_light_switch_sample>` sample.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

If you want to commission the light bulb device and :ref:`control it remotely <matter_light_bulb_network_mode>`, you also need a Matter controller device :ref:`configured on PC or mobile <ug_matter_configuring>`.
This requires additional hardware depending on the setup you choose.

.. note::
    |matter_gn_required_note|


IPv6 network support
====================

The development kits for this sample offer the following IPv6 network support for Matter:

* Matter over Thread is supported for ``nrf52840dk_nrf52840``, ``nrf5340dk_nrf5340_cpuapp``, and ``nrf21540dk_nrf52840``.
* Matter over Wi-Fi is supported for ``nrf5340dk_nrf5340_cpuapp`` with the ``nrf7002ek`` shield attached or for ``nrf7002dk_nrf5340_cpuapp``.

Overview
********

The sample uses buttons to test changing the light bulb and device states, and LEDs to show the state of these changes.
You can test it in the following ways:

* Standalone, by using a single DK that runs the light bulb application.
* Remotely over the Thread or Wi-Fi, which requires more devices.

The remote control testing requires a Matter controller that you can configure either on a PC or a mobile device (for remote testing in a network).
You can enable both methods after :ref:`building and running the sample <matter_light_bulb_sample_remote_control>`.

.. _matter_light_bulb_network_mode:

Remote testing in a network
===========================

.. matter_light_bulb_sample_remote_testing_start

By default, the Matter accessory device has no IPv6 network configured.
You must pair it with the Matter controller over Bluetooth® LE to get the configuration from the controller to use the device within a Thread or Wi-Fi network.
The controller must get the `Onboarding information`_ from the Matter accessory device and provision the device into the network.
For details, see the `Commissioning the device`_ section.

.. matter_light_bulb_sample_remote_testing_end

Configuration
*************

|config|

Matter light bulb build types
=============================

.. matter_light_bulb_sample_configuration_file_types_start

The sample uses different configuration files depending on the supported features.
Configuration files are provided for different build types and they are located in the application root directory.

The :file:`prj.conf` file represents a ``debug`` build type.
Other build types are covered by dedicated files with the build type added as a suffix to the ``prj`` part, as per the following list.
For example, the ``release`` build type file name is :file:`prj_release.conf`.
If a board has other configuration files, for example associated with partition layout or child image configuration, these follow the same pattern.

.. include:: /config_and_build/modifying.rst
   :start-after: build_types_overview_start
   :end-before: build_types_overview_end

Before you start testing the application, you can select one of the build types supported by the sample.
This sample supports the following build types, depending on the selected board:

* ``debug`` -- Debug version of the application - can be used to enable additional features for verifying the application behavior, such as logs or command-line shell.
* ``release`` -- Release version of the application - can be used to enable only the necessary application functionalities to optimize its performance.
* ``no_dfu`` -- Debug version of the application without Device Firmware Upgrade feature support - can be used for the nRF52840 DK, nRF5340 DK, nRF7002 DK, and nRF21540 DK.

.. note::
    `Selecting a build type`_ is optional.
    The ``debug`` build type is used by default if no build type is explicitly selected.

.. matter_light_bulb_sample_configuration_file_types_end

Device Firmware Upgrade support
===============================

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_build_with_dfu_start
    :end-before: matter_door_lock_sample_build_with_dfu_end

FEM support
===========

.. include:: /includes/sample_fem_support.txt

Factory data support
====================

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_factory_data_start
    :end-before: matter_door_lock_sample_factory_data_end

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

    Additionally, the LED starts blinking evenly (500 ms on/500 ms off) when the Identify command of the Identify cluster is received on the endpoint ``1``.
    The command's argument can be used to specify the duration of the effect.

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_button1_start
    :end-before: matter_door_lock_sample_button1_end

Button 2:
    * Changes the light bulb state to the opposite one.

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_jlink_start
    :end-before: matter_door_lock_sample_jlink_end

NFC port with antenna attached:
    Optionally used for obtaining the `Onboarding information`_ from the Matter accessory device to start the :ref:`commissioning procedure <matter_light_bulb_sample_remote_control_commissioning>`.

Building and running
********************

.. |sample path| replace:: :file:`samples/matter/light_bulb`

.. include:: /includes/build_and_run.txt

See `Configuration`_ for information about building the sample with the DFU support.

Selecting a build type
======================

Before you start testing the application, you can select one of the `Matter light bulb build types`_, depending on your building method.

Selecting a build type in |VSC|
-------------------------------

.. include:: /config_and_build/modifying.rst
   :start-after: build_types_selection_vsc_start
   :end-before: build_types_selection_vsc_end

Selecting a build type from command line
----------------------------------------

.. include:: /config_and_build/modifying.rst
   :start-after: build_types_selection_cmd_start
   :end-before: For example, you can replace the

For example, you can replace the *selected_build_type* variable to build the ``release`` firmware for ``nrf52840dk_nrf52840`` by running the following command in the project directory:

.. parsed-literal::
   :class: highlight

   west build -b nrf52840dk_nrf52840 -d build_nrf52840dk_nrf52840 -- -DCONF_FILE=prj_release.conf

The ``build_nrf52840dk_nrf52840`` parameter specifies the output directory for the build files.

.. note::
   If the selected board does not support the selected build type, the build is interrupted.
   For example, if the ``shell`` build type is not supported by the selected board, the following notification appears:

   .. code-block:: console

      File not found: ./ncs/nrf/samples/matter/light_bulb/configuration/nrf52840dk_nrf52840/prj_shell.conf

Testing
=======

After building the sample and programming it to your development kit, complete the following steps to test its basic features.

You can either test the sample's basic features or use the light switch sample to test the light bulb's :ref:`communication with another device <matter_light_bulb_sample_light_switch_tests>`.

.. _matter_light_bulb_sample_basic_features_tests:

Testing basic features
----------------------

After building the sample and programming it to your development kit, complete the following steps to test its basic features:

#. |connect_kit|
#. |connect_terminal_ANSI|
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

After building this sample and the :ref:`Matter light switch <matter_light_switch_sample>` sample and programming them to the development kits, complete the steps in the following sections to test communication between both devices.

Bind both devices
+++++++++++++++++

Complete the following steps to bind both devices:

.. include:: ../light_switch/README.rst
   :start-after: matter_light_switch_sample_prepare_to_testing_start
   :end-before: matter_light_switch_sample_prepare_to_testing_end

Test connection
+++++++++++++++

.. include:: ../light_switch/README.rst
   :start-after: matter_light_switch_sample_testing_start
   :end-before: matter_light_switch_sample_testing_end

.. _matter_light_bulb_sample_remote_control:

Enabling remote control
=======================

Remote control allows you to control the Matter light bulb device from an IPv6 network.

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_remote_control_start
    :end-before: matter_door_lock_sample_remote_control_end

.. _matter_light_bulb_sample_remote_control_commissioning:

Commissioning the device
------------------------

.. matter_light_bulb_sample_commissioning_start

To commission the device, go to the :ref:`ug_matter_gs_testing` page and complete the steps for the Matter over Thread or Matter over Wi-Fi development environment and the Matter controller you want to use.
After choosing the environment configuration, the guide walks you through the following steps:

* Configure the Thread Border Router (only for Matter over Thread communication).
* Build and install the Matter controller.
* Commission the device.
* Send Matter commands that cover scenarios described in the `Testing`_ section.

If you are new to Matter, the recommended approach is to use :ref:`CHIP Tool for Linux or macOS <ug_matter_configuring_controller>`.

.. matter_light_bulb_sample_commissioning_end

Before starting the commissioning procedure, the device must be made discoverable over Bluetooth LE.
The device becomes discoverable automatically upon the device startup, but only for a predefined period of time (15 minutes by default).
If the Bluetooth LE advertising times out, use one of the following buttons to enable it again:

   * On nRF52840 DK, nRF5340 DK and nRF21540 DK:

     * Press **Button 4**.

   * On nRF7002 DK:

     * Press **Button 2** for at least three seconds.

Onboarding information
++++++++++++++++++++++

When you start the commissioning procedure, the controller must get the onboarding information from the Matter accessory device.
The onboarding information representation depends on your commissioner setup.

For this sample, you can use one of the following :ref:`onboarding information formats <ug_matter_network_topologies_commissioning_onboarding_formats>` to provide the commissioner with the data payload that includes the device discriminator and the setup PIN code:

  .. list-table:: Light bulb sample onboarding information
     :header-rows: 1

     * - QR Code
       - QR Code Payload
       - Manual pairing code
     * - Scan the following QR code with the app for your ecosystem:

         .. figure:: /images/matter_qr_code_light_bulb.png
            :width: 200px
            :alt: QR code for commissioning the light bulb device

       - MT:6FCJ142C00KA0648G00
       - 34970112332

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_onboarding_start
    :end-before: matter_door_lock_sample_onboarding_end

|matter_cd_info_note_for_samples|

Upgrading the device firmware
=============================

To upgrade the device firmware, complete the steps listed for the selected method in the :doc:`matter:nrfconnect_examples_software_update` tutorial in the Matter documentation.

Dependencies
************

This sample uses the Matter library that includes the |NCS| platform integration layer:

* `Matter`_

In addition, it uses the following |NCS| components:

* :ref:`dk_buttons_and_leds_readme`
* :ref:`nfc_uri`
* :ref:`lib_nfc_t2t`

The sample depends on the following Zephyr libraries:

* :ref:`zephyr:logging_api`
* :ref:`zephyr:kernel_api`
