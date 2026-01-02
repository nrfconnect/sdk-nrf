.. |matter_name| replace:: Template
.. |matter_type| replace:: sample
.. |matter_dks_thread| replace:: ``nrf52840dk/nrf52840``, ``nrf5340dk/nrf5340/cpuapp``, ``nrf54l15dk/nrf54l15/cpuapp``, and ``nrf54lm20dk/nrf54lm20a/cpuapp`` board targets
.. |matter_dks_wifi| replace:: ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target with the ``nrf7002eb2`` shield attached
.. |matter_dks_internal| replace:: nRF54LM20 DK and nRF54L15 DK
.. |matter_dks_tfm| replace:: ``nrf54l15dk/nrf54l15/cpuapp`` board target
.. |sample path| replace:: :file:`samples/matter/template`
.. |matter_qr_code_payload| replace:: MT:K.K9042C00KA0648G00
.. |matter_pairing_code| replace:: 34970112332
.. |matter_qr_code_image| image:: /images/matter_qr_code_template_sample.png
                          :width: 200px
                          :alt: QR code for commissioning the template device

.. include:: /includes/matter/shortcuts.txt

.. _matter_template_sample:

Matter: Template
################

.. contents::
   :local:
   :depth: 2

This sample demonstrates a minimal implementation of the :ref:`Matter <ug_matter>` application layer.
This basic implementation enables the commissioning on the device, which allows it to join a Matter network built on top of a low-power, 802.15.4 Thread network or on top of a Wi-Fi® network.

.. include:: /includes/matter/introduction/no_sleep_thread_mtd_wifi.txt

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/matter/requirements/thread_wifi.txt
.. include:: /includes/matter/requirements/hardware.txt

Overview
********

The sample starts the Bluetooth® LE advertising automatically and prepares the Matter device for commissioning into a Matter-enabled Thread network.
The sample uses an LED to show the state of the connection.

Configuration
*************

.. include:: /includes/matter/configuration/intro.txt

The |matter_type| supports the following build configurations:

.. include:: /includes/matter/configuration/basic_internal.txt

Advanced configuration options
==============================

.. include:: /includes/matter/configuration/advanced/intro.txt

.. include:: /includes/matter/configuration/advanced/dfu.txt
.. include:: /includes/matter/configuration/advanced/tfm.txt
.. include:: /includes/matter/configuration/advanced/factory_data.txt
.. include:: /includes/matter/configuration/advanced/custom_board.txt
.. include:: /includes/matter/configuration/advanced/internal_memory.txt

User interface
**************

.. include:: /includes/matter/interface/intro.txt

First LED:
   .. include:: /includes/matter/interface/state_led.txt

First Button:
   .. include:: /includes/matter/interface/main_button.txt

.. include:: /includes/matter/interface/segger_usb.txt

Building and running
********************

.. include:: /includes/matter/building_and_running/intro.txt

|matter_ble_advertising_auto|


Advanced building options
=========================

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

Read basic information
----------------------

To see if the communication with the device is working, run the following command to read the basic information from the device:

.. code-block:: console

   chip-tool basicinformation read product-id |node_id| 0

See the output and verify that the product ID is set to ``32768`` (0x8000 in hexadecimal).

Testing with commercial ecosystem
=================================

.. note::

   |sample_not_in_ecosystem|

.. include:: /includes/matter/testing/ecosystem.txt

Dependencies
************

.. include:: /includes/matter/dependencies.txt
