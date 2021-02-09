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

nRF5340 is a wireless ultra-low power multicore System on Chip (SoC) with two fully programmable Arm Cortex-M33 processors: a network core and an application core.
The |NCS| supports Bluetooth Low Energy and NFC communication on the nRF5340 SoC.

See the `nRF5340 Product Specification`_ for more information about the nRF5340 SoC.
:ref:`zephyr:nrf5340dk_nrf5340` gives an overview of the nRF5340 DK support in Zephyr.

Network core
============

The network core is an Arm Cortex-M33 processor with a reduced feature set, designed for ultra-low power operation.

This core is used for radio communication.
With regards to the nRF5340 samples, this means that the network core runs the radio stack and real-time procedures.

Currently, the |NCS| provides the following solutions for the network core:

* :ref:`ug_ble_controller` (both the SoftDevice Controller and the Zephyr Bluetooth LE Controller)
* IEEE 802.15.4 radio driver (for Thread and Zigbee protocols)
* Samples that directly use the radio peripheral

See `Network samples`_ for more information.

In general, this core should be used for real-time processing tasks involving low-level radio protocol layers.

The board name for the network core in Zephyr is ``nrf5340dk_nrf5340_cpunet``.

Application core
================

The application core is a full-featured Arm Cortex-M33 processor including DSP instructions and FPU.

Currently, the |NCS| provides the following solutions for the application core:

* High-level radio stack (the host part of the Bluetooth Low Energy, Thread, and Zigbee stacks) and application logic
* Samples running only on the application core (for example, NFC samples with nRF5340 support)

See `Application samples`_ for more information.

In general, this core should be used for tasks that require high performance and for application-level logic.

The M33 TrustZone divides the application MCU into secure and non-secure domains.
When the MCU boots, it always starts executing from the secure area.
The secure bootloader chain starts the :ref:`secure_partition_manager` sample, which configures a part of memory and peripherals to be non-secure and then jumps to the main application located in the non-secure area.

In Zephyr, :ref:`zephyr:nrf5340dk_nrf5340` is divided into two different build targets:

* ``nrf5340dk_nrf5340_cpuapp`` for the secure domain
* ``nrf5340dk_nrf5340_cpuappns`` for the non-secure domain

When built for the ``nrf5340dk_nrf5340_cpuappns`` build target, the :ref:`secure_partition_manager` sample is automatically included in the build.

.. tfm_support_start

Trusted Firmware-M (TF-M) support
---------------------------------

You can use Trusted Firmware-M (TF-M) as an alternative to :ref:`secure_partition_manager` for running an application from the non-secure area of the memory.

For more information and instructions on how to do this, see :ref:`ug_tfm`.

.. tfm_support_finish

Inter-core communication
========================

Communication between the application core and the network core happens through a shared memory area.
The application core memory is mapped to the network core memory map.
This means that the network core can access and use the application core memory for shared memory communication.

Interprocessor Communication (IPC) is used to indicate to the other core that there is new data available to pick up.
The actual data exchange is handled by Open Asymmetric Multi Processing (OpenAMP).

Zephyr includes the `OpenAMP`_ library, which provides a complete solution for exchanging messages between the cores.
The IPC peripheral is presented to Zephyr as an Interprocessor Mailbox (IPM) device.
The OpenAMP library uses the IPM SHIM layer, which in turn uses the IPC driver in `nrfx`_.

.. |note| replace:: To upgrade firmware on the network core, perform the steps for FOTA upgrade described below, replacing :file:`app_update.bin` (file used when upgrading firmware on the application core) with :file:`net_core_app_update.bin`.
   In addition, ensure that :option:`CONFIG_PCD` is enabled for the MCUBoot child image.
   For more details, see :ref:`nc_bootloader`.


.. include:: ug_nrf52.rst
   :start-after: fota_upgrades_start
   :end-before: fota_upgrades_end


Available samples
*****************

nRF5340 samples usually consist of two separate images: one that runs on the network core and one that runs on the application core.

Most samples that support nRF5340 are not dedicated exclusively to this device, but can also be built for other chips that have the radio peripheral (for example, the nRF52 Series).
The following subsections give a general overview of what samples can run on nRF5340.

Network samples
===============

The nRF5340 network core supports samples that directly use the radio peripheral, for example, :ref:`radio_test`.

Bluetooth Low Energy
--------------------

For Bluetooth Low Energy, the |NCS| provides the :ref:`zephyr:bluetooth-hci-rpmsg-sample` sample.
This Zephyr sample is designed specifically to enable the Bluetooth LE Controller functionality on a remote MCU (for example, the nRF5340 network core) using the `RPMsg Messaging Protocol`_ as a transport for Bluetooth HCI.
The sample implements the RPMsg transport using the `OpenAMP`_ library to communicate with a Bluetooth Host stack that runs on a separate core (for example, the nRF5340 application core).

This sample must be programmed to the network core to run standard Bluetooth Low Energy samples on nRF5340.
You can choose whether to use the SoftDevice Controller or the Zephyr Bluetooth LE Controller for this sample.
See :ref:`ug_ble_controller` for more information.

You might need to adjust the Kconfig configuration of this sample to make it compatible with the peer application.
For example:

* :option:`CONFIG_BT_MAX_CONN` must be greater than or equal to the maximum number of connections configured for the Bluetooth Host in the application core firmware.
* If the application sample uses specific Bluetooth LE functionalities, these functionalities must be enabled in the network sample as well.
  For example, you must modify the configuration of the network sample to make it compatible with the :ref:`ble_throughput` sample::

    CONFIG_BT_CTLR_TX_BUFFER_SIZE=251
    CONFIG_BT_CTLR_DATA_LENGTH_MAX=251

  This configuration guarantees that the network sample can handle the Bluetooth LE DLE update procedure, which is used in the :ref:`ble_throughput` sample.

IEEE 802.15.4 (Thread and Zigbee)
---------------------------------

For IEEE 802.15.4, the nRF Connect SDK makes use of Zephyr's :ref:`zephyr:nrf-ieee802154-rpmsg-sample` sample.
This Zephyr sample is designed specifically to enable the nRF IEEE 802.15.4 radio driver and its serialization library on a remote MCU (for example, the nRF5340 network core) using the `RPMsg Messaging Protocol`_ as a transport for the nRF 802.15.4 radio driver serialization.
The sample implements the RPMsg transport using the `OpenAMP`_ library to communicate with the nRF IEEE 802.15.4 radio driver serialization host that runs on a separate core (for example, the nRF5340 application core).

When working with 802.15.4-based protocols like Thread or Zigbee, program the :ref:`zephyr:nrf-ieee802154-rpmsg-sample` sample to the network core to run :ref:`Thread and Zigbee samples <samples>` on nRF5340.

Application samples
===================

The |NCS| provides a series of :ref:`Bluetooth Low Energy samples <ble_samples>`, in addition to the :ref:`Bluetooth samples in Zephyr <zephyr:bluetooth-samples>`.
Most of these samples should run on the nRF5340 DK, but not all have been thoroughly tested.

Some Bluetooth LE samples require configuration adjustments to the :ref:`zephyr:bluetooth-hci-rpmsg-sample` sample as described in the `Network samples`_ section.

The |NCS| also provides :ref:`Thread <ug_thread>`, :ref:`Zigbee <ug_zigbee>`, and :ref:`CHIP <ug_chip>` samples that can be run on the nRF5340 DK.
These samples are built for the application core, but make use of both cores.
The network core application is built and flashed automatically.

Additionally, the |NCS| provides :ref:`NFC samples <nfc_samples>` that are available for nRF5340.
These samples run only on the application core and do not require any firmware for the network core.

When programming any of these samples to the application core, configure :option:`CONFIG_BOARD_ENABLE_CPUNET` to select whether the network core should be enabled.
When radio protocols (Bluetooth LE, IEEE 802.15.4) are used, this option is enabled by default.

.. _ug_nrf5340_building:

Building and programming a sample
*********************************

Depending on the sample, you must program only the application core (for example, when using NFC samples) or both the network and the application core.

.. note::
   On nRF5340, the application core is responsible for starting the network core and connecting its GPIO pins.
   Therefore, to run any sample on nRF5340, the application core must be programmed, even if the firmware is supposed to run only on the network core, and the firmware for the application core must set :option:`CONFIG_BOARD_ENABLE_CPUNET` to ``y``.
   You can use the :ref:`nrf5340_empty_app_core` sample for this purpose.
   For details, see the code in :file:`zephyr/boards/arm/nrf5340dk_nrf5340/nrf5340_cpunet_reset.c`.

To program only the application core, follow the instructions in :ref:`gs_programming_ses` and use ``nrf5340dk_nrf5340_cpuapp`` or ``nrf5340dk_nrf5340_cpuappns`` as build target.

When programming both the application core and the network core, you can choose whether you want to build and program both images separately or combined as a :ref:`multi-image build <ug_multi_image>`.

In a multi-image build, the image for the application core is the parent image, and the image for the network core is treated as a child image in a separate domain.
For this to work, the network core image must be explicitly added as a child image to one of the application core images.
See :ref:`ug_multi_image_defining` for details.

The network sample :ref:`zephyr:bluetooth-hci-rpmsg-sample` is automatically added to all Bluetooth Low Energy samples in the |NCS|.
When :option:`CONFIG_BT_RPMSG_NRF53` is set to ``y`` (the default), the build system automatically includes the sample as a child image in the ``nrf5340_dk_nrf5340_cpunet`` core.

The network sample :ref:`nrf-ieee802154-rpmsg-sample` is built automatically by the |NCS| build system and included as a child image in the ``nrf5340_dk_nrf5340_cpunet`` core if your application is built for the application core and has the :option:`CONFIG_NRF_802154_SER_HOST` option enabled.
This option is enabled automatically for Thread and Zigbee samples in the |NCS|.

SES is unable to automatically program the network sample :ref:`zephyr:bluetooth-hci-rpmsg-sample` to the network core when it is added as a child image.
To program the network sample to the network core, see `Programming the network sample from SES`_.

A dedicated network sample can be built and flashed by following the instructions in :ref:`gs_programming_ses`.
Make sure to use ``nrf5340dk_nrf5340_cpunet`` as the build target when building the network sample, and ``nrf5340dk_nrf5340_cpuapp`` or ``nrf5340dk_nrf5340_cpuappns`` when building the application sample.

Programming the network sample from SES
=======================================

Follow the instructions in :ref:`gs_programming_ses`, use ``nrf5340dk_nrf5340_cpuapp`` or ``nrf5340dk_nrf5340_cpuappns`` as the build target, and open the Bluetooth Low Energy sample to work with.

Build the sample as described in :ref:`gs_programming_ses`.
This also creates the network core image.

.. note::
   You must reprogram the network core sample :ref:`zephyr:bluetooth-hci-rpmsg-sample` only when changes are made to it.
   You can modify and program the application core sample without reprogramming the network core.

Follow these steps to program the network sample :ref:`zephyr:bluetooth-hci-rpmsg-sample` to the network core when it is included as a child image:

1. Select :guilabel:`File` -> :guilabel:`New Project...`.

    .. figure:: images/ses_nrf5340_netcore_new_project.png
       :alt: Create New Project menu

       Create New Project menu

#. Select :guilabel:`Add the project to the current solution`.

    .. figure:: images/ses_nrf5340_netcore_add_project.png
       :alt: Adding a project target for programming the network core

       Adding a project target for programming the network core

#. Select the project template, project name, and project location.

   * :guilabel:`An externally built executable for Nordic Semiconductor nRF`:
     This allows to specify the network core hex file created by the build system to be flashed.

   * :guilabel:`Name`: Specify the name of the project as it will appear in SES. In this example, ``hci_rpmsg_nrf5340_netcore`` is used.

   * :guilabel:`Location`: Specify the location of the project.
     This must be the same location as the current project / build folder.
     Click :guilabel:`Browse` to open a dialog where you can navigate to the current project / build folder and click :guilabel:`Select Folder`.

   Click :guilabel:`Next`.

    .. figure:: images/ses_nrf5340_netcore_project_template.png
       :alt: Creating a new project for programming the network core

       Creating a new project for programming the network core

#. Configure the project settings.

   * :guilabel:`Target Processor`: Select ``nRF5340_xxAA_Network``.

   * :guilabel:`Load File`: Specify ``$(ProjectDir)/hci_rpmsg/zephyr/merged_CPUNET.hex``.
     This is the merged hex file for the network core that will be programmed.

   Click :guilabel:`Next`.

    .. figure:: images/ses_nrf5340_netcore_project_settings.png
       :alt: Project settings for programming the network core

       Project settings for programming the network core

#. Add the project files.

   This project will only be used for programming the network core so you must only add the default :guilabel:`Script Files`.

   Click :guilabel:`Next`

    .. figure:: images/ses_nrf5340_netcore_files.png
       :alt: Adding script files for programming the network core

       Adding script files for programming the network core


#. Add the project configurations.

   This project will only be used for programming the network core so no build configurations are needed.

   Deselect :guilabel:`Debug` and :guilabel:`Release`.

   Click :guilabel:`Finish`.

    .. figure:: images/ses_nrf5340_netcore_conf.png
       :alt: Deselecting configurations and finishing the configuration

       Deselecting configurations and finishing the configuration

   A new project has been created for programming the network core with the network sample :ref:`zephyr:bluetooth-hci-rpmsg-sample`.

#. To program the network sample, the ``hci_rpmsg_nrf5340_netcore`` project must be active.
   If the ``hci_rpmsg_nrf5340_netcore`` project is not the current active project, then set it as active using :guilabel:`Project` -> :guilabel:`Set Active Project` -> :guilabel:`hci_rpmsg_nrf5340_netcore`.

   .. figure:: images/ses_nrf5340_netcore_set_active.png
     :alt: Set the hci_rpmsg_nrf5340_netcore programming target as active

     Set the hci_rpmsg_nrf5340_netcore programming target as active

#. You can now program the network sample using :guilabel:`Target` -> :guilabel:`Download hci_rpmsg_nrf5340_netcore`.

   .. figure:: images/ses_nrf5340_netcore_flash_active.png
     :alt: Program the network sample hci_rpmsg_nrf5340_netcore

     Program the network sample hci_rpmsg_nrf5340_netcore

   Ignore any warnings regarding project being out of date.
   The ``hci_rpmsg_nrf5340_netcore`` is a pure programming target so it cannot build.

   .. figure:: images/ses_nrf5340_netcore_download.png
     :alt: Ignore any 'Project out of date' warning

     Ignore any 'Project out of date' warning

#. After the network core is programmed, make sure to make the application target active again.

   To do this, select :guilabel:`Project` -> :guilabel:`Set Active Project` -> :guilabel:`zephyr/merged.hex`.

   .. figure:: images/ses_nrf5340_appcore_set_active.png
     :alt: Set the zephyr/merged.hex target as active

     Set the zephyr/merged.hex target as active

.. note:: Flashing the network core erases the application which must be therefore programmed again.

Programming from the command line
=================================

To program the nRF5340 DK from the command line, you can use either :ref:`west <zephyr:west>` or nrfjprog (which is part of the `nRF Command Line Tools`_).

.. note::
   Programming the nRF5340 DK from the command line (with west or nrfjprog) requires the `nRF Command Line Tools`_ v10.12.0 or later.


Multi-image build
-----------------

To program a :ref:`multi-image <ug_multi_image>` HEX file that includes images for both the application sample and the network sample, you must use west.
Programming multi-image builds for different cores is not yet supported in nrfjprog.

.. tabs::

   .. group-tab:: west

      Open a command prompt in the build folder of the application sample and enter the following command to erase the whole flash and program both the application sample and the network sample::

        west flash --erase

See :ref:`readback_protection_error` if you encounter an error.


Separate images
---------------

If you built the application sample and the network sample as separate images, you must program them separately.

.. tabs::

   .. group-tab:: west

      First, open a command prompt in the build folder of the network sample and enter the following command to erase the flash of the network core and program the network sample::

        west flash --erase

      Then navigate to the build folder of the application sample and enter the same command to erase the flash of the application core and program the application sample::

        west flash --erase

   .. group-tab:: nrfjprog

      First, open a command prompt in the build folder of the network sample and enter the following command to erase the flash of the network core and program the network sample::

        nrfjprog -f NRF53 --coprocessor CP_NETWORK --program zephyr/zephyr.hex --chiperase

      Then navigate to the build folder of the application sample and enter the following command to erase the flash of the application core and program the application sample::

        nrfjprog -f NRF53 --program zephyr/zephyr.hex --chiperase

      Finally, reset the development kit::

        nrfjprog --pinreset

See :ref:`readback_protection_error` if you encounter an error.

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

         The ``--recover`` command erases the flash and then writes a small binary into the recovered flash.
         This binary prevents the readback protection from enabling itself again after a pin reset or power cycle.

         Recovering the network core erases the flash of both cores.
         Recovering the application core erases the flash of only the application core.
         Therefore, you must recover the network core first.
         Otherwise, if you recover the application core first and the network core last, the binary written to the application core is deleted and readback protection is enabled again after a reset.

.. _logging_cpunet:

Getting logging output
**********************

When connected to the computer, the nRF5340 DK emulates three virtual COM ports.
In the default configuration:

* Logging output from the application core sample is available on the third (last) COM port.
* Logging output from the network core (if available) is routed to the first COM port.
* The second (middle) COM port is silent.
