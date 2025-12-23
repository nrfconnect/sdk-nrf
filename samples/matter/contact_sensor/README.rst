.. |matter_name| replace:: Contact sensor
.. |matter_type| replace:: sample
.. |matter_dks_thread| replace:: ``nrf52840dk/nrf52840``, ``nrf5340dk/nrf5340/cpuapp``, ``nrf54l15dk/nrf54l15/cpuapp``, and ``nrf54lm20dk/nrf54lm20a/cpuapp`` board targets
.. |matter_dks_internal| replace:: nRF54LM20 DK
.. |sample path| replace:: :file:`samples/matter/contact_sensor`
.. |matter_qr_code_payload| replace:: MT:Y.K9042C00KA0648G00
.. |matter_pairing_code| replace:: 34970112332
.. |matter_qr_code_image| image:: /images/matter_qr_code_contact_sensor.png

.. include:: /includes/matter/shortcuts.txt

.. _matter_contact_sensor_sample:

Matter: Contact sensor
######################

.. contents::
   :local:
   :depth: 2

This |matter_type| demonstrates how to use the :ref:`Matter <ug_matter>` application layer to build a device capable of detecting a contact.

.. include:: /includes/matter/introduction/sleep_thread_sed_lit.txt

Requirements
************

The |matter_type| supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/matter/requirements/thread.txt

Overview
********

The |matter_type| does not use a real contact sensor due to a hardware limitation.
It simulates contact detection, which you can initiate by pressing and releasing a button.
The contact sensor state is represented by the BooleanState cluster on the endpoint ``1``.

You can test the device remotely over a Thread network, which requires more devices.

The remote control testing requires a Matter controller that you can configure either on a PC or a mobile device.
You can enable both methods after `Commissioning the device`_ to the Matter network.

.. include:: /includes/matter/overview/matter_quick_start.txt

Contact sensor features
=======================

The |matter_name| implements the following features.
Click the ``click to show`` toggle to expand the content.

.. _matter_contact_sensor_sample_lit:

.. include:: /includes/matter/overview/icd_lit.txt

.. _matter_contact_sensor_custom_configs:

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
   Shows the state of the contact detection.
   The following states are possible:

   * Solid On - The contact is detected.
   * Off - The contact is not detected.

   Additionally, the LED starts blinking evenly (500 ms on/500 ms off) when the ``Identify`` command of the Identify cluster is received on the endpoint ``1``.
   You can specify the duration of the effect using the command arguments.

First Button:
   .. include:: /includes/matter/interface/main_button.txt

Second Button:
   Simulates contact detection.
   Pressing the button sets the contact state to detected.
   Releasing the button sets the contact state to not detected.

Third Button:
   Functions as the User Active Mode Trigger (UAT) button.
   For more information about Intermittently Connected Devices (ICD) and User Active Mode Trigger, see the :ref:`ug_matter_device_low_power_icd` documentation section.

.. include:: /includes/matter/interface/segger_usb.txt
.. include:: /includes/matter/interface/nfc.txt

Building and running
********************

.. include:: /includes/matter/building_and_running/intro.txt
.. include:: /includes/matter/building_and_running/select_configuration.txt
.. include:: /includes/matter/building_and_running/commissioning.txt

|matter_ble_advertising_auto|

.. include:: /includes/matter/building_and_running/onboarding.txt

Testing
*******

.. |endpoint_name| replace:: **Boolean State cluster**
.. |endpoint_id| replace:: **1**

.. include:: /includes/matter/testing/intro.txt
.. include:: /includes/matter/testing/prerequisites.txt
.. include:: /includes/matter/testing/prepare.txt

Testing steps
=============

1. Enable CHIP Tool interactive mode by running the following command:

   .. code-block:: console

      ./chip-tool interactive start

#. Using the interactive mode, read the contact detection state by invoking the following command:

   .. parsed-literal::
      :class: highlight

      booleanstate read state-value <node_id> 1

   The received output will look similar to the following:

   .. code-block:: console

      [1756375237.321] [61118:61120] [TOO] Endpoint: 1 Cluster: 0x0000_0045 Attribute 0x0000_0000 DataVersion: 2772696482
      [1756375237.321] [61118:61120] [TOO]   StateValue: FALSE


#. Press and hold the |Second Button| to simulate contact detection.
   Read the contact detection state again using the same command as before (do not release the button):

   .. parsed-literal::
      :class: highlight

      booleanstate read state-value <node_id> 1

   The received value will be different, for example:

   .. code-block:: console

      [1756375237.321] [61118:61120] [TOO] Endpoint: 1 Cluster: 0x0000_0045 Attribute 0x0000_0000 DataVersion: 2772696482
      [1756375237.321] [61118:61120] [TOO]   StateValue: TRUE

#. Subscribe to the contact detection state changes to see the state change in real time:

   .. parsed-literal::
      :class: highlight

      booleanstate subscribe state-value 0 300 <node_id> 1

#. Press and release the |Second Button| a few times and observe the state changes reported automatically in the CHIP Tool interactive mode.

Factory reset
=============

|matter_factory_reset|

Dependencies
************

.. include:: /includes/matter/dependencies.txt
