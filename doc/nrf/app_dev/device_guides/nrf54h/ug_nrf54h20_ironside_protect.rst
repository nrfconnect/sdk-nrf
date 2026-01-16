.. _ug_nrf54h20_ironside_se_protecting:
.. _ug_nrf54h20_ironside_protect:

Protecting a device with |ISE|
##############################

By default, |ISE| configures the system with the following access policies on the nRF54H20 SoC:

.. list-table::
   :header-rows: 1
   :widths: auto

   * - Configuration
     - Policy
   * - MRAMC
     - MRAM operates in Direct Write mode with READYNEXTTIMEOUT disabled.
   * - MPC
     - All memory not reserved by the |ISE| is accessible with read, write, and execute (RWX) permissions by any domain.
   * - TAMPC
     - Access ports (AP) for the local domains are enabled, allowing direct programming of all the memory not reserved by the |ISE| in the default configuration.

.. note::
   * The Radio Domain AP is only usable when the Radio domain has booted.
   * Access to external memory (EXMIF) requires a non-default configuration of the GPIO.CTRLSEL register.

The |ISE| UICR interface is used to manage protection mechanisms for the SoC, such as enabling secure boot and disabling the debugger interfaces.

The default ("erased") UICR has no protection features enabled.
Protection features are enabled (or disabled) by writing the relevant UICR field and resetting the SoC.

The UICR.LOCK and UICR.ERASEPROTECT fields determine whether the UICR area itself can be modified.
Enabling UICR.LOCK and resetting the SoC makes the UICR read-only, and as a result protection features can no longer be changed using regular writes.
It is however still possible to perform a mass-erase of the device memory, including the UICR, using an ERASEALL operation.
This causes the UICR to be reset to the default state, with all protection features disabled.
Enabling UICR.ERASEPROTECT blocks the ERASEALL operation.
As a result, enabling both UICR.LOCK and UICR.ERASEPROTECT prevents any change to the UICR, making the protection configuration permanent.

.. _ug_nrf54h20_ironside_se_production_ready_protection:

Protecting a production-ready device
************************************

To protect the nRF54H20 SoC in a production-ready device, you must enable the following UICR-based security mechanisms.
For information on how to configure these UICR settings, see :ref:`ug_nrf54h20_ironside_se_uicr`.

* :kconfig:option:`CONFIG_GEN_UICR_APPROTECT_APPLICATION_PROTECTED` - Disables debugger access to the Application AP.
  It restricts debugger and access-port (AP) permissions, preventing unauthorized read/write access to memory and debug interfaces.
* :kconfig:option:`CONFIG_GEN_UICR_APPROTECT_RADIOCORE_PROTECTED` - Disables debugger access to the Radiocore AP.
  It restricts debugger and access-port (AP) permissions, preventing unauthorized read/write access to memory and debug interfaces.
* :kconfig:option:`CONFIG_GEN_UICR_APPROTECT_CORESIGHT_PROTECTED` - Disables debugger access to the CoreSight AP.
  It restricts debugger and access-port (AP) permissions, preventing unauthorized read/write access to memory and debug interfaces.
* :kconfig:option:`CONFIG_GEN_UICR_LOCK` - Freezes non-volatile configuration registers.
  It locks the UICR, ensuring that no further UICR writes are possible without issuing an ``ERASEALL`` command.
* :kconfig:option:`CONFIG_GEN_UICR_PROTECTEDMEM` - Enforces integrity checks on critical code and data.
  It defines a trailing region of application-owned MRAM whose contents are integrity-checked at each boot, extending the root of trust to your immutable bootloader or critical data.
* :kconfig:option:`CONFIG_GEN_UICR_ERASEPROTECT` - Prevents bulk erasure of protected memory.
  It blocks all ``ERASEALL`` operations.
