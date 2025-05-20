.. _thingy91_update_firmware:

Updating the Thingy:91 firmware using the Cellular Monitor app
##############################################################

.. contents::
   :local:
   :depth: 2

Thingy:91 (v1.5.0 or earlier) comes preloaded with the nRF9160: Asset Tracker firmware and modem firmware on the nRF9160 :term:`System in Package (SiP)`, and the Connectivity bridge application firmware on the nRF52840 :term:`System on Chip (SoC)` that enable the device to use the environment sensors and track the device using :term:`Global Positioning System (GPS)`.
The data is transmitted to nRF Cloud.

.. tip::
   For a more compact nRF Cloud firmware application, you can build and install the :ref:`nrf_cloud_multi_service` sample.
   See the :ref:`building_pgming` section for more information.

You can update the application and modem firmware on a Thingy:91 through a :term:`Universal Serial Bus (USB)` cable using MCUboot.
MCUboot is a secure bootloader that is used to update applications if you do not have an external debugger.
The board enters MCUboot mode if you press one of the following buttons while the Thingy:91 is being powered on (using **SW1**):

* **SW3** - The main button used to flash the nRF9160 SiP.
  You use this button when getting started with the Thingy:91.
* **SW4** - The button used to :ref:`update the nRF52840 SoC <updating_the conn_bridge_52840>`.

Before you start, make sure the Thingy:91 is connected to the computer with a micro-USB cable and powered on.

.. note::

   Do not unplug the Nordic Thingy:91 during this process.

To update the firmware on the Thingy:91, complete the following steps:

1. Install the `Cellular Monitor app`_ on the computer:

   a. Go to `nRF Connect for Desktop Downloads <Download nRF Connect for Desktop_>`_.
   #. Download and install nRF Connect for Desktop.
   #. Open `nRF Connect for Desktop`_.
   #. Find **Cellular Monitor** in the list of apps and click :guilabel:`Install`.

#. Open the `Cellular Monitor app`_.
#. Click :guilabel:`SELECT DEVICE` and select the Thingy:91 from the drop-down list.

   .. figure:: images/cellularmonitor_selectdevice_thingy91.png
      :alt: Cellular Monitor app - Select device

      Cellular Monitor app - Select device

   The drop-down text changes to the type of the selected device, with its SEGGER ID below the name.

#. Click :guilabel:`Program device` in the **ADVANCED OPTIONS** section.

   .. figure:: images/cellularmonitor_programdevice_thingy91.png
      :alt: Cellular Monitor app - Program device

      Cellular Monitor app - Program device

   The **Program sample app** window appears, displaying applications you can program to the Thingy:91.

#. Click :guilabel:`Select` in the **Asset Tracker V2** section.

   .. figure:: images/cellularmonitor_selectassettracker.png
      :alt: Cellular Monitor app - Select Asset Tracker V2

      Cellular Monitor app - Select Asset Tracker V2

   The **Program Modem Firmware (Optional)** window appears.

#. Click :guilabel:`Select` in the section for the latest modem firmware.

   The **Program Mode Firmware (Optional)** window expands to display additional information.

   .. figure:: images/cellularmonitor_enablemcuboot.png
      :alt: Cellular Monitor app - Enable MCUboot

      Cellular Monitor app - Enable MCUboot

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
         :alt: Cellular Monitor app - ICCID

         Cellular Monitor app - ICCID

      .. note::
         The ICCID copied here has 20 digits.
         When activating the SIM, you need to remove the last two digits so that it is 18 digits.
