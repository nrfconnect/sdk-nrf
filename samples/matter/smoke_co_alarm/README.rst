.. _matter_smoke_co_alarm_sample:

Matter: Smoke CO Alarm
######################

.. contents::
   :local:
   :depth: 2

This sample demonstrates the usage of the :ref:`Matter <ug_matter>` application layer to build a device capable of sensing smoke and carbon monoxide, and issuing an alarm if the their concentration is too high.
This device works as a Matter accessory device, meaning it can be paired and controlled remotely over a Matter network built on top of a low-power, 802.15.4 Thread network.
You can use this sample as a reference for creating your own application.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

If you want to commission the device and :ref:`control it remotely <matter_smoke_co_alarm_network_mode>`, you also need a Matter controller device :ref:`configured on PC or mobile <ug_matter_configuring>`.
This requires additional hardware depending on the setup you choose.

.. note::
    |matter_gn_required_note|

Overview
********

The sample does not use real hardware smoke or CO sensors due to hardware limitations.
Instead, it uses :ref:`test event triggers <ug_matter_test_event_triggers>` to test issuing the device alarms, and LEDs to show the current device state.
The smoke CO alarm detects the alarm conditions and issues the appropriate alarm using different LED patterns.

.. note::
   According to the Matter specification requirements, the smoke CO alarm device shall express alarms using both audible and visual indications.
   This sample does not use audible indications because of the lack of suitable hardware on supported development kits.

In case multiple alarm conditions are met, the device expresses only the alarm with the highest priority.
After the alarm with the highest priority is stopped, the device starts expressing the next alarm in the priority order until all alarm conditions have ceased.
The implementation demonstrated in this sample supports issuing the following alarms, listed from the highest priority to the lowest:

* Smoke alarm - Issued if allowed smoke concentration level is exceeded.
* CO alarm - Issued if allowed carbon monoxide concentration level is exceeded.
* Hardware fault alert - Issued if the device hardware is not operating properly.
* Device self-test alert - Issued if the device self-test was started by the user.
* End of service alert - Issued if the device service was ended either by the expiration date or other physical conditions, and it needs to be replaced.
* Battery level alert - Issued if the device battery level is too low.

You can test the device remotely over a Thread network, which requires more devices.

The remote control testing requires a Matter controller that you can configure either on a PC or a mobile device.
You can enable both methods after :ref:`building and running the sample <matter_smoke_co_alarm_sample_remote_control>`.

.. _matter_smoke_co_alarm_sample_lit:

ICD LIT device type
===================

The smoke CO alarm works as a Matter Intermittently Connected Device (ICD) with a :ref:`Long Idle Time (LIT)<ug_matter_device_low_power_icd_sit_lit>`.
The device starts operation in the Short Idle Time (SIT) mode and remains in it until it is commissioned to the Matter fabric and registers the first ICD client.
It then switches the operation mode to LIT to reduce the power consumption.

In the LIT mode, the device responsiveness is much lower than in the SIT mode.
However, you can request the device to become responsive to, for example, change its configuration.
To do that, you need to use the User Active Mode Trigger (UAT) feature by pressing the appropriate button.

See `User interface`_ for information about how to switch the operation modes.

.. _matter_smoke_co_alarm_network_mode:

Remote testing in a network
===========================

.. include:: ../light_bulb/README.rst
    :start-after: matter_light_bulb_sample_remote_testing_start
    :end-before: matter_light_bulb_sample_remote_testing_end

Configuration
*************

|config|

.. _matter_smoke_co_alarm_custom_configs:

Matter smoke CO alarm custom configurations
===========================================

.. include:: ../light_bulb/README.rst
    :start-after: matter_light_bulb_sample_configuration_file_types_start
    :end-before: matter_light_bulb_sample_configuration_file_types_end

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

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      LED 1:
         .. include:: /includes/matter_sample_state_led.txt

      LED 2:
         Shows the state of the alarm.
         The following states are possible:

         * Even Flashing (300 ms on/300 ms off) - The smoke alarm is expressed.
         * Even Flashing (500 ms on/500 ms off) - The Identify command of the Identify cluster is received on the endpoint ``1``.
           The command's argument can be used to specify the duration of the effect.
         * Flashing in combination with **LED3** and **LED4** when other alarms are triggered.

      LED 3:
         Shows the state of the alarm.
         The following states are possible:

         * Even Flashing (300 ms on/300 ms off) - The CO alarm is expressed.
         * Flashing in combination with **LED2** and **LED4** when other alarms are triggered.

      LED 4:
         Shows the state of the alarm.
         The following states are possible:

         * Even Flashing (300 ms on/300 ms off) - The battery level low alarm is expressed.
         * Flashing in combination with **LED2** and **LED3** when other alarms are triggered.

      LED 2, LED 3 and LED 4 combined:
         Shows the state of the alarm.
         The following states are possible:

         * All three LEDs, Short Flash On (300 ms on/700 ms off) - The hardware fault alarm is expressed.
         * All three LEDs, Long Flash On (700 ms on/300 ms off) - The end of service alarm is expressed.
         * Flashing In Sequence from **LED2**, through **LED3** to **LED4** (200 ms interval) - The self-test is triggered.

      Button 1:
         .. include:: /includes/matter_sample_button.txt

      Button 3:
         Functions as the User Active Mode Trigger (UAT) button.
         For more information about Intermittently Connected Devices (ICD) and User Active Mode Trigger, see the :ref:`ug_matter_device_low_power_icd` documentation section.

      .. include:: /includes/matter_segger_usb.txt

   .. group-tab:: nRF54 DKs

      LED 0:
         .. include:: /includes/matter_sample_state_led.txt

      LED 1:
         Shows the state of the alarm.
         The following states are possible:

         * Even Flashing (300 ms on/300 ms off) - The smoke alarm is expressed.
         * Even Flashing (500 ms on/500 ms off) - The Identify command of the Identify cluster is received on the endpoint ``1``.
           The command's argument can be used to specify the duration of the effect.
         * Flashing in combination with **LED2** and **LED3** when other alarms are triggered.

      LED 2:
         Shows the state of the alarm.
         The following states are possible:

         * Even Flashing (300 ms on/300 ms off) - The CO alarm is expressed.
         * Flashing in combination with **LED1** and **LED3** when other alarms are triggered.

      LED 3:
         Shows the state of the alarm.
         The following states are possible:

         * Even Flashing (300 ms on/300 ms off) - The battery level low alarm is expressed.
         * Flashing in combination with **LED1** and **LED2** when other alarms are triggered.

      LED 1, LED 2 and LED 3 combined:
         Shows the state of the alarm.
         The following states are possible:

         * All three LEDs, Short Flash On (300 ms on/700 ms off) - The hardware fault alarm is expressed.
         * All three LEDs, Long Flash On (700 ms on/300 ms off) - The end of service alarm is expressed.
         * Flashing In Sequence from **LED1**, through **LED2** to **LED3** (200 ms interval) - The self-test is triggered.

      Button 0:
         .. include:: /includes/matter_sample_button.txt

      Button 2:
         Functions as the User Active Mode Trigger (UAT) button.
         For more information about Intermittently Connected Devices (ICD) and User Active Mode Trigger, see the :ref:`ug_matter_device_low_power_icd` documentation section.

      .. include:: /includes/matter_segger_usb.txt

NFC port with antenna attached:
    Optionally used for obtaining the `Onboarding information`_ from the Matter accessory device to start the :ref:`commissioning procedure <matter_light_bulb_sample_remote_control_commissioning>`.

Building and running
********************

.. |sample path| replace:: :file:`samples/matter/smoke_co_alarm`

.. include:: /includes/build_and_run.txt

See `Configuration`_ for information about building the sample with the DFU support.

Selecting a custom configuration
================================

Before you start testing the application, you can select one of the :ref:`matter_smoke_co_alarm_custom_configs`.
See :ref:`app_build_file_suffixes` and :ref:`cmake_options` for more information how to select a configuration.

Testing
=======

After building the sample and programming it to your development kit, complete the following steps to test its basic features.

.. note::
   The following steps use the CHIP Tool controller as an example.
   Visit the :ref:`test event triggers <ug_matter_test_event_triggers>` page to see the code values used for triggering specific alarms.

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      #. |connect_kit|
      #. |connect_terminal_ANSI|
      #. If the device was not erased during the programming, press and hold **Button 1** until the factory reset takes place.
      #. Commission the device to the Matter network.
         See `Commissioning the device`_ for more information.
      #. Observe that **LED 2**, **LED 3** and **LED 4** are turned off, which means that the device does not express any alarm.
      #. Trigger the device self-test by invoking the following command with the *<node_id>* and *<endpoint_id>* replaced with values selected by you (for example, ``1`` and ``1``):

         .. code-block:: console

            ./chip-tool smokecoalarm self-test-request <node_id> <endpoint_id>

         The **LED2**, **LED3** and **LED4** will sequentially flash for 5 seconds.

      #. Test the Smoke alarm by invoking triggers from the General Diagnostics cluster.
         Replace the *<test_event_enable_key>* and *<node_id>* arguments in the presented commands with values selected by you (for example, ``00112233445566778899AABBCCDDEEFF`` and ``1``):
         Trigger the Smoke alarm by invoking the following command:

         .. code-block:: console

            generaldiagnostics test-event-trigger hex:<test_event_enable_key> 0x005c00000000009c <node_id> 0

         The **LED2** will start blinking evenly with 300 ms interval.

      #. Trigger the CO alarm by invoking the following command with the *<test_event_enable_key>* and *<node_id>* replaced with the values you selected in the previous steps:

         .. code-block:: console

            generaldiagnostics test-event-trigger hex:<test_event_enable_key> 0x005c00000000009d <node_id> 0

         Nothing will change, because the CO alarm has lower priority than smoke alarm, so it will not be expressed.

      #. Stop the Smoke alarm by invoking the following command with the *<test_event_enable_key>* and *<node_id>* replaced with the values you selected in the previous steps:

         .. code-block:: console

            generaldiagnostics test-event-trigger hex:<test_event_enable_key> 0x005c0000000000a0 <node_id> 0

         The **LED2** will be turned off and the **LED3** will start blinking evenly with 300 ms interval, to express the CO alarm, as the next one in the order.

   .. group-tab:: nRF54 DKs

      #. |connect_kit|
      #. |connect_terminal_ANSI|
      #. If the device was not erased during the programming, press and hold **Button 0** until the factory reset takes place.
      #. Commission the device to the Matter network.
         See `Commissioning the device`_ for more information.
      #. Observe that **LED 1**, **LED 2** and **LED 3** are turned off, which means that the device does not express any alarm.
      #. Trigger the device self-test by invoking the following command with the *<node_id>* and *<endpoint_id>* replaced with values selected by you (for example, ``1`` and ``1``):

         .. code-block:: console

            ./chip-tool smokecoalarm self-test-request <node_id> <endpoint_id>

         The **LED1**, **LED2** and **LED3** will sequentially flash for 5 seconds.

      #. Test the Smoke alarm by invoking triggers from the General Diagnostics cluster.
         Replace the *<test_event_enable_key>* and *<node_id>* arguments in the presented commands with values selected by you (for example, ``00112233445566778899AABBCCDDEEFF`` and ``1``):
         Trigger the Smoke alarm by invoking the following command:

         .. code-block:: console

            generaldiagnostics test-event-trigger hex:<test_event_enable_key> 0x005c00000000009c <node_id> 0

         The **LED1** will start blinking evenly with 300 ms interval.

      #. Trigger the CO alarm by invoking the following command with the *<test_event_enable_key>* and *<node_id>* replaced with the values you selected in the previous steps:

         .. code-block:: console

            generaldiagnostics test-event-trigger hex:<test_event_enable_key> 0x005c00000000009d <node_id> 0

         Nothing will change, because the CO alarm has lower priority than smoke alarm, so it will not be expressed.

      #. Stop the Smoke alarm by invoking the following command with the *<test_event_enable_key>* and *<node_id>* replaced with the values you selected in the previous steps:

         .. code-block:: console

            generaldiagnostics test-event-trigger hex:<test_event_enable_key> 0x005c0000000000a0 <node_id> 0

         The **LED1** will be turned off and the **LED2** will start blinking evenly with 300 ms interval, to express the CO alarm, as the next one in the order.

.. _matter_smoke_co_alarm_sample_remote_control:

Enabling remote control
=======================

Remote control allows you to control the Matter smoke CO alarm device from an IPv6 network.

`Commissioning the device`_ allows you to set up a testing environment and remotely control the sample over a Matter-enabled Thread network.

.. _matter_smoke_co_alarm_sample_remote_control_commissioning:

Commissioning the device
------------------------

.. include:: ../light_bulb/README.rst
    :start-after: matter_light_bulb_sample_commissioning_start
    :end-before: matter_light_bulb_sample_commissioning_end

Before starting the commissioning procedure, the device must be made discoverable over Bluetooth LE.
The device becomes discoverable automatically upon the device startup, but only for a predefined period of time (1 hour by default).
If the Bluetooth LE advertising times out, enable it again.

Onboarding information
++++++++++++++++++++++

When you start the commissioning procedure, the controller must get the onboarding information from the Matter accessory device.
The onboarding information representation depends on your commissioner setup.

For this sample, you can use one of the following :ref:`onboarding information formats <ug_matter_network_topologies_commissioning_onboarding_formats>` to provide the commissioner with the data payload that includes the device discriminator and the setup PIN code:

  .. list-table:: Smoke CO alarm sample onboarding information
     :header-rows: 1

     * - QR Code
       - QR Code Payload
       - Manual pairing code
     * - Scan the following QR code with the app for your ecosystem:

         .. figure:: ../../../doc/nrf/images/matter_qr_code_smoke_co_alarm.png
            :width: 200px
            :alt: QR code for commissioning the smoke co alarm device

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
