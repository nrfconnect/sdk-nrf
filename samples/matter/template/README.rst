.. _matter_template_sample:

Matter: Template
################

.. contents::
   :local:
   :depth: 2

This sample demonstrates a minimal implementation of the :ref:`Matter <ug_matter>` application layer.
This basic implementation enables the commissioning on the device, which allows it to join a Matter network built on top of a low-power, 802.15.4 Thread network or on top of a Wi-Fi network.
Support for both Thread and Wi-Fi is mutually exclusive and depends on the hardware platform, so only one protocol can be supported for a specific Matter device.
In case of Thread, this device works as a Thread :ref:`Minimal End Device <thread_ot_device_types>`.

Use this sample as a reference for developing your own application.
See the :ref:`ug_matter_creating_accessory` page for an overview of the process you need to follow.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

For testing purposes, that is to commission the device and :ref:`control it remotely <matter_template_network_mode>` through a Thread network, you also need a Matter controller device :ref:`configured on PC or smartphone <ug_matter_configuring>`. This requires additional hardware depending on the setup you choose.

.. note::
    |matter_gn_required_note|

IPv6 network support
====================

The development kits for this sample offer the following IPv6 network support for Matter:

* Matter over Thread is supported for ``nrf52840dk/nrf52840``, ``nrf5340dk/nrf5340/cpuapp``, ``nrf21540dk/nrf52840``, ``nrf54l15dk/nrf54l15/cpuapp``, and ``nrf54h20dk/nrf54h20/cpuapp``.
* Matter over Wi-Fi is supported for ``nrf5340dk/nrf5340/cpuapp`` or ``nrf54h20dk/nrf54h20/cpuapp`` with the ``nrf7002ek`` shield attached, or for ``nrf7002dk/nrf5340/cpuapp``.

Overview
********

The sample starts the Bluetooth® LE advertising automatically and prepares the Matter device for commissioning into a Matter-enabled Thread network.
The sample uses an LED to show the state of the connection.
You can press a button to start the factory reset when needed.

.. _matter_template_network_mode:

Remote testing in a network
===========================

Testing in either a Matter-enabled Thread or a Wi-Fi network requires a Matter controller that you can configure on PC or mobile device.
By default, the Matter accessory device has IPv6 networking disabled.
You must pair the device with the Matter controller over Bluetooth® LE to get the configuration from the controller to use the device within a Thread or a Wi-Fi network.
You can enable the controller after :ref:`building and running the sample <matter_template_network_testing>`.

To pair the device, the controller must get the :ref:`matter_template_network_mode_onboarding` from the Matter accessory device and commission the device into the network.

Commissioning in Matter
-----------------------

In Matter, the commissioning procedure takes place over Bluetooth LE between a Matter accessory device and the Matter controller, where the controller has the commissioner role.
When the procedure has completed, the device is equipped with all information needed to securely operate in the Matter network.

During the last part of the commissioning procedure (the provisioning operation), the Matter controller sends the Thread or Wi-Fi network credentials to the Matter accessory device.
As a result, the device can join the IPv6 network and communicate with other devices in the network.

.. _matter_template_network_mode_onboarding:

Onboarding information
++++++++++++++++++++++

When you start the commissioning procedure, the controller must get the onboarding information from the Matter accessory device.
The onboarding information representation depends on your commissioner setup.

For this sample, you can use one of the following :ref:`onboarding information formats <ug_matter_network_topologies_commissioning_onboarding_formats>` to provide the commissioner with the data payload that includes the device discriminator and the setup PIN code:

  .. list-table:: Template sample onboarding information
     :header-rows: 1

     * - QR Code
       - QR Code Payload
       - Manual pairing code
     * - Scan the following QR code with the app for your ecosystem:

         .. figure:: ../../../doc/nrf/images/matter_qr_code_template_sample.png
            :width: 200px
            :alt: QR code for commissioning the template device

       - MT:Y.K9042C00KA0648G00
       - 34970112332

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_onboarding_start
    :end-before: matter_door_lock_sample_onboarding_end

|matter_cd_info_note_for_samples|

Configuration
*************

|config|

.. _matter_template_custom_configs:

Matter template custom configurations
=====================================

.. include:: ../light_bulb/README.rst
    :start-after: matter_light_bulb_sample_configuration_file_types_start
    :end-before: matter_light_bulb_sample_configuration_file_types_end

Matter template with Trusted Firmware-M
=======================================

.. matter_template_build_with_tfm_start

The sample supports using :ref:`Trusted Firmware-M <ug_tfm>` on the nRF54L15 DK.
The memory map of the sample has been aligned to meet the :ref:`ug_tfm_partition_alignment_requirements`.

You can build the sample with Trusted Firmware-M support by adding the ``ns`` suffix to the ``nrf54l15pdk/nrf54l15/cpuapp`` build target.

For example:

.. code-block:: console

    west build -p -b nrf54l15pdk/nrf54l15/cpuapp/ns

.. note::

   The firmware built for ``nrf54l15pdk/nrf54l15/cpuapp/ns`` will not work on the nRF54L15 DK.

.. matter_template_build_with_tfm_end

Device Firmware Upgrade support
===============================

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_build_with_dfu_start
    :end-before: matter_door_lock_sample_build_with_dfu_end

Alternatively, for the nRF54L15 DK, the DFU can be configured to only use the internal MRAM for storage.
This means that both the currently running firmware and the new firmware to be updated will be stored within the device's internal flash memory.
This configuration is only available for the :ref:`release configuration <matter_template_custom_configs>`.

The following is an example command to build the sample on the nRF54L15 DK with support for Matter OTA DFU and DFU over Bluetooth® SMP, and using internal MRAM only:

.. code-block:: console

    west build -p -b nrf54l15dk/nrf54l15/cpuapp -- -DFILE_SUFFIX=release -DCONFIG_CHIP_DFU_OVER_BT_SMP=y -DPM_STATIC_YML_FILE=pm_static_nrf54l15dk_nrf54l15_cpuapp_internal.yml -Dmcuboot_EXTRA_CONF_FILE=<absolute_path_to_the_template_sample>/sysbuild/mcuboot/boards/nrf54l15dk_nrf54l15_cpuapp_internal.conf -Dmcuboot_EXTRA_DTC_OVERLAY_FILE=<absolute_path_to_the_template_sample>/sysbuild/mcuboot/boards/nrf54l15dk_nrf54l15_cpuapp_internal.overlay

Note that in this case, the size of the application partition is half of what it would be when using a configuration with external flash memory support.

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

   .. group-tab:: nRF52, nRF53 and nRF70 DKs

      LED 1:
         .. include:: /includes/matter_sample_state_led.txt

      Button 1:
         .. include:: /includes/matter_sample_button.txt

      .. include:: /includes/matter_segger_usb.txt

   .. group-tab:: nRF54 DKs

      LED 0:
         .. include:: /includes/matter_sample_state_led.txt

      Button 0:
         .. include:: /includes/matter_sample_button.txt

      .. include:: /includes/matter_segger_usb.txt


Building and running
********************

.. |sample path| replace:: :file:`samples/matter/template`

.. include:: /includes/build_and_run.txt

.. matter_template_build_wifi_nrf54h20_start

To use nrf54H20 DK with the ``nrf7002ek`` shield attached (2.4 GHz or 5 GHz), follow the :ref:`ug_nrf7002eb_nrf54h20dk_gs` user guide to connect all required pins and then use the following command to build the sample:

.. matter_template_build_wifi_nrf54h20_end

.. code-block:: console

    west build -b nrf54h20dk/nrf54h20/cpuapp -p -- -DSB_CONFIG_WIFI_NRF70=y -DCONFIG_CHIP_WIFI=y -Dtemplate_SHIELD=nrf700x_nrf54h20dk

Selecting a configuration
=========================

Before you start testing the application, you can select one of the :ref:`matter_template_custom_configs`.
See :ref:`app_build_file_suffixes` and :ref:`cmake_options` for more information how to select a configuration.

Testing
=======

.. tabs::

   .. group-tab:: nRF52, nRF53 and nRF70 DKs

      When you have built the sample and programmed it to your development kit, it automatically starts the Bluetooth LE advertising and the **LED 1** starts flashing (Short Flash On).
      At this point, you can press **Button 1** for six seconds to initiate the factory reset of the device.

   .. group-tab:: nRF54 DKs

      When you have built the sample and programmed it to your development kit, it automatically starts the Bluetooth LE advertising and the **LED 0** starts flashing (Short Flash On).
      At this point, you can press **Button 0** for six seconds to initiate the factory reset of the device.

.. _matter_template_network_testing:

Testing in a network
--------------------

To test the sample in a Matter-enabled Thread network, complete the following steps:

.. tabs::

   .. group-tab:: nRF52, nRF53 and nRF70 DKs

      1. |connect_kit|
      #. |connect_terminal_ANSI|
      #. Commission the device into a Matter network by following the guides linked on the :ref:`ug_matter_configuring` page for the Matter controller you want to use.
         The guides walk you through the following steps:

         * Only if you are configuring Matter over Thread: Configure the Thread Border Router.
         * Build and install the Matter controller.
         * Commission the device.
           You can use the :ref:`matter_template_network_mode_onboarding` listed earlier on this page.
         * Send Matter commands.

         At the end of this procedure, **LED 1** of the Matter device programmed with the sample starts flashing in the Short Flash Off state.
         This indicates that the device is fully provisioned, but does not yet have full IPv6 network connectivity.
      #. Keep the **Button 1** pressed for more than six seconds to initiate factory reset of the device.

         The device reboots after all its settings are erased.

   .. group-tab:: nRF54 DKs

      1. |connect_kit|
      #. |connect_terminal_ANSI|
      #. Commission the device into a Matter network by following the guides linked on the :ref:`ug_matter_configuring` page for the Matter controller you want to use.
         The guides walk you through the following steps:

         * Only if you are configuring Matter over Thread: Configure the Thread Border Router.
         * Build and install the Matter controller.
         * Commission the device.
           You can use the :ref:`matter_template_network_mode_onboarding` listed earlier on this page.
         * Send Matter commands.

         At the end of this procedure, **LED 0** of the Matter device programmed with the sample starts flashing in the Short Flash Off state.
         This indicates that the device is fully provisioned, but does not yet have full IPv6 network connectivity.
      #. Keep the **Button 0** pressed for more than six seconds to initiate factory reset of the device.

         The device reboots after all its settings are erased.

The device reboots after all its settings are erased.

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
