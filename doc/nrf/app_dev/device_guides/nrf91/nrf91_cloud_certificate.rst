.. _nrf9160_ug_updating_cloud_certificate:
.. _nrf9161_ug_updating_cloud_certificate:

Updating the nRF Cloud certificate
##################################

.. contents::
   :local:
   :depth: 2

When you wish to use nRF Cloud's REST Application Programming Interface (API), the Development Kit (DK) requires a valid security certificate.
This means that you need to update the certificate stored on the DK.
You need to download a new certificate and then provision it to your DK.

.. _downloading_cloud_certificate_nRF9160:
.. _downloading_cloud_certificate_nRF9161:
.. _downloading_cloud_certificate_nRF91x1:

Downloading the nRF Cloud certificate
*************************************

To download the nRF Cloud certificate for your DK, complete the following steps.
Make sure you are logged in to the `nRF Cloud`_ portal.

1. Click :guilabel:`Devices` under :guilabel:`Device Management` in the navigation pane on the left.

   .. figure:: /gsg_guides/images/nrfcloud_devices.png
      :alt: nRF Cloud - Devices

      nRF Cloud - Devices

#. Click :guilabel:`Add Devices`.

   .. figure:: /gsg_guides/images/nrfcloud_add_devices.png
      :alt: nRF Cloud - Add Devices

      nRF Cloud - Add Devices

   The **Select Device Type** pop-up opens.

#. Click :guilabel:`LTE Device` in the **Select Device Type** pop-up.

   .. figure:: /gsg_guides/images/nrfcloud_selectdevicetype.png
      :alt: nRF Cloud - Select Device Type

      nRF Cloud - Select Device Type

#. Click **Create JITP Certificates** at the bottom of the page.

   The **Create JITP Certificates** dialog box opens.

   .. figure:: images/nrfcloud_jitpcertificates.png
      :alt: nRF Cloud - Create JITP Certificates dialog box

      nRF Cloud - Create JITP Certificates dialog box

#. Enter the following information:

   * **Device ID:** the device ID is composed of *nrf-* and the 15-digit :term:`International Mobile (Station) Equipment Identity (IMEI)` number that is printed on the label of your DK.
     For example, *nrf-123456789012345*.
   * **Ownership code:** the ownership code is the PIN or the hardware ID of your DK, and it is found on the label of your DK.

     If the label contains a PIN in addition to the IMEI number, enter this pin.
     If it does not contain a PIN, enter the Hardware ID (HWID) HEX code, with or without colons.
     For example, *AA:BB:CC:DD:EE:FF* or *AABBCCDDEEFF*.

     .. note::

        The ownership code serves as a password and proves that you own the specific DK.
        Therefore, do not share it with anyone.

#. Click :guilabel:`Download Certificate` and save the :file:`.cert.json` file to a folder of your choice.

   .. note::

      The certificate contains all the information that is needed to connect your DK to nRF Cloud.
      Therefore, do not share it with anyone.

#. Close the **Create JITP Certificates** dialog box.

.. _provisioning_cloud_certificate:

Provisioning the nRF Cloud certificate
**************************************

After downloading the certificate, you must provision it to your DK.

.. note::

   The application firmware on the DK must support long AT commands up to three kB to provision the certificate.
   If you have updated the application firmware using the instructions in either the :ref:`nrf9160_gs_updating_fw` documentation or the :ref:`nrf9160_ug_updating_fw_programmer` or :ref:`build_pgm_nrf9160` sections, this requirement is fulfilled.

Complete the following steps to provision the certificate:

1. Start nRF Connect for Desktop and install the `Cellular Monitor`_ app.
#. Open the Cellular Monitor app.
#. Connect the DK to the computer with a micro-USB cable, and turn it on.
#. Click :guilabel:`Select device` and select the DK from the drop-down list.

   .. tabs::

      .. group-tab:: nRF9161 DK

         .. figure:: images/cellularmonitor_selectdevice_nrf9161.png
            :alt: Cellular Monitor - Select device

            Cellular Monitor - Select device

      .. group-tab:: nRF9160 DK

         .. figure:: images/cellularmonitor_selectdevice1_nrf9160.png
            :alt: Cellular Monitor - Select device

            Cellular Monitor - Select device

   The drop-down text changes to the type of the selected device, with the SEGGER ID below the name.

#. Click the :guilabel:`Open Serial Terminal` option of the `Cellular Monitor`_ app to open the Serial Terminal.

   .. tabs::

      .. group-tab:: nRF9161 DK

         .. figure:: images/cellularmonitor_open_serial_terminal_nrf9161.png
            :alt: Cellular Monitor - Open Serial Terminal

            Cellular Monitor - Open Serial Terminal

      .. group-tab:: nRF9160 DK

         .. figure:: images/cellularmonitor_open_serial_terminal.png
            :alt: Cellular Monitor - Open Serial Terminal

            Cellular Monitor - Open Serial Terminal

#. Enter ``AT+CFUN=4`` in the text field for AT commands and click :guilabel:`Send`.
   This AT command sets the modem to offline state.
#. Enter ``AT+CFUN?`` in the text field for AT commands and click :guilabel:`Send`.
   This AT command returns the state of the modem.

   The command must return ``+CFUN: 4``, which indicates that the modem is in offline state.
   If it returns a different value, repeat the previous step.
#. Open the Cellular Monitor app.
#. Click :guilabel:`CERTIFICATE MANAGER` in the navigation bar to switch to the certificate manager view.

   .. tabs::

      .. group-tab:: nRF9161 DK

         .. figure:: images/cellularmonitor_navigationcertificatemanager_nrf9161.png
            :alt: Cellular Monitor - Certificate Manager

            Cellular Monitor - Certificate Manager

      .. group-tab:: nRF9160 DK

         .. figure:: images/cellularmonitor_navigationcertificatemanager.png
            :alt: Cellular Monitor - Certificate Manager

            Cellular Monitor - Certificate Manager

#. Click :guilabel:`Load from JSON` and select the :file:`*.cert.json` file that you downloaded from nRF Cloud.
   Alternatively, you can drag and drop the file onto the GUI.
#. Ensure that the **Security tag** is set to ``16842753``, which is the security tag for nRF Cloud credentials.
#. Click :guilabel:`Update certificate`.

   The log message "Certificate update completed" indicates that the certificate was provisioned successfully.
   If you encounter any errors, switch to the terminal view and check the output of the AT commands that were sent to the nRF9160 DK modem.

   .. note::

      If you have connected your DK to nRF Cloud before, you must delete the device there after provisioning the certificate.
      Open the entry for your device from the **Devices** view, then click the gear icon to the right of the device's name, and select :guilabel:`Delete Device`.
      Then, add the DK again as described in :ref:`nrf9160_gs_connecting_dk_to_cloud`.
