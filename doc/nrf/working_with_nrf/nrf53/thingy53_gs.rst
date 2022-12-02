.. _ug_thingy53_gs:

Getting started with Thingy:53
##############################

.. contents::
   :local:
   :depth: 2

This guide helps you get started with the Nordic Thingy:53.
It tells you how to update the firmware on the nRF5340 :term:`System on Chip (SoC)` on the Nordic Thingy:53, and how to get started with machine learning with Edge Impulse.

If you have already set up your Nordic Thingy:53 and want to learn more, see the following documentation:

* :ref:`ug_thingy53` for more advanced topics related to the Nordic Thingy:53.
* The :ref:`introductory documentation <getting_started>` for more information on the |NCS| and the development environment.

.. _thingy53_gs_requirements:

Minimum requirements
********************

Make sure you have all the required hardware and that your computer or mobile device has one of the supported operating systems.

Hardware
========

* :term:`Universal Serial Bus (USB)`-C cable
* Computer or mobile device

Software
========

If you are using a computer, one of the following operating systems:

* Microsoft Windows 8 or 10
* macOS, latest version
* Ubuntu Linux, latest Long Term Support (LTS) version

If you are using a mobile device, one of the following operating systems:

* Android
* iOS

.. _thingy53_gs_installing_software:

Installing the required software
********************************

When using a computer to work with the Nordic Thingy:53 firmware, install `nRF Connect for Desktop`_.
After installing and starting the application, install the Programmer app.

When using a mobile device to work with the Nordic Thingy:53, install the `nRF Programmer`_ mobile application from the corresponding application store.

.. _thingy53_gs_updating_firmware:
.. _thingy53_gs_precompiled_firmware:

Getting started with precompiled firmware samples
*************************************************

The Nordic Thingy:53 is preloaded with Edge Impulse application firmware, MCUboot bootloader, and a SoftDevice controller on the network core of the nRF5340 SoC.
For more information on the firmware and associated features, see :ref:`ug_thingy53`.

In addition to Edge Impulse, the |NCS| contains multiple precompiled samples that you can program to the Nordic Thingy:53.
For a non-exhaustive list, see :ref:`thingy53_compatible_applications` in :ref:`ug_thingy53`.

To try different samples or to update the existing firmware on the Nordic Thingy:53, you can use Bluetooth® Low Energy (LE), USB (MCUboot), or an external debug probe.

.. _thingy53_gs_updating_ble:

Updating through Bluetooth LE
=============================

You can update the Nordic Thingy:53 application and network core over Bluetooth LE using the nRF Programmer mobile application for Android or iOS.

Complete these steps to update the firmware:

1. Open the nRF Programmer app.

   A list of available samples appears.

   .. figure:: images/thingy53_sample_list.png
      :alt: nRF Programmer - list of samples

      nRF Programmer - list of samples

#. Select a sample.

   Application info appears.

   .. figure:: images/thingy53_application_info.png
      :alt: nRF Programmer - Application Info

      nRF Programmer - Application Info

#. Select the version of the sample from the drop-down menu.
#. Tap :guilabel:`Download`.

   When the download is complete, the name of the button changes to :guilabel:`Install`.
#. Tap :guilabel:`Install`.

   A list of nearby devices and their signal strengths appears.
#. Select your Nordic Thingy:53 from the list.
   It is listed as **El Thingy:53**.

   The transfer of the firmware image starts, and a progress wheel appears.

   .. figure:: images/thingy53_progress_wheel.png
      :alt: nRF Programmer - progress wheel

      nRF Programmer - progress wheel

   If you want to pause the update process, tap :guilabel:`Pause`.
   If you want to stop the update process, tap :guilabel:`Stop`.

   The image transfer is complete when the progress wheel reaches 100%.
   The Nordic Thingy:53 is reset and updated to the new firmware sample.
#. Tap :guilabel:`Done` to return to Application info.

.. _thingy53_gs_updating_usb:

Updating through USB
====================

You can update the Nordic Thingy:53 application and network core firmware over USB by using MCUboot, which is a secure bootloader that you can use to update applications without an external debugger.

.. note::
   Do not unplug the Nordic Thingy:53 during this process.

Complete the following steps to update the firmware:

1. Open the `Nordic Thingy:53 Downloads`_ page.
#. Go to the **Precompiled application firmware** section, and download the latest version.
#. Extract the zip file to a location of your choice.

   The :file:`CONTENTS.txt` file in the extracted folder contains the location and names of the different firmware images.

#. Open the connector cover on the side of the Nordic Thingy:53 and plug the Nordic Thingy:53 into the computer using a USB-C cable.

   .. figure:: images/thingy53_sw1_usb.svg
      :alt: The Nordic Thingy:53 schematic - **SW1** and USB connector cover

      The Nordic Thingy:53 schematic - **SW1** and USB connector cover

#. Move the power switch **SW1** to the **ON** position.
#. Open nRF Connect for Desktop and launch the Programmer app.
#. Move the power switch **SW1** to the **OFF** position.
#. Take off the top cover to access the **SW2** button.
#. Press **SW2** while switching **SW1** to the **ON** position.

   .. figure:: images/thingy53_sw1_sw2.svg
      :alt: The Nordic Thingy:53 schematic - **SW1** and **SW2**

      The Nordic Thingy:53 schematic - **SW1** and **SW2**

#. In the Programmer navigation bar, click :guilabel:`SELECT DEVICE`.

   A drop-down menu appears.
#. In the drop-down menu, select :guilabel:`Bootloader Thingy:53`.
#. Click :guilabel:`Add file` in the **FILE** section, and select :guilabel:`Browse`.

   A file explorer window appears.
#. Navigate to the folder where you extracted the application firmware.
#. Open the :file:`Peripheral_LBS` folder, select the update file and click :guilabel:`Open`.

   The update file is titled :file:`peripheral_lbs_<version-number>_thingy53_nrf5340.zip`.
#. Click the :guilabel:`Write` button in the **DEVICE** section.

   The **MCUboot DFU** window appears.

   .. figure:: images/programmer_thingy53_mcuboot_dfu.png
      :alt: Programmer - MCUboot DFU window

      Programmer - MCUboot DFU window

#. Click :guilabel:`Write` in the **MCUboot DFU** window.

   The flash slot is erased.
   When the flash slot has been erased, image transfer starts and a progress bar appears.

   When the image transfer has been completed, the network core part of the image is transferred from RAM to the network core flash.
   This can take up to 20 seconds.

   When the update is complete, a **Completed successfully** message appears.

You can now disconnect the Nordic Thingy:53 from the computer.

.. _thingy53_gs_updating_external_probe:

Updating through external debug probe
=====================================

You can update the Nordic Thingy:53 application and network core firmware by using an external debug probe.

.. note::
   The external debug probe must support Arm Cortex-M33, such as the nRF5340 DK.
   You need a 10-pin 2x5 socket-socket 1.27 mm IDC (:term:`Serial Wire Debug (SWD)`) JTAG cable to connect to the external debug probe.

Complete these steps to update the firmware.
In these steps, the nRF5340 DK is used as the external debug probe.
Do no unplug or power off the devices during this process.

1. Open the `Nordic Thingy:53 Downloads`_ page.
#. Go to the **Precompiled application firmware** section and download the latest version.
#. Extract the zip file to a location of your choice.

   The :file:`CONTENTS.txt` file in the extracted folder contains the location and names of the different firmware images.

#. Open nRF Connect for Desktop and launch the Programmer app.
#. Prepare the hardware:

   a. Open the connector cover on the side of the Nordic Thingy:53.
   #. Use a JTAG cable to connect the Nordic Thingy:53 to the debug out port on a 10-pin external debug probe.

      .. figure:: images/thingy53_nrf5340_dk.svg
         :alt: Nordic Thingy:53 connected to the debug port on a 10-pin external debug probe

         Nordic Thingy:53 connected to the debug port on a 10-pin external debug probe

   #. Power on the Nordic Thingy:53; move the power switch **SW1** to the **ON** position.
   #. Power on the external debug probe.
   #. Connect the external debug probe to the computer with a micro-USB cable.

      In the Programmer app's navigation bar, :guilabel:`No devices available` changes to :guilabel:`SELECT DEVICE`.

      .. figure:: ../nrf52/images/programmer_select_device1.png
         :alt: Programmer - Select device

         Programmer - Select device

#. Click :guilabel:`Select device` and select the appropriate debug probe entry from the drop-down list.

   The icon text changes to board name and the ID of the selected device, and the **Device memory layout** section indicates that the device is connected.

   You can identify the nRF5340 DK by its PCA number PCA10095 and its ID that is printed on the label sticker on the DK.

   If the nRF5340 DK does not show up in the drop-down list, press ``Ctrl+R`` in Windows or ``command+R`` in macOS to restart the Programmer application.

#. Click :guilabel:`Add file` in the **FILE** section, and select :guilabel:`Browse`.

   A file explorer window appears.
#. Navigate to the folder where you extracted the application firmware.
#. Open the folder for the application that you want to transfer to the Nordic Thingy:53.
#. Select the corresponding HEX file to be used with the debug probe and click :guilabel:`Open`.

   The HEX file appears in the **File memory layout** section.
#. Click :guilabel:`Erase & write` in the **DEVICE** section of the side panel.

The update is complete when the animation in the Programmer app's **Device memory layout** section ends.

.. _thingy53_gs_machine_learning:

Getting started with machine learning
*************************************

The Nordic Thingy:53 is preprogrammed with Edge Impulse firmware.
To connect the Nordic Thingy:53 to the Edge Impulse Studio, use the nRF Edge Impulse mobile application to connect over Bluetooth LE, or connect the Nordic Thingy:53 to a computer to connect over USB.

The Edge Impulse firmware enables data collection from all the sensors on the Nordic Thingy:53.
You can use the collected data to train and test machine learning models.
Deploy the trained machine learning model to the Nordic Thingy:53 over Bluetooth LE or USB.

Complete the following steps to get started with Edge Impulse:

1. Go to the `Edge Impulse`_ website.
#. Create a free Edge Impulse account.
#. Follow the instructions in the `Nordic Semi Thingy:53 page`_.

Next steps
**********

You have now completed getting started with the Nordic Thingy:53.
See the following links for where to go next:

* :ref:`ug_thingy53` for more advanced topics related to the Nordic Thingy:53.
* The :ref:`introductory documentation <getting_started>` for more information on the |NCS| and the development environment.
