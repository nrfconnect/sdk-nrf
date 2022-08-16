.. _ug_nrf9160:

Developing with nRF9160 DK
##########################

.. contents::
   :local:
   :depth: 2

The nRF9160 DK is a hardware development platform used to design and develop application firmware on the nRF9160 LTE Cat-M1 and Cat-NB1 :term:`System in Package (SiP)`.

See the `nRF9160 DK Hardware`_ guide for detailed information about the nRF9160 DK hardware.
To get started with the nRF9160 DK, follow the steps in the :ref:`ug_nrf9160_gs` section.
If you are not familiar with the |NCS| and the development environment, see the :ref:`introductory documentation <getting_started>`.

.. _nrf9160_ug_intro:

Board controller
****************

The nRF9160 DK contains an nRF52840 SoC that is used to route some of the nRF9160 SiP pins to different components on the DK, such as LEDs and buttons, and to specific pins of the nRF52840 SoC itself.
For a complete list of all the routing options available, see the `nRF9160 DK board control section in the nRF9160 DK User Guide`_.

The nRF52840 SoC on the DK comes preprogrammed with a firmware.
If you need to restore the original firmware at some point, download the nRF9160 DK board controller firmware from the `nRF9160 DK downloads`_ page.
To program the HEX file, use nrfjprog (which is part of the `nRF Command Line Tools`_).

If you want to route some pins differently from what is done in the preprogrammed firmware, program the :ref:`zephyr:hello_world` sample instead of the preprogrammed firmware.
Build the sample (located under ``ncs/zephyr/samples/hello_world``) for the nrf9160dk_nrf52840 board.
To change the routing options, enable or disable the corresponding devicetree nodes for that board as needed.
See :ref:`zephyr:nrf9160dk_board_controller_firmware` for detailed information.

.. _nrf9160_ug_updating_cloud_certificate:

Updating the nRF Cloud certificate
**********************************

When you wish to use nRF Cloud's REST Application Programming Interface (API), the DK requires a valid security certificate.
This means that you need to update the certificate stored on the DK.
To do so, you need to download a new certificate and then provision it to your DK.

.. _downloading_cloud_certificate:

Downloading the nRF Cloud certificate
=====================================

Complete the following steps to download the nRF Cloud certificate for your nRF9160 DK:

1. Go to `nRF Cloud`_ and sign in.
#. Click :guilabel:`Device Management` in the left pane and select :guilabel:`Provision Devices`.

   .. figure:: /images/nrfcloud_provisiondevices.png
      :alt: nRF Cloud - Provision Devices

      nRF Cloud - Provision Devices

#. In the  page that opens up, click the gear icon near the upper right corner and select :guilabel:`Create JITP Certificates`.

   .. figure:: /images/nrfcloud_provisiondevices_gearicon.png
      :alt: nRF Cloud - Gear icon in Provision Devices

      nRF Cloud - Gear icon in Provision Devices

   The **Create JITP Certificates** dialog box opens.

   .. figure:: /images/nrfcloud_jitpcertificates.png
      :alt: nRF Cloud - Create JITP Certificates dialog box

      nRF Cloud - Create JITP Certificates dialog box

#. Enter the following information:

   * **Device ID:** the device ID is composed of *nrf-* and the 15-digit :term:`International Mobile (Station) Equipment Identity (IMEI)` number that is printed on the label of your nRF9160 DK.
     For example, *nrf-123456789012345*.
   * **Ownership code:** the ownership code is the PIN or the hardware ID of your DK, and it is found on the label of your nRF9160 DK.

     If the label contains a PIN in addition to the IMEI number, enter this pin.
     If it does not contain a PIN, enter the Hardware ID (HWID) HEX code, with or without colons.
     For example, *AA:BB:CC:DD:EE:FF* or *AABBCCDDEEFF*.

     .. note::

        The ownership code serves as a password and proves that you own the specific nRF9160 DK.
        Therefore, do not share it with anyone.

#. Click :guilabel:`Download Certificate` and save the :file:`.cert.json` file to a folder of your choice.

   .. note::

      The certificate contains all the information that is needed to connect your nRF9160 DK to nRF Cloud.
      Therefore, do not share it with anyone.

.. _provisioning_cloud_certificate:

Provisioning the nRF Cloud certificate
======================================

After downloading the certificate, you must provision it to your nRF9160 DK.

.. note::

   The application firmware on the nRF9160 DK must support long AT commands up to 3 kB to provision the certificate.
   If you have :ref:`updated the application firmware <nrf9160_gs_updating_fw_application>`, this requirement is fulfilled.

Complete the following steps to provision the certificate:

1. Open the LTE Link Monitor app from nRF Connect for Desktop.
#. In the **SETTINGS** section of the side panel, deselect the checkbox for :guilabel:`Automatic requests` if it is selected.

   .. figure:: /images/ltelinkmonitor_automaticrequests.png
      :alt: LTE Link Monitor - Automatic requests check box

#. If you have already inserted the SIM card into your DK, remove it before you continue.
#. Connect the DK to the computer with a micro-USB cable, and turn it on.
#. Click :guilabel:`Select device` and select the DK from the drop-down list.
   You can identify the nRF9160 DK by the fact that it has three COM ports.

   .. figure:: /images/ltelinkmonitor_selectdevice.png
      :alt: LTE Link Monitor - COM ports

      LTE Link Monitor - COM ports

   If the three COM ports are not visible, press ``Ctrl+R`` in Windows or ``command+R`` in macOS to restart the Programmer application.

   The drop-down text changes to the type of the selected device, with the SEGGER ID below the name.

#. Click :guilabel:`TERMINAL` in the navigation bar to switch to the terminal view.

   .. figure:: /images/ltelinkmonitor_navigationterminal.png
      :alt: LTE Link Monitor - Terminal

      LTE Link Monitor - Terminal

#. Enter ``AT+CFUN=4`` in the text field for AT commands and click :guilabel:`Send`.
   This AT command puts the modem to offline state.
#. Enter ``AT+CFUN?`` in the text field for AT commands and click :guilabel:`Send`.
   This AT command returns the state of the modem.

   The command must return ``+CFUN: 4``, which indicates that the modem is in offline state.
   If it returns a different value, repeat the previous step.
#. Click :guilabel:`CERTIFICATE MANAGER` in the navigation bar to switch to the certificate manager view.

   .. figure:: /images/ltelinkmonitor_navigationcertificatemanager.png
      :alt: LTE Link Monitor - Certificate Manager

      LTE Link Monitor - Certificate Manager

#. Click :guilabel:`Load from JSON` and select the :file:`*.cert.json` file that you downloaded from nRF Cloud.
   Alternatively, you can drag and drop the file onto the GUI.

   .. figure:: /images/ltelinkmonitor_loadjson.png
      :alt: LTE Link Monitor - Load from JSON

      LTE Link Monitor - Load from JSON

#. Ensure that the **Security tag** is set to ``16842753``, which is the security tag for nRF Cloud credentials.
#. Click :guilabel:`Update certificate`.

   .. figure:: /images/ltelinkmonitor_updatecertificates.png
      :alt: LTE Link Monitor - Update certificates

      LTE Link Monitor - Update certificates

   The log message "Certificate update completed" indicates that the certificate was provisioned successfully.
   If you encounter any errors, switch to the terminal view and check the output of the AT commands that were sent to the nRF9160 DK modem.

   .. note::

      If you have connected your nRF9160 DK to nRF Cloud before, you must delete the device there after provisioning the certificate.
      Open the entry for your device from the **Devices** view, then click the gear icon to the right of the device's name, and select :guilabel:`Delete Device`.
      Then, add the nRF9160 DK again as described in :ref:`nrf9160_gs_connecting_dk_to_cloud`


Build targets
*************

In Zephyr, :ref:`zephyr:nrf9160dk_nrf9160` is divided into two different build targets:

* ``nrf9160dk_nrf9160`` for firmware in the secure domain
* ``nrf9160dk_nrf9160_ns`` for firmware in the non-secure domain

.. note::
   In |NCS| releases before v1.6.1, the build target ``nrf9160dk_nrf9160_ns`` was named ``nrf9160dk_nrf9160ns``.

Make sure to select a suitable build target when building your application.

.. _build_pgm_nrf9160:

Building and programming
************************

You can program applications and samples on the nRF9160 DK after obtaining the corresponding firmware images.

Download the latest application and modem firmware from the `nRF9160 DK Downloads`_ page.

To program applications using the Programmer app from `nRF Connect for Desktop`_, follow the instructions in :ref:`nrf9160_gs_updating_fw_application`.
In Step 2, set the switch to **nRF91** or **nRF52** as appropriate for the application or sample you are programming.
See the `Device programming section in the nRF9160 DK User Guide`_ for more information.
Likewise, in Step 7, choose the :file:`.hex` file for the application you are programming.

.. note::
   When you update the application firmware on an nRF9160 DK, you must also update the modem firmware as described in :ref:`nrf9160_gs_updating_fw_modem`.

.. _build_pgm_nrf9160_vsc:

Building and programming using |VSC|
====================================

|vsc_extension_instructions|

Complete the following steps after installing the |nRFVSC|:

.. |sample_path_vsc| replace:: :file:`ncs/nrf/applications/asset_tracker_v2`

.. |vsc_sample_board_target_line| replace:: you must use the build target ``nrf9160dk_nrf9160_ns`` when building the application code for the nRF9160 DK

.. include:: /includes/vsc_build_and_run.txt

#. Program the application:

.. prog_nrf9160_start
..

   a. Set the **SW10** switch (marked PROG/DEBUG) in the **nRF91** position to program the nRF9160 application.
      In nRF9160 DK v0.9.0 and earlier, the switch is called **SW5**.
   #. Connect the nRF9160 DK to your PC using a USB cable.
   #. Power on the nRF9160 DK.

.. prog_nrf9160_end
..

   d. In |VSC|, click the :guilabel:`Flash` option in the :guilabel:`Actions View`.

      If you have multiple boards connected, you are prompted to pick a device at the top of the screen.

      A small notification banner appears in the bottom-right corner of |VSC| to display the progress and confirm when the flashing is complete.

.. _build_pgm_nrf9160_cmdline:

Building and programming on the command line
============================================

.. |cmd_folder_path| replace:: on the nRF9160 DK

.. |cmd_build_target| replace:: ``nrf9160dk_nrf9160_ns`` when building the application code for the nRF9160 DK


.. include:: /includes/cmd_build_and_run.txt

#. Program the application:

.. include:: ug_nrf9160.rst
   :start-after: prog_nrf9160_start
   :end-before: prog_nrf9160_end
..

   d. Program the sample or application to the device using the following command:

      .. code-block:: console

         west flash

      .. note::
         When programming with the :ref:`asset_tracker_v2` application, use the ``west flash --erase`` command.
         The application has secure boot enabled by default that includes data in the :ref:`One-Time Programmable region (OTP)<bootloader_provisioning_otp>`.
         This means that everything must be erased before flashing.

      The device resets and runs the programmed sample or application.

Board revisions
***************

nRF9160 DK v0.14.0 and later has additional hardware features that are not available on earlier versions of the DK:

* External flash memory
* I/O expander

To make use of these features, specify the board revision when building your application.

.. note::
   You must specify the board revision only if you use features that are not available in all board revisions.
   If you do not specify a board revision, the firmware is built for the default revision (v0.7.0).
   Newer revisions are compatible with the default revision.

To specify the board revision, append it to the build target when building.
For example, when building a non-secure application for nRF9160 DK v1.0.0, use ``nrf9160dk_nrf9106_ns@1.0.0`` as build target.

See :ref:`zephyr:application_board_version` and :ref:`zephyr:nrf9160dk_additional_hardware` for more information.

.. _nrf9160_ug_drivs_libs_samples:

Available drivers, libraries, and samples
*****************************************

See the :ref:`drivers`, :ref:`libraries`, and :ref:`nRF9160 samples <nrf9160_samples>` sections and the respective repository folders for up-to-date information.
