.. _ug_thingy53:

Working with Thingy:53
######################

.. contents::
   :local:
   :depth: 2

Nordic Thingy:53 is a battery-operated prototyping platform for IoT Systems.

Thingy:53 integrates the following components:

* nRF5340 SoC - Supporting Bluetooth® Low Energy, IEEE 802.14.5 based protocols and Near Field Communication (NFC).
* nRF21540 RF FEM - Range extender with integrated power amplifier (PA)/low-noise amplifier (LNA).
* nPM1100 PMIC - Power Management IC with integrated dual-mode buck regulator and battery charger.

For more information on the product, see the `Thingy:53 product page`_ and the `Nordic Thingy:53 User Guide`_.
The |NCS| provides support for developing applications on the Thingy:53.

.. _thingy53_serialports:

Connecting to Thingy:53 serial ports
************************************

By default, Thingy:53 applications and samples use a serial terminal to provide logs.
The serial terminal is accessible through USB CDC ACM class.
The serial port is visible right after the Thingy:53 is connected to the host using a USB cable.
The CDC ACM baudrate is ignored, and transfer goes with USB speed.

.. tip::

   Some of the samples and application compatible with Thingy:53 may provide multiple instances of USB CDC ACM class.
   In that case, the first instance is used to provide logs.

Firmware
********

Nordic Thingy:53 comes preprogrammed with the Edge Impulse firmware.
This allows the Thingy:53 to connect to Edge Impulse Studio over Bluetooth LE using the nRF Edge Impulse mobile application, or through USB on your PC.
The firmware allows you to capture sensor data, which in turn you can use to train and test machine learning models in Edge Impulse Studio.
You can deploy trained machine learning models to the Nordic Thingy:53 through an over-the-air (OTA) update using Bluetooth LE, or update over USB using nRF Connect Programmer.

See the `Nordic Semi Thingy:53 page`_ in the `Edge Impulse`_ documentation on how to use the Nordic Thingy:53 with the nRF Edge Impulse mobile application and Edge Impulse Studio.

There are also multiple samples in the |NCS| that you can run on the Nordic Thingy:53.

.. _thingy53_precompiled_fw:

Programming precompiled firmware images
***************************************

Precompiled firmware image files are useful in the following scenarios:

* Restoring the firmware to its initial image.
* Updating the application firmware to a newly released version.
* Testing different features of the Nordic Thingy:53.

Downloading precompiled firmware images
=======================================

To obtain precompiled images for updating the firmware, perform the following steps:

1. Go to the `Thingy:53 product page`_ and under the :guilabel:`Downloads` tab, navigate to :guilabel:`Precompiled applications`.
#. Download and extract the latest Thingy:53 firmware package.
#. Check the :file:`CONTENTS.txt` file in the extracted folder for the location and names of the different firmware images.

Quick programming of precompiled firmware images
================================================

When you have the precompiled images ready, you can program the images directly onto the Thingy:53 using the nRF Connect Programmer application available in nRF Connect for Desktop.

The Thingy:53 is connected directly to your PC through USB.
This method uses the :doc:`mcuboot:index` feature and the built-in serial recovery mode of Thingy:53.

If the application on the Thingy:53 supports it, you can also use the nRF Programmer mobile application for Android or iOS to update the firmware over-the-air (OTA) using Bluetooth Low Energy.
All precompiled images support OTA using Bluetooth.

Alternatively, you can use an external debug probe, such as the nRF5340 DK, or any J-Link device supporting ARM Cortex-M33 to program firmware on a Thingy:53.

See `Updating Nordic Thingy:53 firmware`_ for the detailed procedures on how to program a Thingy:53 using nRF Connect Programmer or the nRF Programmer for Android or iOS.

.. _thingy53_building_pgming:

Building and programming from the source code
*********************************************

You can also program the Thingy:53 by using the images obtained by building the code in an |NCS| environment.

To set up your system to be able to build a compatible firmware image, follow the :ref:`getting_started` guide for |NCS|.
The build targets of interest for Thingy:53 in |NCS| are listed on the table below.
Note that building for the non-secure application core is not fully supported.

.. note::

   The |NCS| samples and applications that are fully compatible with Thingy:53 follow the :ref:`thingy53_app_guide`.
   This is needed, because the samples must be compatible with partition map and bootloader that is used by the Thingy:53 in NCS.

+--------------------------------+----------------------------------------------------------+
|Component                       |  Build target                                            |
+================================+==========================================================+
| nRF5340 SoC - Application core |``thingy53_nrf5340_cpuapp`` for the secure domain         |
|                                |                                                          |
|                                |``thingy53_nrf5340_cpuapp_ns`` for the non-secure version |
+--------------------------------+----------------------------------------------------------+
| nRF5340 SoC - Network core     |``thingy53_nrf5340_cpunet``                               |
+--------------------------------+----------------------------------------------------------+

|NCS| by default uses :ref:`ug_multi_image` for Thingy:53.
Because of this, when you choose :file:`thingy53_nrf5340_cpuapp` when building a sample or application that is fully compatible with Thingy:53, you will generate firmware for both the application core and network core:

* The applicaton core consists of MCUboot bootloader and an application image.
* The network core firmware consists of network core bootloader (B0n) and application firmware of the network core.

The build process results in generating firmware in two formats:

* HEX file (:file:`merged_domains.hex`) - Used by an external debug probe and nRF Connect Programmer.
  The HEX file contains bootloaders and applications for both cores.
* BIN files (:file:`app_update.bin`, :file:`net_core_app_update.bin`), containing signed application firmwares for the application and network core, respectively.
  The BIN files can be used to update running application using MCUboot bootloader, with either MCUboot serial recovery or OTA DFU using Bluetooth LE.

The table below shows the relevant types of build files that are generated and the different scenarios in which they are used.
The :file:`merged.hex` contains bootloader and firmware only for the application core, and is usually should not be used.

+---------------------------------+--------------------------------------+--------------------------------------------------------------------+
| File                            | File format                          | Programming scenario                                               |
+=================================+======================================+====================================================================+
| :file:`merged_domain.hex`       | Full image, HEX format               | Using an external debug probe and nRF Connect Programmer           |
+---------------------------------+--------------------------------------+--------------------------------------------------------------------+
| :file:`app_update.bin`          | MCUboot compatible image, BIN format | Using the built-in bootloader and the mcumgr command line tool     |
+---------------------------------+--------------------------------------+--------------------------------------------------------------------+
| :file:`net_core_app_update.bin` | MCUboot compatible image, BIN format | Using the built-in bootloader and the mcumgr command line tool     |
+---------------------------------+--------------------------------------+--------------------------------------------------------------------+
| :file:`dfu_application.zip`     | MCUboot compatible image, binary     | Using nRF Programmer for Android and iOS or nRF Connect Programmer |
+---------------------------------+--------------------------------------+--------------------------------------------------------------------+

There are multiple methods of programming a sample or application onto a Thingy:53.
You can choose the method based on the availability or absence of an external debug probe to program.

.. note::

   If you do not have an external debug probe available to program the Thingy:53, you can directly program compatible samples to the device over USB using the built-in MCUboot bootloader and nRF Connect Programmer.
   In this scenario, use the :file:`dfu_application.zip` file.

   Some compatible samples:

   * :ref:`nrf_machine_learning_app`
   * :ref:`matter_weather_station_app`
   * :ref:`peripheral_lbs`
   * :ref:`peripheral_uart`
   * :ref:`bluetooth_mesh_light`
   * :ref:`bluetooth_mesh_light_switch`

.. _thingy53_app_guide:

Application guide
=================

lorem ipsum

.. _thingy53_build_pgm_segger:

Building and programming using SEGGER Embedded Studio
*****************************************************

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

   The output of the build with the merged HEX file containing the application, the net core image and the bootloader is located in the :file:`zephyr` subfolder in the build directory.

#. Program the sample or application:

   a. Connect the Thingy:53 to the debug out port on a 10-pin external debug probe, for example nRF5340 DK (Development Kit), using a 10-pin JTAG cable.
   #. Connect the external debug probe to the PC using a USB cable.
   #. Make sure that the Thingy:53 and the external debug probe are powered on.
   #. in SES, select :guilabel:`Target` > :guilabel:`Connect J-Link`.
   #. Select :guilabel:`Target` > :guilabel:`Download zephyr/merged.hex` to program the sample or application onto Thingy:53.
   #. The device will reset and run the programmed sample or application.

Building and programming using Visual Studio Code
*************************************************

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
********************************************

Before you start building |NCS| projects on the command line, you must :ref:`build_environment_cli`.

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
