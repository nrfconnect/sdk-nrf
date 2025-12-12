.. _ug_nrf54h20_ironside_se_debug:
.. _ug_nrf54h20_ironside_debug:

Debugging |ISE|
###############

|ISE| provides the ``DEBUGWAIT`` boot command to halt the application core immediately after reset.
This ensures that a debugger can attach and take control from the very first instruction.

When ``DEBUGWAIT`` is enabled, |ISE| sets the application domain's CPUWAIT when the application core starts.
This prevents the CPU from executing any instructions until a debugger manually releases it.

.. note::
   You can also use the ``cpuconf`` service to set CPUWAIT when booting other cores.
