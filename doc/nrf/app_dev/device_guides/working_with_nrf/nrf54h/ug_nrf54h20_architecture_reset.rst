.. _ug_nrf54h20_architecture_reset:

nRF54H20 reset behavior
#######################

.. contents::
   :local:
   :depth: 2

.. TODO

nrfutil allows for different types of reset behavior on the various cores of the nRF54H20 SoC, based on the current lifecycle state of the device.

Reset kinds in LCS EMPTY
========================

.. TODO

The following is the reset behavior when the LCS of the nRF54H20 SoC is in ``EMPTY`` state:

.. list-table:: Reset behavior based on LCS ``EMPTY``
   :header-rows: 1

   * - Reset kind in LCS ``EMPTY``
     - Reset behavior
   * - ``RESET_SYSTEM``
     - Mapped to the ``SYS_RESET_REQUEST`` bit as described in the ARM specification for the *Application Interrupt and Register Controller*.
       This reset triggers a reset of the Secure Domain, which subsequently resets the entire system.
       However, it cannot trigger a reset of other cores.
   * - ``RESET_HARD``
     - Initiates a reset using the CTRL-AP through the On-Board Debugger (OBD).
       The register used is ``RESET`` (address offset: ``0x00``).
   * - ``RESET_PIN``
     - JLink toggles the **RESET** pin via OBD.

.. TODO nrfutil device reset syntax

Reset kinds in LCS ROT or DEPLOYED
==================================

.. TODO

The following is the reset behavior when the LCS of the nRF54H20 SoC is either in ``ROT`` or ``DEPLOYED`` state:

.. list-table:: Reset behavior based on LCS ``ROT`` or ``DEPLOYED``
   :header-rows: 1

   * - Reset kind in LCS ``ROT`` or ``DEPLOYED``
     - Reset behavior
   * - ``RESET_SYSTEM``
     - Mapped to the ``SYS_RESET_REQUEST`` bit as described in the ARM specification for the *Application Interrupt and Register Controller*.
       This reset triggers a reset of the Secure Domain, which subsequently resets the entire system.
       If other cores have a valid configuration in the ``CPUCONF`` register, they will be reset by sending a CTRL-AP mailbox request to the Secure Domain Firmware (SDFW).
       No ADAC authentication is required to use this reset command.
       However, ``RESET_SYSTEM`` will not work if the Access Port is protected.
   * - ``RESET_HARD``
     - Initiates a reset using the CTRL-AP through the On-Board Debugger (OBD).
       The register used is ``RESET`` (address offset: ``0x00``).
   * - ``RESET_PIN``
     - JLink toggles the ``RESET`` pin via OBD.

.. TODO nrfutil device reset syntax
