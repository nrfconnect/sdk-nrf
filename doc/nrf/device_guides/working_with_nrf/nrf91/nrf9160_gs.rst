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
.. _nrf9160_gs_updating_fw:

Getting started using Quick Start
*********************************

To work with the nRF9160 DK firmware and certificates, complete the following steps:

1. Install `nRF Connect for Desktop`_.
#. Start nRF Connect for Desktop and install the Quick Start application, a cross-platform tool for guided setup and installation procedures.
#. Prepare your DK:

   a. Punch out the nano-SIM from the SIM card and plug it into the SIM card holder on the nRF9160 DK.
   #. Make sure the **PROG/DEBUG SW10** switch on the nRF9160 DK is set to **nRF91**.
      On DK v0.9.0 and earlier, this is the **SW5** switch.
   #. Connect the nRF9160 DK to the computer with a micro-USB cable, and then turn the DK on.

#. Use the Quick Start app to update the DK firmware.
   Follow the steps in the application and pay attention to the following actions:

   * When selecting which application to program, select the **Asset Tracking** option.
   * In the SIM Setup step, read and copy the :term:`Integrated Circuit Card Identifier (ICCID)` of the inserted micro-SIM.
     This is needed for connecting to nRF Cloud.

   If you have not yet installed the |NCS| and selected the preferred development environment, the application will point you to the related resources.
   It will also provide you with links to the recommended learning materials from Nordic Semiconductor related to cellular development.

.. _nrf9160_gs_connect_to_cloud:
.. _nrf9160_gs_connecting_dk_to_cloud:

Connecting the |DK| to nRF Cloud
*********************************

.. |DK| replace:: nRF9160 DK

.. dk_nrf_cloud_start

To transmit data from your |DK| to nRF Cloud, you need an `nRF Cloud`_ account.
nRF Cloud is Nordic Semiconductor's platform for connecting your IoT devices to the cloud, viewing and analyzing device message data, prototyping ideas that use Nordic Semiconductor's chips, and more.

Once you have an account, activate your SIM card and add the |DK| to your nRF Cloud account.

.. note::

   The instructions assume you are activating an iBasis SIM card that came with your |DK|.

   If you are using a SIM card from another provider, make sure you activate it through your network operator before adding the device to nRF Cloud.

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

.. _nrf9160_gs_cloud_activate_sim:

Activating the SIM card
=======================

.. |firmware_section_link| replace:: :ref:`nrf9160_gs_updating_fw`

.. nrf_cloud_activate_sim_start

To activate the iBasis SIM card that comes shipped with the |DK|, complete the following steps in the `nRF Cloud`_ portal.
Make sure you are logged in to the portal.

1. Click the :guilabel:`+` icon in the top left corner.

   .. figure:: images/nrfcloud_plus_sign_callout.png
      :alt: nRF Cloud - Plus icon

      nRF Cloud - Plus icon

   The :guilabel:`Add New` menu opens.

   .. figure:: images/nrfcloud_addnew.png
      :alt: nRF Cloud - Add New menu

      nRF Cloud - Add New menu

#. In the :guilabel:`Add New` menu, click :guilabel:`SIM Card`.
#. Complete the following steps on the **Add SIM** page to activate your iBasis SIM card:

   .. figure:: images/nrfcloud_activating_sim.png
      :alt: nRF Cloud - Add SIM page

      nRF Cloud - Add SIM page

   a. Enter the 18-digit :term:`Integrated Circuit Card Identifier (ICCID)` or the 19-digit :term:`eUICC Identifier (EID)` in the **SIM ICCID/EID** text box.

      If you followed the instructions in the |firmware_section_link| section, paste the copied ICCID into the **SIM ICCID/EID** box and remove the last two digits.

      .. note::
         The SIM card can have either the EID, the ICCID, or neither printed on it.

   #. Enter the :term:`Personal Unblocking Key (PUK)` in the **PUK** text box.

      The PUK is printed on the SIM card.
      Reveal the PUK by scratching off the area on the back of the SIM card.
   #. Accept the Terms and the Privacy Policy.
   #. Click the :guilabel:`Activate SIM` button.

After the SIM card is activated, you can add your |DK| to nRF Cloud.

.. nrf_cloud_activate_sim_end

.. _nrf9160_gs_cloud_add_device:

Adding the nRF9160 DK to nRF Cloud
==================================

.. |led_cloud_association| replace:: the **LED3** double pulse blinks
.. |led_publishing_data| replace:: blinking of **LED3**
.. |activate_sim_section| replace:: :ref:`nrf9160_gs_cloud_activate_sim`

.. nrf_cloud_add_device_start

To add the |DK| to nRF Cloud, complete the following steps.
Make sure you are logged in to the `nRF Cloud`_ portal.

1. Connect the |DK| to the computer with a USB cable and switch it on, or reset the device if it is already switched on.
#. Wait up to three minutes for the device to find the cellular network and connect to the nRF Cloud server.

   At this stage, the |DK| is provisioned on nRF Cloud, but not yet associated with your nRF Cloud account.
   When the device has connected, |led_cloud_association| to indicate that user association is required and you can move to the next step.
   See :ref:`Asset Tracker v2 LED indication <led_indication>` for more information.

#. Click the :guilabel:`+` icon in the top left corner.

   .. figure:: images/nrfcloud_plus_sign_callout.png
      :alt: nRF Cloud - Plus icon

      nRF Cloud - Plus icon

   The :guilabel:`Add New` menu opens.

   .. figure:: images/nrfcloud_addnew.png
      :alt: nRF Cloud - Add New menu

      nRF Cloud - Add New menu

#. In the :guilabel:`Add New` menu, click :guilabel:`LTE Device`.
   The **Add LTE Device** page opens in the **Verify SIM Info** view.

   .. figure:: images/nrfcloud_activating_sim.png
      :alt: nRF Cloud - Add LTE Device page, Verify SIM Info view

      nRF Cloud - Add LTE Device page, Verify SIM Info view

#. Complete the following steps in the **Activate SIM Card** view to activate your iBasis SIM card:

   a. Enter the 18-digit :term:`Integrated Circuit Card Identifier (ICCID)` or the 19-digit :term:`eUICC Identifier (EID)` in the **SIM ICCID/EID** text box.

      If you followed the instructions in the |firmware_section_link| section, paste the copied ICCID into the **SIM ICCID/EID** box.

      .. note::
         The SIM cards can have either the EID, the ICCID, or neither printed on it.

   #. Enter the :term:`Personal Unblocking Key (PUK)` in the **PUK** text box.

      The PUK is printed on the SIM card.
      Reveal the PUK by scratching off the area on the back of the SIM card.
   #. Accept the Terms and the Privacy Policy.
   #. Click the :guilabel:`Activate SIM` button.

   After the SIM card is activated, you are taken to the **Add LTE Device** view.
   Leave the browser window open and continue with the next step before you enter the information on this window.

.. nrf_cloud_connection_end

5. Connect the nRF9160 DK to the computer with a USB cable and turn it on, or reset the device if it is already turned on.
#. Wait up to three minutes for the device to find the cellular network and connect to the nRF Cloud server.

   At this stage, the device is provisioned on nRF Cloud, but not yet associated with your nRF Cloud account.
   When the DK has connected, the **LED3** double pulse blinks to indicate that user association is required and you can move to the next step.

#. In the **Add LTE Device** view from Step 4, enter your device ID and ownership code (**PIN/HWID**).

   .. figure:: images/nrfcloud_add_lte_device.png
      :alt: nRF Cloud - Add LTE Device

      nRF Cloud - Add LTE Device

   * **Device ID:** The device ID is composed of *nrf-* and the 15-digit :term:`International Mobile (Station) Equipment Identity (IMEI)` number that is printed on the label of your nRF9160 DK.
     For example, *nrf-123456789012345*.
     It is case sensitive, so make sure all the letters are lower-case.
   * **PIN/HWID:** The ownership code is the PIN or the hardware ID of your device, and it is found on the label of your |DK|.
     This is not the PIN code for your SIM card.

     If the label contains a PIN in addition to the IMEI number, enter this pin.
     If it does not contain a PIN, enter the Hardware ID (HWID) HEX code, with or without colons.
     For example, *AA:BB:CC:DD:EE:FF* or *AABBCCDDEEFF*.

     .. note::

        The ownership code serves as a password and proves that you own the specific |DK|.
        Therefore, do not share it with anyone.

#. Click the :guilabel:`Add Device` button.

   The **Do you need to activate an iBasis SIM?** pop-up opens.

#. Click :guilabel:`Device View` if you have an activated SIM.
   You are taken to the page for your device.

   Click :guilabel:`Add iBasis SIM` if you have not yet activated your iBasis SIM card.
   See the |activate_sim_section| section for more information.

#. Wait for the device to reconnect to nRF Cloud.
   It is normal for the device to disconnect and reconnect multiple times during device provisioning.

The |DK| is now added to your nRF Cloud account.
This is indicated by the |led_publishing_data|, which shows that the device is publishing data.
See :ref:`Asset Tracker v2 LED indication <led_indication>` for more information.

To access the page for your device later in the nRF Cloud portal, click :guilabel:`Devices` under :guilabel:`Device Management` in the navigation pane on the left.
This opens the **Devices** page, where you can select your device to access its device page.

.. nrf_cloud_add_device_end

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
