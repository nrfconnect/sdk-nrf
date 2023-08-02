.. _ug_nrf9160_gs:

Getting started with nRF9160 DK
###############################

.. contents::
   :local:
   :depth: 2

This section will get you started with your nRF9160 DK.
You will update the firmware (both the modem firmware and the application firmware) and the nRF Cloud certificates of the DK, and conduct some initial tests.

If you have already set up your nRF9160 DK and want to learn more, see the following documentation:

* :ref:`installation` and :ref:`configuration_and_build` documentation to install the |NCS| and learn more about its development environment.
* :ref:`ug_nrf9160` for more advanced topics related to the nRF9160 DK if you are already familiar with the |NCS|.

If you want to go through a hands-on online training to familiarize yourself with cellular IoT technologies and development of cellular applications, enroll in the `Cellular IoT Fundamentals course`_ in the `Nordic Developer Academy`_.

.. _nrf9160_gs_requirements:

Minimum requirements
********************

Make sure you have all the required hardware and that your computer has one of the supported operating systems.

Hardware
========

* nRF9160 DK
* nano-SIM card that supports :term:`LTE-M` or :term:`Narrowband Internet of Things (NB-IoT)` (the nRF9160 DK contains an iBasis SIM card)
* micro-USB 2.0 cable

Software
========

On your computer, one of the following operating systems:

* Microsoft Windows
* macOS
* Ubuntu Linux

|Supported OS|

.. _nrf9160_gs_installing_software:

Installing the required software
********************************

To work with the nRF9160 DK firmware and certificates, install `nRF Connect for Desktop`_.

After installing and starting the application, install the following apps:

* Programmer
* LTE Link Monitor

.. _nrf9160_gs_updating_fw:

Updating the DK firmware
************************

Download the latest application and modem firmware from the `nRF9160 DK Downloads`_ page and extract to a folder of your choice.

The downloaded zip contains the following firmware:

Application firmware
  The :file:`img_app_bl` folder contains full firmware images for different applications.
  When following this guide, use an image for the :ref:`asset_tracker_v2` application.
  Asset Tracker v2 simulates sensor data and transmits it to Nordic Semiconductor's cloud solution, `nRF Cloud`_.

  Depending on where you are located, you need the image with either LTE-M or NB-IoT support.
  Check with your SIM card provider for the mode they support at your location.
  For the iBasis SIM card provided with the nRF9160 DK, see `iBasis IoT network coverage`_.
  If your location has support for both, you can choose either one.

  Alternatively, for a more concise nRF Cloud firmware application, you can build and install the :ref:`nrf_cloud_multi_service` sample.
  See :ref:`gs_programming` for details on building a firmware sample application.

Application firmware for Device Firmware Update (DFU)
  The images in the :file:`img_fota_dfu_bin` and :file:`img_fota_dfu_hex` folders contain firmware images for DFU.
  When following this guide, you can ignore these images.

Modem firmware
  The modem firmware is in a zip archive instead of a folder.
  The zip is named :file:`mfwnrf9160_` followed by the firmware version number.
  Do not unzip this file.

.. _nrf9160_gs_updating_fw_modem:

Updating the modem firmware
===========================

To update the modem firmware, complete the following steps.
If you experience any problems during the process, restart the Programmer app by pressing ``Ctrl+R`` (``command+R`` on macOS), and try again.

.. note::

   Updating the modem firmware erases the contents of the flash memory, so the application must be programmed again to the nRF9160 DK.

1. Open the Programmer app.
#. Make sure the **PROG/DEBUG SW10** switch on the nRF9160 DK is set to **nRF91**.
   On DK v0.9.0 and earlier, this is the **SW5** switch.
#. Connect the nRF9160 DK to the computer with a micro-USB cable, and then turn the DK on.
#. Click :guilabel:`SELECT DEVICE` and select the DK from the drop-down list.
   You can identify the nRF9160 DK by the fact that it has three COM ports.

   .. figure:: images/programmer_com_ports.png
      :alt: Programmer - COM ports

      Programmer - COM ports

   If the three COM ports are not visible, press ``Ctrl+R`` in Windows or ``command+R`` in macOS to restart the Programmer application.

   The drop-down text changes to the type of the selected device, with its SEGGER ID below the name.
   The Device Memory Layout section also changes its name to the device name, and indicates that the device is connected.
   If the :guilabel:`Auto read memory` option is selected in the **DEVICE** section of the side panel, the memory layout will update.
   If it is not selected and you wish to see the memory layout, click :guilabel:`Read` in the **DEVICE** section of the side panel.

#. Click :guilabel:`Add file` in the **FILE** section, and select :guilabel:`Browse`.

   .. figure:: images/programmer_addfile_nrf9160dk.png
      :alt: Programmer - Add file

      Programmer - Add file

#. Navigate to where you extracted the firmware, and choose the :file:`mfwnrf9160_<version-number>.zip` file.
#. Click :guilabel:`Write` in the **DEVICE** section of the side panel.

   .. figure:: images/programmer_write_nrf9160dk.png
      :alt: Programmer - Write

      Programmer - Write

   The Modem DFU window appears.

   .. figure:: images/programmerapp_modemdfu.png
      :alt: Modem DFU window

      The Modem DFU window

#. Click the :guilabel:`Write` button in the **Modem DFU** window to update the firmware.
   Do not unplug or turn off the device during this process.

When the update is complete, you see a success message.
If you update the application firmware now, you can go directly to Step 5 of :ref:`nrf9160_gs_updating_fw_application`.

.. note::

   If you experience problems updating the modem firmware, click :guilabel:`Erase all` in the **DEVICE** section of the side panel and try updating again.

.. _nrf9160_gs_updating_fw_application:

Updating the application firmware
=================================

To update the application firmware, complete the following steps.
If you experience any problems during the process, restart the Programmer app by pressing ``Ctrl+R`` (``command+R`` in macOS), and try again.

1. Open the Programmer app.
#. Make sure the **PROG/DEBUG SW10** switch on the nRF9160 DK is set to **nRF91**.
   On DK v0.9.0 and earlier, this is the **SW5** switch.
#. Connect the nRF9160 DK to the computer with a micro-USB cable, and then turn the DK on.
#. Click :guilabel:`SELECT DEVICE` and select the DK from the drop-down list.
   You can identify the nRF9160 DK by the fact that it has three COM ports when you expand its entry.

   .. figure:: images/programmer_com_ports.png
      :alt: Programmer - COM ports

      Programmer - COM ports

   If the three COM ports are not visible, press ``Ctrl+R`` in Windows or ``command+R`` in macOS to restart the Programmer application.

   The drop-down text changes to the type of the selected device, with its SEGGER ID below the name.
   The Device Memory Layout section also changes its name to the device name, and indicates that the device is connected.
   If the :guilabel:`Auto read memory` option is selected in the **DEVICE** section, the memory layout will update.
   If it is not selected and you wish to see the memory layout, click :guilabel:`Read` in the **DEVICE** section.

#. Click :guilabel:`Add file` in the FILE section, and select :guilabel:`Browse`.

   .. figure:: images/programmer_addfile_nrf9160dk.png
      :alt: Programmer - Add file

      Programmer - Add file

#. Navigate to where you extracted the firmware, and then to the :file:`img_app_bl` folder there.
#. Select either :file:`nrf9160dk_asset_tracker_v2_ltem_<version-number>.hex` (LTE-M mode) or :file:`nrf9160dk_asset_tracker_v2_nbiot_<version-number>.hex` (NB-IoT mode), depending on where you are located.
   Check with your SIM card provider for the mode supported at your location.
   If you are using the iBasis SIM card provided with the DK, you can see `iBasis IoT network coverage`_ .
   You can use either mode if your location has support for both.

   For NB-IoT, there is a second variant of the firmware in the :file:`nrf9160dk_asset_tracker_v2_nbiot_legacy_pco_<version-number>.hex` file.
   Only use this legacy variant if your network does not support ePCO.

#. Click the :guilabel:`Erase & write` button in the **DEVICE** section to program the DK.
   Do not unplug or turn off the DK during this process.

   .. figure:: images/programmer_erasewrite_nrf9160dk.png
      :alt: Programmer - Erase & write

      Programmer - Erase & write

.. _nrf9160_gs_connecting_dk_to_cloud:

Connecting the |DK| to nRF Cloud
*********************************

.. |DK| replace:: nRF9160 DK

.. dk_nrf_cloud_start

To transmit data from your |DK| to nRF Cloud, you need an `nRF Cloud`_ account.
nRF Cloud is Nordic Semiconductor's platform for connecting your IoT devices to the cloud, viewing and analyzing device message data, prototyping ideas that use Nordic Semiconductor's chips, and more.

.. dk_nrf_cloud_end

.. _creating_cloud_account:

Creating an nRF Cloud account
=============================

.. nrf_cloud_account_start

To create an nRF Cloud account, complete the following steps:

1. Open the `nRF Cloud`_ landing page and click :guilabel:`Register`.
#. Enter your email address and choose a password, then click :guilabel:`Create Account`.
   nRF Cloud will send you a verification email.
#. Copy the 6-digit verification code and paste it into the registration dialog box.
   If you do not see the verification email, check your junk mail for an email from ``no-reply@verificationemail.com``.

   If you closed the registration dialog box, you can repeat Step 1 and then click :guilabel:`Already have a code?`.
   Then enter your email and the verification code.

You can now log in to `nRF Cloud`_ with your email and password.
After logging in, you are taken to the dashboard view that displays your device count and service usage.
Next, you need to activate the SIM card you will use in the |DK|.

.. nrf_cloud_account_end

.. _nrf9160_gs_connect_to_cloud:

Connecting to nRF Cloud
=======================

.. nrf_cloud_connection_start

You must activate your SIM card and add the |DK| to your nRF Cloud account.

.. note::

   If you activated your iBasis SIM card before, click the :guilabel:`Skip SIM setup` in Step 4 instead of filling in the information.

   If you are using a SIM card from another provider, make sure you activate it through your network operator, then click :guilabel:`Skip SIM setup` in Step 4 instead of filling in the information.

To activate the iBasis SIM card that comes shipped with the |DK| and add the |DK| to nRF Cloud, complete the following steps:

1. Log in to the `nRF Cloud`_ portal.
#. Click the :guilabel:`+` icon in the top left corner.

   .. figure:: images/nrfcloud_plus_sign_callout.png
      :alt: nRF Cloud - Plus icon

      nRF Cloud - Plus icon

   The :guilabel:`Add New` menu opens.

   .. figure:: images/nrfcloud_add_lte_device1.png
      :alt: nRF Cloud - Add New menu

      nRF Cloud - Add New menu

#. In the :guilabel:`Add New` menu, click :guilabel:`LTE Device`.
   The **Add LTE Device** page opens in the **Verify SIM Info** view.

   .. figure:: images/nrfcloud_activating_sim.png
      :alt: nRF Cloud - Add LTE Device page, Verify SIM Info view

      nRF Cloud - Add LTE Device page, Verify SIM Info view

#. Complete the following steps in the **Activate SIM Card** view to activate your iBasis SIM card:

   a. Enter the 18-digit :term:`Integrated Circuit Card Identifier (ICCID)` or the 19-digit :term:`eUICC Identifier (EID)` in the **SIM ICCID/EID** text box.

      .. note::
         The SIM cards can have either the EID or the ICCID printed on it.

   #. Enter the :term:`Personal Unblocking Key (PUK)` in the **PUK** text box.

      The PUK is printed on the SIM card.
      Reveal the PUK by scratching off the area on the back of the SIM card.
   #. Accept the Terms and the Privacy Policy.
   #. Click the :guilabel:`Activate SIM` button.

   After the SIM card is activated, you are taken to the **Add LTE Device** view.
   Leave the browser window open and continue with the next step before you enter the information on this window.

.. nrf_cloud_connection_end

5. Punch out the nano-SIM from the SIM card and plug it into the SIM card holder on the nRF9160 DK.
#. Connect the nRF9160 DK to the computer with a USB cable and turn it on, or reset the device if it is already turned on.
#. Wait up to three minutes for the device to find the cellular network and connect to the nRF Cloud server.

   At this stage, the device is provisioned on nRF Cloud, but not yet associated with your nRF Cloud account.
   When the DK has connected, the **LED3** double pulse blinks to indicate that user association is required and you can move to the next step.

#. In the **Add LTE Device** view from Step 4, enter your device ID and ownership code (**PIN/HWID**).

   .. figure:: images/nrfcloud_add_lte_device.png
      :alt: nRF Cloud - Add LTE Device view

      nRF Cloud - Add LTE Device view

   * **Device ID:** The device ID is composed of *nrf-* and the 15-digit :term:`International Mobile (Station) Equipment Identity (IMEI)` number that is printed on the label of your nRF9160 DK.
     For example, *nrf-123456789012345*.
   * **PIN/HWID:** The ownership code is the PIN or the hardware ID of your DK, and it is found on the label of your nRF9160 DK.
     This is not the PIN code for your SIM card.

     If the label contains a PIN in addition to the IMEI number, enter this pin.
     If it does not contain a PIN, enter the Hardware ID (HWID) HEX code, with or without colons.
     For example, *AA:BB:CC:DD:EE:FF* or *AABBCCDDEEFF*.

     .. note::

        The ownership code serves as a password and proves that you own the specific nRF9160 DK.
        Therefore, do not share it with anyone.

#. Click the :guilabel:`Add Device` button and wait for the device to reconnect to nRF Cloud.
   It is normal for the device to disconnect and reconnect multiple times during device provisioning.

The nRF9160 DK is now added to your nRF Cloud account.
This is indicated by the blinking of **LED3** on the DK, which shows that it is publishing data.
See :ref:`Asset Tracker v2 LED indication <led_indication>` for more information.

On nRF Cloud, you can access the device by clicking :guilabel:`Devices` under :guilabel:`Device Management` in the navigation pane on the left.

.. _nrf9160_gs_testing_dk:

Testing the DK
**************

After successfully associating your nRF9160 DK with your nRF Cloud account, you can start testing it.
The application programmed in the DK is :ref:`asset_tracker_v2`, and it is used for the testing.

For a basic test, complete the following steps:

1. Turn on or reset your nRF9160 DK.
#. Log in to the `nRF Cloud`_ portal.
#. Click :guilabel:`Devices` under :guilabel:`Device Management` in the navigation pane on the left.

   .. figure:: images/nrfcloud_devices.png
      :alt: nRF Cloud - Devices

      nRF Cloud - Devices

#. From the **Devices** view, open the entry for your device.
#. Observe that the DK is sending data to nRF Cloud.

If you experience problems and need to check the log messages, open nRF Connect for Desktop and launch the LTE Link Monitor app.
After connecting to your DK, you can see the log messages in the terminal view.

.. _nrf9160_gs_testing_cellular:

Testing the cellular connection with the AT Client sample
=========================================================

The :ref:`at_client_sample` sample enables you to send AT commands to the modem of your nRF9160 DK to test and monitor the cellular connection.
You can use it to troubleshoot and debug any connection problems.

Complete the following steps to test the cellular connection using the AT Client sample:

1. Follow the steps in :ref:`nrf9160_gs_updating_fw_application` to program the sample to the DK.
   When choosing the HEX file, choose `nrf9160dk_at_client_<version-number>.hex` instead of one for Asset Tracker v2.
#. Test the AT Client sample as described in the Testing section of the :ref:`at_client_sample` documentation.

.. _ug_nrf9160_gs_testing_gnss:

Testing the GNSS functionality
==============================

:ref:`asset_tracker_v2` supports acquiring GNSS position and transmitting it to nRF Cloud.

To achieve the fastest Time To First Fix of GNSS position, the following conditions need to be met:

* The device must be able to connect to nRF Cloud.
  You can confirm this by checking whether the status of your DK is displayed correctly on the nRF Cloud portal.
  The cloud connection is used to download GPS assistance data.
* Your network operator should support Power Saving Mode (PSM) or Extended Discontinuous Reception (eDRX) with the SIM card that you are using.
  If you are using an iBasis SIM card, check the `iBasis network coverage spreadsheet`_ to see the supported features and network coverage for different countries.

  The device may be able to acquire a GNSS position fix even if the network does not support PSM or eDRX for your SIM card, but it will likely take longer to do so.

For best results retrieving GNSS data, place the nRF9160 DK outside with a clear view of the sky.
It might also work indoors if the device is near a window.

Complete the following steps to test the GNSS functionality:

1. If you have an external antenna for your nRF9160 DK, attach it to connector **J2** to the left of the LTE antenna.
   See `nRF9160 DK GPS`_ for more information.
#. Turn on or reset your DK.
#. Log in to the `nRF Cloud`_ portal.
#. Click :guilabel:`Devices` under :guilabel:`Device Management` in the navigation pane on the left.

   .. figure:: images/nrfcloud_devices.png
      :alt: nRF Cloud - Devices

      nRF Cloud - Devices

#. From the **Devices** view, open the entry for your device.
#. Observe that after a while, the GNSS data is displayed on the map in the **GPS Data** card on nRF Cloud.

Next steps
**********

You have now completed getting started with the nRF9160 DK.
See the following links for where to go next:

* :ref:`installation` and :ref:`configuration_and_build` documentation to install the |NCS| and learn more about its development environment.
* :ref:`ug_nrf9160` for more advanced topics related to the nRF9160 DK.
