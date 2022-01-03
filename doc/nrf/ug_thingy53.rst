.. _ug_thingy53:

Working with Thingy:53
######################

.. contents::
   :local:
   :depth: 2

Nordic Thingy:53 is a battery-operated prototyping platform for IoT Systems.

.. note::
  |thingy53_unreleased|

Thingy:53 integrates the following components:

* nRF5340 SoC - Supporting Bluetooth® Low Energy, IEEE 802.15.4 based protocols and Near Field Communication (NFC).
* nRF21540 RF FEM - Range extender with integrated power amplifier (PA)/low-noise amplifier (LNA).
* nPM1100 PMIC - Power Management IC with integrated dual-mode buck regulator and battery charger.

.. _thingy53_serialports:

Connecting to Thingy:53 serial ports
************************************

Thingy:53 applications and samples use a serial terminal to provide logs.
By default, the serial terminal is accessible through the USB CDC ACM class handled by application firmware.
The serial port is visible right after the Thingy:53 is connected to the host using a USB cable.
The CDC ACM baudrate is ignored, and transfer goes with USB speed.

.. tip::
   Some of the samples and applications compatible with Thingy:53 may provide multiple instances of USB CDC ACM class.
   In that case, the first instance is used to provide logs, and the others are used for application-specific usages.

Firmware
********

Nordic Thingy:53 comes preprogrammed with the Edge Impulse firmware.
This allows the Thingy:53 to connect to Edge Impulse Studio over Bluetooth LE using the nRF Edge Impulse mobile application, or through USB on your PC.
The firmware allows you to capture sensor data, which in turn you can use to train and test machine learning models in Edge Impulse Studio.
You can deploy trained machine learning models to the Nordic Thingy:53 through an over-the-air (OTA) update using Bluetooth LE, or update over USB using `nRF Connect Programmer`_.

There are also multiple samples in the |NCS| that you can run on the Nordic Thingy:53.

.. _thingy53_precompiled_fw:

Programming precompiled firmware images
***************************************

.. note::
   As the Thingy:53 is not officially released yet, you cannot download the precompiled images.

Precompiled firmware image files are useful in the following scenarios:

* Restoring the firmware to its initial image.
* Updating the application firmware to a newly released version.
* Testing different features of the Nordic Thingy:53.

Downloading precompiled firmware images
=======================================

To obtain precompiled images for updating the firmware, perform the following steps:

1. Go to the Thingy:53 product page (when available) and under the :guilabel:`Downloads` tab, navigate to :guilabel:`Precompiled applications`.
#. Download and extract the latest Thingy:53 firmware package.
#. Check the :file:`CONTENTS.txt` file in the extracted folder for the location and names of the different firmware images.

Quick programming of precompiled firmware images
================================================

After downloading firmware images, you can program them directly onto the Thingy:53 using the `nRF Connect Programmer`_ application available in nRF Connect for Desktop.
The downloaded firmware package is a zip archive containing signed application binaries.

See :ref:`thingy53_app_update` for detailed information about updating firmware on Thingy:53.

.. _thingy53_building_pgming:

Building and programming from the source code
*********************************************

You can also program the Thingy:53 by using the images obtained by building the code in the |NCS| environment.

To set up your system to be able to build a compatible firmware image, follow the :ref:`getting_started` guide for the |NCS|.
The build targets of interest for Thingy:53 in the |NCS| are listed on the table below.
Note that building for the non-secure application core is not fully supported.

+--------------------------------+----------------------------------------------------------+
|Component                       |  Build target                                            |
+================================+==========================================================+
| nRF5340 SoC - Application core |``thingy53_nrf5340_cpuapp`` for the secure domain         |
|                                |                                                          |
|                                |``thingy53_nrf5340_cpuapp_ns`` for the non-secure version |
+--------------------------------+----------------------------------------------------------+
| nRF5340 SoC - Network core     |``thingy53_nrf5340_cpunet``                               |
+--------------------------------+----------------------------------------------------------+

.. note::
   The |NCS| samples and applications that are compatible with Thingy:53 follow the :ref:`thingy53_app_guide`.
   This is needed, because the samples must use consistent partition map and bootloader configuration.

The |NCS| by default uses :ref:`ug_multi_image` for Thingy:53.
Because of this, when you choose ``thingy53_nrf5340_cpuapp`` when building a sample or application that is compatible with Thingy:53, you will generate firmware for both the application core and network core:

* The applicaton core firmware consists of MCUboot bootloader and an application image.
* The network core firmware consists of network core bootloader (B0n) and application firmware of the network core.

The build process generates firmware in two formats:

* Intel Hex file (:file:`merged_domains.hex`) - Used with an external debug probe.
  The file contains bootloaders and applications for both cores.
* Binary files (:file:`app_update.bin`, :file:`net_core_app_update.bin`), containing signed application firmwares for the application and network core, respectively.
  For convenience, the binary files are bundled in :file:`dfu_application.zip` together with manifest that describes them.
  The binary files or the combined zip archive can be used to update application firmware for both cores, with either MCUboot serial recovery or OTA DFU using Bluetooth LE.

The following table shows the relevant types of build files that are generated and the different scenarios in which they are used.

+---------------------------------+-------------------------------------------------+--------------------------------------------------------------------+
| File                            | File format                                     | Programming scenario                                               |
+=================================+=================================================+====================================================================+
| :file:`merged_domain.hex`       | Full image for both cores                       | Using an external debug probe and nRF Connect Programmer           |
+---------------------------------+-------------------------------------------------+--------------------------------------------------------------------+
| :file:`app_update.bin`          | MCUboot compatible application core update      | Using the built-in bootloader and nRF Connect Programmer           |
+---------------------------------+-------------------------------------------------+--------------------------------------------------------------------+
| :file:`net_core_app_update.bin` | MCUboot compatible network core update          | Using the built-in bootloader and nRF Connect Programmer           |
+---------------------------------+-------------------------------------------------+--------------------------------------------------------------------+
| :file:`dfu_application.zip`     | MCUboot compatible update images for both cores | Using nRF Programmer for Android and iOS or nRF Connect Programmer |
+---------------------------------+-------------------------------------------------+--------------------------------------------------------------------+

See the following sections for details regarding building and programming the firmware for Thingy:53 in various environments.
If your Thingy:53 is already programmed with Thingy:53-compatible sample or application, then you can also use the MCUboot bootloader to update the firmware after you finish building.
See :ref:`thingy53_app_update` for more detailed information about updating firmware on Thingy:53.

.. _thingy53_build_pgm_segger:

Building and programming using SEGGER Embedded Studio
=====================================================

.. include:: gs_programming.rst
   :start-after: build_SES_projimport_open_start
   :end-before: build_SES_projimport_open_end

..

   .. figure:: images/ses_thingy53_configuration.png
      :alt: Opening the peripheral_lbs application

      Opening the ``peripheral_lbs`` application for the ``thingy53_nrf5340_cpuapp`` build target

   .. note::

      The ``Board Directory`` folder is at ``ncs/nrf/boards/arm``.

4. Click :guilabel:`OK` to import the project into SES.
   You can now work with the project in IDE.
#. Build the sample or application:

   a. Select your project in the Project Explorer.
   #. From the menu, select :guilabel:`Build` > :guilabel:`Build Solution`.
      This builds the project.

   The output of the build with the merged Intel Hex file, containing the application, the net core image, and the bootloaders, is located in the :file:`zephyr` subfolder in the build directory.

#. Program the sample or application:

   a. Connect the Thingy:53 to the debug out port on a 10-pin external debug probe, for example nRF5340 DK (Development Kit), using a 10-pin JTAG cable.
   #. Connect the external debug probe to the PC using a USB cable.
   #. Make sure that the Thingy:53 and the external debug probe are powered on.
   #. in SES, select :guilabel:`Target` > :guilabel:`Connect J-Link`.
   #. Select :guilabel:`Target` > :guilabel:`Download zephyr/merged.hex` to program the sample or application onto Thingy:53.
   #. The device will reset and run the programmed sample or application.

.. warning::
   Programming :file:`merged.hex` file updates only the application core.
   If you also need to update the network core, you must follow additional steps described in :ref:`ug_nrf5340`.

Building and programming using Visual Studio Code
=================================================

Complete the following steps after installing the `nRF Connect for Visual Studio Code`_ extension in Microsoft Visual Studio Code:

1. Open Visual Studio code.
#. Click on the :guilabel:`nRF Connect for VS Code` icon in the left navigation bar and then click :guilabel:`Open`.

   .. figure:: images/ug_thingy53_vscode_welcome.png
      :alt: nRF Connect for VS Code welcome page
      :width: 80%

      The welcome page for nRF Connect for VS Code.

#. Click on :guilabel:`Add an existing applicaton to workspace...`.
#. In the prompt, navigate to the folder containing the sample you want to build, such as :file:`nrf\\samples\\bluetooth\\peripheral_lbs`.
   You should now see the selected application in the :guilabel:`Applications` window in the lower left corner.

   .. note::

     The sample folder must contain a :file:`prj.conf` file.

   .. figure:: images/ug_thingy53_vscode_appwindow.png
      :alt: nRF Connect for VS Code, application shown in Application window
      :width: 80%

      The peripheral_lbs application is now available in the :guilabel:`Application` window.

#. Click on the :guilabel:`Add Build Configuration` button in the :guilabel:`Application` window, or click on the text stating :guilabel:`No build configurations. Click to create one`.
   This opens the :guilabel:`Generate Configuration` window in a new tab.
#. Select the ``thingy53_nrf5340_cpuapp`` as the target board and click :guilabel:`Generate Config`, which generates the configuration file and triggers the build process.

   .. figure:: images/ug_thingy53_vscode_genconfig1.png
      :alt: nRF Connect for VS Code, Generate Configuration window
      :width: 80%

      The :guilabel:`Generate Configuration` window, with ``thingy53_nrf5340_cpuapp`` as the board.

#. When the build configuration and the build are complete, an :guilabel:`Actions` window appears in the lower left corner of Visual Studio Code.
   In this window, you can trigger the build process, program the built sample or start a debug session.

   .. figure:: images/ug_thingy53_vscode_genconfig2.png
      :alt: nRF Connect for VS Code, Actions window
      :width: 80%

      The :guilabel:`Actions` window is open in the lower left corner

#. Program the sample or application:

   a. Connect the Thingy:53 to the debug out port on a 10-pin external debug probe, for example nRF5340 DK, using a 10-pin JTAG cable.
   #. Connect the external debug probe to the PC using a USB cable.
   #. Make sure that the Thingy:53 and the external debug probe are powered on.
   #. Click :guilabel:`Flash` in the :guilabel:`Actions` window.

Building and programming on the command line
============================================

Before you start building an |NCS| project on the command line, you must :ref:`build_environment_cli`.

To build and program the source code from the command line, complete the following steps:

1. Open a command line or terminal window.
#. Go to the specific directory for the sample or application.

   For example, the full directory path is :file:`ncs/nrf/applications/machine_learning` when building the source code for the :ref:`nrf_machine_learning_app` application.

#. Make sure that you have the required version of the |NCS| repository by pulling the ``sdk-nrf`` repository on GitHub as described in :ref:`dm-wf-get-ncs` and :ref:`dm-wf-update-ncs` sections.
#. Get the rest of the dependencies using west::

      west update

#. Build the sample or application code as follows:

   .. parsed-literal::
      :class: highlight

      west build -b thingy53_nrf5340_cpuapp -d *destination_directory_name*

#. Program the sample or application:

   a. Connect the Thingy:53 to the debug out port on a 10-pin external debug probe, for example nRF5340 DK, using a 10-pin JTAG cable.
   #. Connect the external debug probe to the PC using a USB cable.
   #. Make sure that the Thingy:53 and the external debug probe are powered on.
   #. Use the following command to program the sample or application to the device::

         west flash

   The device resets and runs the programmed sample or application.

.. _thingy53_app_update:

Updating firmware image
=======================

The firmware on Thingy:53 can be programmed using an external debug probe and 10-pin JTAG cable, as described in the sections above for building in the different environments.
You can also update applications running on both network and application core using the built-in MCUboot bootloader and `nRF Connect Programmer`_ or the nRF Programmer for Android or iOS.
Prebuilt application images can also be updated that way.
See subsections below for details about updating firmware image.

Firmware update using external debug probe
------------------------------------------

If you are using an external debug probe, such as the nRF5340 DK, or any J-Link device supporting ARM Cortex-M33, you do not have to use applications that follow the :ref:`thingy53_app_guide`.
In that case, you can program the Thingy:53 in a similar way as nRF53 DK.
See :ref:`ug_nrf5340` for details.
Keep in mind that you will need to program a sample or application compatible with Thingy:53 application guide to bring back serial recovery and DFU OTA.

.. _thingy53_app_update_mcuboot:

Firmware update using MCUboot bootloader
----------------------------------------

The Thingy:53-compatible samples and applications use MCUboot bootloader than can be used to update firmware.
This method uses signed binary files :file:`app_update.bin` and :file:`net_core_app_update.bin` (or :file:`dfu_application.zip`).
You can program the precompiled firmware image using one of the following ways:

* Use the :doc:`mcuboot:index-ncs` feature and the built-in serial recovery mode of Thingy:53.
  In this scenario Thingy is connected directly to your PC through USB.
* Update the firmware over-the-air (OTA) using Bluetooth LE and the nRF Programmer mobile application for Android or iOS.
  To use this method, the application that is currently programmed on Thingy:53 must support it.
  All precompiled images support OTA using Bluetooth.

See `Updating Nordic Thingy:53 firmware`_ for the detailed procedures on how to program a Thingy:53 using `nRF Connect Programmer`_ or the nRF Programmer for Android or iOS.

.. _thingy53_app_guide:

Thingy:53 application guide
***************************

Thingy:53 does not have a built-in J-Link debug IC.
Because of that, the samples and applications that are compatible with Thingy:53 by default use MCUboot bootloader with serial recovery support.
Thingy:53-compatible applications can be updated using DFU functionality with either `nRF Connect Programmer`_ or nRF Programmer mobile application.

Enabling DFU and MCUboot requires consistency in configuration of the samples and applications.
For a sample or application to be compatible with Thingy:53, the application must comply with the following configuration:

Partition manager configuration
  Thingy:53 compatible samples use :ref:`partition_manager` to define memory partitions.
  To ensure that partition layout does not change between builds, the sample must use static partition layout that is the same as the one in Thingy:53-compatible samples in the |NCS|.
  The memory layout must stay consistent, so that MCUboot can perform proper image updates and clean up the settings storage partition.

  The PCD SRAM partition is locked by the MCUboot bootloader to prevent the application from modifying the network core firmware.
  Trying to access data on this partition results in an ARM fault.

MCUboot bootloader
  The sample must enable MCUboot bootloader and use the same configuration of the bootloader as all of the Thingy:53-compatible samples in the |NCS|.
  This is needed to let MCUboot use the sample as an image update.

  The MCUboot bootloader supports serial recovery and a custom command to erase settings storage partition.
  Erasing settings parition is needed to ensure that an application would not be booted with incompatible content of settings partition.

In addition to the configurations mentioned above, an application compatible with Thingy:53 is expected to enable the following features:

* USB CDC ACM as backend for logger.
  The logs are provided using USB CDC ACM to allow accessing them without additional hardware.

  Most of the Thingy:53-compatible applications and samples use only a single instance of USB CDC ACM that works as logger's backend.
  No other USB classes are used.
  These samples can share common USB product name, vendor ID, and product ID.
  If a sample supports additional USB classes or more than one instance of USB CDC ACM, it must use dedicated product name, vendor ID, and product ID.

* DFU over-the-air through Simple Management Protocol over Bluetooth.
  The application acts as GATT server and allows the connected Bluetooth Central to perform the firmware update for both application and network core.
  The Bluetooth configuration is updated to allow quick DFU data transfer.
  The application supports Simple Management Protocol (SMP) handlers related to:

  * Image management
  * Operating System (OS) management -- used to reboot device after firmware upload is complete.
  * Erasing settings partition -- needed to ensure that a new application would not be booted with incompatible content in the settings partition that was written by the previous application.

.. tip::
   To use your application with Thingy:53 preprogrammed bootloader, you only must set proper configuration of Partition Manager and MCUboot.
   Other mentioned features are nice to have, but they are not mandatory.

   All of the Thingy:53-compatible applications in the |NCS| also support the mentioned additional features.
   Refer to the source code of the samples and applications for examples of implementation.

Samples and applications compatible with Thingy:53
==================================================

There are already samples and applications in the |NCS| that comply with Thingy:53 application guide.
Some of the samples that are compatible with Thingy:53 are:

* :ref:`nrf_machine_learning_app`
* :ref:`matter_weather_station_app`
* :ref:`peripheral_lbs`
* :ref:`peripheral_uart`
* :ref:`bluetooth_mesh_light`
* :ref:`bluetooth_mesh_light_switch`
