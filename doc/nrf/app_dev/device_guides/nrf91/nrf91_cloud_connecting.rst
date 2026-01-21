.. _nrf9160_gs_connect_to_cloud:
.. _nrf9160_gs_connecting_dk_to_cloud:

Connecting the nRF91 Series DK to nRF Cloud
###########################################

.. contents::
   :local:
   :depth: 2

.. |DK| replace:: nRF9160 DK

.. dk_nrf_cloud_start

To transmit data from your nRF91 Series DK to nRF Cloud, you need an `nRF Cloud`_ account.
nRF Cloud is Nordic Semiconductor's platform for connecting your IoT devices to the cloud, viewing and analyzing device message data, prototyping ideas that use Nordic Semiconductor's chips, and more.

.. dk_nrf_cloud_end

If you used the :ref:`Quick Start app to get started <ug_nrf9160_gs>`, you have an account and can :ref:`add the nRF91 Series DK to nRF Cloud <nrf9160_gs_cloud_add_device>`.

.. _creating_cloud_account:

Creating an nRF Cloud account
*****************************

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

.. nrf_cloud_account_end

.. _nrf9160_gs_cloud_add_device:

Adding the nRF91 Series DK to nRF Cloud
***************************************

  .. tabs::

     .. group-tab:: nRF91x1 DK

        To add the nRF91x1 DK to nRF Cloud, refer to the `Adding a device to your account`_ section of the nRF Cloud documentation.

     .. group-tab:: nRF9160 DK

        .. |led_cloud_association| replace:: the **LED3** double pulse blinks
        .. |led_publishing_data| replace:: blinking of **LED3**

        .. nrf_cloud_add_device_start

        To add the |DK| to nRF Cloud, complete the following steps.
        Make sure you are logged in to the `nRF Cloud`_ portal and have an activated SIM card in the SIM card slot of the |DK|.

        1. Connect the |DK| to the computer with a USB cable and switch it on, or reset the device if it is already switched on.
        #. Wait up to three minutes for the device to find the cellular network and connect to the nRF Cloud server.

           At this stage, the |DK| is provisioned on nRF Cloud, but not yet associated with your nRF Cloud account.
           When the device has connected, |led_cloud_association| to indicate that user association is required and you can move to the next step.

        #. Click :guilabel:`Devices` under :guilabel:`Device Management` in the navigation pane on the left.

           .. figure:: images/nrfcloud_devices.png
              :alt: nRF Cloud - Devices

              nRF Cloud - Devices

        #. Click :guilabel:`Add Devices`.

           .. figure:: images/nrfcloud_add_devices.png
              :alt: nRF Cloud - Add Devices

              nRF Cloud - Add Devices

           The **Select Device Type** pop-up opens.

        #. Click :guilabel:`LTE Device` in the **Select Device Type** pop-up.

           .. figure:: images/nrfcloud_selectdevicetype.png
              :alt: nRF Cloud - Select Device Type

              nRF Cloud - Select Device Type

        #. Enter your device ID and ownership code (**PIN/HWID**) on the **Add LTE Device** page.

           .. figure:: images/nrfcloud_add_lte_device.png
              :alt: nRF Cloud - Add LTE Device

              nRF Cloud - Add LTE Device

           * **Device ID:** The device ID is composed of *nrf-* and the 15-digit :term:`International Mobile (Station) Equipment Identity (IMEI)` number that is printed on the label of your |DK|.
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

        #. Click :guilabel:`Continue` and wait for the device to reconnect to nRF Cloud.
           It is normal for the device to disconnect and reconnect multiple times during device provisioning.

           If you have not yet activated the iBasis SIM card that came with your |DK|, click :guilabel:`Activate iBasis SIM` instead, and follow the instructions.

           The |DK| is now added to your nRF Cloud account.
           This is indicated by the |led_publishing_data|, which shows that the device is publishing data.

           On nRF Cloud, you can access the device by clicking :guilabel:`Devices` under :guilabel:`Device Management` in the navigation pane on the left.

        .. nrf_cloud_add_device_end
