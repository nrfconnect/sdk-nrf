.. _matter_closure_sample:

Matter: Closure
###############

.. contents::
   :local:
   :depth: 2

This sample demonstrates the usage of the :ref:`Matter <ug_matter>` application layer to build a closure device.
This device works as a Matter accessory device, meaning it can be paired and controlled remotely over a Matter network built on top of a low-power 802.15.4 Thread network or on top of a Wi-Fi® network.
Additionally, this device works as a :ref:`Full Thread Device (FTD) <thread_ot_device_types>`.
Support for both Thread and Wi-Fi is mutually exclusive and depends on the hardware platform, so only one protocol can be supported for a specific closure device.
You can use this sample as a reference for creating your own application.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

If you want to commission the closure device and :ref:`control it remotely <matter_window_cover_network_mode>` through a Thread network, you need to set-up the :ref:`Thread Border Router <ug_thread_tools_tbr>` and control it with the `chip-tool`_.
Later support for this device might be added to ecosystems.
When this happens you will also be able to control it through a Matter controller device :ref:`configured on PC or smartphone <ug_matter_configuring>`.
This requires additional hardware depending on the setup you choose.

.. note::
    |matter_gn_required_note|

IPv6 network support
====================

The development kits for this sample offer the following IPv6 network support for Matter:

* Matter over Thread is supported for the ``nrf52840dk/nrf52840``, ``nrf5340dk/nrf5340/cpuapp``, ``nrf54l15dk/nrf54l15/cpuapp`` and ``nrf54lm20dk/nrf54lm20a/cpuapp`` board targets.
* Matter over Wi-Fi is supported for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target with the ``nrf7002eb2`` shield attached.

Overview
********

Closure is a Matter device type that represents a generic device capable of sealing an opening (such as a window, door, and similar structures).
After the device is commissioned, it operates as a Closure device and identifies itself as a garage door through its ``Descriptor`` cluster.

This sample implements the following features of the closure device:

* Positioning (allowing for setting one of the predefined positions)
* Speed (allowing for controlling the speed of the closure)

It implements the following attributes from the Closure Control cluster:

* ``CountdownTime`` (Time left before the movement is finished)
* ``MainState`` (The general state of the device for example Moving or Stopped)
* ``OverallCurrentState`` (The current state of the device including speed and position)
* ``OverallTargetState`` (The target state of the device including speed and position)

And allows for invoking the following commands:

* ``MoveTo`` (Set the desired position and speed)
* ``Stop`` (Stop the current movement of the closure)

See `User interface`_ for information about LEDs and buttons.

.. _matter_closure_network_mode:

Remote testing in a network
===========================

.. |Bluetoothsc| replace:: Bluetooth®
.. |WiFi| replace:: Wi-Fi

.. include:: ../light_bulb/README.rst
    :start-after: matter_light_bulb_sample_remote_testing_start
    :end-before: matter_light_bulb_sample_remote_testing_end

Configuration
*************

|config|

.. _matter_closure_custom_configs:

Matter Closure custom configurations
====================================

.. include:: ../light_bulb/README.rst
    :start-after: matter_light_bulb_sample_configuration_file_types_start
    :end-before: matter_light_bulb_sample_configuration_file_types_end

.. |Bluetooth| replace:: Bluetooth
.. include:: /includes/advanced_conf_matter.txt

Matter closure using only internal memory
=========================================

For the nRF54LM20 DK, you can configure the sample to use only the internal RRAM for storage.
It applies to the DFU as well, which means that both the currently running firmware and the new firmware to be updated will be stored within the device's internal RRAM memory.
See the `Upgrading the device firmware`_ section for information about the DFU process.

The DFU image can fit in the internal flash memory thanks to the usage of :ref:`MCUboot image compression<ug_matter_device_bootloader_image_compression>`.

This configuration is disabled by default for the Matter closure sample.
To enable it, set the ``FILE_SUFFIX`` CMake option to ``internal``.

The following is an example command to build the sample for the nRF54L15 DK with support for Matter OTA DFU and DFU over Bluetooth SMP, and using internal RRAM only:

.. code-block:: console

    west build -p -b nrf54lm20dk/nrf54lm20a/cpuapp -- -DCONFIG_CHIP_DFU_OVER_BT_SMP=y -DFILE_SUFFIX=internal

To build the sample for the same purpose, but in the ``release`` configuration, use the following command:

.. code-block:: console

    west build -p -b nrf54lm20dk/nrf54lm20a/cpuapp -- -DCONFIG_CHIP_DFU_OVER_BT_SMP=y -DFILE_SUFFIX=internal -Dclosure_EXTRA_CONF_FILE=prj_release.conf

In this case, the size of the MCUboot secondary partition used for storing the new application image is approximately 30-40% smaller than it would be when using a configuration with external flash memory support.

User interface
**************

.. tabs::

   .. group-tab:: nRF52, nRF53 DKs

      LED 1:
         .. include:: /includes/matter_sample_state_led.txt

      LED 2:
         Shows the current state of the closure.
         (Off means fully open while on means fully closed.
         The values in between are represented by dimmed light.)

      Button 1:
         .. include:: /includes/matter_sample_button.txt

      .. include:: /includes/matter_segger_usb.txt

   .. group-tab:: nRF54 DKs

      LED 0:
         .. include:: /includes/matter_sample_state_led.txt

      LED 1:
         Shows the current state of the closure.
         (Off means fully open while on means fully closed.
         The values in between are represented by dimmed light.)

      Button 0:
         .. include:: /includes/matter_sample_button.txt

      .. include:: /includes/matter_segger_usb.txt


Building and running
********************

.. |sample path| replace:: :file:`samples/matter/closure`

.. include:: /includes/build_and_run.txt

.. |sample_or_app| replace:: sample
.. |ipc_radio_dir| replace:: :file:`sysbuild/ipc_radio`

.. include:: /includes/ipc_radio_conf.txt

Building the Matter over Wi-Fi sample variant on nRF54LM20 DK with nRF7002-EB II shield
=======================================================================================

.. include:: /includes/matter_building_nrf54lm20dk_7002eb2

Flashing the Matter over Wi-Fi sample variant
=============================================

.. include:: /includes/matter_sample_wifi_flash.txt

Selecting a configuration
=========================

Before you start testing the application, you can select one of the :ref:`matter_closure_custom_configs`.
See :ref:`app_build_file_suffixes` and :ref:`cmake_options` for more information how to select a configuration.

Testing
=======

.. tabs::

   .. group-tab:: nRF52, nRF53 DKs

      When you have built the sample and programmed it to your development kit, it automatically starts the Bluetooth LE advertising and the **LED 1** starts flashing (Short Flash On).
      At this point, you can press **Button 1** for six seconds to initiate the factory reset of the device.

   .. group-tab:: nRF54 DKs

      When you have built the sample and programmed it to your development kit, it automatically starts the Bluetooth LE advertising and the **LED 0** starts flashing (Short Flash On).
      At this point, you can press **Button 0** for six seconds to initiate the factory reset of the device.

.. _matter_closure_network_testing:

Testing in a network
--------------------

To test the sample in a Matter-enabled Thread network, complete the following steps:

.. note::
   |matter_unique_discriminator_note|

.. tabs::

   .. group-tab:: nRF52, nRF53 DKs

      1. |connect_kit|
      #. |connect_terminal_ANSI|
      #. If the device was not erased during the programming, press and hold **Button 1** until the factory reset takes place.
      #. Commission the device to the Matter network.
         See `Commissioning the device`_ for more information.
      #. Subscribe to the ``MovementCompleted`` event with the following command with the *<node_id>* and *<endpoint_id>* replaced with values selected by you (for example, ``1`` and ``1``):

         .. code-block:: console

            ./chip-tool closurecontrol subscribe-event movement-completed 1 5 <node_id> <endpoint_id>

      #. Set the target position of the closure with the following command:

         .. code-block:: console

            ./chip-tool closurecontrol move-to <node_id> <endpoint_id> --Position <position> --Speed <speed> --timedInteractionTimeoutMs 5000

         Where:

            * *<position>* is of type ``CurrentPositionEnum`` (integer from 0 to 5 ``0`` for ``FullyClosed``, ``1`` for ``FullyOpened``)
            * *<speed>* is of type ``ThreeLevelAutoEnum`` (``0`` for auto, ``1`` to ``3`` for ``Low`` to ``High``)

         **LED 1** will start to glow brighter and brighter, indicating the closing of the closure.
      #. Wait for the closure to finish its movement.
         You should be notified with the ``MovementCompleted`` event after the movement is complete.

      #. Test the remaining functionalities in a similar manner.

   .. group-tab:: nRF54 DKs

      2. |connect_kit|
      #. |connect_terminal_ANSI|
      #. If the device was not erased during the programming, press and hold **Button 0** until the factory reset takes place.
      #. Commission the device to the Matter network.
         See `Commissioning the device`_ for more information.
      #. Subscribe to the ``MovementCompleted`` event with the following command with the *<node_id>* and *<endpoint_id>* replaced with values selected by you (for example, ``1`` and ``1``):

         .. code-block:: console

            ./chip-tool closurecontrol subscribe-event movement-completed 1 5 <node_id> <endpoint_id>

      #. Set the target position of the closure with the following command:

         .. code-block:: console

            ./chip-tool closurecontrol move-to <node_id> <endpoint_id> --Position <position> --Speed <speed> --timedInteractionTimeoutMs 5000

         Where:

            * *<position>* is of type ``CurrentPositionEnum`` (integer from 0 to 5 ``0`` for ``FullyClosed``, ``1`` for ``FullyOpened``)
            * *<speed>* is of type ``ThreeLevelAutoEnum`` (``0`` for auto, ``1`` to ``3`` for ``Low`` to ``High``)

         **LED 0** will start to glow brighter and brighter, indicating the closing of the closure.
      #. Wait for the closure to finish its movement.
         You should be notified with the ``MovementCompleted`` event after the movement is complete.

      #. Test the remaining functionalities in a similar meaner.

After commissioning the Device you can test the following functionalities:

*  Read current state of the device by reading the attributes ``MainState``, ``OverallCurrentState``, and ``OverallTargetState``.
*  Open or close the closure by invoking the ``MoveTo`` and ``Stop`` commands.
*  Get the estimated movement time by reading the ``CountdownTime`` attribute.
*  Subscribe to the attributes or ``MovementComplete`` event

Enabling remote control
=======================

Remote control allows you to control the Matter closure device from an IPv6 network.

`Commissioning the device`_ allows you to set up a testing environment and remotely control the sample over a Matter-enabled Thread network.

.. _matter_closure_sample_remote_control_commissioning:

Commissioning the device
------------------------

.. include:: ../light_bulb/README.rst
    :start-after: matter_light_bulb_sample_commissioning_start
    :end-before: matter_light_bulb_sample_commissioning_end

Before starting the commissioning procedure, the device must be made discoverable over Bluetooth LE.
The device becomes discoverable automatically upon the device startup, but only for a predefined period of time (1 hour by default).
If the Bluetooth LE advertising times out, enable it again.

.. _matter_closure_network_mode_onboarding:

Onboarding information
++++++++++++++++++++++

When you start the commissioning procedure, the controller must get the onboarding information from the Matter accessory device.
The onboarding information representation depends on your commissioner setup.

For this sample, you can use one of the following :ref:`onboarding information formats <ug_matter_network_topologies_commissioning_onboarding_formats>` to provide the commissioner with the data payload that includes the device discriminator and the setup PIN code:

  .. list-table:: Closure sample onboarding information
     :header-rows: 1

     * - QR Code
       - QR Code Payload
       - Manual pairing code
     * - Scan the following QR code with the app for your ecosystem:

         .. figure:: ../../../doc/nrf/images/matter_qr_code_closure.png
            :width: 200px
            :alt: QR code for commissioning the Closure device

       - MT:Y.K9042C00KA0648G00
       - 34970112332

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_onboarding_start
    :end-before: matter_door_lock_sample_onboarding_end

|matter_cd_info_note_for_samples|

Upgrading the device firmware
=============================

To upgrade the device firmware, complete the steps listed for the selected method in the :doc:`matter:nrfconnect_examples_software_update` tutorial of the Matter documentation.

Dependencies
************

This sample uses the Matter library that includes the |NCS| platform integration layer:

* `Matter`_

In addition, the sample uses the following |NCS| components:

* :ref:`dk_buttons_and_leds_readme`

The sample depends on the following Zephyr libraries:

* :ref:`zephyr:logging_api`
* :ref:`zephyr:kernel_api`
