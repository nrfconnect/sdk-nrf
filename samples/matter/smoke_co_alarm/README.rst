.. |matter_name| replace:: Smoke CO Alarm
.. |matter_type| replace:: sample
.. |matter_dks_thread| replace:: ``nrf52840dk/nrf52840``, ``nrf5340dk/nrf5340/cpuapp``, ``nrf54l15dk/nrf54l15/cpuapp``, and ``nrf54lm20dk/nrf54lm20a/cpuapp`` board targets
.. |matter_dks_internal| replace:: nRF54LM20 DK
.. |sample path| replace:: :file:`samples/matter/smoke_co_alarm`
.. |matter_qr_code_payload| replace:: MT:Y.K9042C00KA0648G00
.. |matter_pairing_code| replace:: 34970112332
.. |matter_qr_code_image| image:: /images/matter_qr_code_smoke_co_alarm.png
                          :width: 200px
                          :alt: QR code for commissioning the smoke CO alarm device

.. include:: /includes/matter/shortcuts.txt

.. _matter_smoke_co_alarm_sample:

Matter: Smoke CO Alarm
######################

.. contents::
   :local:
   :depth: 2

This sample demonstrates the usage of the :ref:`Matter <ug_matter>` application layer to build a device capable of sensing smoke and carbon monoxide, and issuing an alarm if the their concentration is too high.

.. include:: /includes/matter/introduction/sleep_thread_sed_lit.txt

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/matter/requirements/thread.txt

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

The sample implements two instances of a Power Source cluster:

* Wired power source on the endpoint 0
* Battery power source on the endpoint 1

The usage of power sources is implemented with a preference to select wired power source and switch to battery power source, only if the wired one is not available.
The power source changes are emulated using :ref:`test event triggers <ug_matter_test_event_triggers>`.
Every power source can be independently enabled or disabled using a dedicated test event trigger.

You can test the device remotely over a Thread network, which requires more devices.

The remote control testing requires a Matter controller that you can configure either on a PC or a mobile device.

Smoke CO Alarm features
=======================

The smoke CO alarm sample implements the following features:

* Smoke alarm - Issued if allowed smoke concentration level is exceeded.
* CO alarm - Issued if allowed carbon monoxide concentration level is exceeded.
* Hardware fault alert - Issued if the device hardware is not operating properly.
* Device self-test alert - Issued if the device self-test was started by the user.
* End of service alert - Issued if the device service was ended either by the expiration date or other physical conditions, and it needs to be replaced.
* Battery level alert - Issued if the device battery level is too low.
* :ref:`ICD LIT <matter_smoke_co_alarm_sample_lit>` - The smoke CO alarm can be used as an Intermittently Connected Device (ICD) with a :ref:`Long Idle Time (LIT)<ug_matter_device_low_power_icd_sit_lit>`.

Use the ``click to show`` toggle to expand the content.

.. _matter_smoke_co_alarm_sample_lit:

.. include:: /includes/matter/overview/icd_lit.txt

Configuration
*************

.. include:: /includes/matter/configuration/intro.txt

The |matter_type| supports the following build configurations:

.. include:: /includes/matter/configuration/basic_internal.txt

Advanced configuration options
==============================

.. include:: /includes/matter/configuration/advanced/intro.txt
.. include:: /includes/matter/configuration/advanced/dfu.txt
.. include:: /includes/matter/configuration/advanced/factory_data.txt
.. include:: /includes/matter/configuration/advanced/custom_board.txt
.. include:: /includes/matter/configuration/advanced/internal_memory.txt

User interface
**************

.. include:: /includes/matter/interface/intro.txt

First LED:
   .. include:: /includes/matter/interface/state_led.txt

Second LED:
   Shows the state of the alarm.
   The following states are possible:

   * Even Flashing (300 ms on/300 ms off) - The smoke alarm is expressed.
   * Even Flashing (500 ms on/500 ms off) - The Identify command of the Identify cluster is received on the endpoint ``1``.
      The command's argument can be used to specify the duration of the effect.
   * Flashing in combination with |Third LED| and |Fourth LED| when other alarms are triggered.

Third LED:
   Shows the state of the alarm.
   The following states are possible:

   * Even Flashing (300 ms on/300 ms off) - The CO alarm is expressed.
   * Flashing in combination with |Second LED| and |Fourth LED| when other alarms are triggered.

Fourth LED:
   Shows the state of the alarm.
   The following states are possible:

   * Even Flashing (300 ms on/300 ms off) - The battery level low alarm is expressed.
   * Flashing in combination with |Second LED| and |Third LED| when other alarms are triggered.

Second, third and fourth LEDs combined:
   Shows the state of the alarm.
   The following states are possible:

   * All three LEDs, Short Flash On (300 ms on/700 ms off) - The hardware fault alarm is expressed.
   * All three LEDs, Long Flash On (700 ms on/300 ms off) - The end of service alarm is expressed.
   * Flashing In Sequence from |Second LED|, through |Third LED| to |Fourth LED| (200 ms interval) - The self-test is triggered.

First Button:
   .. include:: /includes/matter/interface/main_button.txt

Third Button:
   Functions as the User Active Mode Trigger (UAT) button.
   For more information about Intermittently Connected Devices (ICD) and User Active Mode Trigger, see the :ref:`ug_matter_device_low_power_icd` documentation section.

.. include:: /includes/matter/interface/segger_usb.txt
.. include:: /includes/matter/interface/nfc.txt

Building and running
********************

.. include:: /includes/matter/building_and_running/intro.txt

|matter_ble_advertising_auto|

Testing
*******

.. include:: /includes/matter/testing/intro.txt

Testing with CHIP Tool
======================

Complete the following steps to test the |matter_name| device using CHIP Tool:

.. |node_id| replace:: 1

.. include:: /includes/matter/testing/1_prepare_matter_network_thread.txt
.. include:: /includes/matter/testing/2_prepare_dk.txt
.. include:: /includes/matter/testing/3_commission_thread.txt

.. rst-class:: numbered-step

Observe the initial state
-------------------------

Observe that |Second LED|, |Third LED| and |Fourth LED| are turned off, which means that the device does not express any alarm.

.. rst-class:: numbered-step

Trigger the device self-test
----------------------------

In the interactive mode, trigger the device self-test by running the following command:

.. parsed-literal::
   :class: highlight

   chip-tool smokecoalarm self-test-request |node_id| 1

The |Second LED|, |Third LED| and |Fourth LED| will sequentially flash for 5 seconds.

.. rst-class:: numbered-step

Trigger the Smoke alarm
-----------------------

Replace the *<test_event_enable_key>* argument in the presented command with the value selected by you (by default, the enable key value is ``00112233445566778899AABBCCDDEEFF``):

.. parsed-literal::
   :class: highlight

   generaldiagnostics test-event-trigger hex:*<test_event_enable_key>* 0x005c00000000009c |node_id| 0

The |Second LED| will start blinking evenly with 300 ms interval.

.. rst-class:: numbered-step

Trigger the CO alarm
--------------------

Replace the *<test_event_enable_key>* argument in the presented command with the value selected by you (by default, the enable key value is ``00112233445566778899AABBCCDDEEFF``):

.. parsed-literal::
   :class: highlight

   generaldiagnostics test-event-trigger hex:*<test_event_enable_key>* 0x005c00000000009d |node_id| 0

Nothing will change, because the CO alarm has lower priority than smoke alarm, so it will not be expressed.

.. rst-class:: numbered-step

Stop the Smoke alarm
--------------------

In the interactive mode, stop the Smoke alarm by running the following command, replacing the *<test_event_enable_key>* argument with the value selected by you (by default, the enable key value is ``00112233445566778899AABBCCDDEEFF``):

.. parsed-literal::
   :class: highlight

   generaldiagnostics test-event-trigger hex:*<test_event_enable_key>* 0x005c0000000000a0 |node_id| 0

The |Second LED| will be turned off and the |Third LED| will start blinking evenly with 300 ms interval, to express the CO alarm, as the next one in the order.

Testing with commercial ecosystem
=================================

.. include:: /includes/matter/testing/ecosystem.txt

Dependencies
************

.. include:: /includes/matter/dependencies.txt
