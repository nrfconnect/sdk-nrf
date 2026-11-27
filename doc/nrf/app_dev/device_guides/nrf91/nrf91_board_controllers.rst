.. _nrf91_ug_board_controllers:
.. _nrf9151_ug_intro:

Configuring board controller
############################

The nRF91 Series DKs contain additional chips that act as board controllers.
The nRF9151 DKs (nRF9151 and nRF9151 SMA DKs) contain an nRF5340 Interface MCU (IMCU) that acts both as an on-board debugger and board controller.
The board controller controls signal switches on the nRF9151 DKs and can be used to route the nRF9151 SiP pins to different components on the DK, such as pin headers, external memory, a SIM card, or eSIM.

The following sections have a complete list of configuration options available for the nRF9151 DK and the nRF9151 SMA DK respectively:

* `Board control <nRF9151 DK board control section in the nRF9151 DK User Guide_>`_  in the nRF9151 DK Hardware guide
* `Board control <nRF9151 SMA DK board control section in the nRF9151 SMA DK User Guide_>`_  in the nRF9151 SMA DK Hardware guide

The nRF5340 IMCU comes preprogrammed with J-Link SEGGER OB and board controller firmware.
If you want to change the default configuration of the DK, you can use the `Board Configurator app`_ in `nRF Connect for Desktop`_ .
