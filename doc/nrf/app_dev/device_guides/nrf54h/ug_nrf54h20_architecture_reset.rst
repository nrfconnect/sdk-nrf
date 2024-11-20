.. _ug_nrf54h20_architecture_reset:

nRF54H20 reset behavior
#######################

.. contents::
   :local:
   :depth: 2

The reset behavior of the nRF54H20 SoC depends on the core that triggers the reset:

+--------------------------+--------------------------------------------------------------+
|   CPU Triggering Reset   |   Reset Behavior                                             |
+==========================+==============================================================+
| Secure Domain            | Resets the entire SoC                                        |
+--------------------------+--------------------------------------------------------------+
| System Controller        | Resets the entire SoC                                        |
+--------------------------+--------------------------------------------------------------+
| Application domain       | - Resets all peripherals and VPRs owned by the Application   |
|                          |   domain                                                     |
|                          | - Resets the Radio domain                                    |
|                          | - Resets all peripherals and VPRs owned by the Radio domain  |
+--------------------------+--------------------------------------------------------------+
| Radio domain             | - Resets all peripherals and VPRs owned by the Radio domain  |
|                          | - Resets the Application domain                              |
|                          | - Resets all peripherals and VPRs owned by the Application   |
|                          |   domain                                                     |
+--------------------------+--------------------------------------------------------------+

nRF Util allows for different types of reset behavior on the various cores of the nRF54H20 SoC, based on the current lifecycle state of the device.

Reset types in LCS EMPTY
------------------------

The following is the reset behavior when the LCS of the nRF54H20 SoC is in the ``EMPTY`` state:

.. list-table:: Reset behavior based on LCS ``EMPTY``
   :header-rows: 1

   * - Reset kind in LCS ``EMPTY``
     - Reset behavior
   * - ``RESET_SYSTEM``
     - Mapped to the ``SYS_RESET_REQUEST`` bit as described in the ARM specification for the *Application Interrupt and Register Controller*.
       This reset triggers a reset of the Secure Domain, which subsequently resets the entire system.
       It is available only to the Secure Domain.
   * - ``RESET_HARD``
     - Initiates a reset using the CTRL-AP through the On-Board Debugger (OBD).
       The register used is ``RESET`` (address offset: ``0x00``).
   * - ``RESET_PIN``
     - J-Link toggles the **RESET** pin using OBD.
   * - ``RESET_VIA_SECDOM``
     - Not supported.
   * - ``RESET_DEFAULT``
     - Selects ``RESET_HARD``.

Reset types in LCS ROT or DEPLOYED
-----------------------------------

The following is the reset behavior when the LCS of the nRF54H20 SoC is either in the ``ROT`` or ``DEPLOYED`` state:

.. list-table:: Reset behavior based on LCS ``ROT`` or ``DEPLOYED``
   :header-rows: 1

   * - Reset kind in LCS ``ROT`` or ``DEPLOYED``
     - Reset behavior
   * - ``RESET_SYSTEM``
     - Not supported.
   * - ``RESET_HARD``
     - Initiates a reset using the CTRL-AP through the On-Board Debugger (OBD).
       The register used is **RESET** (address offset: ``0x00``).
   * - ``RESET_PIN``
     - J-Link toggles the **RESET** pin using OBD.
   * - ``RESET_VIA_SECDOM``
     - Uses the CTRL-AP mailbox to send a local domain reset request to the Secure Domain Firmware (SDFW).

       SDFW resets nonessential local domains and global peripherals owned by these domains.
       Nonessential domains include all local domains except the Secure Domain.

       Only domains with a populated UICR are reset.
       After the reset, the affected CPUs do not restart.

   * - ``RESET_DEFAULT``
     - Selects ``RESET_HARD``.

Reset your device using nRF Util
================================

To trigger a specific reset type on your nRF54H20 SoC-based device, use the ``nrfutil device reset`` command::

   nrfutil device reset --serial-number <blah> --reset-kind <reset-kind>

For a detailed list of commands for each reset type, run the following::

   nrfutil device reset --help

For more information on ``nrfutil device reset``, see `nRF Util documentation pages <Device command overview_>`_.
