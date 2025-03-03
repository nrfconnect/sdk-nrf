.. _connect_nRF_cloud:
.. _thingy91_connect_to_cloud:

Connecting the |DK| to nRF Cloud
################################

.. contents::
   :local:
   :depth: 2

.. |DK| replace:: Thingy:91

.. include:: ../nrf91/nrf91_cloud_connecting.rst
   :start-after: dk_nrf_cloud_start
   :end-before: dk_nrf_cloud_end

Once you have an account, activate your SIM card and add the Thingy:91 to your nRF Cloud account.

.. note::

   The instructions assume you are activating an iBasis SIM card that came with your |DK|.

   If you are using a SIM card from another provider, make sure you activate it through your network operator before :ref:`adding the device to nRF Cloud <thingy91_cloud_add_device>`.

Creating an nRF Cloud account
*****************************

.. include:: ../nrf91/nrf91_cloud_connecting.rst
   :start-after: nrf_cloud_account_start
   :end-before: nrf_cloud_account_end

.. _thingy91_cloud_activate_sim:

Activating the SIM card
***********************

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

      If you followed the instructions in the :ref:`thingy91_update_firmware` section, paste the copied ICCID into the **SIM ICCID/EID** box and remove the last two digits.

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

.. _thingy91_cloud_add_device:

Adding the Thingy:91 to nRF Cloud
*********************************

.. |led_cloud_association| replace:: the RGB LED double pulse blinks as white
.. |led_publishing_data| replace:: RGB LED blinking green
.. |activate_sim_section| replace:: :ref:`thingy91_cloud_activate_sim`

.. include:: ../nrf91/nrf91_cloud_connecting.rst
   :start-after: nrf_cloud_add_device_start
   :end-before: nrf_cloud_add_device_end
