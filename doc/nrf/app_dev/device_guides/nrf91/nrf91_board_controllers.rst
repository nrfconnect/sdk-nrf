.. _nrf91_ug_board_controllers:

Configuring board controller
############################

.. contents::
   :local:
   :depth: 2

The nRF91 Series DKs contain additional chips that act as board controllers.

.. _nrf9161_ug_intro:

Board controller on the nRF91x1 DKs
***********************************

The nRF91x1 DKs (nRF9161 and nRF9151 DKs) contain an nRF5340 Interface MCU (IMCU) that acts both as an on-board debugger and board controller.
The board controller controls signal switches on the nRF91x1 DKs and can be used to route the nRF91x1 SiPs pins to different components on the DK, such as pin headers, external memory, a SIM card, or eSIM.

The following sections have a complete list of configuration options available for the nRF9161 DK and the nRF9151 DK respectively:

* `Board control <nRF9161 DK board control section in the nRF9161 DK User Guide_>`_  in the nRF9161 DK Hardware guide
* `Board control <nRF9151 DK board control section in the nRF9151 DK User Guide_>`_  in the nRF9151 DK Hardware guide

The nRF5340 IMCU comes preprogrammed with J-Link SEGGER OB and board controller firmware.
If you want to change the default configuration of the DK, you can use the `nRF Connect Board Configurator`_ app in `nRF Connect for Desktop`_ .

To update board controller firmware on the nRF9161 DK, download the `nRF9161 DK board controller firmware`_ from the nRF9161 DK downloads page.
To program the HEX file, use `nRF Util`_.

.. _nrf9160_ug_intro:

Board controller on the nRF9160 DK
**********************************

The nRF9160 DK contains an nRF52840 SoC that routes some of the nRF9160 SiP pins to different components on the DK, such as LEDs and buttons, and to specific pins of the nRF52840 SoC itself.
For a complete list of all the routing options available, see the `nRF9160 DK board control section in the nRF9160 DK User Guide`_.
Make sure to select the correct controller before you program the application to your development kit.

The nRF52840 SoC on the DK comes preprogrammed with a firmware.
If you need to restore the original firmware at some point, download the `nRF9160 DK board controller firmware`_ from the nRF9160 DK downloads page.
To program the HEX file, use nrfjprog (which is part of the `nRF Command Line Tools`_).

If you want to route some pins differently from what is done in the preprogrammed firmware, program the :zephyr:code-sample:`hello_world` sample instead of the preprogrammed firmware.
Build the sample (located under :file:`ncs/zephyr/samples/hello_world`) for the ``nrf9160dk_nrf52840`` board target.
To change the routing options, enable or disable the corresponding devicetree nodes for that board as needed.
See :ref:`zephyr:nrf9160dk_board_controller_firmware` for detailed information.
