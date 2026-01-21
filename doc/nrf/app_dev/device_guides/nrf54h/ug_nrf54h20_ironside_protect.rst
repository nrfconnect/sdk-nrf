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

.. _ug_nrf54h20_ironside_se_production_ready_protection:

Protecting a production-ready device
************************************

To protect the nRF54H20 SoC in a production-ready device, you must enable the following UICR-based security mechanisms.
For information on how to configure these UICR settings, see :ref:`ug_nrf54h20_ironside_se_uicr`.

* :kconfig:option:`CONFIG_GEN_UICR_APPROTECT_APPLICATION_PROTECTED` - Disables all debug and AP access.
  It restricts debugger and access-port (AP) permissions, preventing unauthorized read/write access to memory and debug interfaces.
* :kconfig:option:`CONFIG_GEN_UICR_LOCK` - Freezes non-volatile configuration registers.
  It locks the UICR, ensuring that no further UICR writes are possible without issuing an ``ERASEALL`` command.
* :kconfig:option:`CONFIG_GEN_UICR_PROTECTEDMEM` - Enforces integrity checks on critical code and data.
  It defines a trailing region of application-owned MRAM whose contents are integrity-checked at each boot, extending the root of trust to your immutable bootloader or critical data.
* :kconfig:option:`CONFIG_GEN_UICR_ERASEPROTECT` - Prevents bulk erasure of protected memory.
  It blocks all ``ERASEALL`` operations on NVR0, preserving UICR settings even if an attacker attempts a full-chip erase.
