.. |multi_image| replace:: These samples are built for the application core and, by default, include the network core application as child image in a multi-image build (see :ref:`ug_nrf5340_multi_image`).

.. _ug_nrf5340:

Working with nRF53 Series
#########################

.. contents::
   :local:
   :depth: 2

The |NCS| provides support for developing on the nRF5340 System on Chip (SoC) using the nRF5340 DK (PCA10095).

See the `nRF5340 DK User Guide`_ for detailed information about the nRF5340 DK hardware.
To get started with the nRF5340 DK, follow the steps in the `Getting started with nRF Connect SDK (nRF53 Series)`_ guide.

.. note::
   The nRF5340 PDK has been deprecated with the introduction of the production-level nRF5340 DK.
   To determine if you have a PDK or DK, check the version number on the sticker on your kit.
   If the version is 0.11.0 or higher, the kit is an nRF5340 DK.

   See the `nRF Connect SDK v1.4.0 documentation`_ for the last release supporting the nRF5340 PDK.


Introduction
************

nRF5340 is a wireless ultra-low-power multicore System on Chip (SoC) with two fully programmable Arm Cortex-M33 processors: a network core and an application core.

See the `nRF5340 Product Specification`_ for more information about the nRF5340 SoC.
:ref:`zephyr:nrf5340dk_nrf5340` gives an overview of the nRF5340 DK support in Zephyr.

Network core
============

The network core is an Arm Cortex-M33 processor with a reduced feature set, designed for ultra-low-power operation.
Use this core for radio communication and for real-time processing tasks involving low-level radio protocol layers.

The build target for the network core in Zephyr is ``nrf5340dk_nrf5340_cpunet``.

Application core
================

The application core is a full-featured Arm Cortex-M33 processor including DSP instructions and FPU.
Use this core for tasks that require high performance and for application-level logic.

The M33 TrustZone divides the application MCU into secure and non-secure domains.
When the MCU boots, it always starts executing from the secure area.
The |NCS| provides two alternatives for running applications from the non-secure area of the memory:

Secure Partition Manager (SPM)
  The :ref:`secure_partition_manager` sample uses the SPU peripheral to configure security attributions for flash, SRAM, and peripherals.
  After the configuration setup is complete, the sample loads the application firmware from the non-secure domain.
  In addition, the SPM sample can provide access to secure services to the application firmware.

  The SPM sample is currently the default solution used by most |NCS| samples.
  This means that when you build your application for the non-secure domain, the :ref:`secure_partition_manager` sample is automatically included in the build.

Trusted Firmware-M (TF-M)
  Trusted Firmware-M provides a highly configurable set of software components to create a Trusted Execution Environment.
  TF-M is a framework which will be extended for new functions and use cases beyond the scope of SPM.

  Support for TF-M in |NCS| is currently experimental.
  If your application does not depend on the secure services developed in SPM and does not use them, then TF-M can replace SPM as the secure firmware component in your application.

  For more information and instructions on how to do this, see :ref:`ug_tfm`.
  See :ref:`tfm_hello_world` for a sample that demonstrates how to add TF-M to an application.

In Zephyr, the application core is divided into two different build targets:

* ``nrf5340dk_nrf5340_cpuapp`` for the secure domain
* ``nrf5340dk_nrf5340_cpuappns`` for the non-secure domain

Inter-core communication
========================

Communication between the application core and the network core happens through a shared memory area.
The application core memory is mapped to the network core memory map.
This means that the network core can access and use the application core memory for shared memory communication.

Interprocessor Communication (IPC) is used to indicate to the other core that there is new data available to pick up.
The actual data exchange is handled by Open Asymmetric Multi-Processing (OpenAMP).

Zephyr includes the `OpenAMP`_ library, which provides a complete solution for exchanging messages between the cores.
The IPC peripheral is presented to Zephyr as an Interprocessor Mailbox (IPM) device.
The OpenAMP library uses the IPM SHIM layer, which in turn uses the IPC driver in `nrfx`_.

.. |note| replace:: The following instructions are for the application core.
   To upgrade the firmware on the network core, perform the steps for FOTA upgrade described below, replacing :file:`app_update.bin`, which is the file used when upgrading firmware on the application core, with :file:`net_core_app_update.bin`.
   In addition, ensure that :kconfig:`CONFIG_PCD_APP` is enabled for the MCUboot child image.
   For more details, see :ref:`nc_bootloader`.

Protocols and use cases
***********************

nRF5340 samples usually consist of two separate images: one that runs on the network core and one that runs on the application core.
For specific use cases, you can use only one of the cores.

The following sections describe the recommended architecture for using different protocols on the nRF5340 and list the provided samples.

Bluetooth Low Energy
====================

.. list-table::
   :header-rows: 1

   * - Network core
     - Application core
   * - :ref:`zephyr:bluetooth-hci-rpmsg-sample`
     - | :ref:`Bluetooth Low Energy samples <ble_samples>`
       | :ref:`Bluetooth samples in Zephyr <zephyr:bluetooth-samples>`
   * - :ref:`ble_rpc_host` (supported for development)
     - Some Bluetooth Low Energy samples, for example, :ref:`zephyr:bluetooth-beacon-sample`

When using Bluetooth Low Energy on the nRF5340, you have two options:

* Split the Bluetooth LE Controller and the host part of the Bluetooth LE stack and run them on different cores.
* Run the full Bluetooth LE stack on the network core (currently supported for development only).

Split Controller and Host
-------------------------

When splitting the Bluetooth LE Controller and the Host, run the Bluetooth LE Controller on the network core and the host part of the Bluetooth LE stack and the application logic on the application core.

For the network core, the |NCS| provides the :ref:`zephyr:bluetooth-hci-rpmsg-sample` sample.
This Zephyr sample is designed specifically to enable the Bluetooth LE Controller functionality on a remote MCU using the `RPMsg Messaging Protocol`_ as a transport for Bluetooth HCI.
The sample implements the RPMsg transport using the `OpenAMP`_ library to communicate with a Bluetooth Host stack that runs on a separate core (in this case, the nRF5340 application core).

You can use either the SoftDevice Controller or the Zephyr Bluetooth LE Controller for this sample.
See :ref:`ug_ble_controller` for more information.

For the application core, the |NCS| provides a series of :ref:`Bluetooth Low Energy samples <ble_samples>`, in addition to the :ref:`Bluetooth samples in Zephyr <zephyr:bluetooth-samples>`.
|multi_image|

.. note::
   Most of the provided Bluetooth LE samples should run on the nRF5340 DK, but not all have been thoroughly tested.

Full Bluetooth LE stack
-----------------------

To run the full Bluetooth LE stack on the network core, the |NCS| provides the :ref:`ble_rpc_host` sample.

.. note::
   The :ref:`ble_rpc_host` sample is currently supported for development only.
   It does not support all Bluetooth Host APIs yet.

For the application core, use a compatible Bluetooth LE sample, for example, the :ref:`zephyr:bluetooth-beacon-sample` sample.


IEEE 802.15.4 (Thread and Zigbee)
=================================

.. list-table::
   :header-rows: 1

   * - Network core
     - Application core
   * - :ref:`zephyr:nrf-ieee802154-rpmsg-sample`
     - | :ref:`Thread samples <openthread_samples>`
       | :ref:`Zigbee samples <zigbee_samples>`
       | :ref:`Matter samples <matter_samples>`

When using IEEE 802.15.4 on the nRF5340, run the IEEE 802.15.4 radio driver on the network core and the high-level radio stack (the host part of the Thread and Zigbee stacks) and the application logic on the application core.

.. figure:: /images/ieee802154_nrf53_singleprot_design.svg
   :alt: IEEE 802.15.4 Protocol architecture in multicore SoC

   IEEE 802.15.4 Protocol architecture in multicore SoC

For the network core, the |NCS| provides the :ref:`zephyr:nrf-ieee802154-rpmsg-sample` sample.
This Zephyr sample is designed specifically to enable the nRF IEEE 802.15.4 radio driver and its serialization library on a remote MCU using the `RPMsg Messaging Protocol`_ as a transport for the nRF 802.15.4 radio driver serialization.
The sample implements the RPMsg transport using the `OpenAMP`_ library to communicate with the nRF IEEE 802.15.4 radio driver serialization host that runs on a separate core (in this case, the nRF5340 application core).

For the application core, the |NCS| provides a series of samples for the :ref:`Thread <ug_thread>`, :ref:`Zigbee <ug_zigbee>`, and :ref:`Matter <ug_matter>` protocols.
|multi_image|


Multiprotocol (Thread or Zigbee in combination with Bluetooth LE)
=================================================================

.. list-table::
   :header-rows: 1

   * - Network core
     - Application core
   * - :ref:`multiprotocol-rpmsg-sample`
     - | :ref:`Thread samples <openthread_samples>`
       | :ref:`Zigbee samples <zigbee_samples>`

nRF5340 supports running another protocol in parallel with the :ref:`nrfxlib:softdevice_controller`.
When using Thread or Zigbee in parallel with Bluetooth LE, run the low-level radio protocol layers (thus the IEEE 802.15.4 radio driver and the Bluetooth LE Controller) on the network core and the high-level radio stack (the host part of the Bluetooth LE, Thread, and Zigbee stacks) and the application logic on the application core.

.. figure:: /images/ieee802154_nrf53_multiprot_design.svg
   :alt: Bluetooth LE and IEEE 802.15.4 multiprotocol architecture in multicore SoC

   Bluetooth LE and IEEE 802.15.4 multiprotocol architecture in multicore SoC

For the network core, the |NCS| provides the :ref:`multiprotocol-rpmsg-sample` sample.
It is a combination of the :ref:`zephyr:bluetooth-hci-rpmsg-sample` sample (for Bluetooth LE) and the :ref:`zephyr:nrf-ieee802154-rpmsg-sample` sample (for IEEE 802.15.4).
This means that it enables both the Bluetooth LE Controller and the nRF IEEE 802.15.4 radio driver and simultaneously exposes the functionality of both stacks to the application core using the `RPMsg Messaging Protocol`_.
Separate RPMsg endpoints are used to obtain independent inter-core connections for each stack.

For the application core, the |NCS| provides a series of samples for the :ref:`Thread <ug_thread>` and :ref:`Zigbee <ug_zigbee>` protocols.
|multi_image|
See the :ref:`ug_multiprotocol_support` user guide for instructions on how to enable multiprotocol support for Thread or Zigbee in combination with Bluetooth.


Direct use of the radio peripheral
==================================

.. list-table::
   :header-rows: 1

   * - Network core
     - Application core
   * - | :ref:`direct_test_mode`
       | :ref:`radio_test`
       | :ref:`timeslot_sample`
     - :ref:`nrf5340_empty_app_core`

.. note::
   The above list might not be exhaustive.

Samples that directly use the radio peripheral can run on the network core of the nRF5340.
They do not require any functionality from the application core.

However, on nRF5340, the application core is responsible for starting the network core and connecting its GPIO pins (see :kconfig:`CONFIG_BOARD_ENABLE_CPUNET` and the code in :file:`zephyr/boards/arm/nrf5340dk_nrf5340/nrf5340_cpunet_reset.c`).
Therefore, you must always program the application core, even if the firmware is supposed to run only on the network core.

You can use the :ref:`nrf5340_empty_app_core` sample for this purpose.
Configure the network core application to automatically include this sample as a child image.
This is the default configuration for the listed network core samples.
For more information, see :kconfig:`CONFIG_NCS_SAMPLE_EMPTY_APP_CORE_CHILD_IMAGE` and :ref:`ug_nrf5340_multi_image`.


No radio communication
======================
.. list-table::
   :header-rows: 1

   * - Network core
     - Application core
   * - ---
     - | :ref:`NFC samples <nfc_samples>`
       | :ref:`Crypto samples <crypto_samples>`
       | :ref:`tfm_hello_world`
       | :ref:`lpuart_sample`


.. note::
   The above list might not be exhaustive.

Samples that do not need radio communication can run on the application core of the nRF5340.
They do not require any firmware on the network core.
Therefore, the network core can remain empty.

If you want to enable the network core anyway, set the :kconfig:`CONFIG_BOARD_ENABLE_CPUNET` option in the image for the application core.

.. _ug_nrf5340_multi_image:

Single image vs. multi-image build
**********************************

If a sample consists of several images (in this case, different images for the application core and for the network core), you can build these images separately or combined as a :ref:`multi-image build <ug_multi_image>`, depending on the sample configuration.

In a multi-image build, the image for the application core is usually the parent image, and the image for the network core is treated as a child image in a separate domain.
For this to work, the network core image must be explicitly added as a child image to one of the application core images.
See :ref:`ug_multi_image_defining` for details.

.. note::
   When using the :ref:`nrf5340_empty_app_core` sample, the image hierarchy is inverted.
   In this case, the network core image is the parent image and the application core image is the child image.

Default build configuration
===========================

By default, the two images are built together for all Bluetooth LE, Thread, Zigbee, and Matter samples in the |NCS|.
Samples that are designed to run only on the network core include the :ref:`nrf5340_empty_app_core` sample as a child image.
For other samples, the images are built separately.

The build configuration depends on the following Kconfig options that must be set in the configuration of the parent image:

* :kconfig:`CONFIG_BT_RPMSG_NRF53` - set to ``y`` in all Bluetooth LE samples for the application core
* :kconfig:`CONFIG_NRF_802154_SER_HOST` - set to ``y`` in all Thread, Zigbee, and Matter samples for the application core
* :kconfig:`CONFIG_NCS_SAMPLE_EMPTY_APP_CORE_CHILD_IMAGE` - set to ``y`` in all network core samples that require the :ref:`nrf5340_empty_app_core` sample

The combination of these options determines which (if any) sample is included in the build of the parent image:

.. list-table::
   :header-rows: 1

   * - Enabled options
     - Child image sample for the network core
     - Child image sample for the application core
   * - :kconfig:`CONFIG_BT_RPMSG_NRF53`
     - :ref:`zephyr:bluetooth-hci-rpmsg-sample`
     - ---
   * - :kconfig:`CONFIG_NRF_802154_SER_HOST`
     - :ref:`zephyr:nrf-ieee802154-rpmsg-sample`
     - ---
   * - :kconfig:`CONFIG_BT_RPMSG_NRF53` and :kconfig:`CONFIG_NRF_802154_SER_HOST`
     - :ref:`multiprotocol-rpmsg-sample`
     - ---
   * - :kconfig:`CONFIG_NCS_SAMPLE_EMPTY_APP_CORE_CHILD_IMAGE`
     - ---
     - :ref:`nrf5340_empty_app_core`

Configuration of the child image
================================

When a network sample is built automatically as a child image in a multi-image build, you can define the relevant Kconfig options (if required) in a :file:`.conf` file.
Name the file *network_sample*\ .conf, where *network_sample* is the name of the child image (for example, ``hci_rpmsg.conf``).
Place the file in a :file:`child_image` subfolder of the application sample directory.
See :ref:`ug_multi_image_variables` for more information.

This way of defining the Kconfig options allows to align the configurations of both images.

For example, see the :ref:`ble_throughput` child image configuration in :file:`nrf/samples/bluetooth/throughput/child_image/hci_rpmsg.conf`.

.. _ug_nrf5340_building:

Building and programming a sample
*********************************

Depending on the sample, you must program only the application core (for example, when using NFC samples) or both the network and the application core.

The steps differ depending on whether you work with |SES| or on the command line and whether you are doing a single or multi-image build.

Using SEGGER Embedded Studio
============================

To build and program separate images with |SES|, follow the general instructions for :ref:`gs_programming_ses`.
To program a multi-image HEX file, you must add the project files for the network core after building, as described in :ref:`ug_nrf5340_ses_multi_image`.

Separate images
---------------

To build and program only the application core, follow the instructions in :ref:`gs_programming_ses` and use ``nrf5340dk_nrf5340_cpuapp`` or ``nrf5340dk_nrf5340_cpuappns`` as build target.

To build and program a dedicated network sample, follow the instructions in :ref:`gs_programming_ses` and use ``nrf5340dk_nrf5340_cpunet`` as the build target.

.. _ug_nrf5340_ses_multi_image:

Multi-image build
-----------------

If you are working with Bluetooth LE, Thread, Zigbee, or Matter samples, the network core sample is built as child image when you build the application core image (see :ref:`ug_nrf5340_multi_image` above).

However, |SES| cannot automatically program the network sample to the network core when it is added as a child image.
You must manually add a network core project to the application core project to make sure that both are programmed.

.. note::
   You must reprogram the network core sample only when changes are made to it.
   You can modify and program the application core sample without reprogramming the network core.


Follow these steps to build and program a multi-image build to the nRF5340 application core and network core:

1. Follow the instructions in :ref:`gs_programming_ses` and open the application core sample (for example, :ref:`peripheral_lbs`) in |SES|.
   Use ``nrf5340dk_nrf5340_cpuapp`` or ``nrf5340dk_nrf5340_cpuappns`` as the build target.
#. Build the sample as described in :ref:`gs_programming_ses`.
   This creates both the application core image and the network core image.
#. Select :guilabel:`File` > :guilabel:`New Project`.

    .. figure:: images/ses_nrf5340_netcore_new_project.png
       :alt: Create New Project menu

       Create New Project menu

#. Select :guilabel:`Add the project to the current solution`.

    .. figure:: images/ses_nrf5340_netcore_add_project.png
       :alt: Adding a project target for programming the network core

       Adding a project target for programming the network core

#. Select the project template, project name, and project location.

   * :guilabel:`An externally built executable for Nordic Semiconductor nRF`:
     This template allows you to specify the network core HEX file to be programmed.
     The HEX file is created by the build system.

   * :guilabel:`Name`: Specify the name of the project as it will appear in SES.
     This example uses ``hci_rpmsg_nrf5340_netcore``.

   * :guilabel:`Location`: Specify the location of the project.
     This must be the same location as the current project/build folder.
     Click :guilabel:`Browse` to open a dialog where you can navigate to the current project/build folder and click :guilabel:`Select Folder`.

    .. figure:: images/ses_nrf5340_netcore_project_template.png
       :alt: Creating a new project for programming the network core

       Creating a new project for programming the network core

#. Click :guilabel:`Next`.

#. Configure the project settings.

   * :guilabel:`Target Processor`: Select ``nRF5340_xxAA_Network``.

   * :guilabel:`Load File`: Specify the file name of the merged HEX file for the network core that should be programmed.
     For example, specify ``$(ProjectDir)/hci_rpmsg/zephyr/merged_CPUNET.hex`` for the :ref:`zephyr:bluetooth-hci-rpmsg-sample` sample.

    .. figure:: images/ses_nrf5340_netcore_project_settings.png
       :alt: Project settings for programming the network core

       Project settings for programming the network core

#. Click :guilabel:`Next`.

#. Add the project files.
   This project will only be used for programming the network core so you must only add the default :guilabel:`Script Files`.

   .. figure:: images/ses_nrf5340_netcore_files.png
       :alt: Adding script files for programming the network core

       Adding script files for programming the network core

#. Click :guilabel:`Next`.

#. Add the project configurations.
   This project will only be used for programming the network core so no build configurations are needed.

   Deselect :guilabel:`Debug` and :guilabel:`Release`.

    .. figure:: images/ses_nrf5340_netcore_conf.png
       :alt: Deselecting configurations and finishing the configuration

       Deselecting configurations and finishing the configuration

#. Click :guilabel:`Finish`.

   This creates a new project for programming the network core with the HEX file of the network sample.

#. Set the new network core project as the active project using :guilabel:`Project` > :guilabel:`Set Active Project`.
   For example, select :guilabel:`hci_rpmsg_nrf5340_netcore`.

   .. figure:: images/ses_nrf5340_netcore_set_active.png
     :alt: Set the hci_rpmsg_nrf5340_netcore programming target as active

     Set the hci_rpmsg_nrf5340_netcore programming target as active

#. Program the network sample using :guilabel:`Target` > :guilabel:`Download XXX` (for example, :guilabel:`Download hci_rpmsg_nrf5340_netcore`).

   .. figure:: images/ses_nrf5340_netcore_flash_active.png
     :alt: Program the network sample hci_rpmsg_nrf5340_netcore

     Program the network sample hci_rpmsg_nrf5340_netcore

   Ignore any warnings regarding the project being out of date.
   The network core project is a pure programming target, so it cannot be built.

   .. figure:: images/ses_nrf5340_netcore_download.png
     :alt: Ignore any 'Project out of date' warning

     Ignore any 'Project out of date' warning

   .. note:: Programming the network core erases the application.

#. After the network core is programmed, make the application target active again by selecting :guilabel:`Project` > :guilabel:`Set Active Project` > :guilabel:`zephyr/merged.hex`.

   .. figure:: images/ses_nrf5340_appcore_set_active.png
     :alt: Set the zephyr/merged.hex target as active

     Set the zephyr/merged.hex target as active
#. Program the application sample using :guilabel:`Target` > :guilabel:`Download zephyr/merged.hex`.


Using the command line
======================

To build nRF5340 samples from the command line, use :ref:`west <zephyr:west>`.
To program the nRF5340 DK from the command line, use either west or nrfjprog (which is part of the `nRF Command Line Tools`_).

.. note::
   Programming the nRF5340 DK from the command line (with west or nrfjprog) requires the `nRF Command Line Tools`_ v10.12.0 or later.


Separate images
---------------

To build and program the application sample and the network sample as separate images, follow the instructions in :ref:`gs_programming_cmd` for each of the samples.

See the following instructions for programming the images separately:

.. tabs::

   .. group-tab:: west

      First, open a command prompt in the build folder of the network sample and enter the following command to erase the flash memory of the network core and program the network sample::

        west flash --erase

      Then navigate to the build folder of the application sample and enter the same command to erase the flash memory of the application core and program the application sample::

        west flash --erase

   .. group-tab:: nrfjprog

      First, open a command prompt in the build folder of the network sample and enter the following command to erase the flash memory of the network core and program the network sample::

        nrfjprog -f NRF53 --coprocessor CP_NETWORK --program zephyr/zephyr.hex --chiperase

      Then navigate to the build folder of the application sample and enter the following command to erase the flash memory of the application core and program the application sample::

        nrfjprog -f NRF53 --program zephyr/zephyr.hex --chiperase

      Finally, reset the development kit::

        nrfjprog --pinreset

See :ref:`readback_protection_error` if you encounter an error.

Multi-image build
-----------------

To build and program a multi-image HEX file, follow the instructions in :ref:`gs_programming_cmd` for the application core sample.

To program the multi-image HEX file, you must use west.
Programming multi-image builds for different cores is not yet supported in nrfjprog.

.. _readback_protection_error:

Readback protection
-------------------

When programming the device, you might get an error similar to the following message::

    ERROR: The operation attempted is unavailable due to readback protection in
    ERROR: your device. Please use --recover to unlock the device.

This error occurs when readback protection is enabled.
To disable the readback protection, you must *recover* your device.
See the following instructions.

.. tabs::

   .. group-tab:: west

      Enter the following command to recover both cores::

        west flash --recover

   .. group-tab:: nrfjprog

      Enter the following commands to recover first the network core and then the application core::

        nrfjprog --recover --coprocessor CP_NETWORK
        nrfjprog --recover

      .. note::
         Make sure to recover the network core before you recover the application core.

         The ``--recover`` command erases the flash memory and then writes a small binary into the recovered flash memory.
         This binary prevents the readback protection from enabling itself again after a pin reset or power cycle.

         Recovering the network core erases the flash memory of both cores.
         Recovering the application core erases only the flash memory of the application core.
         Therefore, you must recover the network core first.
         Otherwise, if you recover the application core first and the network core last, the binary written to the application core is deleted and readback protection is enabled again after a reset.


.. include:: ug_nrf52.rst
   :start-after: fota_upgrades_start
   :end-before: fota_upgrades_end

.. _logging_cpunet:

Getting logging output
**********************

When connected to the computer, the nRF5340 DK emulates three virtual COM ports.
In the default configuration:

* Logging output from the application core sample is available on the third (last) COM port.
* Logging output from the network core (if available) is routed to the first COM port.
* The second (middle) COM port is silent.
