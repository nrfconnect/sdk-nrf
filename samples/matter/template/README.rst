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

* Matter over Thread is supported for ``nrf52840dk_nrf52840``, ``nrf5340dk_nrf5340_cpuapp``, and ``nrf21540dk_nrf52840``.
* Matter over Wi-Fi is supported for ``nrf5340dk_nrf5340_cpuapp`` with the ``nrf7002_ek`` shield attached or for ``nrf7002dk_nrf5340_cpuapp``.

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

Configuration
*************

|config|

Matter template build types
===========================

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_configuration_file_types_start
    :end-before: matter_door_lock_sample_configuration_file_types_end

Device Firmware Upgrade support
===============================

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_build_with_dfu_start
    :end-before: matter_door_lock_sample_build_with_dfu_end

FEM support
===========

.. include:: /includes/sample_fem_support.txt

User interface
**************

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_led1_start
    :end-before: matter_door_lock_sample_led1_end

Button 1:
     If pressed for six seconds, it initiates the factory reset of the device.
     Releasing the button within the six-second window cancels the factory reset procedure.

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_jlink_start
    :end-before: matter_door_lock_sample_jlink_end

Building and running
********************

.. |sample path| replace:: :file:`samples/matter/template`

.. include:: /includes/build_and_run.txt

Selecting a build type
======================

Before you start testing the application, you can select one of the `Matter template build types`_, depending on your building method.

Selecting a build type in |VSC|
-------------------------------

.. include:: /getting_started/modifying.rst
   :start-after: build_types_selection_vsc_start
   :end-before: build_types_selection_vsc_end

Selecting a build type from command line
----------------------------------------

.. include:: /getting_started/modifying.rst
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

      File not found: ./ncs/nrf/samples/matter/template/configuration/nrf52840dk_nrf52840/prj_shell.conf

Testing
=======

When you have built the sample and programmed it to your development kit, it automatically starts the Bluetooth LE advertising and the **LED1** starts flashing (Short Flash On).
At this point, you can press **Button 1** for six seconds to initiate the factory reset of the device.

.. _matter_template_network_testing:

Testing in a network
--------------------

To test the sample in a Matter-enabled Thread network, complete the following steps:

.. matter_template_sample_testing_start

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
#. Press **Button 1** for six seconds to initiate the factory reset of the device.

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
