.. _thingy91x_recover_to_factory_firmware:

Recovering the Thingy:91 X to factory firmware
##############################################

.. contents::
   :local:
   :depth: 2

You can recover the Thingy:91 X to factory firmware using the ``nrfutil device`` command in the `nRF Util commands <Device command overview_>`_.

If your Thingy:91 X is powered on and connected using a USB cable, it shows up as a USB device.
Else, something is preventing the :ref:`connectivity_bridge` application on the nRF5340 from running.
For example, this could be an application running on the nRF9151 that is driving the wrong pins.
Another typical case is that the nRF5340 has been flashed by accident.

You need to use an external J-Link to follow these steps.
You can also use a Nordic development kit with a debug-out port.

Complete the following steps to recover the Thingy:91 X to factory firmware:

#. Download the following files from the `hello-nrfcloud/firmware release v2.0.1 <hello-nrfcloud v2.0.1_>`_:

   * nRF5340 application firmware - :file:`connectivity-bridge-v2.0.1-thingy91x-nrf53-app.hex`
   * nRF5340 network firmware - :file:`connectivity-bridge-v2.0.1-thingy91x-nrf53-net.hex`
   * nRF9151 firmware - :file:`hello.nrfcloud.com-v2.0.1+debug-thingy91x-nrf91.hex`

#. Set the SWD switch to **nRF91**.
#. Run the ``nrfutil device recover`` command to recover the nRF9151.
#. Set the SWD switch to **nRF53**.
#. Run the ``nrfutil device recover`` command to recover the nRF5340.

   .. note::
      You can use the ``nrfutil device device-info`` command to check that you are connected to the correct chip.
      When using a DK, there is an ambiguity between targeting the on-board chip and using debug-out.
      Running the ``nrfutil device device-info`` command is recommended to make sure you are not programming the on-board target accidentally.

   If nRF5340 recovery fails, an nRF9151 firmware might have been flashed previously.

#. Run the following command to program the device:

   .. code-block:: none

      nrfutil device recover --x-family nrf53 --core Network --traits jlink
      nrfutil device recover --x-family nrf53 --core Application --traits jlink

   Even though the device shows up as unrecoverable, the soft-lock can be removed with these commands.
   You might have to repeat them for a successful result.

   .. note::
      Do not use these commands on an nRF91 Series device.

   Now both chips are recovered.

#. Set the SWD switch back to **nRF91**.
#. Run the following command to program the device:

   .. code-block:: none

      nrfutil device program --firmware hello.nrfcloud.com-v2.0.1+debug-thingy91x-nrf91.hex

#. Set the SWD switch again to **nRF53**.
#. Run the following command to program the device:

   .. code-block:: none

      nrfutil device program --core Network --options reset=RESET_NONE,chip_erase_mode=ERASE_ALL,verify=VERIFY_NONE --firmware connectivity-bridge-v2.0.1-thingy91x-nrf53-net.hex
      nrfutil device program --options reset=RESET_SYSTEM,chip_erase_mode=ERASE_ALL,verify=VERIFY_NONE --firmware connectivity-bridge-v2.0.1-thingy91x-nrf53-app.hex

   The device shows up as a USB device again.

#. Press **Button 2** to reset the nRF9151 DK.

   If you see that **LED 1** lights up, the recovery was successful.

If you do not have a usable device after this procedure, contact Nordic Semiconductor's technical support on `DevZone`_.
