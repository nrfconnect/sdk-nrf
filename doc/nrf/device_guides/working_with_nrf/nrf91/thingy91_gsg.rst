.. _ug_thingy91_gsg:

Getting started with Thingy:91
##############################

.. contents::
   :local:
   :depth: 2


This guide helps you get started with Thingy:91.
It tells you how to update the Thingy:91 application and modem firmware and connect the Thingy:91 to `nRF Cloud`_.

If you have already set up your Thingy:91 and want to learn more, see the following documentation:

* :ref:`installation` and :ref:`configuration_and_build` documentation to install the |NCS| and learn more about its development environment.
* :ref:`ug_thingy91` for more advanced topics related to the Thingy:91.

If you want to go through a hands-on online training to familiarize yourself with cellular IoT technologies and development of cellular applications, enroll in the `Cellular IoT Fundamentals course`_ in the `Nordic Developer Academy`_.

Requirements for setting up the Thingy:91
*****************************************

Make sure you have all the required hardware and that your computer has one of the supported operating systems.

Hardware
========

* nano-Subscriber Identity Module (SIM) card that supports LTE-M or Narrowband Internet of Things (NB-IoT) (The Thingy:91 comes shipped with an iBasis SIM card.)
* Computer
* For firmware updates through USB:

  * Micro-USB 2.0 cable

* For firmware updates through an external debug probe:

  * 10-pin JTAG cable
  * External debug probe supporting Arm Cortex-M33 (for example, nRF9160 DK)

.. note::
   It is recommended to use an external debug probe when you start developing with your Thingy:91.

Software
========

On your computer, one of the following operating systems:

* Microsoft Windows
* macOS
* Ubuntu Linux

|Supported OS|

Preparing for setup
*******************

Before you start updating the modem firmware and application on the Thingy:91, you must do some preliminary configurations.

Complete the following steps to prepare the Thingy:91 for setup:

1. Install the `nRF Connect Programmer`_ application on the computer:

   a. Go to `nRF Connect for Desktop Downloads <Download nRF Connect for Desktop_>`_.
   #. Download and install nRF Connect for Desktop.
   #. Open `nRF Connect for Desktop`_.
   #. Find **Programmer** in the list of apps and click :guilabel:`Install`.

.. _download_firmware_thingy91:

2. Download the firmware:

   a. Go to `Thingy:91 product website (downloads)`_.
   #. Download the zip file containing the latest Thingy:91 application and modem firmware.
   #. Extract the zip file to a folder of your choice.

   The :file:`CONTENTS.txt` file in the extracted folder contains the location and names of the different firmware images.


   .. note::
      You can also use these precompiled firmware image files for restoring the firmware to its initial image.

#. Prepare the Thingy:91 hardware:

   a. Open the box and take out the Thingy:91 and the iBasis Subscriber Identity Module (SIM) card it comes shipped with.
   #. Plug the Thingy:91 into the computer using a micro-Universal Serial Bus (USB) cable.
   #. Power the Thingy:91 on by switching **SW1** to the **ON** position.

      .. figure:: images/thingy91_pwr_switch.svg
         :alt: Thingy:91 Power switch (SW1)

         Thingy:91 Power switch (SW1)


.. _programming_thingy:

Programming firmware
********************

Thingy:91 (v1.5.0 or lower) comes preloaded with the nRF9160: Asset Tracker firmware and modem firmware on the nRF9160 :term:`System in Package (SiP)`, and the Connectivity bridge application firmware on the nRF52840 :term:`System on Chip (SoC)` that enable the device to use the environment sensors and track the device using :term:`Global Positioning System (GPS)`.
The data is transmitted to nRF Cloud.

Before you start using the Thingy:91, it is recommended that you update the application firmware to :ref:`asset_tracker_v2`.
Alternatively, for a more concise nRF Cloud firmware application, you can build and install the :ref:`nrf_cloud_multi_service` sample.
See :ref:`gs_programming` for details on building a firmware sample application.
You must also update the modem firmware.
You can update the application and modem firmware on a Thingy:91 through a :term:`Universal Serial Bus (USB)` cable using MCUboot.
MCUboot is a secure bootloader that is used to update applications if you do not have an external debugger.

.. note::
   To update the Thingy:91 through USB, the nRF9160 SiP and nRF52840 SoC bootloaders must be factory-compatible.
   The bootloaders might not be factory-compatible if the nRF9160 SiP or nRF52840 SoC has been updated with an external debug probe.
   To restore the bootloaders, program the nRF9160 SiP or nRF52840 SoC with the Thingy:91 firmware files through an external debug probe.

Alternatively, you can use an external debug probe.

.. note::
   The external debug probe must support Arm Cortex-M33, such as the nRF9160 DK.
   You need a 10-pin 2x5 socket-socket 1.27 mm IDC (:term:`Serial Wire Debug (SWD)`) JTAG cable to connect to the external debug probe.

The board enters bootloader mode if any of the following buttons are pressed while the Thingy:91 is being powered on (using **SW1**):

* **SW3** - The main button used to flash the nRF9160 SiP.
* **SW4** - The button used to update the nRF52840 SoC.

Following are the three recommended steps when you start to program a Thingy:91:

1. :ref:`Updating the Connectivity bridge application firmware in the nRF52840 SoC <updating_the conn_bridge_52840>`.
#. :ref:`Updating the modem firmware on the nRF9160 SoC (especially if you want to incorporate the latest changes from a newly released modem firmware) <update_modem_fw_nrf9160>`.
#. :ref:`Programming the application firmware on the nRF9160 SiP <update_nrf9160_application>`.


Before you start, make sure the Thingy:91 is connected to the computer with a micro-USB cable.

.. note::

   Do not unplug the Nordic Thingy:91 during this process.

.. rst-class:: numbered-step

.. _updating_the conn_bridge_52840:

Updating the firmware in the nRF52840 SoC
=========================================

.. tabs::

   .. group-tab:: Through USB

      To update the firmware, complete the following steps:

      1. Open `nRF Connect for Desktop`_ and launch the Programmer app.
      #. Scroll down in the menu on the left and make sure **Enable MCUboot** is selected.

         .. figure:: images/programmer_enable_mcuboot.png
            :alt: Programmer - Enable MCUboot

            Programmer - Enable MCUboot

      #. Switch off the Thingy:91.
      #. Press **SW4** while switching **SW1** to the **ON** position.

         .. figure:: images/thingy91_sw1_sw4.svg
            :alt: thingy91_sw1_sw4

            Thingy:91 - SW1 SW4 switch

      #. In the Programmer navigation bar, click :guilabel:`SELECT DEVICE`.
         A drop-down menu appears.

         .. figure:: images/programmer_select_device2.png
            :alt: Programmer - Select device

            Programmer - Select device

      #. In the menu, select the entry corresponding to your device (:guilabel:`MCUBOOT`).

         .. note::
            The device entry might not be the same in all cases and can vary depending on the application version and the operating system.

      #. In the menu on the left, click :guilabel:`Add file` in the **FILE** section, and select :guilabel:`Browse`.
         A file explorer window appears.

         .. figure:: images/programmer_add_file2.png
            :alt: Programmer - Add file

            Programmer - Add file

      #. Navigate to the folder you downloaded and extracted from the `Nordic Semiconductor website`_ in the :ref:`Download firmware <download_firmware_thingy91>` step.

      #. Open the folder :file:`img_fota_dfu_hex` that contains the HEX files for updating over USB.
         See the :file:`CONTENTS.txt` file for information on which file you need.

      #. Select the Connectivity bridge firmware file.

      #. Click :guilabel:`Open`.

      #. Scroll down in the menu on the left to the **DEVICE** section and click :guilabel:`Write`.

         .. figure:: images/programmer_hex_write1.png
            :alt: Programmer - Writing of HEX files

            Programmer - Writing of HEX files

         The MCUboot DFU window appears.

         .. figure:: images/thingy91_mcuboot_dfu.png
            :alt: Programmer - MCUboot DFU

            Programmer - MCUboot DFU

      #. In the **MCUboot DFU** window, click :guilabel:`Write`.
         When the update is complete, a "Completed successfully" message appears.
      #. Scroll up in the menu on the left to the **FILE** section and click :guilabel:`Clear files`.

   .. group-tab:: Through external debug probe

      To update the firmware using the nRF9160 DK as the external debug probe, complete the following steps:

      1. Open `nRF Connect for Desktop`_ and launch the Programmer app.

      .. _prepare_hw_ext_dp:

      2. Prepare the hardware:

         a. Connect the Thingy:91 to the debug out port on a 10-pin external debug probe using a JTAG cable.

            .. figure:: images/programmer_thingy91_connect_dk_swd_vddio.svg
               :alt: Thingy:91 - Connecting external debug probe

               Thingy:91 - Connecting external debug probe

            .. note::
               When using nRF9160 DK as the debug probe, make sure that VDD_IO (SW11) is set to 1.8 V on the nRF9160 DK.

         #. Make sure that the Thingy:91 and the external debug probe are powered on.

            .. note::
               Do not unplug or power off the devices during this process.

         #. Connect the external debug probe to the computer with a micro-USB cable.

            In the Programmer navigation bar, :guilabel:`No devices available` changes to :guilabel:`SELECT DEVICE`.

            .. figure:: ../nrf52/images/programmer_select_device1.png
               :alt: Programmer - Select device

               Programmer - SELECT DEVICE
         #. Click :guilabel:`SELECT DEVICE` and select the appropriate debug probe entry from the drop-down list.

            Select nRF9160 DK from the list.

            .. figure:: images/programmer_com_ports.png
               :alt: Programmer - nRF9160 DK

               Programmer - nRF9160 DK

            The button text changes to the SEGGER ID of the selected device, and the Device Memory Layout section indicates that the device is connected.

      #. Set the SWD selection switch **SW2** to **nRF52** on the Thingy:91.
         See `SWD Select`_ for more information on the switch.

      #. In the menu on the left, click :guilabel:`Add file` in the **FILE** section, and select :guilabel:`Browse`.
         A file explorer window appears.

         .. figure:: images/programmer_add_file1.png
            :alt: Programmer - Add file

            Programmer - Add file

      #. Navigate to the folder you downloaded and extracted from the `Nordic Semiconductor website`_ in the :ref:`Download firmware <download_firmware_thingy91>` step.

      #. Open the folder :file:`img_app_bl` that contains the HEX files for flashing with a debugger.
         See the :file:`CONTENTS.txt` file for information on which file you need.

      #. Select the Connectivity bridge firmware file.
      #. Click :guilabel:`Open`.
      #. Scroll down in the menu on the left to the **DEVICE** section and click :guilabel:`Erase & write`.
         The update is completed when the animation in Programmer's Device memory layout window ends.

         .. figure:: images/programmer_ext_debug_hex_write.png
            :alt: Programming using External debug probe

            Programming using External debug probe

      #. Scroll up in the menu on the left to the **FILE** section and click :guilabel:`Clear files`.

.. rst-class:: numbered-step

.. _update_modem_fw_nrf9160:

Update the modem firmware on the nRF9160 SiP
============================================

.. tabs::

   .. group-tab:: Through USB

     To update the modem firmware using USB, complete the following steps:

      1. Open `nRF Connect for Desktop`_ and launch the Programmer app if you do not have it open already.
      #. Make sure that **Enable MCUboot** is selected.
      #. Switch off the Thingy:91.
      #. Press **SW3** while switching **SW1** to the **ON** position.

         .. figure:: images/thingy91_sw1_sw3.svg
            :alt: Thingy:91 - SW1 SW3 switch

            Thingy:91 - SW1 SW3 switch

      #. In the menu, select Thingy:91.

      #. In the menu on the left, click :guilabel:`Add file` in the **FILE** section, and select :guilabel:`Browse`.
         A file explorer window appears.

         .. figure:: images/programmer_add_file.png
            :alt: Programmer - Add file

            Programmer - Add file

      .. update_modem_start

      5. Navigate to the folder you downloaded and extracted from the `Nordic Semiconductor website`_ in the :ref:`Download firmware <download_firmware_thingy91>` step.
      #. Find the modem firmware zip file with the name similar to :file:`mfw_nrf9160_*.zip` and the number of the latest version.

         .. note::
            Do not extract the modem firmware zip file.

      #. Select the zip file and click :guilabel:`Open`.

      .. update_modem_end

      8. In the Programmer app, scroll down in the menu on the left to the **DEVICE** section and click :guilabel:`Write`.

         .. figure:: images/programmer_usb_update_modem.png
            :alt: Programmer - Update modem

            Programmer - Update modem

         The Modem DFU via MCUboot window appears.

         .. figure:: images/thingy91_modemdfu_mcuboot.png
            :alt: Programmer - Modem DFU via MCUboot

            Programmer - Modem DFU via MCUboot

      #. In the **Modem DFU via MCUboot** window, click :guilabel:`Write`.
         When the update is complete, a **Completed successfully** message appears.

   .. group-tab:: Through external debug probe

      To update the modem firmware using external debug probe, complete the following steps:

      1. Open `nRF Connect for Desktop`_ and launch the Programmer app and :ref:`prepare the hardware <prepare_hw_ext_dp>` if you have not done it already.
      #. Set the SWD selection switch **SW2** to **nRF91** on the Thingy:91.

      #. In the menu on the left, click :guilabel:`Add file` in the **FILE** section, and select :guilabel:`Browse`.
         A file explorer window appears.

         .. figure:: images/programmer_add_file1.png
            :alt: Programmer - Add file

            Programmer - Add file

      #. Navigate to the folder you downloaded and extracted from the `Nordic Semiconductor website`_ in the :ref:`Download firmware <download_firmware_thingy91>` step.
      #. Find the modem firmware zip file with the name similar to :file:`mfw_nrf9160_*.zip` and the number of the latest version and click :guilabel:`Open`.

         .. note::
            Do not extract the modem firmware zip file.

      #. Select the zip file and click :guilabel:`Open`.
      #. In the Programmer app, scroll down in the menu on the left to the **DEVICE** section and click :guilabel:`Write`.

         .. figure:: images/programmer_ext_debug_update_modem.png
            :alt: Programmer - Update modem

            Programmer - Update modem

         The Modem DFU window appears.

         .. figure:: images/programmer_modemdfu.png
            :alt: Programmer - Modem DFU

            Programmer - Modem DFU

      #. In the **Modem DFU** window, click :guilabel:`Write`.
         When the update is complete, a "Completed successfully" message appears.

         .. note::
            Before trying to update the modem again, click the :guilabel:`Erase all` button. In this case, the contents of the flash memory are deleted and the applications must be reprogrammed.

..

.. rst-class:: numbered-step

.. _update_nrf9160_application:

Program the nRF9160 SiP application
===================================

.. tabs::

   .. group-tab:: Through USB

      To program the application firmware using USB, complete the following steps:

      1. Open `nRF Connect for Desktop`_ and launch the Programmer app if you have not done already.
      #. Make sure that **Enable MCUboot** is selected.
      #. Switch off the Thingy:91.
      #. Press **SW3** while switching **SW1** to the **ON** position.

         .. figure:: images/thingy91_sw1_sw3.svg
            :alt: Thingy:91 - SW1 SW3 switch

            Thingy:91 - SW1 SW3 switch

      #. In the Programmer navigation bar, click :guilabel:`SELECT DEVICE`.
         A drop-down menu appears.

         .. figure:: images/programmer_select_device.png
            :alt: Programmer - Select device

            Programmer - Select device

      #. In the menu, select Thingy:91.

      #. In the menu on the left, click :guilabel:`Add file` in the **FILE** section, and select :guilabel:`Browse`.
         A file explorer window appears.

         .. figure:: images/programmer_add_file.png
            :alt: Programmer - Add file

            Programmer - Add file

      #. Navigate to the folder you downloaded and extracted from the `Nordic Semiconductor website`_ in the :ref:`Download firmware <download_firmware_thingy91>` step.

      #. Open the folder :file:`img_fota_dfu_hex` that contains the HEX files for updating over USB.
         See the :file:`CONTENTS.txt` file for information on which file you need.

      #. Select the appropriate Asset Tracker v2 firmware file.

         .. note::

            If you are connecting over NB-IoT and your operator does not support extended Protocol Configuration Options (ePCO), select the file that has legacy Protocol Configuration Options (PCO) mode enabled.

      #. Click :guilabel:`Open`.

      #. Scroll down in the menu on the left to the **DEVICE** section and click :guilabel:`Write`.

         .. figure:: images/programmer_hex_write.png
            :alt: Programmer - Writing of HEX files

            Programmer - Writing of HEX files

         The MCUboot DFU window appears.

         .. figure:: images/thingy91_mcuboot_dfu1.png
            :alt: Programmer - MCUboot DFU

            Programmer - MCUboot DFU

      #. In the **MCUboot DFU** window, click :guilabel:`Write`.
         When the update is complete, a **Completed successfully** message appears.
      #. Scroll up in the menu on the left to the **FILE** section and click :guilabel:`Clear files`.

   .. group-tab:: Through external debug probe

      To program the application firmware using external debug probe, complete the following steps:

      1. Open `nRF Connect for Desktop`_ and launch the Programmer app and :ref:`prepare the hardware <prepare_hw_ext_dp>` if you have not done it already.
      #. Make sure the SWD selection switch **SW2** is set to **nRF91** on the Thingy:91.

      #. In the menu on the left, click :guilabel:`Add file` in the **FILE** section, and select :guilabel:`Browse`.
         A file explorer window appears.

         .. figure:: images/programmer_add_file1.png
            :alt: Programmer - Add file

            Programmer - Add file

      #. Navigate to the folder you downloaded and extracted from the `Nordic Semiconductor website`_ in the :ref:`Download firmware <download_firmware_thingy91>` step.

      #. Open the folder :file:`img_app_bl` that contains the HEX files for updating using a debugger.
         See the :file:`CONTENTS.txt` file for information on which file you need.

      #. Select the appropriate Asset Tracker v2 firmware file.

         .. note::

            If you are connecting over NB-IoT and your operator does not support extended Protocol Configuration Options (ePCO), select the file that has legacy Protocol Configuration Options (PCO) mode enabled.

      #. Click :guilabel:`Open`.
      #. Scroll down in the menu on the left to the **DEVICE** section and click :guilabel:`Erase & write`.
         The update is completed when the animation in Programmer's **Device memory layout** window ends.

         .. figure:: images/programmer_ext_debug_hex_write.png
            :alt: Programming using External debug probe

            Programming using External debug probe

      #. Scroll up in the menu on the left to the **FILE** section and click :guilabel:`Clear files`.

..

You can now disconnect the Thingy:91 from the computer.

Next, you need to create a cloud account if you do not have one already.

Connecting the |DK| to nRF Cloud
*********************************

.. |DK| replace:: Thingy:91

.. include:: nrf9160_gs.rst
   :start-after: dk_nrf_cloud_start
   :end-before: dk_nrf_cloud_end

Creating an nRF Cloud account
=============================

.. include:: nrf9160_gs.rst
   :start-after: nrf_cloud_account_start
   :end-before: nrf_cloud_account_end

.. _connect_nRF_cloud:

Connecting to nRF Cloud
=======================

.. include:: nrf9160_gs.rst
   :start-after: nrf_cloud_connection_start
   :end-before: nrf_cloud_connection_end

5. Punch out the nano-SIM from the SIM card and plug it into the SIM card holder on the Thingy:91.

   .. figure:: images/thingy91_insert_sim.svg
      :alt: Inserting SIM

      Inserting SIM

#. Connect the Thingy:91 to the computer with a USB cable and turn it on, or reset the device if it is already turned on.
#. Wait up to three minutes for the Thingy:91 to connect to the LTE network and to nRF Cloud.

   After a few moments, the nRF Cloud user association process starts.
   This is indicated by white double pulse blinking of the Thingy:91's RGB LED as indicated in :ref:`Operating states <led_indication>`.

#. In the Add LTE Device view from Step 4, enter your device ID and ownership code (**PIN/HWID**).

   .. figure:: images/nrfcloud_add_lte_device.png
      :alt: nRF Cloud - Add LTE Device view

      nRF Cloud - Add LTE Device view

   * **Device ID**: The device ID is composed of *nrf-* and the 15-digit :term:`International Mobile (Station) Equipment Identity (IMEI)` number that is printed on the label of your Thingy:91. It is case sensitive, so make sure all the letters are lower-case.
   * **PIN/HWID**: The ownership code is the PIN or the hardware ID of your Thingy:91, and it is found on the label of your Thingy:91.

   .. figure:: images/thingy91_pin_imei.svg
      :alt: PIN and IMEI on Thingy:91

      PIN and IMEI on Thingy:91

   If the label contains a PIN in addition to the IMEI number, enter this pin.
   If it does not contain a PIN, enter the Hardware ID (HWID) HEX code, with or without colons.
   For example, *AA:BB:CC:DD:EE:FF* or *AABBCCDDEEFF*.

   .. note::

      The ownership code serves as a password and proves that you own the specific Thingy:91.
      Therefore, do not share it with anyone.


#. Click the :guilabel:`Add Device` button.

   The message "Device added to account. Waiting for it to connect..." appears.

   .. note::

      If you see an error message, check the error code and see `nRF Cloud REST API (v1)`_ to find out what is causing the error.

#. When the message has disappeared, go to the menu on the left and click :guilabel:`Devices`.

   You can see the Thingy:91 in your device list and all the sensor data being transmitted to the cloud from the  Thingy:91.
   The LED on the Thingy:91 should be blinking green, which indicates that it is transmitting all the data to the cloud.

   .. note::

      It might take a while for the sensor data to appear in the nRF Cloud portal, depending on the duration of time GNSS uses to search for a fix.

Next steps
**********

You have now completed getting started with the Thingy:91.
See the following links for where to go next:

* :ref:`installation` and :ref:`configuration_and_build` documentation to install the |NCS| and learn more about its development environment.
* :ref:`ug_thingy91` for more advanced topics related to the Thingy:91.
* :ref:`connectivity_bridge` about using the nRF UART Service with Thingy:91.
