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

The sample can also communicate with `AWS IoT Core`_ over a Wi-Fi network using the nRF7002 DK.
For more details, see the :ref:`matter_light_bulb_aws_iot` section.

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

* Matter over Thread is supported for ``nrf52840dk/nrf52840``, ``nrf5340dk/nrf5340/cpuapp``, ``nrf21540dk/nrf52840``, and ``nrf54l15pdk/nrf54l15/cpuapp``.
* Matter over Wi-Fi is supported for ``nrf5340dk/nrf5340/cpuapp`` with the ``nrf7002ek`` shield attached or for ``nrf7002dk/nrf5340/cpuapp``.

Overview
********

The sample uses buttons to test changing the light bulb and device states, and LEDs to show the state of these changes.
You can test it in the following ways:

* Standalone, by using a single DK that runs the light bulb application.
* Remotely over Thread or Wi-Fi, which requires more devices.

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

.. _matter_light_bulb_custom_configs:

Matter light bulb custom configurations
=======================================

.. matter_light_bulb_sample_configuration_file_types_start

.. include:: /includes/sample_custom_config_intro.txt

The sample supports the following configurations:

.. list-table:: Sample configurations
   :widths: auto
   :header-rows: 1

   * - Configuration
     - File name
     - :makevar:`FILE_SUFFIX`
     - Supported board
     - Description
   * - Debug (default)
     - :file:`prj.conf`
     - No suffix
     - All from `Requirements`_
     - Debug version of the application.

       Enables additional features for verifying the application behavior, such as logs.
   * - Release
     - :file:`prj_release.conf`
     - ``release``
     - All from `Requirements`_
     - Release version of the application.

       Enables only the necessary application functionalities to optimize its performance.

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

.. _matter_light_bulb_aws_iot:

AWS IoT integration
===================

The sample can be configured to communicate with `AWS IoT Core`_ to control attributes in supported clusters on the device.
After a connection has been established, the sample will mirror these attributes in the AWS IoT shadow document.
This makes it possible to remotely control the device using the `AWS IoT Device Shadow Service`_.
The supported attributes are ``OnOff`` from the ``OnOff`` cluster and ``CurrentLevel`` from the ``LevelControl`` cluster.

The following figure illustrates the relationship between the AWS IoT integration layer and the light bulb sample:

.. figure:: /images/aws_matter_integration.svg
   :alt: Sample implementation of the AWS IoT integration layer

   AWS IoT integration layer implementation diagram

The following figure illustrates the interaction with the AWS IoT shadow Service:


.. figure:: /images/aws_matter_interaction.svg
   :alt: Interaction with the AWS IoT shadow service

   AWS IoT Shadow and Matter interaction diagram

AWS IoT setup and configuration
-------------------------------

To set up an AWS IoT instance and configure the sample, complete the following steps:

1. Complete the setup and configuration described in the :ref:`lib_aws_iot` documentation to get the host name, device ID, and certificates used in the connection.
#. Set the :kconfig:option:`CONFIG_AWS_IOT_BROKER_HOST_NAME` and :kconfig:option:`CONFIG_AWS_IOT_CLIENT_ID_STATIC` Kconfig options in the :file:`overlay-aws-iot-integration.conf` file.
#. Import the certificates to the :file:`light_bulb/src/aws_iot_integration/certs` folder.

   The certificates will vary in size depending on the method you chose when generating the certificates.
   Due to this, you might need to increase the value of the :kconfig:option:`CONFIG_MBEDTLS_SSL_OUT_CONTENT_LEN` option to be able to establish a connection.
#. Build the sample using the following command:

   .. code-block:: console

      west build -p -b nrf7002dk/nrf5340/cpuapp -- -DEXTRA_CONF_FILE="overlay-aws-iot-integration.conf"

#. Flash the firmware and boot the sample.
#. |connect_kit|
#. |connect_terminal_ANSI|
#. Commission the device to the Matter network.
   See `Commissioning the device`_ for more information.
#. Observe that the device automatically connects to AWS IoT when an IP is obtained and the device is able to maintain the connection.
#. Use the following bash function to populate the desired section of the shadow:

   .. code-block:: console

      function aws-update-desired() {
            aws iot-data update-thing-shadow --cli-binary-format raw-in-base64-out --thing-name my-thing --payload "{\"state\":{\"desired\":{\"onoff\":$1,\"level_control\":$2}}}" "output.txt"
      }

   You can also use ``aws-update-desired 0 0``, or ``aws-update-desired 1 128 (onoff, levelcontrol)``.
   Alternatively, you can alter the device shadow directly through the `AWS IoT console`_.

#. Observe that the light bulb changes state.
   The local changes to the attributes always take precedence over what is set in the shadow's desired state.

.. note::
   The integration layer has built-in reconnection logic and tries to maintain the connection as long as the device is connected to the internet.
   The reconnection interval can be configured using the :kconfig:option:`CONFIG_AWS_IOT_RECONNECTION_INTERVAL_SECONDS` option.

User interface
**************

.. tabs::

   .. group-tab:: nRF52, nRF53 and nRF70 DKs

      LED 1:
         .. include:: /includes/matter_sample_state_led.txt

      LED 2:
         Shows the state of the light bulb.
         The following states are possible:

         * Solid On - The light bulb is on.
         * Off - The light bulb is off.

         Additionally, the LED starts blinking evenly (500 ms on/500 ms off) when the Identify command of the Identify cluster is received on the endpoint ``1``.
         The command's argument can be used to specify the duration of the effect.

      Button 1:
         .. include:: /includes/matter_sample_button.txt

      Button 2:
         Changes the light bulb state to the opposite one.

      .. include:: /includes/matter_segger_usb.txt

   .. group-tab:: nRF54 DKs

      LED 0:
         .. include:: /includes/matter_sample_state_led.txt

      LED 1:
         Shows the state of the light bulb.
         The following states are possible:

         * Solid On - The light bulb is on.
         * Off - The light bulb is off.

         Additionally, the LED starts blinking evenly (500 ms on/500 ms off) when the Identify command of the Identify cluster is received on the endpoint ``1``.
         The command's argument can be used to specify the duration of the effect.

      Button 0:
         .. include:: /includes/matter_sample_button.txt

      Button 1:
         Changes the light bulb state to the opposite one.

      .. include:: /includes/matter_segger_usb.txt

NFC port with antenna attached:
    Optionally used for obtaining the `Onboarding information`_ from the Matter accessory device to start the :ref:`commissioning procedure <matter_light_bulb_sample_remote_control_commissioning>`.

Building and running
********************

.. |sample path| replace:: :file:`samples/matter/light_bulb`

.. include:: /includes/build_and_run.txt

See `Configuration`_ for information about building the sample with the DFU support.

Selecting a custom configuration
================================

Before you start testing the application, you can select one of the :ref:`matter_light_bulb_custom_configs`.
See :ref:`app_build_file_suffixes` and :ref:`cmake_options` for more information how to select a configuration.

Testing
=======

After building the sample and programming it to your development kit, complete the following steps to test its basic features.

You can either test the sample's basic features or use the light switch sample to test the light bulb's :ref:`communication with another device <matter_light_bulb_sample_light_switch_tests>`.

.. _matter_light_bulb_sample_basic_features_tests:

Testing basic features
----------------------

After building the sample and programming it to your development kit, complete the following steps to test its basic features:

.. tabs::

   .. group-tab:: nRF52, nRF53 and nRF70 DKs

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

      #. Keep the **Button 1** pressed for more than six seconds to initiate factory reset of the device.

         The device reboots after all its settings are erased.

   .. group-tab:: nRF54 DKs

      #. |connect_kit|
      #. |connect_terminal_ANSI|
      #. Observe that **LED 1** is off.
      #. Press **Button 1** on the light bulb device.
         The **LED 1** turns on and the following messages appear on the console:

         .. code-block:: console

            I: Turn On Action has been initiated
            I: Turn On Action has been completed

      #. Press **Button 1** again.
         The **LED 1** turns off and the following messages appear on the console:

         .. code-block:: console

            I: Turn Off Action has been initiated
            I: Turn Off Action has been completed

      #. Keep the **Button 0** pressed for more than six seconds to initiate factory reset of the device.

         The device reboots after all its settings are erased.

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
The device becomes discoverable automatically upon the device startup, but only for a predefined period of time (1 hour by default).
If the Bluetooth LE advertising times out, enable it again.

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
