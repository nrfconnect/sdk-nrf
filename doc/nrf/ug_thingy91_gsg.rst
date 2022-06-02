.. _ug_thingy91_gsg:

Getting started with Thingy:91
##############################

.. contents::
   :local:
   :depth: 2


This guide helps you get started with Thingy:91.
It tells you how to update the Thingy:91 application and modem firmware and connect the Thingy:91 to `nRF Cloud`_.

If you have already set up your Thingy:91 and want to learn more, see the following documentation:

* :ref:`ug_thingy91` for more advanced topics related to the Thingy:91.
* The :ref:`introductory documentation <getting_started>` for more information on the |NCS| and the development environment.

Requirements for setting up the Thingy:91
*****************************************

Make sure you have all the required hardware and that your computer has one of the supported operating systems.

Hardware
========

* nano-Subscriber Identity Module (SIM) card that supports LTE-M or Narrowband Internet of Things (NB-IoT) (The Thingy:91 comes shipped with an iBasis SIM card.)
* Micro-USB 2.0 cable
* Computer

Software
========

* Microsoft Windows 10
* macOS X, latest version
* Ubuntu Linux, latest Long Term Support (LTS) version

Preparing for setup
*******************

Before you start updating the modem firmware and application on the Thingy:91, you must do some preliminary configurations.

Complete the following steps to prepare the Thingy:91 for setup:

1. Install the `nRF Connect Programmer`_ application on the computer:

   a. Go to `nRF Connect for Desktop Downloads <Download nRF Connect for Desktop_>`_.
   #. Download and install nRF Connect for Desktop.
   #. Open `nRF Connect for Desktop`_.
   #. Find :guilabel:`Programmer` in the list of applications and click :guilabel:`Install`.

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

      .. figure:: /images/thingy91_pwr_switch.svg
         :alt: Thingy:91 Power switch (SW1)

         Thingy:91 Power switch (SW1)


.. _programming_thingy:

Updating firmware
*****************

Thingy:91 (v1.5.0 or lower) comes preloaded with the nRF9160: Asset Tracker firmware and modem firmware on the nRF9160 :term:`System in Package (SiP)`, and the Connectivity bridge application firmware on the nRF52840 :term:`System on Chip (SoC)` that enable the device to use the environment sensors and track the device using :term:`Global Positioning System (GPS)`.
The data is transmitted to nRF Cloud.

Before you start using the Thingy:91, it is recommended that you update the application firmware to :ref:`asset_tracker_v2`.
You must also update the modem firmware.
You can do this through Universal Serial Bus (USB) (MCUboot) or an external debug probe using the Programmer application.

.. note::
   To update the Thingy:91 through USB, the nRF9160 SiP and nRF52840 SoC bootloaders must be factory-compatible.
   The bootloaders might not be factory-compatible if the nRF9160 SiP or nRF52840 SoC has been updated with an external debug probe.
   To restore the bootloaders, program the nRF9160 SiP or nRF52840 SoC with the Thingy:91 firmware files through an external debug probe.

.. _programming_usb:

Updating firmware through USB
=============================

You can update the Thingy:91 application and modem firmware over :term:`Universal Serial Bus (USB)` by using MCUboot, which is a secure bootloader that can be used to update applications without an external debugger.

Before you start, make sure the Thingy:91 is connected to the computer with a micro-USB cable.

.. note::

   Do not unplug the Nordic Thingy:91 during this process.

To update the firmware, complete the following steps:

1. Open `nRF Connect for Desktop`_ and launch the Programmer application.
#. Scroll down in the menu on the left and make sure :guilabel:`Enable MCUboot` is selected.

      .. figure:: /images/programmer_enable_mcuboot.png
         :alt: Programmer - Enable MCUboot

         Programmer - Enable MCUboot

#. Update the nRF52840 :term:`System on Chip (SoC)` application:

   a. Switch off the Thingy:91.
   #. Press **SW4** while switching **SW1** to the **ON** position.

      .. figure:: /images/thingy91_sw1_sw4.svg
         :alt: thingy91_sw1_sw4

         Thingy:91 - SW1 SW4 switch

   c. In the Programmer navigation bar, click :guilabel:`Select device`.
      A drop-down menu appears.

      .. figure:: /images/programmer_select_device.png
         :alt: Programmer - Select device

         Programmer - Select device

   #. In the menu, select the entry corresponding to your device.

      .. note::
         The device entry might not be the same in all cases and can vary depending on the application version.

   #. In the menu on the left, click :guilabel:`Add file` in the File section, and select :guilabel:`Browse`.
      A file explorer window appears.

      .. figure:: /images/programmer_add_file.png
         :alt: Programmer - Add file

         Programmer - Add file

   #. Navigate to the folder you downloaded and extracted from the `Nordic Semiconductor website`_ in the :ref:`Download firmware <download_firmware_thingy91>` step.

   #. Open the folder that contains the HEX files for updating over USB.
      See the :file:`CONTENTS.txt` file for information on which file you need.

   #. Select the Connectivity bridge firmware file.

   #. Click :guilabel:`Open`.

   #. Scroll down in the menu on the left to :guilabel:`Device` and click :guilabel:`Write`.

      .. figure:: /images/programmer_hex_write.png
         :alt: Programmer - Writing of HEX files

         Programmer - Writing of HEX files

      The :guilabel:`MCUboot DFU` window appears.

      .. figure:: /images/thingy91_mcuboot_dfu.png
         :alt: Programmer - MCUboot DFU

         Programmer - MCUboot DFU

   #. In the :guilabel:`MCUboot DFU` window, click :guilabel:`Write`.
      When the update is complete, a :guilabel:`Completed successfully` message appears.
   #. Scroll up in the menu on the left to :guilabel:`File` and click :guilabel:`Clear files`.

#. Update the modem firmware on the nRF9160 System in Package (SiP):

   a. Switch off the Thingy:91.
   #. Press **SW3** while switching **SW1** to the **ON** position.

      .. figure:: /images/thingy91_sw1_sw3.svg
         :alt: Thingy:91 - SW1 SW3 switch

         Thingy:91 - SW1 SW3 switch

   #. In the menu, select Thingy:91.

   #. In the menu on the left, click :guilabel:`Add file` in the File section, and select :guilabel:`Browse`.
      A file explorer window appears.

      .. figure:: /images/programmer_add_file.png
         :alt: Programmer - Add file

         Programmer - Add file

   .. update_modem_start

   e. Navigate to the folder you downloaded and extracted from the `Nordic Semiconductor website`_ in the :ref:`Download firmware <download_firmware_thingy91>` step.
   #. Find the modem firmware zip file with the name similar to :file:`mfw_nrf9160_*.zip` and the number of the latest version.

      .. note::
         Do not extract the modem firmware zip file.

   #. Select the zip file and click :guilabel:`Open`.

   .. update_modem_end

   h. In the Programmer application, scroll down in the menu on the left to :guilabel:`Device` and click :guilabel:`Write`.

      .. figure:: /images/programmer_usb_update_modem.png
         :alt: Programmer - Update modem

         Programmer - Update modem

      The :guilabel:`Modem DFU via MCUboot` window appears.

      .. figure:: /images/thingy91_modemdfu_mcuboot.png
         :alt: Programmer - Modem DFU via MCUboot

         Programmer - Modem DFU via MCUboot

   #. In the :guilabel:`Modem DFU via MCUboot window`, click :guilabel:`Write`.
      When the update is complete, a :guilabel:`Completed successfully` message appears.

#. Update the nRF9160 SiP application:

   a. Switch off the Thingy:91.
   #. Press **SW3** while switching **SW1** to the **ON** position.

      .. figure:: /images/thingy91_sw1_sw3.svg
         :alt: Thingy:91 - SW1 SW3 switch

         Thingy:91 - SW1 SW3 switch

   #. In the Programmer navigation bar, click :guilabel:`Select device`.
      A drop-down menu appears.

      .. figure:: /images/programmer_select_device.png
         :alt: Programmer - Select device

         Programmer - Select device

   #. In the menu, select Thingy:91.

   #. In the menu on the left, click :guilabel:`Add file` in the File section, and select :guilabel:`Browse`.
      A file explorer window appears.

      .. figure:: /images/programmer_add_file.png
         :alt: Programmer - Add file

         Programmer - Add file

   #. Navigate to the folder you downloaded and extracted from the `Nordic Semiconductor website`_ in the :ref:`Download firmware <download_firmware_thingy91>` step.

   #. Open the folder that contains the HEX files for updating over USB.
      See the :file:`CONTENTS.txt` file for information on which file you need.

   #. Select the appropriate Asset Tracker v2 firmware file.

       .. note::

          If you are connecting over NB-IoT and your operator does not support ePCO, select the file that has legacy Protocol Configuration Options (PCO) mode enabled.

   #. Click :guilabel:`Open`.

   #. Scroll down in the menu on the left to :guilabel:`Device` and click :guilabel:`Write`.

      .. figure:: /images/programmer_hex_write.png
         :alt: Programmer - Writing of HEX files

         Programmer - Writing of HEX files

      The :guilabel:`MCUboot DFU` window appears.

      .. figure:: /images/thingy91_mcuboot_dfu1.png
         :alt: Programmer - MCUboot DFU

         Programmer - MCUboot DFU

   #. In the :guilabel:`MCUboot DFU` window, click :guilabel:`Write`.
      When the update is complete, a :guilabel:`Completed successfully` message appears.
   #. Scroll up in the menu on the left to :guilabel:`File` and click :guilabel:`Clear files`.

   You can now disconnect the Thingy:91 from the computer.

Next, you need to create an nRF Cloud account if you do not have one already.

Updating firmware through external debug probe
==============================================

You can update the Thingy:91 application and modem firmware by using an external debug probe.

.. note::
   The external debug probe must support Arm Cortex-M33, such as the nRF9160 DK.
   You need a 10-pin 2x5 socket-socket 1.27 mm IDC (Serial Wire Debug (SWD)) JTAG cable to connect to the external debug probe.

To update the firmware, complete the following steps. In these steps, the nRF9160 DK is used as the external debug probe.

1. Open `nRF Connect for Desktop`_ and launch the Programmer application.
#. Prepare the hardware:

   a. Connect the Thingy:91 to the debug out port on a 10-pin external debug probe using a JTAG cable.

      .. figure:: /images/programmer_thingy91_connect_dk_swd_vddio.svg
         :alt: Thingy:91 - Connecting external debug probe

         Thingy:91 - Connecting external debug probe

      .. note::
         When using nRF9160 DK as the debug probe, make sure that VDD_IO (SW11) is set to 1.8 V on the nRF9160 DK.

   #. Make sure that the Thingy:91 and the external debug probe are powered on.

      .. note::
         Do not unplug or power off the devices during this process.

   #. Connect the external debug probe to the computer with a micro-USB cable.

      In the Programmer navigation bar, :guilabel:`No devices available` changes to :guilabel:`Select device`.

      .. figure:: /images/programmer_select_device1.png
         :alt: Programmer - Select device

         Programmer - Select device
   #. Click :guilabel:`Select device` and select the appropriate debug probe entry from the drop-down list.

      You can identify the nRF9160 DK by the fact that it has three COM ports.

      .. figure:: /images/programmer_com_ports.png
         :alt: Programmer - COM ports

         Programmer - COM ports

      If the three COM ports are not visible, press ``Ctrl+R`` in Windows or ``command+R`` in macOS to restart the Programmer application.
      The button text changes to the SEGGER ID of the selected device, and the :guilabel:`Device Memory Layout` section indicates that the device is connected.

#. Update the nRF52840 System on Chip (SoC) application:

   a. Set the SWD selection switch **SW2** to **nRF52**.
      See `SWD Select`_ for more information on the switch.

   #. In the menu on the left, click :guilabel:`Add file` in the File section, and select :guilabel:`Browse`.
      A file explorer window appears.

      .. figure:: /images/programmer_add_file1.png
         :alt: Programmer - Add file

         Programmer - Add file

   #. Navigate to the folder you downloaded and extracted from the `Nordic Semiconductor website`_ in the :ref:`Download firmware <download_firmware_thingy91>` step.

   #. Open the folder that contains the HEX files for updating over USB.
      See the :file:`CONTENTS.txt` file for information on which file you need.

   #. Select the Connectivity bridge firmware file.

   .. programing_ext_dp_start

   f. Click :guilabel:`Open`.
   #. Scroll down in the menu on the left to :guilabel:`Device` and click :guilabel:`Erase & write`.
      The update is completed when the animation in Programmer's Device memory layout window ends.

      .. figure:: /images/programmer_ext_debug_hex_write.png
         :alt: Programming using External debug probe

         Programming using External debug probe

   #. Scroll up in the menu on the left to :guilabel:`File` and click :guilabel:`Clear files`.

   .. programing_ext_dp_end

4. Update the modem firmware on the nRF9160 System in Package (SiP):

   a. Set the SWD selection switch **SW2** to **nRF91**.

   #. In the menu on the left, click :guilabel:`Add file` in the File section, and select :guilabel:`Browse`.
      A file explorer window appears.

      .. figure:: /images/programmer_add_file1.png
         :alt: Programmer - Add file

         Programmer - Add file

   #. Navigate to the folder you downloaded and extracted from the `Nordic Semiconductor website`_ in the :ref:`Download firmware <download_firmware_thingy91>` step.
   #. Find the modem firmware zip file with the name similar to :file:`mfw_nrf9160_*.zip` and the number of the latest version and click :guilabel:`Open`.

      .. note::
         Do not extract the modem firmware zip file.

   #. Select the zip file and click :guilabel:`Open`.
   #. In the Programmer application, scroll down in the menu on the left to :guilabel:`Device` and click :guilabel:`Write`.

      .. figure:: /images/programmer_ext_debug_update_modem.png
         :alt: Programmer - Update modem

         Programmer - Update modem

      The :guilabel:`Modem DFU` window appears.

      .. figure:: /images/programmer_modemdfu.png
         :alt: Programmer - Modem DFU

         Programmer - Modem DFU

   #. In the :guilabel:`Modem DFU` window, click :guilabel:`Write`.
      When the update is complete, a :guilabel:`Completed successfully` message appears.

      .. note::
         If you have issues updating modem firmware, click :guilabel:`Erase all` before trying to update the modem again. In this case, the contents of the flash memory are deleted and the applications must be reprogrammed.

5. Update the nRF9160 SiP application:

   a. Make sure the SWD selection switch **SW2** is set to **nRF91**.

   #. In the menu on the left, click :guilabel:`Add file` in the File section, and select :guilabel:`Browse`.
      A file explorer window appears.

      .. figure:: /images/programmer_add_file1.png
         :alt: Programmer - Add file

         Programmer - Add file



   #. Navigate to the folder you downloaded and extracted from the `Nordic Semiconductor website`_ in the :ref:`Download firmware <download_firmware_thingy91>` step.

   #. Open the folder that contains the HEX files for updating over USB.
      See the :file:`CONTENTS.txt` file for information on which file you need.

   #. Select the appropriate Asset Tracker v2 firmware file.

.. include:: ug_thingy91_gsg.rst
   :start-after: programing_ext_dp_start
   :end-before: programing_ext_dp_end

..

Creating an nRF Cloud account
*****************************

You must sign up with `nRF Cloud`_ before you can start using the service.

To create an nRF Cloud account, complete the following steps:

1. Open the `nRF Cloud`_ landing page and click :guilabel:`Register`.
#. Enter your email address and choose a password, then click :guilabel:`Create Account`.
   nRF Cloud will send you a verification email.
#. Copy the 6-digit verification code and paste it into the registration dialog box.
   If you do not see the verification email, check your junk mail for an email from no-reply@verificationemail.com.

   If you closed the registration dialog box, you can repeat Step 1 and then click :guilabel:`Already have a code?`.
   Then enter your email and the verification code.

You can now log in to `nRF Cloud`_ with your email and the password your chose.
After logging in, you are directed to the dashboard view that displays your device count and service usage.
Next, you need to activate the SIM card for the Thingy:91.

Activating the iBasis SIM card
******************************

.. contents::
   :local:
   :depth: 2


If you are using the iBasis Subscriber Identity Module (SIM) card that comes shipped with the Thingy:91, you need to activate it in nRF Cloud.

To activate the SIM card, complete the following steps:

1. Scratch off the bottom area on the back of the iBasis SIM to reveal the :term:`Personal Unblocking Key (PUK)` code.
   The :term:`Integrated Circuit Card Identifier (ICCID)` code is the first 18 digits printed on the back.

      .. figure:: /images/thingy91_sim_iccid_puk.svg
         :alt: iBASIS SIM

         iBASIS SIM

#. Make a note of the ICCID and PUK codes.
   You will need them later.
#. Pop out the nano-SIM card.
#. Take off the silicone cover on the Thingy:91, and put the card in the nano-SIM card holder.

      .. figure:: /images/thingy91_insert_sim.svg
         :alt: Inserting SIM

         Inserting SIM

#. Log in to the nRF Cloud portal.
#. Click the large plus sign in the upper-left corner.
   The :guilabel:`Add New` window appears.

      .. figure:: /images/nrfcloud_plus_sign_callout.png
         :alt: nRF Cloud

         nRF Cloud

#. In the :guilabel:`Add New` window, click :guilabel:`LTE Device`.
   The :guilabel:`Activate SIM Card` window appears.

      .. figure:: /images/nrfcloud_add_lte_device1.png
         :alt: nRF Cloud - Add new device

         nRF Cloud - Add new device

      .. figure:: /images/nrfcloud_activating_sim.png
         :alt: nRF Cloud - Activating SIM Card

         nRF Cloud - Activating SIM Card

#. In the :guilabel:`SIM ICCID` field, enter the ICCID code that you wrote down.
#. In the :guilabel:`PUK` field, enter the PUK code that you wrote down.
#. Read the Terms and Privacy Policy. Then select the checkbox next to them.
#. Click :guilabel:`Activate SIM`.
#. The :guilabel:`Activate SIM Card` window appears.
#. Enter your information in the :guilabel:`Activate SIM Card` window. Then, click :guilabel:`Submit` .

   The :guilabel:`SIM Activated successfully` message appears.

Connecting to nRF Cloud
***********************

To start using nRF Cloud, you need to associate the Thingy:91 to your user account.

To connect the Thingy:91 to nRF Cloud and to associate it with your user account,
complete the following steps:

1. Make sure the Thingy:91 is switched on.
#. Wait for the Thingy:91 to connect to the LTE network and to nRF Cloud.

   After a few moments, the nRF Cloud user association process starts.
   This is indicated by white double pulse blinking of the Thingy:91's RGB LED as indicated in :ref:`Operating states <led_indication>`.

#. To complete the user association, open the `nRF Cloud`_ portal.
#. Click the large plus sign in the upper-left corner.
   The :guilabel:`Add New` window appears.

      .. figure:: /images/nrfcloud_plus_sign_callout.png
         :alt: nRF Cloud

         nRF Cloud

#. In the :guilabel:`Add New` window, click :guilabel:`LTE Device`.

   The :guilabel:`Activate SIM Card` window appears.

      .. figure:: /images/nrfcloud_activating_sim.png
         :alt: nRF Cloud - Activating SIM Card

         nRF Cloud - Activating SIM Card

#. Skip the :guilabel:`Active SIM Card` section by clicking on :guilabel:`Skip this step`.

   The :guilabel:`Add LTE Device` window appears.

      .. figure:: /images/nrfcloud_add_lte_device.png
         :alt: Add LTE Device

         Add LTE Device

#. In the :guilabel:`Device ID` field, enter the text ``nrf-`` and after it the Thingy:91 International Mobile (Station) Equipment Identity (IMEI) code.

   The IMEI is the 15 digit code which you can find on a white sticker on the Thingy:91.
   The Device ID is case sensitive, so make sure all the letters are lower-case.

   .. figure:: /images/thingy91_pin_imei.svg
      :alt: PIN and IMEI on Thingy:91

      PIN and IMEI on Thingy:91

#. In the :guilabel:`PIN/HWID` field, enter the Personal Identification Number (PIN) which is printed on the white sticker on the Thingy:91.
#. Click :guilabel:`Add Device`.

   The message :guilabel:`Device added to account. Waiting for it to connect...` appears.

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

* :ref:`ug_thingy91` for more advanced topics related to the Thingy:91.
* The :ref:`introductory documentation <getting_started>` for more information on the |NCS| and the development environment.
