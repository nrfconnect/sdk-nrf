.. _nrf9160_gs_connect_to_cloud:
.. _nrf9160_gs_connecting_dk_to_cloud:
.. _nrf91_gs_connecting_dk_to_cloud:

Connecting the nRF91 Series DK to nRF Cloud
###########################################

.. contents::
   :local:
   :depth: 2

.. |DK| replace:: nRF91 Series DK

.. dk_nrf_cloud_start

To transmit data from your |DK| to nRF Cloud, you need an `nRF Cloud`_ account.
nRF Cloud is Nordic Semiconductor's platform for connecting your IoT devices to the cloud, viewing and analyzing device message data, prototyping ideas that use Nordic Semiconductor's chips, and more.

Once you have an account, activate your SIM card and add the device to your nRF Cloud account.

.. note::

   The instructions assume you are activating an iBasis SIM card that came with your |DK|.

   If you are using a SIM card from another provider, make sure you activate it through your network operator before adding the device to nRF Cloud.

.. dk_nrf_cloud_end

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

Activating the SIM card
***********************

.. nrf_cloud_activate_sim_start

To activate the iBasis SIM card that comes shipped with the |DK|, complete the following steps in the `nRF Cloud`_ portal.
Make sure you are logged in to the portal.

1. Click :guilabel:`SIM Cards` under :guilabel:`Device Management` in the navigation pane on the left.

   .. figure:: ../nrf91/images/nrfcloud_simcards.png
      :alt: nRF Cloud - SIM Cards

      nRF Cloud - SIM Cards

#. Click :guilabel:`Add SIM`.

   .. figure:: ../nrf91/images/nrfcloud_addsim.png
      :alt: nRF Cloud - Add SIM

      nRF Cloud - Add SIM

   The **Add SIM** page opens in the **Verify SIM Card** view.

#. Complete the following steps on the **Add SIM** page to activate your iBasis SIM card:

   .. figure:: ../nrf91/images/nrfcloud_activating_sim.png
      :alt: nRF Cloud - Add SIM page

      nRF Cloud - Add SIM page

   a. Enter the 18-digit :term:`Integrated Circuit Card Identifier (ICCID)` or the 19-digit :term:`eUICC Identifier (EID)` in the **SIM ICCID/EID** text box.

      .. note::
         The SIM cards can have either the EID, the ICCID, or neither printed on it.

   #. Enter the :term:`Personal Unblocking Key (PUK)` in the **PUK** text box.

      The PUK is printed on the SIM card.
      Reveal the PUK by scratching off the area on the back of the SIM card.
   #. Accept the Terms and the Privacy Policy.
   #. Click the :guilabel:`Activate SIM` button.

      The **Add SIM** page changes to the **Activate SIM Card** view.

   #. Fill in the required information.
   #. Click :guilabel:`Save` to complete the activation.

After the SIM card is activated, you can add your |DK| to nRF Cloud.

.. nrf_cloud_activate_sim_end

Adding the nRF91 Series DK to nRF Cloud
***************************************

.. nrf_cloud_add_device_start

To add your |DK| device to nRF Cloud, refer to the `Adding a device to your account`_ section of the nRF Cloud documentation.

.. nrf_cloud_add_device_end
