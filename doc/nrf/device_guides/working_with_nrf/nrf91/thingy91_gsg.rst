.. _ug_thingy91_gsg:

Getting started with Thingy:91
##############################

.. contents::
   :local:
   :depth: 2


This guide helps you get started with the Nordic Thingy:91.
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
* Micro-USB 2.0 cable

.. note::
   For getting started, you use a micro-USB cable to connect to the Thingy:91.
   When you start developing with your Thingy:91, it is recommended to use an external debug probe.

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

1. Install the `Cellular Monitor`_ app on the computer:

   a. Go to `nRF Connect for Desktop Downloads <Download nRF Connect for Desktop_>`_.
   #. Download and install nRF Connect for Desktop.
   #. Open `nRF Connect for Desktop`_.
   #. Find **Cellular Monitor** in the list of apps and click :guilabel:`Install`.

#. Prepare the Thingy:91 hardware:

   a. Open the box and take out the Thingy:91 and the iBasis SIM card it comes shipped with.
   #. Remove the protective cover.
   #. Punch out the nano-SIM from the SIM card and plug it into the SIM card holder on the Thingy:91.

      .. figure:: images/thingy91_insert_sim.svg
         :alt: Inserting SIM

         Inserting SIM

   #. Plug the Thingy:91 into the computer using a micro-USB cable.
   #. Power the Thingy:91 on by switching **SW1** to the **ON** position.

      .. figure:: images/thingy91_pwr_switch.svg
         :alt: Thingy:91 Power switch (SW1)

         Thingy:91 Power switch (SW1)

.. _thingy91_update_firmware:

Updating the Thingy:91 firmware
*******************************

Thingy:91 (v1.5.0 or lower) comes preloaded with the nRF9160: Asset Tracker firmware and modem firmware on the nRF9160 :term:`System in Package (SiP)`, and the Connectivity bridge application firmware on the nRF52840 :term:`System on Chip (SoC)` that enable the device to use the environment sensors and track the device using :term:`Global Positioning System (GPS)`.
The data is transmitted to nRF Cloud.

Before you start using the Thingy:91, it is recommended that you update the application firmware on the nRF9160 SiP to :ref:`asset_tracker_v2`.
You must also update the modem firmware.

.. tip::
   For a more compact nRF Cloud firmware application, you can build and install the :ref:`nrf_cloud_multi_service` sample.
   See the :ref:`building_pgming` section of the :ref:`ug_thingy91` page for more information.

You can update the application and modem firmware on a Thingy:91 through a :term:`Universal Serial Bus (USB)` cable using MCUboot.
MCUboot is a secure bootloader that is used to update applications if you do not have an external debugger.
The board enters MCUboot mode if any of the following buttons are pressed while the Thingy:91 is being powered on (using **SW1**):

* **SW3** - The main button used to flash the nRF9160 SiP.
  You use this button when getting started with the Thingy:91.
* **SW4** - The button used to :ref:`update the nRF52840 SoC <updating_the conn_bridge_52840>`.

Before you start, make sure the Thingy:91 is connected to the computer with a micro-USB cable and powered on.

.. note::

   Do not unplug the Nordic Thingy:91 during this process.

To update the firmware on the Thingy:91, complete the following steps:

1. Open the `Cellular Monitor`_ app.
#. Click :guilabel:`SELECT DEVICE` and select the Thingy:91 from the drop-down list.

   .. figure:: images/cellularmonitor_selectdevice_thingy91.png
      :alt: Cellular Monitor - Select device

      Cellular Monitor - Select device

   The drop-down text changes to the type of the selected device, with its SEGGER ID below the name.

#. Click :guilabel:`Program device` in the **ADVANCED OPTIONS** section.

   .. figure:: images/cellularmonitor_programdevice_thingy91.png
      :alt: Cellular Monitor - Program device

      Cellular Monitor - Program device

   The **Program sample app** window appears, displaying applications you can program to the Thingy:91.

#. Click :guilabel:`Select` in the **Asset Tracker V2** section.
   Asset Tracker v2 is an application that simulates sensor data and transmits it to Nordic Semiconductor's cloud solution, `nRF Cloud`_.

   .. figure:: images/cellularmonitor_selectassettracker.png
      :alt: Cellular Monitor - Select Asset Tracker V2

      Cellular Monitor - Select Asset Tracker V2

   The **Program Modem Firmware (Optional)** window appears.

#. Click :guilabel:`Select` in the section for the latest modem firmware.

   The **Program Mode Firmware (Optional)** window expands to display additional information.

   .. figure:: images/cellularmonitor_enablemcuboot.png
      :alt: Cellular Monitor - Enable MCUboot

      Cellular Monitor - Enable MCUboot

#. Switch off the Thingy:91.
#. Press **SW3** while switching **SW1** to the **ON** position to enable MCUboot mode.
#. Click :guilabel:`Program` to program the modem firmware to the Thingy:91.
   Do not unplug or turn off the device during this process.

   When the process is complete, you see a success message.

   If you see an error message, switch off the Thingy:91, enable MCUboot mode again, and click :guilabel:`Program`.

#. Click :guilabel:`Continue` to move to the next step.

   The **Program Mode Firmware (Optional)** window changes to the **Program Asset Tracker V2** window.

#. Switch off the Thingy:91.
#. Press **SW3** while switching **SW1** to the **ON** position to enable MCUboot mode.
#. Click :guilabel:`Program` to program the application to the Thingy:91.
   Do not unplug or turn off the device during this process.

   When the process is complete, you see a success message.
   Click :guilabel:`Close` to close the **Program Asset Tracker V2** window.

   If you see an error message, switch off the Thingy:91, enable MCUboot mode again, and click :guilabel:`Program`.

#. Copy the :term:`Integrated Circuit Card Identifier (ICCID)` of the inserted micro-SIM.
   This is required for activating the iBasis SIM when :ref:`thingy91_connect_to_cloud`.

   If you have activated your iBasis SIM card before or are using a SIM card from a different provider, you can skip this step.

   a. Click :guilabel:`Start` to begin the modem trace.
      The button changes to :guilabel:`Stop` and is greyed out.
   #. Click :guilabel:`Refresh dashboard` to refresh the information.

      If the information does not load, switch the Thingy:91 off and on, select the device from the :guilabel:`SELECT DEVICE` drop-down, and click :guilabel:`Start` to begin the modem trace again.

   #. Copy the ICCID by clicking on the **ICCID** label or the displayed ICCID number in the **Sim** section.

      .. figure:: images/cellularmonitor_iccid.png
         :alt: Cellular Monitor - ICCID

         Cellular Monitor - ICCID

      .. note::
         The ICCID copied here has 20 digits.
         When activating the SIM, you need to remove the last two digits so that it is 18 digits.

.. _connect_nRF_cloud:
.. _thingy91_connect_to_cloud:

Connecting the |DK| to nRF Cloud
*********************************

.. |DK| replace:: Thingy:91

.. include:: nrf9160_gs.rst
   :start-after: dk_nrf_cloud_start
   :end-before: dk_nrf_cloud_end

Once you have an account, activate your SIM card and add the Thingy:91 to your nRF Cloud account.

.. note::

   The instructions assume you are activating an iBasis SIM card that came with your |DK|.

   If you are using a SIM card from another provider, make sure you activate it through your network operator before :ref:`adding the device to nRF Cloud <thingy91_cloud_add_device>`.

Creating an nRF Cloud account
=============================

.. include:: nrf9160_gs.rst
   :start-after: nrf_cloud_account_start
   :end-before: nrf_cloud_account_end

.. _thingy91_cloud_activate_sim:

Activating the SIM card
=======================

To activate the iBasis SIM card that comes shipped with the |DK|, complete the following steps in the `nRF Cloud`_ portal.
Make sure you are logged in to the portal.

1. Click :guilabel:`SIM Cards` under :guilabel:`Device Management` in the navigation pane on the left.

   .. figure:: images/nrfcloud_simcards.png
      :alt: nRF Cloud - SIM Cards

      nRF Cloud - SIM Cards

#. Click :guilabel:`Add SIM`.

   .. figure:: images/nrfcloud_addsim.png
      :alt: nRF Cloud - Add SIM

      nRF Cloud - Add SIM

   The **Add SIM** page opens in the **Verify SIM Card** view.

#. Complete the following steps on the **Add SIM** page to activate your iBasis SIM card:

   .. figure:: images/nrfcloud_activating_sim.png
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
=================================

.. |led_cloud_association| replace:: the RGB LED double pulse blinks as white
.. |led_publishing_data| replace:: the RGB LED blinking green
.. |activate_sim_section| replace:: :ref:`thingy91_cloud_activate_sim`

.. include:: nrf9160_gs.rst
   :start-after: nrf_cloud_add_device_start
   :end-before: nrf_cloud_add_device_end

Next steps
**********

You have now completed getting started with the Thingy:91.
See the following links for where to go next:

* :ref:`installation` and :ref:`configuration_and_build` documentation to install the |NCS| and learn more about its development environment.
* :ref:`ug_thingy91` for more advanced topics related to the Thingy:91.
* :ref:`connectivity_bridge` about using the nRF UART Service with Thingy:91.
