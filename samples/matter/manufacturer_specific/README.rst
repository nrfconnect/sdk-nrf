.. |matter_name| replace:: Manufacturer-specific
.. |matter_type| replace:: sample
.. |matter_dks_thread| replace:: ``nrf52840dk/nrf52840``, ``nrf5340dk/nrf5340/cpuapp``, ``nrf54l15dk/nrf54l15/cpuapp``, and ``nrf54lm20dk/nrf54lm20a/cpuapp`` board targets
.. |matter_dks_wifi| replace:: ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target with the ``nrf7002eb2`` shield attached
.. |matter_dks_internal| replace:: nRF54LM20 DK
.. |matter_dks_tfm| replace:: ``nrf54l15dk/nrf54l15/cpuapp`` board target
.. |sample path| replace:: :file:`samples/matter/manufacturer_specific`
.. |matter_qr_code_payload| replace:: MT:K.K9042C00KA0648G00
.. |matter_pairing_code| replace:: 34970112332
.. |matter_qr_code_image| image:: /images/matter_qr_code_template_sample.png

.. include:: /includes/matter/shortcuts.txt

.. _matter_manufacturer_specific_sample:

Matter: Manufacturer-specific
#############################

.. contents::
   :local:
   :depth: 2

This sample demonstrates an implementation of custom manufacturer-specific clusters used by the application layer.
This sample uses development kit's buttons and LEDs to demonstrate the functionality of the custom ``NordicDevkit`` cluster.

.. include:: /includes/matter/introduction/no_sleep_thread_mtd_wifi.txt

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/matter/requirements/thread_wifi.txt

Overview
********

.. tabs::

   .. group-tab:: nRF52, nRF53 and nRF70 DKs

      **Button 2** is used to set the state of the ``NordicDevkit`` cluster's attribute, ``UserButton``.
      **LED 2** reflects the state of the ``UserLED``.

   .. group-tab:: nRF54 DKs

      **Button 1** is used to set the state of the ``NordicDevkit`` cluster's attribute, ``UserButton``.
      **LED 1** reflects the state of the ``UserLED``.

The Matter command ``SetLED`` is used to control the state of ``UserLED``.
It takes one argument - the action to be performed (``0`` to turn the LED off, ``1`` to turn it on, ``2`` to toggle the state).
The ``UserLED`` attribute is persistent and stored across the reboots.
The ``UserButtonChanged`` event is generated when the ``UserButton`` attribute is changed.

The ``NordicDevkit`` cluster introduces a writable ``DevKitName`` attribute, of string type as well.
The ``DevKitName`` attribute is persistent and stored across the reboots.

The sample additionally extends the ``Basic Information`` cluster with a ``RandomNumber`` attribute and ``GenerateRandom`` command that updates the ``RandomNumber`` with a random value.
The ``RandomNumberChanged`` event is generated when the ``RandomNumber`` attribute is changed.
The ``RandomNumber`` attribute value is not persistent and it is generated on each application's boot.

Custom manufacturer-specific cluster
====================================

The sample provides a custom ``NordicDevkit`` cluster, configured in the :file:`src/default_zap/NordicDevKitCluster.xml` file.
To learn more about adding custom clusters to your Matter application, see the :ref:`ug_matter_creating_custom_cluster` section.

Configuration
*************

.. include:: /includes/matter/configuration/intro.txt

The |matter_type| supports the following build configurations:

.. include:: /includes/matter/configuration/basic_internal.txt

Advanced configuration options
==============================

.. include:: /includes/matter/configuration/advanced/intro.txt
.. include:: /includes/matter/configuration/advanced/tfm.txt
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
   Reflects the state of ``UserLED`` attribute in the ``NordicDevkit`` cluster.

First Button:
   .. include:: /includes/matter/interface/main_button.txt

Second Button:
   Sets the state of ``UserButton`` attribute in the ``NordicDevkit`` cluster to ``true`` on press and ``false`` on release.

.. include:: /includes/matter/interface/segger_usb.txt
.. include:: /includes/matter/interface/nfc.txt

Building and running
********************

.. include:: /includes/matter/building_and_running/intro.txt
.. include:: /includes/matter/building_and_running/select_configuration.txt
.. include:: /includes/matter/building_and_running/commissioning.txt

|matter_ble_advertising_auto|

.. include:: /includes/matter/building_and_running/onboarding.txt

Advanced building options
=========================

.. include:: /includes/matter/building_and_running/advanced/intro.txt
.. include:: /includes/matter/building_and_running/advanced/building_nrf54lm20dk_7002eb2.txt
.. include:: /includes/matter/building_and_running/advanced/wifi_flash.txt

Testing
*******

.. |endpoint_name| replace:: **NordicDevkit cluster**
.. |endpoint_id| replace:: **1**

.. include:: /includes/matter/testing/intro.txt
.. include:: /includes/matter/testing/prerequisites.txt
.. include:: /includes/matter/testing/prepare.txt

Testing steps
=============

#. Run the chip-tool in the interactive mode:

   .. code-block:: console

      chip-tool interactive start

#. Read the ``NordicDevkit`` cluster's attributes by index:

   .. code-block:: console

      any read-by-id 0xFFF1FC01 *attribute-id* 1 1

   * *attribute-id* is the attribute's ID, equal to ``0xFFF10000`` for ``DevKitName``, ``0xFFF10001`` for ``UserLED`` and ``0xFFF10002`` for ``UserButton`` attributes for the ``NordicDevkit`` cluster in this sample.

#. Verify that all attributes have been read correctly and are equal to the default values defined in cluster's configuration.
#. Write the ``DevkitName`` attribute:

   .. code-block:: console

      any write-by-id 0xFFF1FC01 0xFFF10000 "NewName" 1 1

#. Read the ``DevkitName`` attribute again to check if it has changed.
#. Send the ``SetLED`` command to the device to control the LED state:

   .. code-block:: console

      any command-by-id 0xFFF1FC01 0xFFF10000 '{ "0x0": "u:*action*" }' 1 1

   * *action* is the action that should be performed on LED attribute: ``0`` to turn the LED off, ``1`` to turn it on, ``2`` to toggle the state.

#. Verify that the LED state has changed and the attribute value is updated.
#. Subscribe to the ``UserButton`` attribute to monitor the button state:

   .. code-block:: console

      any subscribe-by-id 0xFFF1FC01 0xFFF10002 0 120 1 1

#. Press the button assigned to the ``UserButton`` and check if the attribute state is updated in the chip-tool.
#. Read the ``UserButtonChanged`` event to check that events were generated on ``UserButton`` attribute changes.

   .. code-block:: console

      any read-event-by-id 0xFFF1FC01 0xFFF10000 1 1

#. Read the ``Basic Information`` cluster's ``RandomNumber`` attribute:

   .. code-block:: console

      any read-by-id 0x0028 0x17 1 0

#. Send the ``GenerateRandom`` command to the device to update the ``RandomNumber`` attribute:

   .. code-block:: console

      any command-by-id 0x0028 0 '{}' 1 0

#. Verify that the random value has been generated and the attribute value is updated.
#. Read the ``Basic Information`` cluster's ``RandomNumberChanged`` event to check that events were generated on ``RandomNumber`` attribute changes.

   .. code-block:: console

      any read-event-by-id 0x0028 0x4 1 0

Factory reset
=============

|matter_factory_reset|

Dependencies
************

.. include:: /includes/matter/dependencies.txt
