.. _programming_thingy91x:

Updating the Thingy:91 X firmware using nRF Util
################################################

.. contents::
   :local:
   :depth: 2

You can use the ``nrfutil device`` command for the following:

* :ref:`Update the firmware on the nRF5340 SoC <updating_firmware_nRF5340>`.
* :ref:`Update the application firmware on the nRF9151 SiP <update_nRF9151_application>`.
* :ref:`Update the modem firmware on the nRF9151 SiP <update_modem_fw_nRF9151>`.

You can perform these operations through USB using MCUboot, or through an external debug probe.
When developing with your Thingy:91 X, it is recommended to use an external debug probe.

.. note::
   The external debug probe must support Arm Cortex-M33, such as the nRF9151 DK.
   You need a 10-pin 2x5 socket-socket 1.27 mm IDC (:term:`Serial Wire Debug (SWD)`) JTAG cable to connect to the external debug probe.

See `Installing nRF Util`_ and `Installing and upgrading nRF Util commands`_ for instructions on how to install the nrfutil device utility.

.. _updating_firmware_nRF5340:

Updating the firmware on the nRF5340 SoC
****************************************

This section describes how you can update the firmware of the nRF5340 SoC on the Nordic Thingy:91 X through USB or with an external debug probe.

.. tabs::

   .. group-tab:: Through USB and MCUboot

      To update the nRF5340 SoC firmware over USB, complete the following steps:

      1. Install the ``nrfutil device`` command package by completing the steps in the `Installing and upgrading nRF Util commands`_ documentation.
      #. Connect the Thingy:91 X to your computer with a USB-C cable.
      #. Power on the device by switching **SW1** to the **ON** position.
      #. Open a terminal window.
      #. Enter the following command to list the connected devices and their traits::

          nrfutil device list

         The Nordic Thingy\:91 X will be listed as a Thingy\:91 X UART product and have the following details:

         * A 21-character serial number, for example, ``THINGY91X_C2E0AC7F599``.
         * ``mcuboot``, ``nordicUsb``, ``serialPorts``, and ``usb`` traits.


      #. Enter the following command to update both the application and network core of the nRF5340 application core with a multi-image :file:`dfu_application.zip` file:

         .. code-block:: console

            nrfutil device program --firmware dfu_application.zip --serial-number <Thingy:91 X Serial number> --traits mcuboot --x-family nrf53

   .. group-tab:: Through external debug probe

      To update the nRF5340 firmware using an external debug probe, complete the following steps:

      1. Install the ``nrfutil device`` command package by completing the steps in the `Installing and upgrading nRF Util commands`_ documentation.
      #. Connect the Thingy:91 X to your computer with a USB-C cable.
      #. Connect the 10-pin :term:`Serial Wire Debug (SWD)` programming cable from the external debug probe to the programming connector (**P8**) on the Thingy:91 X.
      #. Connect the external debug probe to your computer.
      #. Power on the device by switching **SW1** to the **ON** position.
      #. Set the programming target select switch **SW2** to **nRF53** on the Thingy:91 X.
      #. Open a terminal window.
      #. Enter the following command to list the connected devices and their traits::

          nrfutil device list

         The external debug probe will be listed as a J-Link product with the ``jlink`` trait and will have a 9 or 10 digit J-Link serial number depending on the J-Link probe used.

      #. Enter the following command to program the application binary to the nRF5340 application core:

         .. code-block:: console

            nrfutil device program --firmware <name_of_application_binary.hex> --serial-number <J-Link Serial number> --traits jlink --x-family nrf53 --core Application

      #. Enter the following command to program the application binary to the nRF5340 network core:

         .. code-block:: console

            nrfutil device program --firmware <name_of_network_core_binary.hex> --serial-number <J-Link Serial number> --traits jlink --x-family nrf53 --core Network

.. _update_nRF9151_application:

Updating the application firmware on the nRF9151 SiP
****************************************************

This section describes how you can update the application firmware of the nRF9151 SiP on the Nordic Thingy:91 X through USB or with an external debug probe.

.. tabs::

   .. group-tab:: Through USB and MCUboot

      To update the nRF5340 SoC firmware over USB, complete the following steps:

      1. Install the ``nrfutil device`` command package by completing the steps in the `Installing and upgrading nRF Util commands`_ documentation.
      #. Connect the Thingy:91 X to your computer with a USB-C cable.
      #. Power on the device by switching **SW1** to the **ON** position.
      #. Open a terminal window.
      #. Enter the following command to list the connected devices and their traits::

          nrfutil device list

         The Nordic Thingy\:91 X will be listed as a Thingy\:91 X UART product and have the following details:

         * A 21-character serial number, for example, ``THINGY91X_C2E0AC7F599``.
         * ``mcuboot``, ``nordicUsb``, ``serialPorts``, and ``usb`` traits.

      #. Enter the following command to program the application binary to the nRF9151 application core:

         .. code-block:: console

            nrfutil device program --firmware dfu_application.zip --serial-number <J-Link Serial number> --traits mcuboot --x-family nrf91 --core Application

   .. group-tab:: Through external debug probe

      To update the nRF5340 firmware using an external debug probe, complete the following steps:

      1. Install the ``nrfutil device`` command package by completing the steps in the `Installing and upgrading nRF Util commands`_ documentation.
      #. Connect the Thingy:91 X to your computer with a USB-C cable.
      #. Connect the 10-pin :term:`Serial Wire Debug (SWD)` programming cable from the external debug probe to the programming connector (**P8**) on the Thingy:91 X.
      #. Connect the external debug probe to your computer.
      #. Power on the device by switching **SW1** to the **ON** position.
      #. Set the programming target select switch **SW2** to **nRF91** on the Thingy:91 X.
      #. Open a terminal window.
      #. Enter the following command to list the connected devices and their traits::

          nrfutil device list

         The external debug probe will be listed as a J-Link product with the ``jlink`` trait and will have a 9 or 10 digit J-Link serial number depending on the J-Link probe used.

      #. Enter the following command to program the application binary to the nRF9151 application core:

         .. code-block:: console

            nrfutil device program --firmware <name_of_application_binary.hex> --serial-number <Thingy:91 X Serial number> --traits jlink --x-family nrf91 --core Application

.. _update_modem_fw_nRF9151:

Updating the modem firmware on the nRF9151 SiP
**********************************************

.. note::
   Modem firmware update through USB and MCUboot is currently not supported.

To update the nRF5340 firmware using an external debug probe, complete the following steps:

1. Install the ``nrfutil device`` command package by completing the steps in the `Installing and upgrading nRF Util commands`_ documentation.
#. Connect the Thingy:91 X to your computer with a USB-C cable.
#. Connect the 10-pin :term:`Serial Wire Debug (SWD)` programming cable from the external debug probe to the programming connector (**P8**) on the Thingy:91 X.
#. Connect the external debug probe to your computer.
#. Power on the device by switching **SW1** to the **ON** position.
#. Set the programming target select switch **SW2** to **nRF91** on the Thingy:91 X.
#. Open a terminal window.
#. Enter the following command to list the connected devices and their traits::

    nrfutil device list

   The external debug probe will be listed as a J-Link product with the ``jlink`` trait and will have a 9 or 10 digit J-Link serial number depending on the J-Link probe used.

#. Enter the following command to program the modem firmware on the nRF9151 SiP:

   .. code-block:: console

      nrfutil device program --firmware <modem.zip> --serial-number <Thingy:91 X Serial number> --traits jlink modem --x-family nrf91
