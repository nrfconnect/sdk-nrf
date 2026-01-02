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
                          :width: 200px
                          :alt: QR code for commissioning the manufacturer-specific device

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

|matter_ble_advertising_auto|

Advanced building options
=========================

.. include:: /includes/matter/building_and_running/advanced/intro.txt
.. include:: /includes/matter/building_and_running/advanced/building_nrf54lm20dk_7002eb2.txt
.. include:: /includes/matter/building_and_running/advanced/wifi_flash.txt

Testing
*******

.. include:: /includes/matter/testing/intro.txt

Testing with CHIP Tool
======================

Complete the following steps to test the |matter_name| device using CHIP Tool:

.. |node_id| replace:: 1

.. include:: /includes/matter/testing/1_prepare_matter_network_thread_wifi.txt
.. include:: /includes/matter/testing/2_prepare_dk.txt
.. include:: /includes/matter/testing/3_commission_thread_wifi.txt

.. rst-class:: numbered-step

Run CHIP Tool interactive mode
------------------------------

Enter the interactive mode by running the following command:

.. code-block:: console

   chip-tool interactive start

.. rst-class:: numbered-step

Read the ``NordicDevkit`` cluster's attributes by index
-------------------------------------------------------

.. parsed-literal::
   :class: highlight

   any read-by-id 0xFFF1FC01 *attribute-id* |node_id| 1

Where:

* *attribute-id* is the attribute's ID, equal to ``0xFFF10000`` for ``DevKitName``, ``0xFFF10001`` for ``UserLED`` and ``0xFFF10002`` for ``UserButton`` attributes for the ``NordicDevkit`` cluster in this sample.

.. rst-class:: numbered-step

Verify all the attributes
-------------------------

In the interactive mode, search the logs received by CHIP Tool and verify that all attributes have been read correctly and are equal to the default values defined in cluster's configuration.

.. rst-class:: numbered-step

Write the ``DevkitName`` attribute
----------------------------------

.. parsed-literal::
   :class: highlight

   any write-by-id 0xFFF1FC01 0xFFF10000 "NewName" |node_id| 1

Where:

* *NewName* is the new name to be set for the ``DevkitName`` attribute.

.. rst-class:: numbered-step

Read the ``DevkitName`` attribute again
---------------------------------------

In the interactive mode, read the ``DevkitName`` attribute again by running the following command, and check if it has changed:

.. parsed-literal::
   :class: highlight

   any read-by-id 0xFFF1FC01 *attribute-id* |node_id| 1

.. rst-class:: numbered-step

Send the ``SetLED`` command to control the LED state
----------------------------------------------------

In the interactive mode, send the ``SetLED`` command to control the LED state by running the following command:

.. parsed-literal::
   :class: highlight

   any command-by-id 0xFFF1FC01 0xFFF10000 '{ "0x0": "u:*action*" }' |node_id| 1

* *action* is the action that should be performed on LED attribute: ``0`` to turn the LED off, ``1`` to turn it on, ``2`` to toggle the state.

.. rst-class:: numbered-step

The LED state has changed and the attribute value is updated.

.. rst-class:: numbered-step

Subscribe to the ``UserButton`` attribute
-----------------------------------------

In the interactive mode, subscribe to the ``UserButton`` attribute to monitor the button state by running the following command:

.. parsed-literal::
   :class: highlight

   any subscribe-by-id 0xFFF1FC01 0xFFF10002 0 120 |node_id| 1

.. rst-class:: numbered-step

Press the button assigned to the ``UserButton``
-----------------------------------------------

Being subscribed to the ``UserButton`` attribute, press the button assigned to the ``UserButton`` and wait for the attribute state to be updated.
You should see the attribute state updated in the CHIP Tool interactive mode.

.. rst-class:: numbered-step

Read the ``UserButtonChanged`` event
------------------------------------

In the interactive mode, read the ``UserButtonChanged`` event to check that events were generated on ``UserButton`` attribute changes by running the following command:

.. parsed-literal::
   :class: highlight

   any read-event-by-id 0xFFF1FC01 0xFFF10000 |node_id| 1

.. rst-class:: numbered-step

Read the ``Basic Information`` cluster's ``RandomNumber`` attribute
--------------------------------------------------------------------

In the interactive mode, read the ``Basic Information`` cluster's ``RandomNumber`` attribute by running the following command:

.. parsed-literal::
   :class: highlight

   any read-by-id 0x0028 0x17 |node_id| 0

.. rst-class:: numbered-step

Send the ``GenerateRandom`` command
-----------------------------------

In the interactive mode, send the ``GenerateRandom`` command to update the ``RandomNumber`` attribute by running the following command:

.. parsed-literal::
   :class: highlight

   any command-by-id 0x0028 0 '{}' |node_id| 0

.. rst-class:: numbered-step

Verify that the random value
----------------------------

In the interactive mode, read the ``Basic Information`` cluster's ``RandomNumber`` attribute again by running the following command:

.. code-block:: console

   any read-by-id 0x0028 0x17 |node_id| 0

You should see the random value updated in the CHIP Tool logs.

.. rst-class:: numbered-step

Read the ``Basic Information`` cluster's ``RandomNumberChanged`` event
----------------------------------------------------------------------

In the interactive mode, read the ``Basic Information`` cluster's ``RandomNumberChanged`` event to check that events were generated on ``RandomNumber`` attribute changes by running the following command:

.. code-block:: console

   any read-event-by-id 0x0028 0x4 |node_id| 0

Testing with commercial ecosystem
=================================

.. note::

   |sample_not_in_ecosystem|

.. include:: /includes/matter/testing/ecosystem.txt

Dependencies
************

.. include:: /includes/matter/dependencies.txt
